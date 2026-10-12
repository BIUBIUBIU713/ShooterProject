#include "ShooterBTDecoratorBossShootReady.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "ShooterBossEnemy.h"

UShooterBTDecoratorBossShootReady::UShooterBTDecoratorBossShootReady()
{
	NodeName = TEXT("Boss Ordinary Shoot Ready");
	INIT_DECORATOR_NODE_NOTIFY_FLAGS();
	bTickIntervals = true;
	FlowAbortMode = EBTFlowAbortMode::LowerPriority;
	// 开枪后会变为忙碌，不能因此中止自己的射击任务。
	bAllowAbortChildNodes = false;
}

bool UShooterBTDecoratorBossShootReady::CalculateRawConditionValue(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AAIController* AI = OwnerComp.GetAIOwner();
	const AShooterBossEnemy* Boss = AI ? Cast<AShooterBossEnemy>(AI->GetPawn()) : nullptr;
	return IsValid(Boss) && Boss->CanStartOrdinaryShoot();
}

void UShooterBTDecoratorBossShootReady::TickNode(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	SetNextTickTime(NodeMemory, 0.2f);
	// 条件包含忙碌检查：正在近战或特殊攻击时绝不抢占。
	ConditionalFlowAbort(OwnerComp, EBTDecoratorAbortRequest::ConditionResultChanged);
}
