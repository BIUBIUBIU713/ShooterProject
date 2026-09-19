// Fill out your copyright notice in the Description page of Project Settings.


#include "UpgradeTerminal.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "ShooterSamProjectCharacter.h"
#include "Engine/Engine.h"
#include "ENgine/World.h"

// Sets default values
AUpgradeTerminal::AUpgradeTerminal()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(
		TEXT("InteractionSphere")	
	);
	
	InteractionSphere->InitSphereRadius(180.0f);
	InteractionSphere->SetGenerateOverlapEvents(true);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionSphere->SetCanEverAffectNavigation(false);
	
	TerminalMesh = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("TerminalMesh")	
	);
	TerminalMesh->SetupAttachment(InteractionSphere);
	TerminalMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	TerminalMesh->SetGenerateOverlapEvents(false);

}

bool AUpgradeTerminal::CanInteract(AShooterSamProjectCharacter* Player) const
{
	if (!IsValid(Player) ||
		!Player->IsAlive ||
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

FText AUpgradeTerminal::GetInteractionText() const
{
	const AShooterSamProjectGameMode* GameMode = IsValid(GetWorld())
		? GetWorld()->GetAuthGameMode<AShooterSamProjectGameMode>()
		: nullptr;
	
	if (!IsValid(GameMode) || GameMode->WaveState == EWaveState::GameOver)
	{
		return NSLOCTEXT(
			"UpgradeTerminal",
			"Unavailable",
			"增益终端暂不可用"
		);
	}
	
	if (!GameMode->IsPowerOn())
	{
		return NSLOCTEXT(
			"UpgradeTerminal", "PowerRequired", "需要先恢复供电"
		);
	}
	
	if (!GameMode->IsUpgradeStationUnlocked())
	{
		return NSLOCTEXT(
			"UpgradeTerminal", "UnlockRequired", "需要先付费解锁增益站"
		);
	}
	
	const int32 Level = GameMode->GetUpgradeLevel(UpgradeType);
	const int32 Cost = GameMode->GetUpgradeCost(UpgradeType);
	
	if (Level < 0)
	{
		return NSLOCTEXT(
			"UpgradeTerminal", "InvalidUpgrade", "该增益暂不可用"
		);
	}
	
	if (Cost <= 0)
	{
		return FText::Format(
			NSLOCTEXT(
				"UpgradeTerminal",
				"NoFurtherUpgrade",
				"{0}：当前 {1} 级，无法继续升级"
			),
			GetUpgradeName(),
			FText::AsNumber(Level)
		);
	}

	return FText::Format(
		NSLOCTEXT(
			"UpgradeTerminal",
			"PurchasePrompt",
			"[E] {0}：{1} → {2} 级，花费 {3} 金币"
		),
		GetUpgradeName(),
		FText::AsNumber(Level),
		FText::AsNumber(Level + 1),
		FText::AsNumber(Cost)
	);
}

bool AUpgradeTerminal::TryInteract(AShooterSamProjectCharacter* Player)
{
	if (!CanInteract(Player))
	{
		return false;
	}
	
	AShooterSamProjectGameMode* GameMode = 
		GetWorld()->GetAuthGameMode<AShooterSamProjectGameMode>();
	
	if (!IsValid(GameMode))
	{
		return false;
	}
	
	//扣款，升级和应用效果统一由GameMode处理
	const EUpgradePurchaseResult Result =
		GameMode->TryPurchaseUpgrade(Player, UpgradeType);

	const bool bSucceeded = Result == EUpgradePurchaseResult::Success;

	FText Message = GetPurchaseResultText(Result);
	
	if (bSucceeded)
	{
		Message = FText::Format(
			NSLOCTEXT(
				"UpgradeTerminal",
				"PurchaseSucceeded",
				"{0}已提升至 {1} 级"
			),
			GetUpgradeName(),
			FText::AsNumber(GameMode->GetUpgradeLevel(UpgradeType))
		);
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			64004,
			3.0f,
			bSucceeded ? FColor::Green : FColor::Yellow,
			Message.ToString()
		);
	}

	return bSucceeded;
}

FText AUpgradeTerminal::GetUpgradeName() const
{
	switch (UpgradeType)
	{
	case EPlayerUpgradeType::Reload:
		return  NSLOCTEXT(
			"UpgradeTerminal",
			"Reload",
			"快速换弹"
		);
		
	case EPlayerUpgradeType::Movement:
		return NSLOCTEXT(
			"UpgradTerminal",
			"Movement",
			"灵活移动"
		);
		
	default:
		return NSLOCTEXT(
			"UpgradeTerminal",
			"Unknown",
			"未知增益"
		);
	}
}

FText AUpgradeTerminal::GetPurchaseResultText(EUpgradePurchaseResult Result) const
{
	switch (Result)
	{
	case EUpgradePurchaseResult::Success:
		return NSLOCTEXT(
			"UpgradeTerminal", "Success", "购买成功"
		);

	case EUpgradePurchaseResult::PowerOff:
		return NSLOCTEXT(
			"UpgradeTerminal", "PowerRequired", "需要先恢复供电"
		);

	case EUpgradePurchaseResult::StationLocked:
		return NSLOCTEXT(
			"UpgradeTerminal", "UnlockRequired", "需要先付费解锁增益站"
		);

	case EUpgradePurchaseResult::MaxLevelReached:
		return NSLOCTEXT(
			"UpgradeTerminal", "MaxLevel", "该增益已达到最高等级"
		);

	case EUpgradePurchaseResult::InsufficientCoins:
		return NSLOCTEXT(
			"UpgradeTerminal", "NotEnoughCoins", "金币不足"
		);

	case EUpgradePurchaseResult::InvalidConfiguration:
		return NSLOCTEXT(
			"UpgradeTerminal", "InvalidUpgrade", "该增益暂不可用"
		);

	default:
		return NSLOCTEXT(
			"UpgradeTerminal", "PurchaseUnavailable", "当前无法购买"
		);
	}
}



