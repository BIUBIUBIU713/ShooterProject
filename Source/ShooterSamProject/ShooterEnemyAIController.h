// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"

#include "ShooterEnemyAIController.generated.h"

class AShooterEnemyBase;
class UAISenseConfig_Sight;

/**
 * 
 */
UCLASS()
class SHOOTERSAMPROJECT_API AShooterEnemyAIController : public AAIController
{
	GENERATED_BODY()
	
private:
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Enemy|AI",
		meta = (AllowPrivateAccess = "true")
	)
	TObjectPtr<AShooterEnemyBase> ControlledEnemy = nullptr;
	
	//保存视觉感知位置
	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	
protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	
public:
	AShooterEnemyAIController();
};
