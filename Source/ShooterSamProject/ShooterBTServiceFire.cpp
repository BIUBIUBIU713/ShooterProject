// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterBTServiceFire.h"

#include "AIController.h"
#include "ShooterRangedEnemy.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"

UShooterBTServiceFire::UShooterBTServiceFire(
	const FObjectInitializer& ObjectInitializer	
) : Super(ObjectInitializer)
{
	NodeName = TEXT("Focus And Fire");
	
	INIT_SERVICE_NODE_NOTIFY_FLAGS();
	
	//父类本来不需要定时Tick， 这里启用间隔谓度
	bTickIntervals = true;
	
	Interval = 0.1f;
	RandomDeviation = 0.0f;
	
	//等父类激活并设置好焦点后，再开始定时检查
	bCallTickOnSearchStart = false;
	
	//战斗朝向优先于路径移动朝向
	FocusPriority = EAIFocusPriority::Gameplay;
}

void UShooterBTServiceFire::TickNode(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds
)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	
	if (!IsValid(AIController) || !IsValid(BlackboardComp))
	{
		return;
	}
	
	AShooterRangedEnemy* Enemy = Cast<AShooterRangedEnemy>(AIController->GetPawn());
	
	if (!IsValid(Enemy))
	{
		return;
	}
	
	//使用父类提供的Blackboard Key配置
	APawn* TargetPawn = Cast<APawn>(
		BlackboardComp->GetValueAsObject(
			BlackboardKey.SelectedKeyName	
		)
	);
	
	//当前单人版本只向玩家Pawn开火
	if (!IsValid(TargetPawn) || !TargetPawn->IsPlayerControlled())
	{
		return;
	}
	
	const FName AttackRangedKey(TEXT("IsInAttackRange"));

	if (!BlackboardComp->GetValueAsBool(AttackRangedKey))
	{
		return;
	}
	
	if (!AIController->LineOfSightTo(TargetPawn))
	{
		return;
	}
	
	//避免焦点尚未建立，或被其他更高优先级逻辑替换时开枪
	if (AIController->GetFocusActor() != TargetPawn)
	{
		return;
	}
	
	//刷新射击使用的控制器视线，但不直接强制旋转角色身体
	AIController->UpdateControlRotation(DeltaSeconds, false);
	
	if (Enemy->TryFire())
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("%s fired at %s"),
			*GetNameSafe(Enemy),
			*GetNameSafe(TargetPawn)
		);
	}
	
}
