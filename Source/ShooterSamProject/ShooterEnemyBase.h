// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShooterEnemyBase.generated.h"

class UBehaviorTree;
class AShooterEnemyBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnShooterEnemyDied,
	AShooterEnemyBase*,
	DeadEnemy
);

UCLASS(Abstract)
class SHOOTERSAMPROJECT_API AShooterEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AShooterEnemyBase();
	
	UPROPERTY(
		BlueprintAssignable,
		Category = "Enemy|Events"
	)
	FOnShooterEnemyDied OnEnemyDied;
	
	UFUNCTION(
		BlueprintPure,
		Category = "Enemy|State"
	)
	bool IsDead() const;
	
	UFUNCTION(
		BlueprintPure,
		Category = "Enemy|Stats"
	)
	float GetCurrentHealth() const;
	
	UFUNCTION(
		BlueprintPure,
		Category = "Enemy|Stats"
	)
	float GetMaxHealth() const;
	
	virtual float TakeDamage(
		float DamageAmount,
		const FDamageEvent& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser
	) override;
	
	UFUNCTION(BlueprintPure, Category = "Enemy|AI")
	UBehaviorTree* GetBehaviorTree() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Enemy|Stats",
		meta = (ClampMin = "1.0")
	)
	float MaxHealth = 100.0f;
	
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Enemy|Stats"
	)
	float CurrentHealth = 0.0f;
	
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Enemy|State"
	)
	bool bIsDead = false;
	
	//此类敌人使用的行为树资产
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Enemy|AI"
	)
	TObjectPtr<UBehaviorTree> BehaviorTree = nullptr; 
	
	//接受造成致死伤害的控制器，用于判断奖励归属
	void HandleDeath(AController* KillerController);
	
	//死亡后保留多久，再销毁敌人
	UPROPERTY(
		EditDefaultsOnly,
		Category = "Enemy|Death",
		meta = (ClampMin = "0.1")
	)
	float DeathLifeSpan = 1.5f;
	
	//玩家击杀此敌人获得的金币
	UPROPERTY(
		EditDefaultsOnly,
		Category = "Enemy|Reward",
		meta = (ClampMin = "0")
	)
	int32 KillRewardCoins = 10;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
