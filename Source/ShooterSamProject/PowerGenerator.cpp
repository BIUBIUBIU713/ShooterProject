// Fill out your copyright notice in the Description page of Project Settings.


#include "PowerGenerator.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "ShooterSamProjectCharacter.h"
#include "ShooterSamProjectGameMode.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

// Sets default values
APowerGenerator::APowerGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(
		TEXT("InteractionSphere")	
	);
	SetRootComponent(InteractionSphere);
	
	InteractionSphere->InitSphereRadius(180.0f);
	InteractionSphere->SetGenerateOverlapEvents(true);
	InteractionSphere->SetCollisionEnabled(
		ECollisionEnabled::QueryOnly	
	);
	
	InteractionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(
		ECC_Pawn, ECR_Overlap	
	);
	
	GeneratorMesh = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("GeneratorMesh")	
	);
	GeneratorMesh->SetupAttachment(InteractionSphere);
	
	GeneratorMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	GeneratorMesh->SetGenerateOverlapEvents(false);
}

bool APowerGenerator::CanInteract(AShooterSamProjectCharacter* Player) const
{
	if (!IsValid(Player) || !Player->IsAlive ||
		!Player->IsPlayerControlled() || 
		!IsValid(GetWorld())
	)
	{
		return false;
	}
	
	return InteractionSphere->IsOverlappingComponent(
		Player->GetCapsuleComponent()	
	);
}

FText APowerGenerator::GetInteractionText() const
{
	const AShooterSamProjectGameMode* GameMode = IsValid(GetWorld())
		? GetWorld()->GetAuthGameMode<AShooterSamProjectGameMode>()
		: nullptr;
	
	if (!IsValid(GameMode))
	{
		return NSLOCTEXT(
			"PowerGenerator",
			"Unavailable",
			"供电设备暂不可用"
		);
	}
	
	if (GameMode->IsPowerOn())
	{
		return NSLOCTEXT(
			"PowerGenerator",
			"Powered",
			"电源已恢复"
		);
	}
	
	if (!GameMode->HasPowerPart())
	{
		return NSLOCTEXT(
			"PowerGenerator",
			"MissingPart",
			"缺少通电道具"
		);
	}
	
	return NSLOCTEXT(
		"PowerGenerator",
		"Activate",
		"[E] 安装道具，恢复供电"
	);
}

bool APowerGenerator::TryInteract(AShooterSamProjectCharacter* Player)
{
	if (!CanInteract(Player))
	{
		return false;
	}
	
	AShooterSamProjectGameMode* GameMode =
		GetWorld()->GetAuthGameMode<AShooterSamProjectGameMode>();
	
	//GameMode 统一检查道具，通电状态以及游戏是否结束
	if (!IsValid(GameMode) || !GameMode->TryActivatePower())
	{
		return false;
	}
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			64003,
			3.0f,
			FColor::Green,
			TEXT("电源已恢复")
		);	
	}
	
	return true;
}


