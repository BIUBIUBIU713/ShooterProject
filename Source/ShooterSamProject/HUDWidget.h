// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

#include "HUDWidget.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERSAMPROJECT_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	void RefreshCombatHUD();
	
	FTimerHandle CombatHUDTimerHandle;
	
	
public:
	UPROPERTY(EditAnywhere, meta = (BindWidgetOptional))
	UProgressBar* HealthBar;
	
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* CoinsText = nullptr;
	
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* EnergyText = nullptr;
	
	UPROPERTY(meta = (BindWidgetoptional))
	UTextBlock* BatteryText = nullptr;
	
	UPROPERTY(meta = (BindWidgetoptional))
	UTextBlock* WaveText = nullptr;
	
	void SetHealthPercent(float NewPercent);
	void SetCoins(int32 NewCoins);
	
};
