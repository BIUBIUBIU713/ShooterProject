// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_shoot.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERSAMPROJECT_API UBTTask_shoot : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_shoot();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
};
