// Fill out your copyright notice in the Description page of Project Settings.


#include "HUDWidget.h"

#include "ShooterSamProjectCharacter.h"
#include "ShooterSamProjectGameMode.h"
#include "Gun.h"
#include "Engine/World.h"

void UHUDWidget::SetHealthPercent(float NewPercent)
{
	if (!IsValid(HealthBar) || !FMath::IsFinite(NewPercent))
	{
		return;
	}
	
	HealthBar->SetPercent(FMath::Clamp(NewPercent, 0.0f, 1.0f));
}

void UHUDWidget::SetCoins(int32 NewCoins)
{
	if (!IsValid(CoinsText))
	{
		return;
	}
	
	CoinsText->SetText(
		FText::Format(
			NSLOCTEXT("ShooterHUD", "CoinsLabel", "TheCoins: {0}"),
			FText::AsNumber(FMath::Max(NewCoins, 0))
		)	
	);
}

void UHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	RefreshCombatHUD();
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			CombatHUDTimerHandle,
			this,
			&UHUDWidget::RefreshCombatHUD,
			0.1f,
			true
		);
	}
}

void UHUDWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(
			CombatHUDTimerHandle	
		);
	}
	
	Super::NativeDestruct();
}

void UHUDWidget::RefreshCombatHUD()
{
	AShooterSamProjectCharacter* PlayerCharacter =
		Cast<AShooterSamProjectCharacter>(GetOwningPlayerPawn());
	
	AGun* EquippedGun = IsValid(PlayerCharacter)
		? PlayerCharacter->GetEquippedGun()
		: nullptr;
	
	//武器能量与充能状态
	if (IsValid(EnergyText))
	{
		if (IsValid(EquippedGun))
		{
			const FText ChargeState = EquippedGun->IsRecharging()
				? NSLOCTEXT("ShooterHUD", "Recharging", "充能中")
				: FText::GetEmpty();
			
			EnergyText->SetText(
				FText::Format(
					NSLOCTEXT(
						"ShooterHUD",
						"Energylabel",
						"能量：{0}/{1}  {2}"
					),	
					FText::AsNumber(EquippedGun->GetCurrentEnergy()),
					FText::AsNumber(EquippedGun->GetMaxEnergy()),
					ChargeState
				)	
			);
		}
		else
		{
			EnergyText->SetText(
				NSLOCTEXT(
					"ShooterHUD",
					"NoWeapon",
					"能量：--"
				)	
			);
		}
	}
	
	//玩家备用电池
	if (IsValid(BatteryText))
	{
		if (IsValid(PlayerCharacter))
		{
			BatteryText->SetText(
				FText::Format(
					NSLOCTEXT(
						"ShooterHUD",
						"BatteryLabel",
						"电池： {0}/{1}"
					),	
					FText::AsNumber(PlayerCharacter->GetCurrentBatteryCount()),
					FText::AsNumber(PlayerCharacter->GetMaxBatteryCount())
				)	
			);
		}
		else
		{
			BatteryText->SetText(
				NSLOCTEXT(
					"ShooterHUD",
					"NoBatteryData",
					"电池： --"
				)	
			);
		}
	}
	
	//当前波次，本阶段使用单机GameMode 数据
	UWorld* World = GetWorld();
	
	AShooterSamProjectGameMode* ShooterGameMode = World
		? World->GetAuthGameMode<AShooterSamProjectGameMode>()
		: nullptr;
	
	if (IsValid(WaveText))
	{
		if (IsValid(ShooterGameMode))
		{
			WaveText->SetText(
				FText::Format(
					NSLOCTEXT(
						"ShooterHUD",
						"WaveLabel",
						"波次： {0}"
					),	
					FText::AsNumber(
						ShooterGameMode->CurrentWave	
					)
				)	
			);
		}
		else
		{
			WaveText->SetText(
				NSLOCTEXT(
					"ShooterHUD",
					"NoWaveData",
					"波次： --"
				)	
			);
		}
	}
}
