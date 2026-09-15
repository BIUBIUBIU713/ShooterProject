// Fill out your copyright notice in the Description page of Project Settings.


#include "RegionDoor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "ShooterSamProjectCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"

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
	if (UnlockCost < 0)
	{
		return NSLOCTEXT(
			"RegionDoor", "InvalidCost", "门的价格配置无效"	
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
	if (!CanInteract(Player) || UnlockCost < 0)
	{
		return false;
	}
	
	//动画配置无效时不扣款
	if (!FMath::IsFinite(SinkDistance) || SinkDistance <= 0.0f || 
		!FMath::IsFinite(SinkSpeed) || SinkSpeed <= 0.0f	
	)
	{
		return false;
	}
	
	//现有TrySpendCoins 不接受0， 因此免费门跳过扣款
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
	
	//先锁定状态，避免下沉过程中重复交互和扣款
	bUnlocked = true;
	
	InteractionSphere->SetCollisionEnabled(
		ECollisionEnabled::NoCollision	
	);
	
	//开门动画只负责表现，不能推挤玩家
	DoorMesh->SetCollisionEnabled(
		ECollisionEnabled::NoCollision	
	);
	
	RemainingSinkDistance = SinkDistance;
	SetActorTickEnabled(true);
	
	return true;
}



