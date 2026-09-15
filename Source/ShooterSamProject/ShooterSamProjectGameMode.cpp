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
