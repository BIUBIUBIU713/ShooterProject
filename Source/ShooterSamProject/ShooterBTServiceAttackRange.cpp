// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterBTServiceAttackRange.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "ShooterMeleeEnemy.h"

UShooterBTServiceAttackRange::UShooterBTServiceAttackRange()
{
	NodeName = TEXT("Update Attack Range");
	
	INIT_SERVICE_NODE_NOTIFY_FLAGS();
	
	Interval = 0.2f;
	RandomDeviation = 0.0f; 
	
	bCallTickOnSearchStart = true;
}

bool UShooterBTServiceAttackRange::IsDistanceInAttackRange(
	float Distance,
	float AttackRange
)
{
	return Distance <= AttackRange;
}

void UShooterBTServiceAttackRange::TickNode(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds
)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	
	UBlackboardComponent* BlackboardComp =
		OwnerComp.GetBlackboardComponent();
	
	if (!IsValid(BlackboardComp))
	{
		return;
	}
	
	const FName TargetActorKey(TEXT("TargetActor"));
	const FName AttackRangeKey(TEXT("IsInAttackRange"));
	
	//默认不在攻击范围内，避免无目标时保留上次的true
	bool bIsInAttackRange = false;
	
	AAIController* AIController = OwnerComp.GetAIOwner();
	
	APawn* ControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
	
	AActor* TargetActor = Cast<AActor>(
		BlackboardComp->GetValueAsObject(TargetActorKey)
	);
	
	if (IsValid(ControlledPawn) && IsValid(TargetActor))
	{
		const float Distance = 
			ControlledPawn->GetDistanceTo(TargetActor);
		
		float EffectiveAttackRange = AttackRange;
		
		if (const AShooterMeleeEnemy* MeleeEnemy = 
			Cast<AShooterMeleeEnemy>(ControlledPawn)	
		)
		{
			EffectiveAttackRange = MeleeEnemy->GetMeleeAttackRange();
		}
		
		bIsInAttackRange = IsDistanceInAttackRange(
			Distance,
			EffectiveAttackRange
		);
	}
	
	BlackboardComp->SetValueAsBool(
		AttackRangeKey,
		bIsInAttackRange
	);
}