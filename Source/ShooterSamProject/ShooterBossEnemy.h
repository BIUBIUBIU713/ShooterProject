// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ShooterEnemyBase.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "TimerManager.h"

#include "ShooterBossEnemy.generated.h"

/**
 * 
 */
class UAnimMontage;
class UBehaviorTreeComponent;
class AShooterSamProjectCharacter;

UENUM(BlueprintType)
enum class EBossMeleeAttack : uint8
{
	Light = 0,
	Heavy = 1,
	Random = 2,
	Stomp = 3,
	GroundSmash = 4,
	Charge = 5
};

UCLASS()
class SHOOTERSAMPROJECT_API AShooterBossEnemy : public AShooterEnemyBase
{
	GENERATED_BODY()
	
public:
	AShooterBossEnemy();

	// 正式关卡勾选，独立战斗测试地图仍可自动入场。
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Boss|Encounter")
	bool bWaitForEncounterSwitch = false;

	bool CanStartEncounter() const;
	bool StartEncounterEntrance();
	void CancelEncounter();

	// 只接在现有入场蒙太奇的 On Completed 上，成功启动 AI 后才开放受伤。
	UFUNCTION(BlueprintCallable, Category = "Boss|Encounter")
	bool FinishEncounterEntrance(UBehaviorTree* EntranceBehaviorTree);

	UFUNCTION(BlueprintPure, Category = "Boss|Encounter")
	bool IsCombatReady() const { return bCombatReady; }
	
	//成功开始播放时返回true
	UFUNCTION(BlueprintCallable, Category = "Boss|Attack")
	bool TryStartLightAttack();
	
	//动作播放或恢复期间，不允许开始下一招
	UFUNCTION(BlueprintPure, Category = "Boss|Attack")
	bool IsLightAttackBusy() const;

	UFUNCTION(BlueprintCallable, Category = "Boss|Attack")
	bool TryStartMeleeAttack(EBossMeleeAttack AttackType);

	UFUNCTION(BlueprintPure, Category = "Boss|Attack")
	bool IsMeleeAttackBusy() const;
	
	//尝试开始一轮普通射击
	UFUNCTION(BlueprintCallable, Category = "Boss|Attack")
	bool TryStartShoot();

	// 普通射击独立选招条件，供行为树观察；不改变特殊扫射的距离规则。
	bool CanStartOrdinaryShoot() const;
	bool IsDistanceInOrdinaryShootRange(float Distance) const;
	
	//主动停止射击，同时停止射击蒙太奇
	UFUNCTION(BlueprintCallable, Category = "Boss|Attack")
	void StopShoot();
	
	//统一判断近战，射击以及攻击恢复期间是否忙碌
	UFUNCTION(BlueprintPure, Category = "Boss|Attack")
	bool IsAttackBusy() const;

	// 行为树负责后续概率选招；首次半血由 TakeDamage 立即抢占。
	UFUNCTION(BlueprintCallable, Category = "Boss|Special Attack")
	bool TryStartSpecialAttack();

	UFUNCTION(BlueprintCallable, Category = "Boss|Special Attack")
	void StopSpecialAttack();

	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;
	virtual void Tick(float DeltaSeconds) override;
	
protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Encounter")
	void PlayEncounterEntrance();

	virtual void OnDeathStarted() override;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Death")
	TObjectPtr<UAnimMontage> DeathMontage = nullptr;
	
	//离开世界时清理射击定时器
	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason	
	) override;
	
	//Boss轻击使用的蒙太奇
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Light Attack")
	TObjectPtr<UAnimMontage> LightAttackMontage = nullptr;
	
	//单次轻击伤害
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Light Attack", meta = (ClampMin = "0.0"))
	float LightAttackDamage = 20.0f;
	
	//攻击距离，单位为厘米：后续结合实际模型尺寸调整
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Light Attack", meta = (ClampMin = "1.0", Units = "cm"))
	float LightAttackRange = 500.0f;
	
	//蒙太奇结束后的额外恢复时间，单位为秒
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Light Attack", meta = (ClampMin = "0.0", Units = "s"))
	float LightAttackRecoveryTime = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Heavy Attack")
	TObjectPtr<UAnimMontage> HeavyAttackMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Heavy Attack", meta = (ClampMin = "0.0"))
	float HeavyAttackDamage = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Heavy Attack", meta = (ClampMin = "1.0", Units = "cm"))
	float HeavyAttackRange = 500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Heavy Attack", meta = (ClampMin = "0.0", Units = "s"))
	float HeavyAttackRecoveryTime = 1.0f;

	// Per-move cooldown starts when its montage ends; recovery still blocks both moves.
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Light Attack", meta = (ClampMin = "0.0", Units = "s"))
	float LightAttackCooldown = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Heavy Attack", meta = (ClampMin = "0.0", Units = "s"))
	float HeavyAttackCooldown = 6.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack Selection", meta = (ClampMin = "0.0"))
	float LightAttackWeight = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack Selection", meta = (ClampMin = "0.0"))
	float HeavyAttackWeight = 1.0f;

	// Keep repeats possible, but less likely when both attacks are eligible.
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack Selection", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float RepeatAttackWeightMultiplier = 0.25f;

	//践踏使用的蒙太奇
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Stomp Attack")
	TObjectPtr<UAnimMontage> StompAttackMontage = nullptr;
	
	//单次践踏伤害
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Stomp Attack", meta = (ClampMin = "0.0"))
	float StompAttackDamage = 60.0f;
	
	// 命中距离，单位：厘米
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Stomp Attack",
		meta = (ClampMin = "1.0", Units = "cm"))
	float StompAttackRange = 500.0f;
	
	//蒙太奇结束后，额外等待多久才能继续攻击
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Stomp Attack", meta = (ClampMin = "0.0", Units = "s"))
	float StompAttackRecoveryTime = 0.75f;
	
	//蒙太奇结束后，多久才能再次使用践踏
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Stomp Attack", meta = (ClampMin = "0.0", Units = "s"))
	float StompAttackCooldown = 4.0f;
	
	// 践踏参与随机选招时的基础权重
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack Selection", meta = (ClampMin = "0.0"))
	float StompAttackWeight = 1.0f;
	
	//左手砸地蒙太奇
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Ground Smash")
	TObjectPtr<UAnimMontage> GroundSmashMontage = nullptr;
	
	//单次伤害
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Ground Smash", meta = (ClampMin = "0.0"))
	float GroundSmashDamage = 70.0f;
	
	//命中距离，单位：厘米
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Ground Smash", meta = (ClampMin = "1.0", Units = "cm"))
	float GroundSmashRange = 500.0f;
	
	//动作结束后，所有攻击需要等待的恢复时间
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Ground Smash", meta = (ClampMin = "0.0", Units = "s"))
	float GroundSmashRecoveryTime = 0.75f;
	
	//动作结束后，这一招的独立冷却
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Ground Smash", meta = (ClampMin = "0.0", Units = "s"))
	float GroundSmashCooldown = 5.0f;
	
	//随机选招的基础权重
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack Selection", meta = (ClampMin = "0.0"))
	float GroundSmashWeight = 1.0f;
	
	//前冲攻击蒙太奇
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Charge")
	TObjectPtr<UAnimMontage> ChargeMontage = nullptr;
	
	//单次前冲伤害
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Charge", meta = (ClampMin = "0.0"))
	float ChargeDamage = 80.0f;
	
	//命中是允许的目标距离：不是前冲移动距离
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Charge", meta = (ClampMin = "1.0", Units = "cm"))
	float ChargeRange = 500.0f;
	
	//动作结束后，所有攻击需要等待的恢复时间
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Charge", meta = (ClampMin = "0.0", Units = "s"))
	float ChargeRecoveryTime = 1.0f;
	
	//动作结束后，前冲攻击的独立冷却
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Charge", meta = (ClampMin = "0.0", Units = "s"))
	float ChargeCooldown = 6.0f;
	
	//前冲参与随机选招时的基础权重
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Attack Selection", meta = (ClampMin = "0.0"))
	float ChargeWeight = 1.0f;
	
	//普通射击使用的循环蒙太奇
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Shoot")
	TObjectPtr<UAnimMontage> ShootMontage = nullptr;
	
	//每轮持续射击的时间
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Shoot", meta = (ClampMin = "0.01", Units = "s"))
	float ShootDuration = 2.0f;
	
	//两次开火的间隔
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Shoot", meta = (ClampMin = "0.01", Units = "s"))
	float ShootInterval = 0.25f;
	
	//每发命中的伤害
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Shoot", meta = (ClampMin = "0.0"))
	float ShootDamage = 10.0f;
	
	//普通射击的最大距离
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Shoot", meta = (ClampMin = "1.0", Units = "cm"))
	float ShootRange = 3000.0f;

	// 进入近战范围后不再使用普通射击；下界不包含此值。
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Shoot", meta = (ClampMin = "0.0", Units = "cm"))
	float ShootMinRange = 500.0f;
	
	//射击动作结束后的恢复时间
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Shoot", meta = (ClampMin = "0.0", Units = "s"))
	float ShootRecoveryTime = 0.5f;
	
	//本轮结束后，再次使用普通射击需要等待的时间
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Shoot", meta = (ClampMin = "0.0", Units = "s"))
	float ShootCooldown = 5.0f;
	
	//Boss右手机枪的枪口插槽，稍后在谷歌编辑器中配置
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Shoot")
	FName ShootMuzzleSocketName = TEXT("BossMuzzle");

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Special Attack")
	TObjectPtr<UAnimMontage> RoarMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Special Attack", meta = (ClampMin = "0.01", Units = "s"))
	float SpecialShootDuration = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Special Attack", meta = (ClampMin = "0.01", Units = "s"))
	float SpecialShootInterval = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Special Attack", meta = (ClampMin = "0.0"))
	float SpecialShootDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Special Attack", meta = (ClampMin = "1.0", Units = "cm"))
	float SpecialShootRange = 3000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Special Attack", meta = (ClampMin = "0.0", Units = "s"))
	float SpecialRecoveryTime = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Special Attack", meta = (ClampMin = "0.0", Units = "s"))
	float SpecialCooldown = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Special Attack", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SpecialAttackChance = 0.25f;

	// 每秒最多转动的角度，仅用于特殊扫射。
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Special Attack", meta = (ClampMin = "0.0"))
	float SpecialTurnSpeed = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Enrage", meta = (ClampMin = "1.0"))
	float EnrageMoveSpeedMultiplier = 1.2f;

	// 缩短普通攻击的收招、独立冷却和普通射击的发射间隔，不加速动画。
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Enrage", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float EnrageAttackIntervalMultiplier = 0.8f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Boss|Enrage")
	bool bEnraged = false;
	
private:
	friend class FShooterBossEncounterTest;
	bool bEntranceStarted = false;
	bool bCombatReady = false;
	bool bEncounterCancelled = false;
	void BeginEntrance();
	void ClearCombatForDeathOrEndPlay();
	bool bEndingPlay = false;
	friend class FShooterBossDeathTest;
	friend class FShooterBossShootRangeTest;
	bool StartSpecialAttack(bool bInterruptCurrentAction, AShooterSamProjectCharacter* DamageTarget = nullptr);
	void InterruptForFirstSpecialAttack();
	void RestartBehaviorTreeAfterSpecial();
	// 只恢复这次半血抢占主动停止的行为树，等特殊攻击收招结束再恢复。
	TWeakObjectPtr<UBehaviorTreeComponent> InterruptedBehaviorTree;

	bool StartShoot(bool bSpecial);
	bool HasValidShootSettings(bool bSpecial) const;
	float GetOrdinaryIntervalMultiplier() const;
	void ApplyEnrage();
	void HandleRoarEnd(UAnimMontage* Montage, bool bInterrupted);
	void MarkSpecialShootStopped();
	void FinishSpecialAttack();
	void RestoreSpecialRotation();

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveRoarMontage = nullptr;
	UPROPERTY()
	TWeakObjectPtr<AShooterSamProjectCharacter> SpecialTarget;
	bool bHalfHealthTriggered = false;
	bool bFirstSpecialPending = false;
	bool bSpecialActive = false;
	bool bRoaring = false;
	bool bSpecialStopping = false;
	bool bSpecialSelectionRolled = false;
	bool bSpecialShootStopped = false;
	bool bSpecialRotationSaved = false;
	bool bSavedUseControllerRotationYaw = false;
	bool bSavedOrientRotationToMovement = false;
	bool bSavedUseControllerDesiredRotation = false;
	int32 RoarInstanceId = INDEX_NONE;
	double NextSpecialReadyTime = 0.0;
	float ActiveShootDamage = 0.0f;
	float ActiveShootRange = 0.0f;
	float ActiveShootRecovery = 0.0f;
	float ActiveShootCooldown = 0.0f;

	//一次读取某一招的配置
	struct FMeleeAttackSettings
	{
		UAnimMontage* Montage = nullptr;
		float Damage = 0.0f;
		float Range = 0.0f;
		float Recovery = 0.0f;
		float Cooldown = 0.0f;
		double ReadyTime = 0.0;
		FName HitNotify = NAME_None;
	};
	
	FMeleeAttackSettings GetMeleeAttackSettings(
		EBossMeleeAttack AttackType	
	) const;
	
	bool IsMeleeAttackAvailable(EBossMeleeAttack AttackType, float Distance, double Now) const;

	// 一招期间固定目标，命中时重新检查是否仍在范围内。
	UPROPERTY()
	TWeakObjectPtr<AShooterSamProjectCharacter> AttackTarget;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveAttackMontage = nullptr;

	UFUNCTION()
	void HandleAttackNotify(
		FName NotifyName,
		const FBranchingPointNotifyPayload& Payload
	);
	
	void HandleAttackEnd(
		UAnimMontage* Montage,
		bool bInterrupted
	);
	
	bool bAttackPlaying = false;
	bool bHitHandled = false;
	
	int32 AttackInstanceId = INDEX_NONE;
	double NextAttackTime = 0.0;
	EBossMeleeAttack ActiveAttackType = EBossMeleeAttack::Light;
	FName ExpectedHitNotify;
	float ActiveDamage = 0.0f;
	float ActiveRange = 0.0f;
	float ActiveRecoveryTime = 0.0f;
	float ActiveCooldown = 0.0f;
	
	double NextLightReadyTime = 0.0;
	double NextHeavyReadyTime = 0.0;
	
	//游戏时间到达这个时刻后，践踏才结束冷却
	double NextStompReadyTime = 0.0;
	
	//游戏时间到达这个时刻后，左手砸地才可使用
	double NextGroundSmashReadyTime = 0.0;
	
	//游戏时间到达这个时刻后，前冲攻击才可正常使用
	double NextChargeReadyTime = 0.0;
	
	bool bHasPreviousAttack = false;
	
	//执行一次开火，由循环定时器调用
	void FireShootRound();
	
	//蒙太奇开始淡出时，立即停止继续开火
	void HandleShootBlendingOut(
		UAnimMontage* Montage,
		bool bInterrupted
	);
	
	//蒙太奇完全结束时，接触忙碌并计算恢复与冷却
	void HandleShootEnd(
		UAnimMontage* Montage,
		bool bInterrupted
	);
	
	//控制连续开火的定时器
	FTimerHandle ShootFireTimerHandle;
	
	//控制本轮射击持续时间的定时器
	FTimerHandle ShootDurationTimerHandle;
	
	//本轮射击固定的目标
	UPROPERTY()
	TWeakObjectPtr<AShooterSamProjectCharacter> ShootTarget;
	
	//本轮实际播放的射击蒙太奇
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveShootMontage = nullptr;
	
	//从射击开始到蒙太奇完全结束，都保持为true
	bool bShooting = false;
	
	//已进入停止或淡出阶段，不再允许开火
	bool bShootStopping = false;
	
	//用于确认当前播放的仍是本轮蒙太奇实例
	int32 ShootInstanceId = INDEX_NONE;
	
	//本轮射击截止的游戏时间
	double ShootEndTime = 0.0;
	
	//下一轮普通射击可开始的游戏时间
	double NextShootReadyTime = 0.0;
};
