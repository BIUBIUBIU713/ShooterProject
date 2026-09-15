// Fill out your copyright notice in the Description page of Project Settings.


#include "EnergyPickup.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ShooterSamProjectCharacter.h"
#include "Components/CapsuleComponent.h"

// Sets default values
AEnergyPickup::AEnergyPickup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	
	SetRootComponent(PickupSphere);
	
	PickupSphere->InitSphereRadius(80.f);
	PickupSphere->SetGenerateOverlapEvents(true);
	
	//只检测重叠，不阻挡角色
	PickupSphere->SetCollisionEnabled(
		ECollisionEnabled::QueryOnly	
	);
	
	PickupSphere->SetCollisionObjectType(ECC_WorldDynamic);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(
		ECC_Pawn,
		ECR_Overlap
	);
	
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	
	PickupMesh->SetupAttachment(PickupSphere);
	
	//外观不参与碰撞，拾取范围由球体负责
	PickupMesh->SetCollisionEnabled(
		ECollisionEnabled::NoCollision	
	);
	
	PickupMesh->SetGenerateOverlapEvents(false);
}

void AEnergyPickup::BeginPlay()
{
	BatteryAmount = FMath::Max(BatteryAmount, 1);
	RemainingBatteries = BatteryAmount;
	
	Super::BeginPlay();
}

bool AEnergyPickup::CanInteract(
	AShooterSamProjectCharacter* Player	
) const
{
	return !bConsumed &&
		RemainingBatteries > 0 &&
			IsValid(Player) &&
				Player->IsAlive &&
				Player->IsPlayerControlled() &&
				PickupSphere->IsOverlappingComponent(
				Player->GetCapsuleComponent()	
			);
}

FText AEnergyPickup::GetInteractionText() const
{
	return FText::FromString(
		FString::Printf(TEXT("[E] Get the batteries * %d"), RemainingBatteries)	
	);
}

bool AEnergyPickup::TryInteract(
	AShooterSamProjectCharacter* Player	
)
{
	if (!CanInteract(Player))
	{
		return false;
	}
	
	const int32 AddedAmount =
		Player->AddBatteries(RemainingBatteries);
	
	if (AddedAmount <= 0)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("%s pickup rejected: battery inventory full"),
			*GetName()
		);
		
		return false;
	}
	
	RemainingBatteries -= AddedAmount;
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s supplied %d batteries, %d remaining"),
		*GetName(),
		AddedAmount,
		RemainingBatteries
	);
	
	if (RemainingBatteries == 0)
	{
		bConsumed = true;
		SetActorEnableCollision(false);
		Destroy();
	}
	
	return true;
}





