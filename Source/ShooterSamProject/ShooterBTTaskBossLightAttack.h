// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ShooterBossEnemy.h"
#include "ShooterBTTaskBossLightAttack.generated.h"

/**
 * 
 */
class AShooterBossEnemy;
class UAnimMontage;

UCLASS()
class SHOOTERSAMPROJECT_API UShooterBTTaskBossLightAttack : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UShooterBTTaskBossLightAttack();
	
protected:
	// Keep this task class to preserve existing behavior-tree asset references.
	UPROPERTY(EditAnywhere, Category = "Boss|Attack")
	EBossMeleeAttack AttackType = EBossMeleeAttack::Light;

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
	
	UPROPERTY()
	TWeakObjectPtr<UAnimMontage> CachedMontage;
	
};
