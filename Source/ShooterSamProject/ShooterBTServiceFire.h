// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_DefaultFocus.h"
#include "ShooterBTServiceFire.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERSAMPROJECT_API UShooterBTServiceFire : public UBTService_DefaultFocus
{
	GENERATED_BODY()
	
public:
	UShooterBTServiceFire(
		const FObjectInitializer& ObjectInitializer =
		 FObjectInitializer::Get()
	);
	
protected:
	virtual void TickNode(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds
	) override;
};
