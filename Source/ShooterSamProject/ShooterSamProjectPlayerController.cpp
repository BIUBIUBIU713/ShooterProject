// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterSamProjectPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "ShooterSamProject.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "ShooterSamProjectCharacter.h"
#include "SupplyStation.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

void AShooterSamProjectPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogShooterSamProject, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
	
	HUDWidget = CreateWidget<UHUDWidget>(this, HUDWidgetClass);
	if (IsValid(HUDWidget))
	{
		HUDWidget->AddToViewport();
		
		AShooterSamProjectCharacter* PlayerCharacter =
			Cast<AShooterSamProjectCharacter>(GetPawn());
		
		if (IsValid(PlayerCharacter))
		{
			HUDWidget->SetHealthPercent(
				PlayerCharacter->GetHealthPercent()	
			);
			
			HUDWidget->SetCoins(
				PlayerCharacter->GetCoins()	
			);
		}
	}
	
	RestoreGameplayInput();
}

void AShooterSamProjectPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool AShooterSamProjectPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

bool AShooterSamProjectPlayerController::OpenSupplyMenu(
	ASupplyStation* Station	
)
{
	AShooterSamProjectCharacter* PlayerCharacter =
		Cast<AShooterSamProjectCharacter>(GetPawn());
	
	if (!IsLocalPlayerController() || bGameOverHandled || bSupplyMenuOpen || !IsValid(Station) || !Station->CanInteract(PlayerCharacter))
	{
		return false;
	}
	
	if (!SupplyMenuClass)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("SupplyMenuClass is not configured"));
		return false;
	}
	
	SupplyMenuWidget = 
		CreateWidget<UUserWidget>(this, SupplyMenuClass);
	
	if (!IsValid(SupplyMenuWidget))
	{
		return false;
	}
	
	//蓝图的Construct 事件需要读取这个站点
	ActiveSupplyStation = Station;
	bSupplyMenuOpen = true;
	bCursorVisibleBeforeSupply = bShowMouseCursor;
	
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	
	PlayerCharacter->ConsumeMovementInputVector();
	PlayerCharacter->GetCharacterMovement()->StopMovementImmediately();
	
	SupplyMenuWidget->AddToViewport(10);
	
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(SupplyMenuWidget->TakeWidget());
	SetInputMode(InputMode);
	
	bShowMouseCursor = true;
	
	UE_LOG(LogTemp, Log,
		TEXT("Supply menu opened: %s"),
		*GetNameSafe(Station));
	
	return true;
}

void AShooterSamProjectPlayerController::CloseSupplyMenu()
{
	if (!bSupplyMenuOpen)
	{
		return;
	}
	
	bSupplyMenuOpen = false;
	ActiveSupplyStation = nullptr;
	
	if (IsValid(SupplyMenuWidget))
	{
		SupplyMenuWidget->RemoveFromParent();
	}
	
	SupplyMenuWidget = nullptr;
	
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
	
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	
	bShowMouseCursor =bCursorVisibleBeforeSupply;
	
	UE_LOG(LogTemp, Log, TEXT("Supply menu closed"));
}

void AShooterSamProjectPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (!bSupplyMenuOpen)
	{
		return;
	}

	AShooterSamProjectCharacter* PlayerCharacter =
		Cast<AShooterSamProjectCharacter>(GetPawn());

	if (!IsValid(SupplyMenuWidget) ||
		!IsValid(ActiveSupplyStation) ||
		!ActiveSupplyStation->CanInteract(PlayerCharacter))
	{
		CloseSupplyMenu();
	}
}

void AShooterSamProjectPlayerController::ShowGameOverScreen(
	int32 WaveNumber,
	int32 RemainingCoins
)
{
	if (!IsLocalPlayerController() || bGameOverHandled)
	{
		return;
	}
	
	//必须先关闭补给菜单，再切换到结算界面
	CloseSupplyMenu();
	
	bGameOverHandled = true;
	
	//在创建控件前保存数据，供蓝图Construct读取
	FinalWave = FMath::Max(WaveNumber, 0);
	FinalCoins = FMath::Max(RemainingCoins, 0);
	
	if (IsValid(HUDWidget))
	{
		HUDWidget->RemoveFromParent();
	}
	
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	
	//禁止游戏输入，但保持世界运行，让死亡动画继续播放
	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);
	
	bShowMouseCursor = false;
	
	const float Delay = FMath::IsFinite(DeathPresentationDuration)
	? FMath::Max(DeathPresentationDuration, 0.1f)
	: 2.0f;
	
	GetWorldTimerManager().SetTimer(
		DeathPresentationTimerHandle,
		this,
		&AShooterSamProjectPlayerController::FinishDeathPresentation,
		Delay,
		false
	);
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Death presentation started: duration=%.2f"),
		Delay
	);
}

void AShooterSamProjectPlayerController::RestartCurrentRun()
{
	if (!bGameOverHandled || bRestartRequested)
	{
		return;
	}
	
	//去掉PIE运行时给地图添加的前缀
	const FString LevelName = 
		UGameplayStatics::GetCurrentLevelName(this, true);
	
	if (LevelName.IsEmpty())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Restart failed: current level name is empty")
		);
		return;
	}
	
	//防止连续点击重复发起加载
	bRestartRequested = true;
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Restarting current run: %s"),
		*LevelName
	);
	
	GetWorldTimerManager().ClearTimer(
		DeathPresentationTimerHandle
	);
	
	if (IsValid(GameOverWidget))
	{
		GameOverWidget->RemoveFromParent();
		GameOverWidget = nullptr;
	}
	
	RestoreGameplayInput();
	
	SetPause(false);
	
	UGameplayStatics::OpenLevel(
		this,
		FName(*LevelName)
	);
}

void AShooterSamProjectPlayerController::FinishDeathPresentation()
{
	if (!bGameOverHandled || bRestartRequested || IsValid(GameOverWidget))
	{
		return;
	}
	
	if (GameOverWidgetClass)
	{
		GameOverWidget = 
			CreateWidget<UUserWidget>(this, GameOverWidgetClass);
	}
	
	FInputModeUIOnly InputMode;
	
	if (IsValid(GameOverWidget))
	{
		GameOverWidget->AddToViewport(20);
		InputMode.SetWidgetToFocus(GameOverWidget->TakeWidget());
	}
	else
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Game over widget could not be created; check GameOverWidgetClass")
		);
	}
	
	SetInputMode(InputMode);
	bShowMouseCursor = true;
	
	//死亡表现时间结束，现在才暂停
	const bool bPausedSuccessfully = SetPause(true);
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Game over screen: wave=%d, coins=%d, paused=%d"),
		FinalWave,
		FinalCoins,
		bPausedSuccessfully ? 1 : 0
	);
}

//清理计时器
void AShooterSamProjectPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(
		DeathPresentationTimerHandle	
	);
	
	Super::EndPlay(EndPlayReason);
}

void AShooterSamProjectPlayerController::RestoreGameplayInput()
{
	if (!IsLocalPlayerController())
	{
		return;
	}
	
	//新一局开始时，清空移动与视角的输入禁用计数
	ResetIgnoreMoveInput();
	ResetIgnoreLookInput();
	
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	
	bShowMouseCursor = false;
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Gameplay input restored: moveIgnored=%d, lookIgnored=%d"),
		IsMoveInputIgnored() ? 1 : 0,
		IsLookInputIgnored() ? 1 : 0
	);
}
