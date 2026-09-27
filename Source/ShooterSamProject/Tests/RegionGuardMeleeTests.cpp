// 区域守卫与近战攻击的资产配置测试。
//
// 为什么放在 C++ 而不是 Python 检查脚本里：
//   UBehaviorTree::RootNode 被声明为私有 UPROPERTY，UE 5.7 的 Python 接口既没有
//   GetRootNode() 也没有节点遍历入口，读不到行为树的内部结构；而 UBTCompositeNode
//   的 Children / Services 与 FBTCompositeChild 的 Decorators 都是 public，
//   只有 RootNode 这一个成员挡路。这里用一次局部 private->public 打开它，
//   属于测试自己的翻译单元，不影响引擎与其他文件。
//
// 因此本文件负责检查「连线」类问题：服务、装饰器、任务、攻击数组、黑板键；
// Scripts/Inspect-RegionGuards.py 负责检查「引用」类问题：蓝图指向哪棵树、
// 地图生成器实例值、区域门门组。

#include "CoreMinimal.h"

// 三处 protected/private 成员需要在测试里直读：
//   UBehaviorTree::RootNode（私有）、UShooterBTTaskMeleeAttack::Attacks / SlotName（protected）。
// 都只用在本翻译单元，不影响引擎与其它文件。
#define private public
#define protected public
#include "BehaviorTree/BehaviorTree.h"
#include "ShooterBTTaskMeleeAttack.h"
#undef protected
#undef private

#include "Misc/AutomationTest.h"

#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BTNode.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/Blackboard/BlackboardKeyEnums.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Decorators/BTDecorator_Blackboard.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "ShooterBTServiceGuardTarget.h"
#include "ShooterBTServicePlayerTarget.h"
#include "ShooterMeleeEnemy.h"
#include "UObject/UnrealType.h"

namespace ShooterRegionGuardTree
{
	// 行为树连线在编辑器里改不到编译期，这里用资产路径做唯一入口；
	// 资产被改名或移动时，测试会直接报「加载失败」，不会静默通过。
	const TCHAR* const RegionMeleeTreePath =
		TEXT("/Game/MyStuff/EnemyBT/BT_RegionMeleeGuard.BT_RegionMeleeGuard");
	const TCHAR* const EliteMeleeTreePath =
		TEXT("/Game/MyStuff/EnemyBT/BT_ShooterEliteMeleeEnemy.BT_ShooterEliteMeleeEnemy");
	const TCHAR* const RegionGuardBlueprintPath =
		TEXT("/Game/MyStuff/BluePrints/Enemies/BP_RegionMeleeGuard.BP_RegionMeleeGuard_C");

	const FName TargetActorKey(TEXT("TargetActor"));
	const FName HomeKey(TEXT("GuardHomeLocation"));
	const FName ReturnKey(TEXT("GuardReturning"));

	/** 行为树节点名称，读不到时退回类名，避免断言信息里出现空串。 */
	FString NodeLabel(const UBTNode* Node)
	{
		if (!Node)
		{
			return TEXT("(null)");
		}

		FString Label = Node->GetNodeName();
		if (Label.IsEmpty())
		{
			Label = Node->GetClass()->GetName();
		}

		return Label;
	}

	/** 读取 protected 属性的值，用于查询枚举等没有公开读取入口的细节。 */
	bool ReadBoolProperty(const UObject* Object, const TCHAR* PropertyName, uint8& OutValue)
	{
		const FNumericProperty* Property =
			FindFProperty<FNumericProperty>(Object->GetClass(), PropertyName);

		if (!Property)
		{
			return false;
		}

		OutValue = static_cast<uint8>(
			Property->GetUnsignedIntPropertyValue_InContainer(Object));

		return true;
	}

	/** 按深度优先顺序收集节点；服务与装饰器归到所属节点名下。 */
	struct FNodeVisit
	{
		const UBTNode* Node = nullptr;
		int32 Depth = 0;
		TArray<const UBTService*> Services;
		TArray<const UBTDecorator*> Decorators;
	};

	void CollectNodes(
		const UBTNode* Node,
		int32 Depth,
		TArray<FNodeVisit>& OutVisits,
		int32 MaxDepth = 32)
	{
		if (!Node || Depth > MaxDepth)
		{
			return;
		}

		FNodeVisit Visit;
		Visit.Node = Node;
		Visit.Depth = Depth;

		const UBTCompositeNode* Composite = Cast<UBTCompositeNode>(Node);

		if (Composite)
		{
			for (const TObjectPtr<UBTService>& Service : Composite->Services)
			{
				if (Service)
				{
					Visit.Services.Add(Service);
				}
			}
		}

		if (Composite)
		{
			const int32 ChildCount = Composite->GetChildrenNum();

			for (int32 Index = 0; Index < ChildCount; ++Index)
			{
				const FBTCompositeChild& Child = Composite->Children[Index];

				// 装饰器与目标子节点同级挂在父节点的 Children 条目里。
				for (const TObjectPtr<UBTDecorator>& Decorator : Child.Decorators)
				{
					if (Decorator)
					{
						Visit.Decorators.Add(Decorator);
					}
				}

				CollectNodes(Child.ChildComposite, Depth + 1, OutVisits, MaxDepth);
				CollectNodes(Child.ChildTask, Depth + 1, OutVisits, MaxDepth);
			}
		}

		// 先收完本节点的子节点再登记自己，深度优先顺序与编辑器显示一致。
		OutVisits.Add(MoveTemp(Visit));
	}

	const FNodeVisit* FindFirstNode(
		const TArray<FNodeVisit>& Visits,
		const UClass* NodeClass)
	{
		for (const FNodeVisit& Visit : Visits)
		{
			if (Visit.Node && Visit.Node->IsA(NodeClass))
			{
				return &Visit;
			}
		}

		return nullptr;
	}

	/** 节点集合里是否存在标题为 Label 的服务。 */
	bool HasServiceNamed(const TArray<FNodeVisit>& Visits, const FString& Label)
	{
		for (const FNodeVisit& Visit : Visits)
		{
			for (const UBTService* Service : Visit.Services)
			{
				if (NodeLabel(Service) == Label)
				{
					return true;
				}
			}
		}

		return false;
	}

	/** 收集整棵树引用的所有黑板键名。 */
	void CollectDecoratorKeys(
		const TArray<FNodeVisit>& Visits,
		TArray<FName>& OutKeys)
	{
		for (const FNodeVisit& Visit : Visits)
		{
			for (const UBTDecorator* Decorator : Visit.Decorators)
			{
				const UBTDecorator_BlackboardBase* BlackboardDecorator =
					Cast<UBTDecorator_BlackboardBase>(Decorator);

				if (!BlackboardDecorator)
				{
					continue;
				}

				const FName Key = BlackboardDecorator->GetSelectedBlackboardKey();
				if (!Key.IsNone())
				{
					OutKeys.AddUnique(Key);
				}
			}
		}
	}

	UBTCompositeNode* RootOf(UBehaviorTree* Tree)
	{
		return Tree ? Tree->RootNode.Get() : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShooterRegionMeleeGuardTreeTest,
	"ShooterSam.RegionGuards.MeleeTree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShooterRegionMeleeGuardTreeTest::RunTest(const FString& Parameters)
{
	using namespace ShooterRegionGuardTree;

	UBehaviorTree* Tree = LoadObject<UBehaviorTree>(
		nullptr, RegionMeleeTreePath);

	if (!TestNotNull(
		FString::Printf(TEXT("行为树 %s 可以加载"), RegionMeleeTreePath),
		Tree))
	{
		return false;
	}

	UBTCompositeNode* Root = RootOf(Tree);

	if (!TestNotNull(TEXT("行为树有根节点（RootNode 可读）"), Root))
	{
		return false;
	}

	TArray<FNodeVisit> Visits;
	CollectNodes(Root, 0, Visits);

	AddInfo(FString::Printf(TEXT("节点数=%d"), Visits.Num()));

	for (const FNodeVisit& Visit : Visits)
	{
		FString Line = FString::Printf(
			TEXT("%s%s"),
			*FString::ChrN(Visit.Depth * 2, TEXT(' ')),
			*NodeLabel(Visit.Node));

		for (const UBTService* Service : Visit.Services)
		{
			Line += FString::Printf(TEXT("  [服务] %s"), *NodeLabel(Service));
		}

		for (const UBTDecorator* Decorator : Visit.Decorators)
		{
			Line += FString::Printf(
				TEXT("  [装饰器] %s(%s)"),
				*Decorator->GetClass()->GetName(),
				*NodeLabel(Decorator));
		}

		AddInfo(Line);
	}

	// 1. 根节点必须是 Selector，守卫目标服务挂在它上面，覆盖返岗与战斗全过程。
	TestTrue(
		TEXT("根节点是 Selector"),
		Root->GetClass()->GetName().Contains(TEXT("Selector")));

	TestTrue(
		TEXT("根节点挂有 Update Guard Target 服务"),
		HasServiceNamed(Visits, TEXT("Update Guard Target")));

	// 2. 区域树必须替换掉原服务，否则返回岗位的守卫会被重新指派玩家目标。
	TestFalse(
		TEXT("已移除 Update Player Target 服务"),
		HasServiceNamed(Visits, TEXT("Update Player Target")));

	// 3. 守卫目标服务是 C++ 类，且黑板键由代码固定，不需要在节点上选择。
	const FNodeVisit* GuardServiceVisit = nullptr;

	for (const FNodeVisit& Visit : Visits)
	{
		for (const UBTService* Service : Visit.Services)
		{
			if (Service->IsA<UShooterBTServiceGuardTarget>())
			{
				GuardServiceVisit = &Visit;
				break;
			}
		}

		if (GuardServiceVisit)
		{
			break;
		}
	}

	if (TestNotNull(
		TEXT("找到 ShooterBTServiceGuardTarget 实例"),
		GuardServiceVisit))
	{
		TestTrue(
			TEXT("Update Guard Target 挂在根节点上"),
			GuardServiceVisit->Node == Root);
	}

	// 4. 返岗分支：GuardReturning 的装饰器 + Move To(GuardHomeLocation)。
	bool bHasReturningDecorator = false;
	bool bHasHomeMoveTo = false;

	for (const FNodeVisit& Visit : Visits)
	{
		for (const UBTDecorator* Decorator : Visit.Decorators)
		{
			const UBTDecorator_BlackboardBase* BlackboardDecorator =
				Cast<UBTDecorator_BlackboardBase>(Decorator);

			if (!BlackboardDecorator)
			{
				continue;
			}

			if (BlackboardDecorator->GetSelectedBlackboardKey() != ReturnKey)
			{
				continue;
			}

			bHasReturningDecorator = true;

			const UBTDecorator_Blackboard* ConcreteDecorator =
				Cast<UBTDecorator_Blackboard>(Decorator);

			if (!ConcreteDecorator)
			{
				continue;
			}

			// BasicOperation 是 protected，通过反射读取；Set 对应界面上的 Is Set。
			uint8 OperationValue = 0;

			if (ReadBoolProperty(
				ConcreteDecorator, TEXT("BasicOperation"), OperationValue))
			{
				TestEqual(
					TEXT("GuardReturning 的 Key Query 是 Is Set"),
					static_cast<int32>(OperationValue),
					static_cast<int32>(EBasicKeyOperation::Set));
			}
			else
			{
				AddWarning(TEXT("读不到 BasicOperation，无法核对 Key Query 是否为 Is Set"));
			}
		}

		const UBTTaskNode* Task = Cast<UBTTaskNode>(Visit.Node);

		if (!Task)
		{
			continue;
		}

		if (Task->GetClass()->GetName().Contains(TEXT("MoveTo")))
		{
			// Move To 的 BlackboardKey 是 protected，按属性名反射查找。
			const FStructProperty* KeyProperty =
				FindFProperty<FStructProperty>(
					Task->GetClass(), TEXT("BlackboardKey"));

			if (!KeyProperty)
			{
				AddInfo(FString::Printf(
					TEXT("Move To 节点 %s (%s) 上找不到 BlackboardKey 属性"),
					*NodeLabel(Task), *Task->GetClass()->GetName()));
				continue;
			}

			const FBlackboardKeySelector* Selector =
				KeyProperty->ContainerPtrToValuePtr<FBlackboardKeySelector>(Task);

			if (!Selector)
			{
				AddInfo(FString::Printf(
					TEXT("Move To 节点 %s 的 BlackboardKey 读不出值"),
					*NodeLabel(Task)));
				continue;
			}

			if (Selector->SelectedKeyName == HomeKey)
			{
				bHasHomeMoveTo = true;
			}
			else
			{
				// 断言只报「没有正确键的 Move To」，这里补出实际值，便于定位。
				AddInfo(FString::Printf(
					TEXT("Move To 节点 %s (%s) 的 Blackboard Key 是 %s，应为 %s"),
					*NodeLabel(Task),
					*Task->GetClass()->GetName(),
					Selector->SelectedKeyName.IsNone()
						? TEXT("None") : *Selector->SelectedKeyName.ToString(),
					*HomeKey.ToString()));
			}
		}
	}

	TestTrue(
		TEXT("存在 GuardReturning 的黑板装饰器"),
		bHasReturningDecorator);

	TestTrue(
		TEXT("存在以 GuardHomeLocation 为目标的 Move To 任务"),
		bHasHomeMoveTo);

	// 5. Random Melee Attack 的攻击数组。
	const FNodeVisit* AttackVisit = FindFirstNode(
		Visits, UShooterBTTaskMeleeAttack::StaticClass());

	if (TestNotNull(
		TEXT("存在 Random Melee Attack 任务"),
		AttackVisit))
	{
		const UShooterBTTaskMeleeAttack* AttackTask =
			Cast<UShooterBTTaskMeleeAttack>(AttackVisit->Node);

		TestTrue(
			TEXT("Slot Name 不为空"),
			!AttackTask->SlotName.IsNone());

		TestTrue(
			TEXT("Attacks 至少有一个元素"),
			AttackTask->Attacks.Num() > 0);

		for (int32 Index = 0; Index < AttackTask->Attacks.Num(); ++Index)
		{
			const FShooterMeleeAttackOption& Option = AttackTask->Attacks[Index];
			const FString Prefix = FString::Printf(
				TEXT("Attacks[%d] %s"), Index, *Option.AttackName.ToString());

			const UAnimSequence* Animation = Option.Animation.Get();

			if (!TestNotNull(
				*FString::Printf(TEXT("%s 配置了动画"), *Prefix),
				Animation))
			{
				continue;
			}

			const float Length = Animation->GetPlayLength();

			TestTrue(
				*FString::Printf(TEXT("%s 的 Weight 大于 0"), *Prefix),
				Option.Weight > 0.0f);

			TestTrue(
				*FString::Printf(
					TEXT("%s 的 Hit Time %.3f 小于动画长度 %.3f"),
					*Prefix, Option.HitTime, Length),
				Option.HitTime < Length);

			AddInfo(FString::Printf(
				TEXT("%s: 动画=%s 长度=%.3f 倍率=%.2f HitTime=%.3f 恢复=%.3f 权重=%.2f 锁朝向=%d"),
				*Prefix,
				*Animation->GetName(),
				Length,
				Option.DamageMultiplier,
				Option.HitTime,
				Option.RecoveryTime,
				Option.Weight,
				Option.bLockFacing ? 1 : 0));
		}
	}

	// 6. 黑板必须具备守卫服务固定读写的两个键。
	UBlackboardData* Blackboard = Tree->GetBlackboardAsset();

	if (TestNotNull(TEXT("行为树指定了 Blackboard Asset"), Blackboard))
	{
		TArray<FName> KeyNames;
		for (const FBlackboardEntry& Entry : Blackboard->Keys)
		{
			KeyNames.Add(Entry.EntryName);
		}

		for (const FName Required : { TargetActorKey, HomeKey, ReturnKey })
		{
			TestTrue(
				*FString::Printf(
					TEXT("黑板存在键 %s"), *Required.ToString()),
				KeyNames.Contains(Required));
		}
	}

	// 7. 所有黑板装饰器引用的键都应该真实存在，避免改名后静默失效。
	TArray<FName> ReferencedKeys;
	CollectDecoratorKeys(Visits, ReferencedKeys);

	if (Blackboard)
	{
		for (const FName Key : ReferencedKeys)
		{
			TestTrue(
				*FString::Printf(
					TEXT("装饰器引用的键 %s 存在于黑板"), *Key.ToString()),
				Blackboard->GetKeyID(Key) != FBlackboard::InvalidKey);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShooterRegionMeleeGuardBlueprintTest,
	"ShooterSam.RegionGuards.MeleeBlueprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShooterRegionMeleeGuardBlueprintTest::RunTest(const FString& Parameters)
{
	using namespace ShooterRegionGuardTree;

	UBlueprintGeneratedClass* GuardClass = LoadObject<UBlueprintGeneratedClass>(
		nullptr, RegionGuardBlueprintPath);

	if (!TestNotNull(
		FString::Printf(TEXT("蓝图 %s 可以加载"), RegionGuardBlueprintPath),
		GuardClass))
	{
		return false;
	}

	AShooterMeleeEnemy* GuardDefaults =
		Cast<AShooterMeleeEnemy>(GuardClass->GetDefaultObject());

	if (!TestNotNull(TEXT("蓝图默认对象是 AShooterMeleeEnemy"), GuardDefaults))
	{
		return false;
	}

	// 区域近战必须换用带返岗逻辑的区域树，沿用精英树就不会返回岗位。
	TestNotNull(
		TEXT("蓝图设置了 Behavior Tree"),
		GuardDefaults->GetBehaviorTree());

	const FString ActualTree = GetNameSafe(
		GuardDefaults->GetBehaviorTree());

	TestEqual(
		TEXT("Behavior Tree 是 BT_RegionMeleeGuard"),
		ActualTree,
		FString(TEXT("BT_RegionMeleeGuard")));

	AddInfo(FString::Printf(TEXT("Behavior Tree = %s"), *ActualTree));

	// 与精英近战共用同一套数值，是「区域类型套用精英近战」的既定设计。
	UBehaviorTree* EliteTree = LoadObject<UBehaviorTree>(
		nullptr, EliteMeleeTreePath);

	if (TestNotNull(
		FString::Printf(TEXT("行为树 %s 可以加载"), EliteMeleeTreePath),
		EliteTree))
	{
		TestTrue(
			TEXT("区域树与精英树是两个不同资产"),
			EliteTree != GuardDefaults->GetBehaviorTree());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShooterRegionMeleeGuardSharesEliteStatsTest,
	"ShooterSam.RegionGuards.MeleeSharesEliteStats",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShooterRegionMeleeGuardSharesEliteStatsTest::RunTest(const FString& Parameters)
{
	using namespace ShooterRegionGuardTree;

	UBlueprintGeneratedClass* GuardClass = LoadObject<UBlueprintGeneratedClass>(
		nullptr, RegionGuardBlueprintPath);

	const TCHAR* const EliteBlueprintPath =
		TEXT("/Game/MyStuff/BluePrints/Enemies/BP_ShooterEliteMeleeEnemy.BP_ShooterEliteMeleeEnemy_C");

	UBlueprintGeneratedClass* EliteClass = LoadObject<UBlueprintGeneratedClass>(
		nullptr, EliteBlueprintPath);

	if (!TestNotNull(TEXT("区域近战蓝图可加载"), GuardClass) ||
		!TestNotNull(TEXT("精英近战蓝图可加载"), EliteClass))
	{
		return false;
	}

	AShooterMeleeEnemy* GuardDefaults =
		Cast<AShooterMeleeEnemy>(GuardClass->GetDefaultObject());

	AShooterMeleeEnemy* EliteDefaults =
		Cast<AShooterMeleeEnemy>(EliteClass->GetDefaultObject());

	if (!TestNotNull(TEXT("区域近战默认对象正确"), GuardDefaults) ||
		!TestNotNull(TEXT("精英近战默认对象正确"), EliteDefaults))
	{
		return false;
	}

	TestEqual(
		TEXT("近战攻击范围与精英一致"),
		GuardDefaults->GetMeleeAttackRange(),
		EliteDefaults->GetMeleeAttackRange());

	// 这是「区域类型直接套用精英近战」的显式记录：
	// 设计上两者共用数值，所以这里是相等断言；哪天要按区域缩放，先改这条测试。
	TestEqual(
		TEXT("最大生命与精英一致"),
		GuardDefaults->GetMaxHealth(),
		EliteDefaults->GetMaxHealth());

	AddInfo(FString::Printf(
		TEXT("区域近战: 生命=%.1f 攻击范围=%.1f；精英近战: 生命=%.1f 攻击范围=%.1f"),
		GuardDefaults->GetMaxHealth(),
		GuardDefaults->GetMeleeAttackRange(),
		EliteDefaults->GetMaxHealth(),
		EliteDefaults->GetMeleeAttackRange()));

	return true;
}
