// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemySpawnPoint.h"

#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"

// Sets default values
AEnemySpawnPoint::AEnemySpawnPoint()
{
 	//出生点不需要每帧运行代码
	PrimaryActorTick.bCanEverTick = false;
	
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	
	SetRootComponent(SceneRoot);
	
	SpawnArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnArrow"));
	
	SpawnArrow->SetupAttachment(SceneRoot);
	
	//方便从编辑器中观察出生方向
	SpawnArrow->ArrowColor = FColor::Red;
	SpawnArrow->ArrowSize = 2.0f;
}

FTransform AEnemySpawnPoint::GetSpawnTransform() const
{
	if (IsValid(SpawnArrow))
	{
		return SpawnArrow->GetComponentTransform();
	}
	
	//箭头不存在时使用Actor自身Transform
	return GetActorTransform();
}

bool AEnemySpawnPoint::IsSpawnEnabled() const
{
	return bSpawnEnabled;
}

bool AEnemySpawnPoint::IsMainBattlefield() const
{
	return SpawnPointType == EEnemySpawnPointType::MainBattlefield;
}

// Called when the game starts or when spawned
void AEnemySpawnPoint::BeginPlay()
{
	Super::BeginPlay();
	
}


