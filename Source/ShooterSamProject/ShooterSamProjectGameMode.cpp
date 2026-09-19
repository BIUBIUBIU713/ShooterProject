// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterSamProjectGameMode.h"

#include "ShooterSamProjectCharacter.h"
#include "shootAI.h"

#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AShooterSamProjectGameMode::AShooterSamProjectGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

bool AShooterSamProjectGameMode::TryCollectPowerPart()
{
	//游戏结束，已持有道具或已经通电时，不允许重复领取
	if (WaveState == EWaveState::GameOver ||
		bHasPowerPart ||
		bPowerOn
	)
	{
		return false;
	}
	
	bHasPowerPart = true;
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Progression: power part collected"));
	
	return true;
}

bool AShooterSamProjectGameMode::TryActivatePower()
{
	//必须持有道具， 且尚未通电
	if (WaveState == EWaveState::GameOver ||
		!bHasPowerPart ||
		bPowerOn
	)
	{
		return false;
	}
	
	//将道具安装到供电设备中
	bHasPowerPart = false;
	bPowerOn = true;
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Progression: power activated, power part consumed"));
	
	return true;
}


void AShooterSamProjectGameMode::RecordRegionUnlocked(FName RegionId)
{
	if (!RegionId.IsNone())
	{
		UnlockedRegions.Add(RegionId);
	}
}

void AShooterSamProjectGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	InitializePlacedEnemies();
	
	WaveState = EWaveState::WaitingToStarting;
	
	//延迟进入第一波，让玩家和场景完成初始化
	GetWorldTimerManager().SetTimer(WaveTimerHandle, this, &AShooterSamProjectGameMode::StartNextWave, FirstWaveDelay, false);
}

void AShooterSamProjectGameMode::StartNextWave()
{
	if (WaveState == EWaveState::GameOver || WaveState == EWaveState::InProgress)
	{
		return;
	}
	
	++CurrentWave;
	AliveEnemyCount = 0;
	bWaveSpawningFinished = false;
	WaveState = EWaveState::InProgress;
	
	UE_LOG(LogTemp, Warning, TEXT("Wave %d started"), CurrentWave);
	
	//GameMode不生成敌人，以后WaveManager监听这个委托，并根据WaveNumber生成敌人
	OnWaveStarted.Broadcast(CurrentWave);
	OnAliveEnemyCountChanged.Broadcast(AliveEnemyCount);
}

void AShooterSamProjectGameMode::RegisterSpawnedEnemy()
{
	if (WaveState != EWaveState::InProgress)
	{
		UE_LOG(LogTemp, Warning, TEXT("Tired to register an enemy outside an active wave"));
		
		return;
	}
	
	++AliveEnemyCount;
	
	OnAliveEnemyCountChanged.Broadcast(AliveEnemyCount);
	
	UE_LOG(LogTemp, Warning, TEXT("Enemy registered. Alive enemies: %d"), AliveEnemyCount);
}

void AShooterSamProjectGameMode::NotifyEnemyDied()
{
	if (WaveState != EWaveState::InProgress)
	{
		return;
	}
	
	if (AliveEnemyCount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("NotifyEnemyDied called while AliveEnemyCount is already zero"));
		
		return;
	}
	
	--AliveEnemyCount;
	
	OnAliveEnemyCountChanged.Broadcast(AliveEnemyCount);
	
	UE_LOG(LogTemp, Warning, TEXT("Enemy died. Alive enemies: %d"), AliveEnemyCount);
	
	
		EndCurrentWave();
}

void AShooterSamProjectGameMode::NotifyWaveSpawningFinished()
{
	if (WaveState != EWaveState::InProgress || bWaveSpawningFinished)
	{
		return;
	}
	
	bWaveSpawningFinished = true;
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Wave %d spawning finished. Alive enemies: %d"),
		CurrentWave,
		AliveEnemyCount
	);
	
	EndCurrentWave();
}

void AShooterSamProjectGameMode::EndCurrentWave()
{
	if (WaveState != EWaveState::InProgress || !bWaveSpawningFinished || AliveEnemyCount != 0)
	{
		return;
	}
	
	WaveState = EWaveState::Intermission;
	
	UE_LOG(LogTemp, Warning, TEXT("Wave %d Ended"), CurrentWave);
	
	OnWaveEnded.Broadcast(CurrentWave);
	
	GetWorldTimerManager().SetTimer(WaveTimerHandle, this, &AShooterSamProjectGameMode::StartNextWave, IntermissionDuration, false);
	
}

void AShooterSamProjectGameMode::SetGameOver()
{
	//游戏结束只能处理一次
	if (WaveState == EWaveState::GameOver)
	{
		return;
	}
	
	WaveState = EWaveState::GameOver;
	
	GetWorldTimerManager().ClearTimer(WaveTimerHandle);
	
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Game Over at wave %d"),
		CurrentWave
	);
	
	OnGameOver.Broadcast();
}

int32 AShooterSamProjectGameMode::GetUpgradeLevel(EPlayerUpgradeType UpgradeType) const
{
	switch (UpgradeType)
	{
	case EPlayerUpgradeType::Reload:
		return ReloadUpgradeLevel;
		
	case EPlayerUpgradeType::Movement:
		return MovementUpgradeLevel;
		
	default:
		return -1;
	}
}

int32 AShooterSamProjectGameMode::GetUpgradeCost(EPlayerUpgradeType UpgradeType) const
{
	const int32 CurrentLevel = GetUpgradeLevel(UpgradeType);
	
	//无效类型，无效等级或已经满级
	if (CurrentLevel < 0 || CurrentLevel >= MaxUpgradeLevel)
	{
		return -1;
	}
	
	int32 Cost = 0;

	switch (UpgradeType)
	{
	case EPlayerUpgradeType::Reload:
		Cost = ReloadBaseCost;
		break;
		
	case EPlayerUpgradeType::Movement:
		Cost = MovementBaseCost;
		break;
		
	default:
		return -1;
	}
	
	if (Cost <= 0)
	{
		return -1;
	}
	
	//当前等级每增加一级，下一次后买费用翻倍
	for (int32 Index = 0; Index < CurrentLevel; ++Index)
	{
		//翻倍前检查，防止整数溢出
		if (Cost > MAX_int32 / 2)
		{
			return -1;
		}
		
		Cost *= 2;
	}
	
	return Cost;
}

float AShooterSamProjectGameMode::GetReloadDurationMultiplier() const
{
	const float Bonus = FMath::IsFinite(ReloadSpeedBonusPerLevel)
		? FMath::Clamp(ReloadSpeedBonusPerLevel, 0.0f, 1.0f)
		: 0.0f;
	
	const int32 Level = FMath::Max(ReloadUpgradeLevel, 0);
	
	return 1.0f / (1.0f + Bonus * Level);
}

float AShooterSamProjectGameMode::GetMovementSpeedMultiplier() const
{
	const float Bonus = FMath::IsFinite(MovementSpeedBonusPerLevel)
		? FMath::Clamp(MovementSpeedBonusPerLevel, 0.0f, 1.0f)
		: 0.0f;
	
	const int32 Level = FMath::Max(MovementUpgradeLevel, 0);
	
	return 1.0f + Bonus * Level;
}

bool AShooterSamProjectGameMode::IsRegionUnlocked(FName RegionId) const
{
	return !RegionId.IsNone() && UnlockedRegions.Contains(RegionId);
}

EUpgradePurchaseResult AShooterSamProjectGameMode::TryPurchaseUpgrade(AShooterSamProjectCharacter* Player,
	EPlayerUpgradeType UpgradeType)
{
	//对局结束，玩家死亡或传入其他世界的角色时拒绝购买
	if (WaveState == EWaveState::GameOver || 
		!IsValid(Player) ||
		!Player->IsAlive ||
		!Player->IsPlayerControlled() ||
		Player->GetWorld() != GetWorld()
	)
	{
		return EUpgradePurchaseResult::Unavailable;
	}
	
	if (!bPowerOn)
	{
		return EUpgradePurchaseResult::PowerOff;
	}
	
	if (UpgradeStationRegionId.IsNone())
	{
		return EUpgradePurchaseResult::InvalidConfiguration;
	}
	
	if (!IsRegionUnlocked(UpgradeStationRegionId))
	{
		return EUpgradePurchaseResult::StationLocked;
	}
	
	const int32 CurrentLevel = GetUpgradeLevel(UpgradeType);
	
	if (CurrentLevel < 0 || MaxUpgradeLevel <= 0)
	{
		return EUpgradePurchaseResult::InvalidConfiguration;
	}
	
	if (CurrentLevel >= MaxUpgradeLevel)
	{
		return EUpgradePurchaseResult::MaxLevelReached;
	}
	
	const int32 Cost = GetUpgradeCost(UpgradeType);
	
	if (Cost <= 0)
	{
		return EUpgradePurchaseResult::InvalidConfiguration;
	}
	
	//全部条件通过后才扣款
	if (!Player->TrySpendCoins(Cost))
	{
		return EUpgradePurchaseResult::InsufficientCoins;
	}
	
	//前面已通过GetUpgradeLevel排除了无效类型
	switch (UpgradeType)
	{
	case EPlayerUpgradeType::Reload:
		++ReloadUpgradeLevel;
		break;
		
	case EPlayerUpgradeType::Movement:
		++MovementUpgradeLevel;
		break;
		
	default:
		break;
	}
	
	//移速立即刷新；充能速度在下一次开始充能时读取
	Player->RefreshUpgradeEffects();
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Upgrade purchased: type=%s, level=%d, cost=%d, nextCost=%d"),
		UpgradeType == EPlayerUpgradeType::Reload
			? TEXT("Reload")
			: TEXT("Movement"),
		GetUpgradeLevel(UpgradeType),
		Cost,
		GetUpgradeCost(UpgradeType)
	);

	return EUpgradePurchaseResult::Success;
}

void AShooterSamProjectGameMode::InitializePlacedEnemies()
{
	AShooterSamProjectCharacter* Player = Cast<AShooterSamProjectCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(),0));
	
	if (!Player)
	{
		UE_LOG(LogTemp, Warning, TEXT("GameMode could not find player character"));
		
		return;
	}
	
	TArray<AActor*> ShooterAIActors;
	
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AshootAI::StaticClass(), ShooterAIActors);
	
	for (AActor* Actor : ShooterAIActors)
	{
		AshootAI* ShooterAIController = Cast<AshootAI>(Actor);
		
		if (ShooterAIController)
		{
			ShooterAIController->StartBehaviorTree(Player);
		}
	}
}
