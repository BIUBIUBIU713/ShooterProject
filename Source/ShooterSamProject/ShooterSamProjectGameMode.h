// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DiffResults.h"
#include "NiagaraStatelessDefinitions.h"
#include "GameFramework/GameModeBase.h"
#include "ShooterSamProjectGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */

class AShooterSamProjectCharacter;

// 当前波次所属状态
UENUM(BlueprintType)
enum class EWaveState : uint8
{
	WaitingToStarting UMETA(DisplayName = "WaitingToStarting"),
	InProgress UMETA(DisplayName = "InProgress"),
	Intermission UMETA(DisplayName = "Intermission"),
	GameOver UMETA(DisplayName = "GameOver"),
};

UENUM(BlueprintType)
enum class EPlayerUpgradeType : uint8
{
	Reload UMETA(DisplayName = "快速换弹"),
	Movement UMETA(DisplayName = "灵活移动")
};

UENUM(BlueprintType)
enum class EUpgradePurchaseResult : uint8
{
	Success UMETA(DisplayName = "购买成功"),
	Unavailable UMETA(DisplayName = "当前无法购买"),
	PowerOff UMETA(DisplayName = "尚未通电"),
	StationLocked UMETA(DisplayName = "增益站尚未解锁"),
	MaxLevelReached UMETA(DisplayName = "已达到最高等级"),
	InsufficientCoins UMETA(DisplayName = "金币不足"),
	InvalidConfiguration UMETA(DisplayName = "增益配置无效")
};

//波次开始时，广播发送当前状态
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveStarted, int32, WaveNumber);

//波次结束事件
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveEnded, int32, WaveNumber);

//存货敌人人数变化
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAliveEnemyCountChanged, int32, AliveEnemyCount);

//GameOver
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRunGameOver);

UCLASS(abstract)
class AShooterSamProjectGameMode : public AGameModeBase
{
	GENERATED_BODY()
	

public:
	
	/** Constructor */
	AShooterSamProjectGameMode();
	
	//领取兵营中的通电道具
	UFUNCTION(BlueprintCallable, Category = "Progression")
	bool TryCollectPowerPart();
	
	//使用道具恢复供电
	UFUNCTION(BlueprintCallable, Category = "Progression")
	bool TryActivatePower();
	
	//当前是否持有通电道具
	UFUNCTION(BlueprintPure, Category = "Progression")
	bool HasPowerPart() const
	{
		return bHasPowerPart;
	}
	
	//当前是否已经通电
	UFUNCTION(BlueprintCallable, Category = "Progression")
	bool IsPowerOn() const
	{
		return bPowerOn;
	}
	
	//开始下一波
	UFUNCTION(BlueprintCallable, Category = "Wave")
	void StartNextWave();
	
	//生成一个敌人时的调用
	UFUNCTION(BlueprintCallable, Category = "Wave")
	void RegisterSpawnedEnemy();
	
	//每有一个敌人死亡时调用
	UFUNCTION(BlueprintCallable, Category = "Wave")
	void NotifyEnemyDied();
	
	//WaveManager 完成本波全部敌人的生成后调用
	UFUNCTION(BlueprintCallable, Category = "Wave")
	void NotifyWaveSpawningFinished();
	
	//玩家撤离失败时调用
	UFUNCTION(BlueprintCallable, Category = "Wave")
	void SetGameOver();
	
	//返回当前等级，无效类型返回-1
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	int32 GetUpgradeLevel(EPlayerUpgradeType UpgradeType) const;
	
	//返回下一次升级费用：无法升级时返回-1
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	int32 GetUpgradeCost(EPlayerUpgradeType UpgradeType) const;
	
	// 充能耗时倍率：1 表示原时长，0.8 表示原时长的 80%
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	float GetReloadDurationMultiplier() const;

	// 移动速度倍率
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	float GetMovementSpeedMultiplier() const;
	
	UFUNCTION(BlueprintPure, Category = "Progression")
	bool IsRegionUnlocked(FName RegionId) const;
	
	//尝试购买一级增益
	UFUNCTION(BlueprintCallable, Category = "Upgrades")
	EUpgradePurchaseResult TryPurchaseUpgrade(
		AShooterSamProjectCharacter* Player,
		EPlayerUpgradeType UpgradeType
	);
	
	UFUNCTION(BlueprintPure, Category = "Upgrades")
	bool IsUpgradeStationUnlocked() const
	{
		return IsRegionUnlocked(UpgradeStationRegionId);
	}
	
	//当前波次
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wave")
	int32 CurrentWave = 0;
	
	//当前存活敌人数
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wave")
	int32 AliveEnemyCount = 0;
	
	//当前波次状态
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wave")
	EWaveState WaveState = EWaveState::WaitingToStarting;
	
	//游戏开始后多久进入第一波
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wave")
	float FirstWaveDelay = 3.0f;
	
	//两个波次之间的休息时间
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wave")
	float IntermissionDuration = 5.0f;
	
	//波次开始委托
	UPROPERTY(BlueprintAssignable, Category = "Wave|Events")
	FOnWaveStarted OnWaveStarted;
	
	//波次结束委托
	UPROPERTY(BlueprintAssignable, Category = "Wave|Events")
	FOnWaveEnded OnWaveEnded;
	
	//敌人数量变化委托
	UPROPERTY(BlueprintAssignable, Category = "Wave|Events")
	FOnAliveEnemyCountChanged OnAliveEnemyCountChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "Wave|Events")
	FOnRunGameOver OnGameOver;
	
	//区域门成功解锁后调用
	void RecordRegionUnlocked(FName RegionId);
	
protected:
	//受此购买快速换弹的价格
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrades", meta = (ClampMin = "1"))
	int32 ReloadBaseCost = 100;
	
	//首次购买灵活移动的价格
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrades", meta = (ClampMin = "1"))
	int32 MovementBaseCost = 100;
	
	//两种增益各自的最高等级
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrades", meta = (ClampMin = "1"))
	int32 MaxUpgradeLevel = 5;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrades", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReloadSpeedBonusPerLevel = 0.25f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrades", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MovementSpeedBonusPerLevel = 0.1f;
	
	//必须与场景中增益站门的DoorGroupId一致
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Upgrades")
	FName UpgradeStationRegionId = FName(TEXT("UpgradeStation"));
	
	virtual void BeginPlay() override;

	//当前波次敌人全部死亡后执行
	void EndCurrentWave();
	
	//暂时保留：启动地图中手动放置的旧AI
	void InitializePlacedEnemies();
	
private:
	FTimerHandle WaveTimerHandle;
	
	//表示本波生成任务已全部完成，不是“暂时暂停生成”
	UPROPERTY(
		VisibleInstanceOnly,
		Category = "Wave",
		meta = (AllowPRivateAccess = "true")
	)
	bool bWaveSpawningFinished = false;
	
	//当前是否持有通电道具
	UPROPERTY(VisibleInstanceOnly, Category = "Progression")
	bool bHasPowerPart = false;
	
	//本局是否已经恢复供电
	UPROPERTY(VisibleInstanceOnly, Category = "Progression")
	bool bPowerOn = false;
	
	//本局快速换弹等级，0 表示未购买
	UPROPERTY(VisibleInstanceOnly, Category = "Upgrades")
	int32 ReloadUpgradeLevel = 0;
	
	//本局灵活移动等级，0 表示购买
	UPROPERTY(VisibleInstanceOnly, Category = "Upgrades")
	int32 MovementUpgradeLevel = 0;
	
	//仅记录当前对局的解锁状态
	UPROPERTY(VisibleInstanceOnly, Category = "Progression")
	TSet<FName> UnlockedRegions;
};



