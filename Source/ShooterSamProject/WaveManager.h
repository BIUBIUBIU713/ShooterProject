// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "TimerManager.h"

#include "WaveManager.generated.h"

class AEnemySpawnPoint;
class AShooterSamProjectGameMode;
class AShooterEnemyBase;


// 从 StartWave 开始生效，直到下一阶段开始。
USTRUCT(BlueprintType)
struct FShooterWaveCompositionPhase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Wave", meta = (ClampMin = "1"))
    int32 StartWave = 1;

    // 先预留精英名额，再计算普通敌人中远程所占的百分比。
    UPROPERTY(EditAnywhere, Category = "Wave",
        meta = (ClampMin = "0", ClampMax = "100"))
    int32 RangedPercent = 25;

};

UCLASS()
class SHOOTERSAMPROJECT_API AWaveManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWaveManager();
	
	// 根据当前波次和可用出生点数量
	// 计算本波应该使用多少个出生点
	
	static int32 CalculateSpawnPointCountForWave(int32 WaveNumber, int32 AvailableSpawnPointCount);

	static TArray<int32> SelectUniqueSpawnPointIndices(int32 RequestedCount, int32 AvailableCount, FRandomStream& RandomStream);
	
	static int32 CalculateEnemyCountForWave(int32 WaveNumber);
	
	static bool ShouldPauseEnemySpawning(
		int32 SpawnedEnemyCount,
		int32 WaveEnemyTarget,
		int32 AliveEnemyCount,
		int32 MaxAliveEnemies
	);
	
	static bool ShouldResumeEnemySpawning(
		int32 SpawnedEnemyCount,
		int32 WaveEnemyTarget,
		int32 AliveEnemyCount,
		int32 ResumeSpawnThreshold
	);
	
	static bool IsEnemyWaveComplete(
		int32 SpawnedEnemyCount,
		int32 WaveEnemyTarget,
		int32 AliveEnemyCount
	);
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
private:
	//根据波次选择本波实际启用的出生点
	void SelectActiveSpawnPointsForWave(int32 WaveNumber);
	
	void InitializeWaveRuntimeState(int32 WaveNumber);
	
	//寻找当前地图使用的GameMode
	void FindGameMode();
	
	//收集地图中所有启用的出生点
	void CollectSpawnPoints();
	
	//登记一个已经成功生成的波次敌人
	bool RegisterWaveEnemy(AShooterEnemyBase* Enemy);
	
	//接收已经登记的敌人的死亡广播
	UFUNCTION()
	void HandleWaveEnemyDied(AShooterEnemyBase* DeadEnemy);
	
	//当前仍然存活，由本管理器登记的波次敌人
	UPROPERTY(VisibleInstanceOnly, Category = "Wave|Runtime")
	TArray<TObjectPtr<AShooterEnemyBase>> LivingWaveEnemies;
	
	//GameMode 开始新波次，由委托调用
	UFUNCTION()
	void HandleWaveStarted(int32 WaveNumber);
	
	//当前使用的GameMode
	UPROPERTY()
	TObjectPtr<AShooterSamProjectGameMode> ShooterGameMode;
	
	//地图中所有允许使用的出生点
	UPROPERTY()
	TArray<TObjectPtr<AEnemySpawnPoint>> SpawnPoints;
	
	//当前波次实际使用的出生点
	UPROPERTY()
	TArray<TObjectPtr<AEnemySpawnPoint>> ActiveSpawnPoints;
	
	//场上允许同时存活的最大波次敌人数
	UPROPERTY(
		EditInstanceOnly,
		Category = "Wave|Spawn Control",
		meta = (ClampMin = "1")
	)
	int32 MaxAliveEnemies = 12;
	
	//存活敌人降低到这个数量时恢复生成
	UPROPERTY(
		EditInstanceOnly,
		Category = "Wave|Spawn Control",
		meta = (ClampMin = "0")
	)
	int32 ResumeSpawnThreshold = 8;
	
	//相邻敌人之间生成间隔
	UPROPERTY(
		EditInstanceOnly,
		Category = "Wave|Spawn Control",
		meta = (ClampMin = "0.1")
	)
	float SpawnInterval = 0.75f;
	
	//当前波计划生成的敌人总数
	UPROPERTY(VisibleAnywhere, Category = "Wave|Runtime")
	int32 WaveEnemyTarget = 0;
	
	//当前波已经累计生成的敌人数
	UPROPERTY(VisibleInstanceOnly, Category = "Wave|Runtime")
	int32 SpawnedEnemyCount = 0;
	
	//当前仍然存活的波次敌人数
	UPROPERTY(visibleInstanceOnly, Category = "Wave|Runtime")
	int32 AliveEnemyCount = 0;
	
	//当前波已经被击败的敌人数
	UPROPERTY(VisibleInstanceOnly, Category = "Wave|Runtime")
	int32 DefeatedEnemyCount = 0;
	
	//生成计时器当前是否处于工作状态
	UPROPERTY(VisibleInstanceOnly, CateGory = "Wave|Runtime")
	bool bIsSpawning = false;
	
	//跨波次持续使用的随机值
	FRandomStream SpawnSelectionRandomStream;
	
	//为当前波次安排每一只敌人使用哪个出生点
	bool BuildSpawnQueue(int32 WaveNumber);
	
	//启动，停止生成计时器
	void StartEnemySpawning();
	
	UFUNCTION()
	void StopEnemySpawning();
	
	//每次计时器触发时，尝试生成一只敌人
	void SpawnNextEnemy();
	
    // 保留原属性名，避免地图中已经配置的普通远程蓝图引用丢失。
    UPROPERTY(EditInstanceOnly, Category = "Wave|Enemy Classes",
        meta = (DisplayName = "Ranged Enemy Class"))
    TSubclassOf<AShooterEnemyBase> EnemyClass;

    UPROPERTY(EditInstanceOnly, Category = "Wave|Enemy Classes")
    TSubclassOf<AShooterEnemyBase> MeleeEnemyClass;

    UPROPERTY(EditInstanceOnly, Category = "Wave|Enemy Classes")
    TSubclassOf<AShooterEnemyBase> EliteMeleeEnemyClass;

    UPROPERTY(EditInstanceOnly, Category = "Wave|Enemy Classes")
    TSubclassOf<AShooterEnemyBase> EliteRangedEnemyClass;

    UPROPERTY(EditInstanceOnly, Category = "Wave|Composition",
        meta = (TitleProperty = "StartWave"))
    TArray<FShooterWaveCompositionPhase> CompositionPhases;

    UPROPERTY(EditInstanceOnly, Category = "Wave|Elite Progression",
        meta = (ClampMin = "1"))
    int32 EliteStartWave = 21;

    // 精英比例基于整波总人数，而不是普通敌人数。
    UPROPERTY(EditInstanceOnly, Category = "Wave|Elite Progression",
        meta = (ClampMin = "0", ClampMax = "100"))
    int32 EliteStartPercent = 10;

    // 每回合增加的百分点，例如 10% -> 11%，不是乘以 1.01。
    UPROPERTY(EditInstanceOnly, Category = "Wave|Elite Progression",
        meta = (ClampMin = "0", ClampMax = "100"))
    int32 ElitePercentIncreasePerWave = 1;

    UPROPERTY(EditInstanceOnly, Category = "Wave|Elite Progression",
        meta = (ClampMin = "0", ClampMax = "100"))
    int32 EliteMaxPercent = 30;

    // 精英内部的远程比例，其余精英为近战。
    UPROPERTY(EditInstanceOnly, Category = "Wave|Elite Progression",
        meta = (ClampMin = "0", ClampMax = "100"))
    int32 EliteRangedPercent = 30;

    bool BuildEnemyClassQueue(int32 WaveNumber);

    // 和出生点队列共用 SpawnedEnemyCount 下标，一一对应。
    UPROPERTY()
    TArray<TSubclassOf<AShooterEnemyBase>> EnemyClassQueue;
	
	//每一个元素代表一只待生成敌人所使用的出生点
	//同一个出生点可以多次出现
	UPROPERTY()
	TArray<TObjectPtr<AEnemySpawnPoint>> SpawnQueue;
	
	//用于管理生成计时器
	FTimerHandle EnemySpawnTimerHandle;
};
