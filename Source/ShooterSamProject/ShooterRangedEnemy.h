// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ShooterEnemyBase.h"
#include "ShooterRangedEnemy.generated.h"

class AGun;
/**
 * 
 */
UCLASS()
class SHOOTERSAMPROJECT_API AShooterRangedEnemy : public AShooterEnemyBase
{
	GENERATED_BODY()
	
public:
	//尝试开一枪，true表示执行了开枪，不代表命中了目标
	bool TryFire();
	
protected:
	virtual void BeginPlay() override;
	
	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason	
	) override;
	
	//在蓝图中选择要生成哪一种枪
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Weapon")
	TSubclassOf<AGun> GunClass;
	
	//运行时实际生成的枪
	UPROPERTY(VisibleInstanceOnly, Category = "Enemy|Weapon")
	TObjectPtr<AGun> EquippedGun = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Weapon")
	FName WeaponSocketName = TEXT("WeaponSocket");
	
	UPROPERTY(
		EditDefaultsOnly,
		Category = "Enemy|Weapon",
		meta = (ClampMin = "0.01")
	)
	float FireInterval = 0.8f;
	
	
private:
	double NextAllowedFireTime = 0.0f;
	
};
