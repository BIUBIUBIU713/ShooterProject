#pragma once

#include "CoreMinimal.h"
#include "ShooterEnemyBase.h"
#include "ShooterMeleeEnemy.generated.h"

class AShooterSamProjectCharacter;

UCLASS()
class SHOOTERSAMPROJECT_API AShooterMeleeEnemy : public AShooterEnemyBase
{
	GENERATED_BODY()

public:
	AShooterMeleeEnemy();

	bool IsMeleeAttackReady() const;

	// 延长冷却，不会缩短已有冷却
	void ReserveMeleeCooldown(float Duration);

	// 只负责命中判定和伤害，调用时机由攻击 Task 控制
	bool TryApplyMeleeHit(
		AShooterSamProjectCharacter* Target,
		float DamageMultiplier
	);

	UFUNCTION(BlueprintPure, Category = "Enemy|Melee")
	float GetMeleeAttackRange() const
	{
		return MeleeAttackRange;
	}

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Enemy|Melee", meta = (ClampMin = "1.0"))
	float MeleeDamage = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "Enemy|Melee", meta = (ClampMin = "1.0"))
	float MeleeAttackRange = 150.0f;

private:
	double NextAllowedAttackTime = 0.0;
};