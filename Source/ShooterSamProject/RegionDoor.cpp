// Fill out your copyright notice in the Description page of Project Settings.


#include "RegionDoor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "ShooterSamProjectCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "LocalizationDescriptor.h"
#include "ShooterSamProjectGameMode.h"
#include "Engine/World.h"

// Sets default values
ARegionDoor::ARegionDoor()
{
	//具备Tick能力，但开局不运行，解锁后才开启
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	
	SceneRoot = CreateDefaultSubobject<USceneComponent>(
		TEXT("SceneRoot")	
	);
	SetRootComponent(SceneRoot);
	
	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("DoorMesh")	
	);
	DoorMesh->SetupAttachment(SceneRoot);
	
	//门需要在运行时移动
	DoorMesh->SetMobility(EComponentMobility::Movable);

	DoorMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	DoorMesh->SetGenerateOverlapEvents(false);
	
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(
		TEXT("InteractionSphere")	
	);
	InteractionSphere->SetupAttachment(SceneRoot);

	InteractionSphere->InitSphereRadius(180.0f);
	InteractionSphere->SetGenerateOverlapEvents(true);
	InteractionSphere->SetCollisionEnabled(
		ECollisionEnabled::QueryOnly	
	);
	
	InteractionSphere->SetCollisionResponseToAllChannels(
		ECR_Ignore
	);
	InteractionSphere->SetCollisionResponseToChannel(
		ECC_Pawn, ECR_Overlap	
	);
}

void ARegionDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!bUnlocked)
	{
		return;
	}
	
	const float MoveDistance = FMath::Min(
		SinkSpeed* DeltaTime,
		RemainingSinkDistance
	);
	
	//沿世界坐标的负Z方向下沉
	DoorMesh->AddWorldOffset(
		FVector(0.0f, 0.0f, -MoveDistance)
	);
	
	RemainingSinkDistance = FMath::Max(
		RemainingSinkDistance - MoveDistance,
		0.0f
	);
	
	if (RemainingSinkDistance <= 0.0f)
	{
		//销毁整个Actor， 其组件也会一起销毁
		Destroy();
	}
}

bool ARegionDoor::CanInteract(AShooterSamProjectCharacter* Player) const
{
	if (bUnlocked || !IsValid(Player) || !Player->IsAlive || !Player->IsPlayerControlled())
	{
		return false;
	}
	
	return InteractionSphere->IsOverlappingComponent(
		Player->GetCapsuleComponent()	
	);
}

FText ARegionDoor::GetInteractionText() const
{
	if (UnlockCost < 0 || RequiredWave < 0)
	{
		return NSLOCTEXT(
			"RegionDoor", "InvalidCost", "门的价格配置无效"	
		);
	}
	
	if (!IsPowerRequiredmenMet())
	{
		return NSLOCTEXT(
			"RegionDoor",
			"PowerRequired",
			"需要先前往车库2恢复供电"
		);
	}
	
	if (!HasReachedRequiredWave())
	{
		return FText::Format(
			NSLOCTEXT(
				"RegionDoor",
				"WaveLocked",
				"第 {0} 回合开放解锁"
			),	
			FText::AsNumber(RequiredWave)
		);
	}
	
	if (UnlockCost == 0)
	{
		return NSLOCTEXT(
			"RegionDoor", "FreeUnlock", "[E] 解锁门（免费）"
		);
	}
	
	return FText::Format(
		NSLOCTEXT(
			"RegionDoor", 
			"UnlockPrompt", 
			"[E] 解锁门：{0} 金币"	
		),	
		FText::AsNumber(UnlockCost)
	);
}

bool ARegionDoor::TryInteract(AShooterSamProjectCharacter* Player)
{
	if (!CanInteract(Player) || !CanStartOpening() || UnlockCost < 0 || !IsValid(GetWorld()))
	{
		return false;
	}
	
	AShooterSamProjectGameMode* GameMode = 
		GetWorld()->GetAuthGameMode<AShooterSamProjectGameMode>();
	
	if (!IsValid(GameMode))
	{
		return false;
	}
	
	//未通电时拒绝解锁，不扣金币
	if (!IsPowerRequiredmenMet())
	{
		UE_LOG(
		LogTemp,
		Log,
		TEXT("Door unlock rejected: %s requires power"),
		*GetName());

		return false;
	}
	
	//未达到要求时，不扣款，不启动动画
	if (!HasReachedRequiredWave())
	{
		UE_LOG(
		LogTemp,
		Log,
		TEXT("Door unlock rejected: %s requires wave %d"),
		*GetName(),
		RequiredWave);

		return false;
	}
	
	//当前交互的门一定要打开
	TArray<ARegionDoor*> DoorsToOpen;
	DoorsToOpen.Add(this);
	
	//只有填写了分组标识， 才寻找其他同组门
	if (!DoorGroupId.IsNone())
	{
		for (TActorIterator<ARegionDoor> It(GetWorld()); It; ++It)
		{
			ARegionDoor* OtherDoor = *It;
			
			if (!IsValid(OtherDoor) || OtherDoor == this)
			{
				continue;
			}
			
			if (OtherDoor->DoorGroupId == DoorGroupId)
			{
				DoorsToOpen.Add(OtherDoor);
			}
		}
	}
	
	//全部检查通过后才扣钱，避免付费只打开部分门
	for (ARegionDoor* Door : DoorsToOpen)
	{
		if (!Door->CanStartOpening())
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Door unlock rejected: %s cannot start opening"),
				*Door->GetName());

			return false;
		}
		
		//同组门必须使用相同价格，避免不同入口收费不同
		if (Door->UnlockCost != UnlockCost)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Door group %s has inconsistent prices: %s=%d, %s=%d"),
				*DoorGroupId.ToString(),
				*GetName(),
				UnlockCost,
				*Door->GetName(),
				Door->UnlockCost);

			return false;
		}
		
		//同组门各入口使用相同的回合要求
		if (Door->RequiredWave != RequiredWave)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Door group %s has inconsistent wave requirements: %s=%d, %s=%d"),
				*DoorGroupId.ToString(),
				*GetName(),
				RequiredWave,
				*Door->GetName(),
				Door->RequiredWave
			);

			return false;
		}
		
		//同组门必须使用相同的供电要求
		if (Door->bRequiresPower != bRequiresPower)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Door group %s has inconsistent power requirements: %s and %s"),
				*DoorGroupId.ToString(),
				*GetName(),
				*Door->GetName()
			);

			return false;
		}
	}
	
	//扣款放在循环外：无论有几扇门，都只支付一次
	if (UnlockCost > 0 && !Player->TrySpendCoins(UnlockCost))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				64002,
				2.0f,
				FColor::Yellow,
				TEXT("金币不足")
			);
		}
		
		return false;
	}
	
	//同组门在这一帧一起进入打开状态
	for (ARegionDoor* Door : DoorsToOpen)
	{
		Door->StartOpening();
	}
	
	//付费解锁成功，记录本局区域状态
	GameMode->RecordRegionUnlocked(DoorGroupId);
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Doors unlocked: group=%s, count=%d, cost=%d"),
		*DoorGroupId.ToString(),
		DoorsToOpen.Num(),
		UnlockCost);
	
	return true;
}

bool ARegionDoor::CanStartOpening() const
{
	return !bUnlocked
		&& FMath::IsFinite(SinkDistance)
		&& SinkDistance > 0.0f
		&& FMath::IsFinite(SinkSpeed)
		&& SinkSpeed > 0.0f;
}

void ARegionDoor::StartOpening()
{
	if (!CanStartOpening())
	{
		return;
	}
	
	//防止重复启动动画
	bUnlocked = true;
	
	InteractionSphere->SetCollisionEnabled(
		ECollisionEnabled::NoCollision	
	);
	
	DoorMesh->SetCollisionEnabled(
		ECollisionEnabled::NoCollision	
	);
	
	RemainingSinkDistance = SinkDistance;
	SetActorTickEnabled(true);
}

bool ARegionDoor::HasReachedRequiredWave() const
{
	if (RequiredWave < 0)
	{
		return false;
	}
	
	if (RequiredWave == 0)
	{
		return true;
	}
	
	const UWorld* World = GetWorld();
	
	if (!IsValid(World))
	{
		return false;
	}
	
	const AShooterSamProjectGameMode* GameMode = 
		World->GetAuthGameMode<AShooterSamProjectGameMode>();
	
	if (!IsValid(GameMode))
	{
		return false;
	}
	
	return GameMode->CurrentWave >= RequiredWave;
}

bool ARegionDoor::IsPowerRequiredmenMet() const
{
	//普通门不需要检查供电
	if (!bRequiresPower)
	{
		return true;
	}
	
	const UWorld* World = GetWorld();
	
	if (!IsValid(World))
	{
		return false;
	}
	
	const AShooterSamProjectGameMode* GameMode = 
		World->GetAuthGameMode<AShooterSamProjectGameMode>();
	
	return IsValid(GameMode) && GameMode->IsPowerOn();
}



