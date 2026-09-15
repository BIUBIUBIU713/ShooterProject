// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_shoot.h"

#include "shootAI.h"

UBTTask_shoot::UBTTask_shoot()
{
	NodeName = TEXT("Shoot to Player");
}

EBTNodeResult::Type UBTTask_shoot::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::ExecuteTask(OwnerComp, NodeMemory);
	
	EBTNodeResult::Type Result = EBTNodeResult::Failed;
	
	AshootAI* OwnerController = Cast<AshootAI>(OwnerComp.GetAIOwner());
	if (OwnerController)
	{
		AShooterSamProjectCharacter* OwnerCharacter = OwnerController->MyCharacter;
		AShooterSamProjectCharacter* PlayerCharacter = OwnerController->PlayerCharacter;
		if (PlayerCharacter && OwnerCharacter && PlayerCharacter->IsAlive)
		{
			OwnerCharacter->Shoot();
			Result = EBTNodeResult::Succeeded;
		}
	}
	
	return Result;
}