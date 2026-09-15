// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterSamProjectCharacter.h"

#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ShooterSamProject.h"
#include "ShooterSamProjectPlayerController.h"

#include "ShooterInteractable.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "ShooterSamProjectGameMode.h"


AShooterSamProjectCharacter::AShooterSamProjectCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
	
	PrimaryActorTick.bCanEverTick = true;
}

void AShooterSamProjectCharacter::BeginPlay()
{
	MaxBatteryCount = FMath::Max(MaxBatteryCount, 0);
	CurrentBatteryCount = MaxBatteryCount;
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s batteries initialized: %d/%d"),
		*GetName(),
		CurrentBatteryCount,
		MaxBatteryCount
	);
	
	MaxHealth = FMath::IsFinite(MaxHealth) ? FMath::Max(MaxHealth, 1.0f) : 100.0f;
	
	Health = MaxHealth;
	IsAlive = true;
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s health initialized: %.1f/%.1f"),
		*GetName(),
		Health,
		MaxHealth
	);
	
	StartingCoins = FMath::Max(StartingCoins, 0);
	Coins = StartingCoins;
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s coins initialized: %d"),
		*GetName(),
		Coins
	);
	
	Super::BeginPlay();
	
	OnTakeAnyDamage.AddDynamic(this, &AShooterSamProjectCharacter::OnDamageTaken);
	UpdateHUD();
	
	GetMesh()->HideBoneByName("weapon_r", EPhysBodyOp::PBO_None);
	
	GunMember = GetWorld()->SpawnActor<AGun>(GunClass);
	if (GunMember)
	{
		GunMember->SetOwner(this);
		GunMember->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, TEXT("WeaponSocket"));
		GunMember->OwnerController = GetController();
	}
}

void AShooterSamProjectCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AShooterSamProjectCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AShooterSamProjectCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AShooterSamProjectCharacter::Look);
		
		//Shooting
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Started, this, &AShooterSamProjectCharacter::Shoot);
		
		//Recharging
		if (RechargeAction)
		{
			EnhancedInputComponent->BindAction(RechargeAction, ETriggerEvent::Started, this, &AShooterSamProjectCharacter::Recharge);
		}
		else
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("%s:RechargeAction is not configured"),
				*GetName()
			);
		}
		
		//Interacting
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(
				InteractAction,
				ETriggerEvent::Started,
				this,
				&AShooterSamProjectCharacter::Interact
			);
		}
		else
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("%s: InteractAction is not configured"),
				*GetName()
			);
		}
	}
	else
	{
		UE_LOG(LogShooterSamProject, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AShooterSamProjectCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AShooterSamProjectCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AShooterSamProjectCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AShooterSamProjectCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AShooterSamProjectCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AShooterSamProjectCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}

void AShooterSamProjectCharacter::OnDamageTaken(AActor* DamagedActor, float Damage, const  UDamageType* DamageType,
	class AController* InstigatedBy, AActor* DamageCauser)
{
	if (!IsAlive || !FMath::IsFinite(Damage) || Damage <= 0.0f)
	{
		return;
	}
	
	Health = FMath::Clamp(
		Health - Damage,
		0.0f,
		MaxHealth
	);
	
	if (Health <= 0.0f)
	{
		IsAlive = false;
	}
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s took %.1f damage, health: %.1f/%.1f"),
		*GetName(),
		Damage,
		Health,
		MaxHealth
	);
	
	//先刷新血条，此时仍能取得玩家控制器
	UpdateHUD();
	
	if (!IsAlive)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("%s player died"),
			*GetName()
		);
		
		SetCanBeDamaged(false);
		
		if (UCharacterMovementComponent* Movement = GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
		
		AShooterSamProjectGameMode* ShooterGameMode = 
			GetWorld()->GetAuthGameMode<AShooterSamProjectGameMode>();
		
		int32 ReachedWave = 0;
		
		if (IsValid(ShooterGameMode))
		{
			ReachedWave = ShooterGameMode->CurrentWave;
			ShooterGameMode->SetGameOver();
		}
		else
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("Player death: ShooterGameMode was not found")
			);
		}
		
		AShooterSamProjectPlayerController* PlayerController = 
			Cast<AShooterSamProjectPlayerController>(GetController());
		
		if (IsValid(PlayerController))
		{
			PlayerController->ShowGameOverScreen(
				ReachedWave,
				GetCoins()
			);
		}
	}
}


void AShooterSamProjectCharacter::Shoot()
{
	if (!IsAlive || !IsValid(GunMember) || !IsValid(GetController()))
	{
		return;
	}
	
	GunMember->OwnerController = GetController();
	GunMember->PullTrigger();
}

void AShooterSamProjectCharacter::UpdateHUD()
{
	AShooterSamProjectPlayerController* PlayerController = Cast<AShooterSamProjectPlayerController>(GetController());
	
	if (!IsValid(PlayerController) || !IsValid(PlayerController->HUDWidget))
	{
		return;
	}
	
	PlayerController->HUDWidget->SetHealthPercent(GetHealthPercent());
	PlayerController->HUDWidget->SetCoins(GetCoins());
}

void AShooterSamProjectCharacter::Recharge()
{
	if (!IsAlive || !IsPlayerControlled() || !IsValid(GetController()) || !IsValid(GunMember))
	{
		return;
	}
	
	if (CurrentBatteryCount <= 0)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("%s recharge rejected: no spare batteries"),
			*GetName()
		);
		
		return;
	}
	
	//枪未满且没有正在充能时，才会成功
	if (!GunMember->TryStartRecharge())
	{
		return;
	}
	
	//成功启动后，立即消耗一块备用电池
	--CurrentBatteryCount;
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s used one battery: %d/%d remaining"),
		*GetName(),
		CurrentBatteryCount,
		MaxBatteryCount
	);
}

int32 AShooterSamProjectCharacter::AddBatteries(int32 Amount)
{
	if (!IsAlive || !IsPlayerControlled() || Amount <= 0)
	{
		return 0;
	}
	
	const int32 AvailableSpace =
		FMath:: Max(MaxBatteryCount - CurrentBatteryCount, 0);
	
	const int32 AddedAmount = 
		FMath::Min(Amount, AvailableSpace);

	if (AddedAmount <= 0)
	{
		return 0;
	}
	
	CurrentBatteryCount += AddedAmount;
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s received %d batteries: %d/%d"),
		*GetName(),
		AddedAmount,
		CurrentBatteryCount,
		MaxBatteryCount
	);
	
	return AddedAmount;
}

//目标查找函数
AActor* AShooterSamProjectCharacter::FindInteractionTarget()
{
	if (!IsAlive || !IsPlayerControlled() || !IsValid(GetWorld()))
	{
		return nullptr;
	}
	
	TArray<AActor*> Candidates;
	GetOverlappingActors(Candidates);
	
	AActor* BestTarget = nullptr;
	double BestDistanceSquared = TNumericLimits<double>::Max();
	
	for (AActor* Candidate : Candidates)
	{
		if (!IsValid(Candidate))
		{
			continue;
		}
		
		IShooterInteractable* Interactable= Cast<IShooterInteractable>(Candidate);
		
		if (!Interactable || !Interactable->CanInteract(this))
		{
			continue;
		}
		
		//防止隔着墙领取补给
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		Params.AddIgnoredActor(Candidate);
		
		if (IsValid(GunMember))
		{
			Params.AddIgnoredActor(GunMember);
		}
		
		const bool bBlocked =
			GetWorld()->LineTraceTestByChannel(
				GetPawnViewLocation(),
				Candidate->GetActorLocation(),
				ECC_Visibility,
				Params
			);
		
		if (bBlocked)
		{
			continue;
		}
		
		const double DistanceSquared = FVector::DistSquared(
			GetActorLocation(),
			Candidate->GetActorLocation()
		);
		
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTarget = Candidate;
		}
	}
	
	return BestTarget;
}

void AShooterSamProjectCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	CurrentInteractionTarget = FindInteractionTarget();
	
	AActor* Target = CurrentInteractionTarget.Get();
	
	if (!IsValid(Target))
	{
		return;
	}
	
	IShooterInteractable* Interactable = Cast<IShooterInteractable>(Target);
	
	if (!Interactable)
	{
		return;
	}
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			64001,
			0.1f,
			FColor::Green,
			Interactable->GetInteractionText().ToString()
		);
	}
	
	//绿色标记对应当前会被交互的那个补给物
	DrawDebugSphere(
		GetWorld(),
		Target->GetActorLocation(),
		25.0f,
		12,
		FColor::Green,
		false,
		0.0f,
		0,
		2.0f
	);
}

void AShooterSamProjectCharacter::Interact()
{
	AActor* Target = CurrentInteractionTarget.Get();
	
	if (!IsValid(Target))
	{
		return;
	}
	
	//按键时再次核对目标
	//如果位置或目标已经变化，本次不领取另一个物体
	if (FindInteractionTarget() != Target)
	{
		return;
	}
	
	IShooterInteractable* Interactable = Cast<IShooterInteractable>(Target);
	
	if (Interactable && Interactable->TryInteract(this))
	{
		CurrentInteractionTarget.Reset();
	}
}

float AShooterSamProjectCharacter::RestoreHealth(float Amount)
{
	//普通回血不能复活，也不能接受无效或负数恢复量
	if (!IsAlive || Health <= 0.0f || !FMath::IsFinite(Amount) || Amount <= 0.0f)
	{
		return 0.0f;
	}
	
	const float MissingHealth = FMath::Max(MaxHealth - Health, 0.0f);
	
	const float RecoveryAmount = FMath::Min(Amount, MissingHealth);
	
	if (RecoveryAmount <= 0.0f)
	{
		return 0.0f;
	}
	
	const float PreviousHealth = Health;
	
	Health = FMath::Min(Health + RecoveryAmount, MaxHealth);
	
	const float ActualRecovery = Health - PreviousHealth;
	
	UpdateHUD();
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s restored %.1f health: %.1f/%.1f"),
		*GetName(),
		ActualRecovery,
		Health,
		MaxHealth
	);
	
	return ActualRecovery;
}

int32 AShooterSamProjectCharacter::AddCoins(int32 Amount)
{
	if (!IsPlayerControlled() || Amount <= 0)
	{
		return 0;
	}
	
	//避免整数溢出
	const int32 AddedAmount = 
		FMath::Min(Amount, MAX_int32 - Coins);
	
	if (AddedAmount <= 0)
	{
		return 0;
	}
	
	Coins += AddedAmount;
	UpdateHUD();
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s gained %d coins, balance: %d"),
		*GetName(),
		AddedAmount,
		Coins
	);
	
	return AddedAmount;
}

bool AShooterSamProjectCharacter::TrySpendCoins(int32 Cost)
{
	if (!IsAlive || !IsPlayerControlled() || Cost <= 0)
	{
		return false;
	}
	
	if (Coins < Cost)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("%s purchase rejected: cost=%d, balance=%d"),
			*GetName(),
			Cost,
			Coins
		);

		return false;
	}
	
	Coins -= Cost;
	UpdateHUD();
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s spent %d coins, balance: %d"),
		*GetName(),
		Cost,
		Coins
	);
	
	return true;
}

ESupplyPurchaseResult AShooterSamProjectCharacter::TryBuyBatteries(int32 Amount, int32 Cost)
{
	if (!IsAlive || !IsPlayerControlled())
	{
		return ESupplyPurchaseResult::InvalidPlayer;
	}
	
	if (Amount <= 0 || Cost <= 0)
	{
		return ESupplyPurchaseResult::InvalidOffer;
	}
	
	const int32 AvailableSpace = 
		FMath::Max(MaxBatteryCount - CurrentBatteryCount, 0);
	
	if (AvailableSpace == 0)
	{
		return ESupplyPurchaseResult::ResourceFull;
	}
	
	if (Coins < Cost)
	{
		return ESupplyPurchaseResult::InsufficientCoins;
	}
	
	const int32 AddedAmount = FMath::Min(Amount, AvailableSpace);
	
	//检查全部通过后，一起提交资源和金币变化
	CurrentBatteryCount += AddedAmount;
	Coins -= Cost;
	
	UpdateHUD();
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s purchased batteries: added=%d, cost=%d, batteries=%d/%d, coins=%d"),
		*GetName(),
		AddedAmount,
		Cost,
		CurrentBatteryCount,
		MaxBatteryCount,
		Coins
	);
	
	return ESupplyPurchaseResult::Success;
}

ESupplyPurchaseResult AShooterSamProjectCharacter::TryBuyHealing(
	float Amount,
	int32 Cost
)
{
	if (!IsAlive || !IsPlayerControlled() || !FMath::IsFinite(Health) || Health <= 0.0f)
	{
		return ESupplyPurchaseResult::InvalidPlayer;
	}
	
	if (!FMath::IsFinite(Amount) || Amount <= 0.0f || !FMath::IsFinite(MaxHealth) || MaxHealth <= 0.0f
		|| Cost <= 0	
	)
	{
		return ESupplyPurchaseResult::InvalidOffer;
	}
	
	if (Health >= MaxHealth)
	{
		return ESupplyPurchaseResult::ResourceFull;
	}
	
	if (Coins < Cost)
	{
		return ESupplyPurchaseResult::InsufficientCoins;
	}
	
	const float MissingHealth = MaxHealth - Health;
	const float RecoveryAmount = FMath::Min(Amount, MissingHealth);
	const float NewHealth = FMath::Min(Health + RecoveryAmount, MaxHealth);
	const float ActualRecovery = NewHealth - Health;
	
	//确认生命值确实能够增加，再允许扣款
	if (ActualRecovery <= 0.0f)
	{
		return ESupplyPurchaseResult::InvalidOffer;
	}
	
	Health = NewHealth;
	Coins -= Cost;
	
	UpdateHUD();
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s purchased healing: restored=%.1f, cost=%d, health=%.1f/%.1f, coins=%d"),
		*GetName(),
		ActualRecovery,
		Cost,
		Health,
		MaxHealth,
		Coins
	);
		
	return ESupplyPurchaseResult::Success;
}