// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ShooterProgressSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERSAMPROJECT_API UShooterProgressSaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
	//快速换弹登记，0 表示未购买
	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly, Category = "Upgrades")
	int32 ReloadUpgradeLevel = 0;
	
	//灵活移动等级， 0 表示未购买
	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly, Category = "Upgrades")
	int32 MovementUpgradeLevel = 0;
};
