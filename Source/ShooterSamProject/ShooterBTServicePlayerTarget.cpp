// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterBTServicePlayerTarget.h"

#include "AIController.h"
#include "ShooterEnemyBase.h"
#include "ShooterSamProjectCharacter.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"

UShooterBTServicePlayerTarget::UShooterBTServicePlayerTarget()
{
	NodeName = TEXT("Update Player Target");
	
	INIT_SERVICE_NODE_NOTIFY_FLAGS();
	
	Interval = 0.2f;
	RandomDeviation = 0.0f;
	bCallTickOnSearchStart = true;
}

void UShooterBTServicePlayerTarget::TickNode(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds
)
{
	Super::TickNode(OwnerComp,NodeMemory,DeltaSeconds);
	
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	
	if (!IsValid(AIController) || !IsValid(BlackboardComp))
	{
		return;
	}
	
	const FName TargetKey(TEXT("TargetActor"));
	const FName SightKey(TEXT("HasLineOfSight"));
	const FName RangeKey(TEXT("IsInAttackRange"));
	
	UObject* PreviousTarget = BlackboardComp->GetValueAsObject(TargetKey);
	
	AShooterSamProjectCharacter* Player = 
		Cast<AShooterSamProjectCharacter>(PreviousTarget);
	
	//只有原目标不可用时，才重新查找玩家
	if (!IsValid(Player) || !Player->IsAlive || !Player->IsPlayerControlled())
	{
		Player = Cast<AShooterSamProjectCharacter>(UGameplayStatics::GetPlayerPawn(
				AIController->GetWorld(),
				0
			)
		);
	}
	
	//查找到的玩家也可能已经死亡或尚未准备好
	if (!IsValid(Player) || !Player->IsAlive || !Player->IsPlayerControlled())
	{
		Player = nullptr;
	}
	
	AShooterEnemyBase* Enemy = Cast<AShooterEnemyBase>(AIController->GetPawn());
	
	if (!IsValid(Enemy) || Enemy->IsDead() || Enemy->GetCurrentHealth() <= 0.0f)
	{
		Player = nullptr;
	}
	
	//目标对象真正变化时，才更新目标键并输出日志
	if (PreviousTarget != Player)
	{
		if (Player)
		{
			BlackboardComp->SetValueAsObject(TargetKey, Player);
		}
		else
		{
			BlackboardComp->ClearValue(TargetKey);
		}
		
		UE_LOG(
			LogTemp,
			Log,
			TEXT("%s target changed: %s -> %s"),
			*GetNameSafe(AIController->GetPawn()),
			*GetNameSafe(PreviousTarget),
			*GetNameSafe(Player)
		);
	}
	
	const bool bHasLineOfSight =
		IsValid(Player) && AIController->LineOfSightTo(Player);

	BlackboardComp->SetValueAsBool(SightKey, bHasLineOfSight);

	if (!IsValid(Player))
	{
		BlackboardComp->SetValueAsBool(RangeKey, false);
	}
}

