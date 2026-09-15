// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_PlayerLocation.h"

#include "shootAI.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTService_PlayerLocation::UBTService_PlayerLocation()
{
}

void UBTService_PlayerLocation::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	
	AshootAI* OwnerAIController = Cast<AshootAI>(OwnerComp.GetAIOwner());
	AShooterSamProjectCharacter* Player = OwnerAIController->PlayerCharacter;
	UBlackboardComponent* Blackboard = OwnerAIController->GetBlackboardComponent();
	
	if (OwnerAIController && Blackboard)
	{
		Blackboard->SetValueAsVector(GetSelectedBlackboardKey(), Player->GetActorLocation());
	}
}
