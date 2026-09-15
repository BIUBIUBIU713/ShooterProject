// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DiffResults.h"
#include "GameFramework/GameModeBase.h"
#include "ShooterSamProjectGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */

// 当前波次所属状态
UENUM(BlueprintType)
enum class EWaveState : uint8
{
	WaitingToStarting UMETA(DisplayName = "WaitingToStarting"),
	InProgress UMETA(DisplayName = "InProgress"),
	Intermission UMETA(DisplayName = "Intermission"),
	GameOver UMETA(DisplayName = "GameOver"),
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
	
protected:
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
};



