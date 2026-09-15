// Fill out your copyright notice in the Description page of Project Settings.


#include "WaveManager.h"

#include "EnemySpawnPoint.h"
#include "ShooterSamProjectGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "Math/RandomStream.h"
#include "ShooterEnemyBase.h"

#include "Engine/World.h"
#include "TimerManager.h"


// Sets default values
AWaveManager::AWaveManager()
{
 	//WaveManager不需要每一帧都运行代码
	PrimaryActorTick.bCanEverTick = false;

}

//测试代码
int32 AWaveManager::CalculateSpawnPointCountForWave(int32 WaveNumber, int32 AvailableSpawnPointCount)
{
	//非法波次或没有可用出生点
	if (WaveNumber < 1 || AvailableSpawnPointCount < 1)
	{
		return 0;
	}
	
	//第一波展示所有出生路线
	if (WaveNumber == 1)
	{
		return AvailableSpawnPointCount;
	}
	
	//第二，三波使用两个出生点
	if (WaveNumber <= 3)
	{
		return FMath::Min(2, AvailableSpawnPointCount);
	}
	
	//第四，五波使用三个出生点
	if (WaveNumber <= 5)
	{
		return FMath::Min(3, AvailableSpawnPointCount);
	}
	
	//第六，七波使用四个出生点
	if (WaveNumber <= 7)
	{
		return FMath::Min(4, AvailableSpawnPointCount);
	}
	
	//第八波以后使用所有出生点
	return AvailableSpawnPointCount;
}

int32 AWaveManager::CalculateEnemyCountForWave(int32 WaveNumber)
{
	if (WaveNumber < 1)
	{
		return 0;
	}
	
	constexpr int32 BaseEnemyCount = 8;
	constexpr int32 EarlyWaveIncrease = 4;
	constexpr int32 LateWaveIncrease = 2;
	constexpr int32 FirstLateGrowthWave = 15;
	constexpr int32 LastEarlyGrowthWave = FirstLateGrowthWave - 1;
	
	if (WaveNumber < FirstLateGrowthWave)
	{
		return BaseEnemyCount + (WaveNumber - 1) * EarlyWaveIncrease;
	}
	
	constexpr int32 EnemyCountAtLastEarlyWave = BaseEnemyCount + (LastEarlyGrowthWave - 1) * EarlyWaveIncrease;
	
	return EnemyCountAtLastEarlyWave + (WaveNumber - LastEarlyGrowthWave) * LateWaveIncrease;
}

bool AWaveManager::ShouldPauseEnemySpawning(
	int32 SpawnedEnemyCount,
	int32 WaveEnemyTarget,
	int32 AliveEnemyCount,
	int32 MaxAliveEnemies
)
{
	return SpawnedEnemyCount >= WaveEnemyTarget || AliveEnemyCount >= MaxAliveEnemies;
}

bool AWaveManager::ShouldResumeEnemySpawning(
	int32 SpawnedEnemyCount,
	int32 WaveEnemyTarget,
	int32 AliveEnemyCount,
	int32 ResumeSpawnThreshold
)
{
	return SpawnedEnemyCount < WaveEnemyTarget && AliveEnemyCount <= ResumeSpawnThreshold;
}

bool AWaveManager::IsEnemyWaveComplete(
	int32 SpawnedEnemyCount,
	int32 WaveEnemyTarget,
	int32 AliveEnemyCount
)
{
	return SpawnedEnemyCount >= WaveEnemyTarget && AliveEnemyCount <= 0;
}

TArray<int32> AWaveManager::SelectUniqueSpawnPointIndices(int32 RequestedCount, int32 AvailableCount, FRandomStream& RandomStream)
{
	TArray<int32> SelectedIndices;
	
	if (RequestedCount <= 0 || AvailableCount <= 0)
	{
		return SelectedIndices;
	}
	
	const int32 SelectionCount = FMath::Min(RequestedCount, AvailableCount);
	
	TArray<int32> CandidateIndices;
	CandidateIndices.Reserve(AvailableCount);
	
	for (int32 Index = 0; Index < AvailableCount; ++Index)
	{
		CandidateIndices.Add(Index);
	}
	
	SelectedIndices.Reserve(SelectionCount);
	
	for (int32 SelectionIndex = 0; SelectionIndex < SelectionCount; ++SelectionIndex)
	{
		const int32 CandidateIndexArrayIndex = RandomStream.RandRange(0, CandidateIndices.Num() - 1);
		
		SelectedIndices.Add(CandidateIndices[CandidateIndexArrayIndex]);
		
		CandidateIndices.RemoveAtSwap(CandidateIndexArrayIndex);
	}
	
	return SelectedIndices;
}

// Called when the game starts or when spawned
void AWaveManager::BeginPlay()
{
	Super::BeginPlay();
	
	SpawnSelectionRandomStream.GenerateNewSeed();
	
	FindGameMode();
	CollectSpawnPoints();
	
	if (!IsValid(ShooterGameMode))
	{
		return;
	}
	
	//监听GameMode的波次开始事件
	ShooterGameMode->OnWaveStarted.AddUniqueDynamic(this, &AWaveManager::HandleWaveStarted);
	
	ShooterGameMode->OnGameOver.AddUniqueDynamic(
		this,
		&AWaveManager::StopEnemySpawning
	);
	
	UE_LOG(LogTemp, Warning, TEXT("WaveManager successfully bound to GameMode"));
}

void AWaveManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopEnemySpawning();
	
	if (IsValid(ShooterGameMode))
	{
		ShooterGameMode->OnWaveStarted.RemoveDynamic(this, &AWaveManager::HandleWaveStarted);
	
		ShooterGameMode->OnGameOver.RemoveDynamic(
			this,
			&AWaveManager::StopEnemySpawning
		);
	}
	
	for (const TObjectPtr<AShooterEnemyBase>& Enemy : LivingWaveEnemies)
	{
		if (IsValid(Enemy.Get()))
		{
			Enemy->OnEnemyDied.RemoveDynamic(
				this,
				&AWaveManager::HandleWaveEnemyDied
			);
		}
	}
	LivingWaveEnemies.Reset();
	
	Super::EndPlay(EndPlayReason);
}

void AWaveManager::FindGameMode()
{
	AGameModeBase* FoundGameMode = UGameplayStatics::GetGameMode(this);
	
	ShooterGameMode = Cast<AShooterSamProjectGameMode>(FoundGameMode);
	
	if (!IsValid(ShooterGameMode))
	{
		UE_LOG(LogTemp, Error, TEXT("WaveManager could not find ShooterSamProjectGameMode"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("WaveManager found GameMode"));
}

void AWaveManager::CollectSpawnPoints()
{
	//防止函数重复调用时保留上一次的结果
	SpawnPoints.Reset();
	
	TArray<AActor*> FoundActors;
	
	UGameplayStatics::GetAllActorsOfClass(this, AEnemySpawnPoint::StaticClass(), FoundActors);
	
	int32 MainSpawnPointCount = 0;
	int32 OuterSpawnPointCount = 0;
	
	for (AActor* FoundActor : FoundActors)
	{
		AEnemySpawnPoint* SpawnPoint = Cast<AEnemySpawnPoint>(FoundActor);
		
		if (!IsValid(SpawnPoint))
		{
			continue;
		}
		
		//被禁用的出生点不参与当前游戏
		if (!SpawnPoint->IsSpawnEnabled())
		{
			continue;
		}
		
		SpawnPoints.Add(SpawnPoint);
		
		if (SpawnPoint->IsMainBattlefield())
		{
			++MainSpawnPointCount;
		}
		else
		{
			++OuterSpawnPointCount;
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("WaveManager collected %d enabled spawn points: %d main, %d outer"), SpawnPoints.Num(), MainSpawnPointCount, OuterSpawnPointCount);
	
	//游戏规则只要求至少存在一个主战场出生点
	if (MainSpawnPointCount == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("WaveManager requires at least one" "main battlefield spawn point"));
	}
	
	//没有外围出生点时游戏仍能运行，但会缺少线路变化
	if (OuterSpawnPointCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("WaveManager found no outer spawn points"));
	}
}

void AWaveManager::SelectActiveSpawnPointsForWave(int32 WaveNumber)
{
	//清除上一波的选择结果
	ActiveSpawnPoints.Reset();
	
	const int32 RequestedSpawnPointCount = CalculateSpawnPointCountForWave(WaveNumber, SpawnPoints.Num());
	
	const TArray<int32> SelectedIndices =
		SelectUniqueSpawnPointIndices(
			RequestedSpawnPointCount,
			SpawnPoints.Num(),
			SpawnSelectionRandomStream
		);
	
	for (int32 SpawnPointIndex : SelectedIndices)
	{
		if (!SpawnPoints.IsValidIndex(SpawnPointIndex))
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("Wave %d selected invalid spawn point %d"),
				WaveNumber,
				SpawnPointIndex
			);
			
			continue;
		}
		
		AEnemySpawnPoint* SelectedSpawnPoint = SpawnPoints[SpawnPointIndex].Get();
		
		if (!IsValid(SelectedSpawnPoint))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Wave %d selected an invalid spawn point actor"),
				WaveNumber
			);
			
			continue;
		}
		
		ActiveSpawnPoints.Add(SelectedSpawnPoint);
		
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Wave %d selected spawn point: %s"),
			WaveNumber,
			*GetNameSafe(SelectedSpawnPoint)
		);
	}
	
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Wave %d activated %d of %d available spawn points"),
		WaveNumber,
		ActiveSpawnPoints.Num(),
		SpawnPoints.Num()
	);
}

void AWaveManager::HandleWaveStarted(int32 WaveNumber)
{
	//正常情况下，新波次开始时上一次登记的敌人已经全部死亡
	if (!LivingWaveEnemies.IsEmpty())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Cannot start spawing: previous wave enemies remain.")
		);
		return;
	}
	
	if (!EnemyClass || EnemyClass->HasAnyClassFlags(CLASS_Abstract))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Assign a non-abstract EnemyClass on WaveManager.")
		);
		return;
	}
	
	//编辑器限制不能代替运行时检查
	MaxAliveEnemies = FMath::Max(1, MaxAliveEnemies);
	
	ResumeSpawnThreshold = FMath::Clamp(
		ResumeSpawnThreshold,
		0,
		MaxAliveEnemies - 1
	);
	
	SpawnInterval = FMath::IsFinite(SpawnInterval) ? FMath::Max(0.1f, SpawnInterval) : 0.75f;
	
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("WaveManager received Wave %d"),
		WaveNumber
	);
	
	SelectActiveSpawnPointsForWave(WaveNumber);
	InitializeWaveRuntimeState(WaveNumber);
	
	if (!BuildSpawnQueue(WaveNumber))
	{
		return;
	}
	
	StartEnemySpawning();
}

void AWaveManager::InitializeWaveRuntimeState(int32 WaveNumber)
{
	WaveEnemyTarget = CalculateEnemyCountForWave(WaveNumber);
	
	SpawnedEnemyCount = 0;
	AliveEnemyCount = 0;
	DefeatedEnemyCount = 0;
	bIsSpawning = false;
	
	UE_LOG(
		LogTemp,
		Warning,
		TEXT(
			"Wave %d initialized: "
			"Target=%d, "
			"Spawned=%d, "
			"Alive=%d, "
			"Defeated=%d, "
			"MaxAlive=%d, "
			"ResumeAt=%d, "
			"Interval=%.2f, "
			"Spawning=%s"
		),
		WaveNumber,
		WaveEnemyTarget,
		SpawnedEnemyCount,
		AliveEnemyCount,
		DefeatedEnemyCount,
		MaxAliveEnemies,
		ResumeSpawnThreshold,
		SpawnInterval,
		bIsSpawning ? TEXT("true") : TEXT("false")
	);
}

bool AWaveManager::RegisterWaveEnemy(AShooterEnemyBase* Enemy)
{
	if (!IsValid(ShooterGameMode) || ShooterGameMode->WaveState != EWaveState::InProgress)
	{
		return false;
	}
	
	if (!IsValid(Enemy) || Enemy->IsDead() || Enemy->GetCurrentHealth() <= 0.0f)
	{
		return false;
	}
	
	//防止同一只敌人重复登记
	if (LivingWaveEnemies.Contains(Enemy))
	{
		return false;
	}
	
	if (WaveEnemyTarget <= 0 || SpawnedEnemyCount >= WaveEnemyTarget)
	{
		return false;
	}
	
	LivingWaveEnemies.Add(Enemy);
	
	Enemy->OnEnemyDied.AddUniqueDynamic(
		this,
		&AWaveManager::HandleWaveEnemyDied
	);
	
	++SpawnedEnemyCount;
	AliveEnemyCount = LivingWaveEnemies.Num();
	
	ShooterGameMode->RegisterSpawnedEnemy();
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"Wave %d registered %s"
			"Spawned = %d / %d, Alive = %d, Defeated = %d"	
		),
		ShooterGameMode->CurrentWave,
		*GetNameSafe(Enemy),
		SpawnedEnemyCount,
		WaveEnemyTarget,
		AliveEnemyCount,
		DefeatedEnemyCount
	);
	
	if (SpawnedEnemyCount >= WaveEnemyTarget)
	{
		ShooterGameMode->NotifyWaveSpawningFinished();
	}
	
	return true;
}

void AWaveManager::HandleWaveEnemyDied(AShooterEnemyBase* DeadEnemy)
{
	if (!IsValid(DeadEnemy) || !DeadEnemy->IsDead())
	{
		return;
	}
	
	//不在登记表里的敌人，不影响本波计数
	if (!LivingWaveEnemies.Contains(DeadEnemy))
	{
		return;
	}
	
	//先移除，再通知其它系统，避免重复计数
	LivingWaveEnemies.RemoveSingleSwap(DeadEnemy);
	
	DeadEnemy->OnEnemyDied.RemoveDynamic(
		this,
		&AWaveManager::HandleWaveEnemyDied
	);
	
	AliveEnemyCount = LivingWaveEnemies.Num();
	++DefeatedEnemyCount;
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"Wave enemy defeated: %s,"
			"Spawned = %d / %d, Alive = %d, Defeated = %d"
			),
			*GetNameSafe(DeadEnemy),
			SpawnedEnemyCount,
			WaveEnemyTarget,
			AliveEnemyCount,
			DefeatedEnemyCount
	);
	
	if (IsValid(ShooterGameMode) && ShooterGameMode->WaveState == EWaveState::InProgress)
	{
		ShooterGameMode->NotifyEnemyDied();
	}
	
	//通知GameMode以后，本波可能已经结束
	//因此必须重新检查波次状态
	if (IsValid(ShooterGameMode) && ShooterGameMode->WaveState == EWaveState::InProgress && !bIsSpawning 
		&& ShouldResumeEnemySpawning(
			SpawnedEnemyCount,
			WaveEnemyTarget,
			AliveEnemyCount,
			ResumeSpawnThreshold
		)	
	)
	{
		StartEnemySpawning();
	}
}

bool AWaveManager::BuildSpawnQueue(int32 WaveNumber)
{
	SpawnQueue.Reset();
	
	if (WaveEnemyTarget <= 0 || ActiveSpawnPoints.IsEmpty())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Cannot build spawn queue: invalid target or no active points.")
		);
		return false;
	}
	
	TArray<AEnemySpawnPoint*> MainPoints;
	
	for (const TObjectPtr<AEnemySpawnPoint>& Point : ActiveSpawnPoints)
	{
		if (!IsValid(Point.Get()) || !Point->IsSpawnEnabled())
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("Cannot build spawn queue: invalid or disabled point.")
			);
			return false;
		}
		
		if (Point->IsMainBattlefield())
		{
			MainPoints.Add(Point.Get());
		}
	}
	
	SpawnQueue.Reserve(WaveEnemyTarget);
	
	if (WaveNumber == 1)
	{
		// 第一波要求每个启用点至少生成一只，
		//所以目标数量不能少于出生点数量
		if (MainPoints.IsEmpty() || WaveEnemyTarget < ActiveSpawnPoints.Num())
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("Wave 1 requires a main point and enough enemy slots.")
			);
			return false;
		}
		
		//所有出生点先各安排一只
		for (const TObjectPtr<AEnemySpawnPoint>& Point : ActiveSpawnPoints)
		{
			SpawnQueue.Add(Point);
		}
	
		//剩余名额只分配给主战场出生点
		int32 MainPointIndex = 0;
	
		while (SpawnQueue.Num() < WaveEnemyTarget)
		{
			SpawnQueue.Add(MainPoints[MainPointIndex]);
		
			MainPointIndex = (MainPointIndex + 1) % MainPoints.Num();
		}
	}
	else
	{
		
		//第二波开始，在本波随机选中的出生点之间轮流分配
		for (int32 Index = 0; Index < WaveEnemyTarget; ++Index)
		{
			const int32 PointIndex = Index % ActiveSpawnPoints.Num();
			
			SpawnQueue.Add(ActiveSpawnPoints[PointIndex]);
		}
	}
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Wave %d spawn queue ready: %d enemies."),
		WaveNumber,
		SpawnQueue.Num()
	);
		
	return true;
}

void AWaveManager::StartEnemySpawning()
{
	if (bIsSpawning)
	{
		return;
	}
	
	if (!IsValid(ShooterGameMode) || ShooterGameMode->WaveState != EWaveState::InProgress)
	{
		return;
	}
	
	if (ShouldPauseEnemySpawning(
		SpawnedEnemyCount,
		WaveEnemyTarget,
		AliveEnemyCount,
		MaxAliveEnemies
	))
	{
		return;
	}
	
	bIsSpawning = true;
	
	GetWorldTimerManager().SetTimer(
		EnemySpawnTimerHandle,
		this,
		&AWaveManager::SpawnNextEnemy,
		SpawnInterval,
		true
	);
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Enemy spawning started: Spawned = %d / %d, Alive = %d"),
		SpawnedEnemyCount,
		WaveEnemyTarget,
		AliveEnemyCount
	);
}

void AWaveManager::StopEnemySpawning()
{
	GetWorldTimerManager().ClearTimer(EnemySpawnTimerHandle);
	bIsSpawning = false;
}

void AWaveManager::SpawnNextEnemy()
{
	if (!IsValid(ShooterGameMode) || ShooterGameMode->WaveState != EWaveState::InProgress)
	{
		StopEnemySpawning();
		return;
	}
	
	//总量已经生成完毕，或者场上敌人已经达到上限
	if (ShouldPauseEnemySpawning(
		SpawnedEnemyCount,
		WaveEnemyTarget,
		AliveEnemyCount,
		MaxAliveEnemies
	))
	{
		StopEnemySpawning();
		return;
	}
	
	if (!EnemyClass || !SpawnQueue.IsValidIndex(SpawnedEnemyCount))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Enemy spawning stopped: invalid class queue index.")
		);
		StopEnemySpawning();
		return;
	}
	
	//累计成功生成数量，也是下一份出生安排的下标
	AEnemySpawnPoint* SpawnPoint = SpawnQueue[SpawnedEnemyCount].Get();
	
	if (!IsValid(SpawnPoint) || !SpawnPoint->IsSpawnEnabled())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Enemy spawning stopped: spawn point os unavailable")
		);
		StopEnemySpawning();
		return;
	}
	
	UWorld* World = GetWorld();
	
	if (!IsValid(World))
	{
		StopEnemySpawning();
		return;
	}
	
	const FTransform SpawnTransform = SpawnPoint->GetSpawnTransform();
	
	FActorSpawnParameters SpawnParameters;
	
	//如果位置被挡住，尝试调整
	//调整后仍然碰撞，就不生成
	SpawnParameters.SpawnCollisionHandlingOverride = 
		ESpawnActorCollisionHandlingMethod::
		AdjustIfPossibleButDontSpawnIfColliding;
	
	AShooterEnemyBase* NewEnemy = 
		World->SpawnActor<AShooterEnemyBase>(
			EnemyClass.Get(),
			SpawnTransform.GetLocation(),
			SpawnTransform.Rotator(),
			SpawnParameters
		);
	
	if (!IsValid(NewEnemy))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Spawn failed at %s. Will retry next interval"),
			*GetNameSafe(SpawnPoint)
		);
		
		//失败不消耗名额，下次继续尝试当前安排
		return;
	}
	
	if (!RegisterWaveEnemy(NewEnemy))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Spawned enemy could not be registered. Spawning stopped.")
		);
		
		//避免留下不受波次系统管理的敌人
		NewEnemy->Destroy();
		StopEnemySpawning();
		return;
	}
	
	UE_LOG(LogTemp, Log,
		TEXT("Spawned %s at %s: Spawned=%d/%d, Alive=%d"),
		*GetNameSafe(NewEnemy),
		*GetNameSafe(SpawnPoint),
		SpawnedEnemyCount,
		WaveEnemyTarget,
		AliveEnemyCount);

	// 登记后数量发生了变化，再判断是否应该停下
	if (ShouldPauseEnemySpawning(
		SpawnedEnemyCount,
		WaveEnemyTarget,
		AliveEnemyCount,
		MaxAliveEnemies))
	{
		StopEnemySpawning();

		UE_LOG(LogTemp, Log,
			TEXT("Enemy spawning stopped: Spawned=%d/%d, Alive=%d"),
			SpawnedEnemyCount,
			WaveEnemyTarget,
			AliveEnemyCount);
	}
}
