// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "HUDWidget.h"
#include "TimerManager.h"

#include "ShooterSamProjectPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class ASupplyStation;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AShooterSamProjectPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;
	
	UPROPERTY(EditDefaultsOnly, Category = "Supply")
	TSubclassOf<UUserWidget> SupplyMenuClass;
	
	UPROPERTY()
	TObjectPtr<UUserWidget> SupplyMenuWidget = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "GameOver")
	TSubclassOf<UUserWidget> GameOverWidgetClass;
	
	UPROPERTY()
	TObjectPtr<UUserWidget> GameOverWidget = nullptr;
	
	//死亡后预留给动画的播放时间
	UPROPERTY(
		EditDefaultsOnly,
		Category = "GameOver",
		meta = (ClampMin = "0.1")
	)
	float DeathPresentationDuration = 2.0f;
	
	FTimerHandle DeathPresentationTimerHandle;
	
	//延迟结束后，显示结算并暂停
	void FinishDeathPresentation();
	
	void RestoreGameplayInput();
	
	bool bGameOverHandled = false;
	bool bRestartRequested = false;
	
	bool bSupplyMenuOpen = false;
	bool bCursorVisibleBeforeSupply = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;
	
	virtual void PlayerTick(float DeltaTime)override;
	
	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason	
	) override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

public:
	void ShowGameOverScreen(int32 WaveNumber, int32 RemainingCoins);
	
	UFUNCTION(BlueprintCallable, Category = "GameOver")
	void RestartCurrentRun();
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> HUDWidgetClass;
	
	UPROPERTY(VisibleAnywhere)
	UHUDWidget* HUDWidget;
	
	UFUNCTION(BlueprintCallable, Category = "Supply")
	bool OpenSupplyMenu(ASupplyStation* Station);
	
	UFUNCTION(BlueprintCallable, Category = "Supply")
	void CloseSupplyMenu();
	
	UPROPERTY(BlueprintReadOnly, Category = "Supply")
	TObjectPtr<ASupplyStation> ActiveSupplyStation = nullptr;
	
	UPROPERTY(BlueprintReadOnly, Category = "GameOver")
	int32 FinalWave = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "GameOver")
	int32 FinalCoins = 0;
};
