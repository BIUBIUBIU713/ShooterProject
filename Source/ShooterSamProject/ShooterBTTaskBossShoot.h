// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ShooterBTTaskBossShoot.generated.h"

class AShooterBossEnemy;

UCLASS()
class SHOOTERSAMPROJECT_API UShooterBTTaskBossShoot : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UShooterBTTaskBossShoot();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory
	) override;

	virtual void TickTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds
	) override;

	virtual void OnTaskFinished(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		EBTNodeResult::Type TaskResult
	) override;

private:
	UPROPERTY()
	TWeakObjectPtr<AShooterBossEnemy> CachedBoss;
};
