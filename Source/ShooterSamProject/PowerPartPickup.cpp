// Fill out your copyright notice in the Description page of Project Settings.


#include "PowerPartPickup.h"

#include "LocalizationDescriptor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "ShooterSamProjectCharacter.h"
#include "ShooterSamProjectGameMode.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

// Sets default values
APowerPartPickup::APowerPartPickup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	PickupSphere = CreateDefaultSubobject<USphereComponent>(
		TEXT("PickupSphere")	
	);
	SetRootComponent(PickupSphere);
	
	PickupSphere->InitSphereRadius(120.0f);
	PickupSphere->SetGenerateOverlapEvents(true);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	PickupSphere->SetCollisionObjectType(ECC_WorldDynamic);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(
		ECC_Pawn, ECR_Overlap	
	);
	
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("PickupMesh")	
	);
	PickupMesh->SetupAttachment(PickupSphere);
	
	PickupMesh->SetCollisionEnabled(
		ECollisionEnabled::NoCollision	
	);
	PickupMesh->SetGenerateOverlapEvents(false);

}

bool APowerPartPickup::CanInteract(AShooterSamProjectCharacter* Player) const
{
	if (bCollected || !IsValid(Player) || !Player->IsAlive || !Player->IsPlayerControlled() ||
		!IsValid(GetWorld())	
	)
	{
		return false;
	}
	
	if (!PickupSphere->IsOverlappingComponent(
		Player->GetCapsuleComponent()	
	))
	{
		return false;
	}
	
	const AShooterSamProjectGameMode* GameMode = 
		GetWorld()->GetAuthGameMode<AShooterSamProjectGameMode>();
	
	if (!IsValid(GameMode))
	{
		return false;
	}
	
	//已经取得道具或已经通电， 就不再提供拾取交互
	return GameMode->WaveState != EWaveState::GameOver
		&& !GameMode->HasPowerPart()
		&& !GameMode->IsPowerOn();
}

FText APowerPartPickup::GetInteractionText() const
{
	return NSLOCTEXT(
		"PowerPartPickup",
		"CollectPrompt",
		"[E] 拾取通电道具");
}

bool APowerPartPickup::TryInteract(AShooterSamProjectCharacter* Player)
{
	if (!CanInteract(Player))
	{
		return false;
	}
	
	AShooterSamProjectGameMode* GameMode = 
		GetWorld()->GetAuthGameMode<AShooterSamProjectGameMode>();
	
	if (!IsValid(GameMode) || !GameMode->TryCollectPowerPart())
	{
		return false;
	}
	
	//必须先成功记录主线状态，再销毁场景中的道具
	bCollected = true;
	SetActorEnableCollision(false);
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			64003,
			3.0f,
			FColor::Green,
			TEXT("已取得通电道具，前往车库恢复供电")
		);
	}
	
	Destroy();
	return true;
}





