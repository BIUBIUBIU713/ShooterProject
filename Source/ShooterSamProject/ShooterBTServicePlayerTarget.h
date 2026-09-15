// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "ShooterBTServicePlayerTarget.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERSAMPROJECT_API UShooterBTServicePlayerTarget : public UBTService
{
	GENERATED_BODY()
	
public:
	UShooterBTServicePlayerTarget();
	
protected:
	virtual void TickNode(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds
	) override;
};
