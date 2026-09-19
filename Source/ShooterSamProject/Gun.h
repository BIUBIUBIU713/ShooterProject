// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "TimerManager.h"

#include "Gun.generated.h"

UCLASS()
class SHOOTERSAMPROJECT_API AGun : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGun();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	//是否启用能量消耗，由每种武器的蓝图进行分配
	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Energy")
	bool bUsesEnergy = false;
	
	//最大能量，当前每枪消耗一点，因此也代表满能量能开几枪
	UPROPERTY(
		EditDefaultsOnly,
		Category = "Weapon|Energy",
		meta = (ClampMin = "1")
	)
	int32 MaxEnergy = 20;
	
	//运行时的剩余能量
	UPROPERTY(
		VisibleInstanceOnly,
		Category = "Weapon|Energy"
	)
	int32 CurrentEnergy = 0;
	
	//能量耗尽后，需要等待的充能时间，单位为秒
	UPROPERTY(
		EditDefaultsOnly,
		Category = "Weapon|Energy",
		meta = (ClampMin = "0.1")
	)
	float RechargeDuration = 2.5f;
	
	//当前是否正在充能
	UPROPERTY(
		VisibleInstanceOnly,
		Category = "Weapon|Energy"
	)
	bool bIsRecharging = false;
	
	//武器离开世界时清理充能计时器
	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason	
	) override;
	
	//计时结束后恢复能量
	void FinishRecharge();
	
	//用于管理本武器的充能计时器
	FTimerHandle RechargeTimerHandle;
	

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(VisibleAnywhere)
	USceneComponent* SceneRoot;
	
	UPROPERTY(VisibleAnywhere)
	USkeletalMeshComponent* SkeletalMesh;
	
	UPROPERTY(EditAnywhere)
	float MaxRange = 10000.0f;
	
	//射击散步半角，单位为度
	//0 表示没有随机便宜，数值越大，射击越不精准
	UPROPERTY(
		EditDefaultsOnly,
		Category = "Weapon|Accuracy",
		meta = (ClampMin = "0.0", ClampMax = "45.0")
	)
	float SpreadHalfAngleDegrees = 0.0f;
	
	UPROPERTY(VisibleAnywhere)
	UNiagaraComponent* MuzzleFlashParticleSystem;
	
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* ImpactParticleSystem;
	
	UPROPERTY(EditAnywhere)
	float BulletDamage = 10.0f;
	
	UPROPERTY(EditAnywhere)
	USoundBase* ImpactSound;
	
	UPROPERTY(EditAnywhere)
	USoundBase* ShootSound;
	
	UPROPERTY()
	AController* OwnerController = nullptr;
	
	//返回true， 表示实际开了一枪， false表示本次开火被拒绝
	bool PullTrigger();
	
	//尝试开始充能，由玩家检查并支付电池
	//返回true，表示本次成功启动充能
	bool TryStartRecharge();
	
	UFUNCTION(BlueprintPure, Category = "Weapon|Energy")
	bool UsesEnergy() const
	{
		return bUsesEnergy;
	}
	
	UFUNCTION(BlueprintPure, Category = "Weapon|Energy")
	int32 GetCurrentEnergy() const
	{
		return CurrentEnergy;
	}
	
	UFUNCTION(BlueprintPure, Category = "Weapon|Energy")
	int32 GetMaxEnergy() const
	{
		return MaxEnergy;
	}
	
	UFUNCTION(BlueprintPure, Category = "Weapon|Energy")
	bool IsRecharging() const
	{
		return bIsRecharging;
	}
	
	UFUNCTION(BlueprintPure, Category = "Weapon|Energy")
	float GetEffectiveRechargeDuration() const;

};
