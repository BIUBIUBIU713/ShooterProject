// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPickup.h"

#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ShooterSamProjectCharacter.h"

// Sets default values
AHealthPickup::AHealthPickup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	
	SetRootComponent(PickupSphere);
	
	PickupSphere->InitSphereRadius(50.0f);
	PickupSphere->SetGenerateOverlapEvents(true);
	
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
	PickupMesh->SetCollisionEnabled(
		ECollisionEnabled::NoCollision	
	);
	PickupMesh->SetGenerateOverlapEvents(false);
}

// Called when the game starts or when spawned
void AHealthPickup::BeginPlay()
{
	HealingAmount = FMath::IsFinite(HealingAmount)
		? FMath::Max(HealingAmount, 0.1f)
		: 50.0f;
	
	RemainingHealing = HealingAmount;
	
	Super::BeginPlay();
}

bool AHealthPickup::CanInteract(AShooterSamProjectCharacter* Player) const
{
	return !bConsumed &&
		RemainingHealing > 0.0f &&
		IsValid(Player) &&
		Player->IsAlive &&
		Player->IsPlayerControlled() &&
		PickupSphere->IsOverlappingComponent(
			Player->GetCapsuleComponent()	
		);
}

FText AHealthPickup::GetInteractionText() const
{
	return FText::FromString(
		FString::Printf(
			TEXT("[E] Restore health (%.1f remaining)"), RemainingHealing	
		)	
	);
}

bool AHealthPickup::TryInteract(AShooterSamProjectCharacter* Player)
{
	if (!CanInteract(Player))
	{
		return false;
	}
	
	const float ActualRecovery = Player->RestoreHealth(RemainingHealing);
	
	if (ActualRecovery <= 0.0f)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("%s pickup rejected: no health restored"),
			*GetName()
		);
		
		return false;
	}
	
	RemainingHealing = FMath::Max(
		RemainingHealing - ActualRecovery,
		0.0f
	);
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s supplied %.1f health, %.1f remaining"),
		*GetName(),
		ActualRecovery,
		RemainingHealing
	);
	
	//浮点运算可能留下非常小的误差
	if (RemainingHealing <= KINDA_SMALL_NUMBER)
	{
		RemainingHealing = 0.0f;
		bConsumed = true;
		
		SetActorEnableCollision(false);
		Destroy();
	}
	
	return true;
}

