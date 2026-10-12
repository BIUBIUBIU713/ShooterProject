#include "ShooterBossEnemy.h"

#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ShooterEnemyAIController.h"
#include "ShooterSamProjectCharacter.h"
#include "ShooterSamProjectGameMode.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/LatentActionManager.h"
#include "UObject/ConstructorHelpers.h"

AShooterBossEnemy::AShooterBossEnemy()
{
	AIControllerClass = AShooterEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	static ConstructorHelpers::FObjectFinder<UAnimMontage> DeathAsset(
		TEXT("/Game/SciFi_ToiletMech/Animation/AM_Boss_Death.AM_Boss_Death"));
	DeathMontage = DeathAsset.Object;
	SetCanBeDamaged(false);
}

void AShooterBossEnemy::BeginPlay()
{
	Super::BeginPlay();
	if (!bWaitForEncounterSwitch)
	{
		BeginEntrance();
	}
}

bool AShooterBossEnemy::CanStartEncounter() const
{
	return HasActorBegunPlay() && bWaitForEncounterSwitch && !bEntranceStarted &&
		!bCombatReady && !bEncounterCancelled && !bEndingPlay && !IsDead();
}

bool AShooterBossEnemy::StartEncounterEntrance()
{
	if (!CanStartEncounter()) return false;
	BeginEntrance();
	return bEntranceStarted && !bEncounterCancelled;
}

void AShooterBossEnemy::BeginEntrance()
{
	const AShooterSamProjectGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<AShooterSamProjectGameMode>() : nullptr;
	if (bEntranceStarted || bEncounterCancelled || bEndingPlay || IsDead() ||
		(IsValid(GameMode) && GameMode->WaveState == EWaveState::GameOver)) return;
	bEntranceStarted = true;
	SetCanBeDamaged(false);
	PlayEncounterEntrance();
}

bool AShooterBossEnemy::FinishEncounterEntrance(UBehaviorTree* EntranceBehaviorTree)
{
	const AShooterSamProjectGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<AShooterSamProjectGameMode>() : nullptr;
	if (!bEntranceStarted || bCombatReady || bEncounterCancelled || bEndingPlay || IsDead() ||
		!IsValid(EntranceBehaviorTree) || !EntranceBehaviorTree->RootNode ||
		(IsValid(GameMode) && GameMode->WaveState == EWaveState::GameOver)) return false;

	AAIController* AI = Cast<AAIController>(GetController());
	if (!IsValid(AI) || !AI->RunBehaviorTree(EntranceBehaviorTree)) return false;
	const UBehaviorTreeComponent* Tree = Cast<UBehaviorTreeComponent>(AI->GetBrainComponent());
	if (!Tree || !Tree->IsRunning() || Tree->GetRootTree() != EntranceBehaviorTree) return false;
	// 生命周期的一次性转换；半血特殊攻击 StopTree/RestartLogic 不改变此状态。
	bCombatReady = true;
	SetCanBeDamaged(true);
	UE_LOG(LogTemp, Log, TEXT("[Boss] Entrance complete: combat and damage enabled"));
	return true;
}

void AShooterBossEnemy::CancelEncounter()
{
	if (bEncounterCancelled || bEndingPlay || IsDead()) return;
	bEncounterCancelled = true;
	bCombatReady = false;
	SetCanBeDamaged(false);
	ClearCombatForDeathOrEndPlay();
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (UBehaviorTreeComponent* BT = Cast<UBehaviorTreeComponent>(AI->GetBrainComponent()))
		{
			BT->StopTree(EBTStopMode::Forced);
		}
		AI->StopMovement();
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
}

void AShooterBossEnemy::ClearCombatForDeathOrEndPlay()
{
	// 先让所有旧回调失去资格，再停止蒙太奇（停止可能同步触发回调）。
	InterruptedBehaviorTree.Reset();
	bFirstSpecialPending = false;
	bSpecialActive = false;
	bRoaring = false;
	bSpecialStopping = true;
	ActiveRoarMontage = nullptr;
	RoarInstanceId = INDEX_NONE;
	SpecialTarget.Reset();
	RestoreSpecialRotation();
	bAttackPlaying = false;
	bHitHandled = true;
	AttackInstanceId = INDEX_NONE;
	ExpectedHitNotify = NAME_None;
	ActiveAttackMontage = nullptr;
	AttackTarget.Reset();
	bShooting = false;
	bShootStopping = true;
	ShootTarget.Reset();
	ActiveShootMontage = nullptr;
	ShootInstanceId = INDEX_NONE;
	ShootEndTime = 0.0;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ShootFireTimerHandle);
		World->GetTimerManager().ClearTimer(ShootDurationTimerHandle);
		World->GetTimerManager().ClearAllTimersForObject(this);
		// 入场蓝图中尚未完成的 Delay 也不能在死亡后重新启动 AI。
		World->GetLatentActionManager().RemoveActionsForObject(this);
	}
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(this, &AShooterBossEnemy::HandleAttackNotify);
		AnimInstance->StopAllMontages(0.0f);
	}
}

void AShooterBossEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	ClearCombatForDeathOrEndPlay();
	Super::EndPlay(EndPlayReason);
}

void AShooterBossEnemy::OnDeathStarted()
{
	Super::OnDeathStarted();
	bCombatReady = false;
	ClearCombatForDeathOrEndPlay();
	USkeletalMeshComponent* BossMesh = GetMesh();
	UAnimInstance* AnimInstance = BossMesh->GetAnimInstance();
	if (!AnimInstance || !IsValid(DeathMontage))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Boss] Death montage or animation instance missing"));
		return;
	}
	// 即使离开镜头，死亡动画也应推进到最终姿态。
	BossMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	BossMesh->bPauseAnims = false;
	const float Duration = AnimInstance->Montage_Play(DeathMontage, 1.0f, EMontagePlayReturnType::Duration);
	if (Duration <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Boss] Death montage failed to play"));
		return;
	}
	if (FAnimMontageInstance* Instance = AnimInstance->GetActiveInstanceForMontage(DeathMontage))
	{
		// 对本次实例禁用自动淡出；不依赖结束通知，避免回到待机。
		Instance->bEnableAutoBlendOut = false;
	}
	// 基类稍后统一设置寿命、关闭碰撞并结算奖励，其他敌人仍使用原规则。
	DeathLifeSpan = FMath::Max(DeathLifeSpan, Duration + 0.5f);
	UE_LOG(LogTemp, Log, TEXT("[Boss] Death started: montage=%.2fs lifespan=%.2fs"), Duration, DeathLifeSpan);
}

bool AShooterBossEnemy::IsAttackBusy() const
{
	return !bCombatReady || bEncounterCancelled || bSpecialActive || bShooting || IsMeleeAttackBusy();
}

float AShooterBossEnemy::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (!bCombatReady || bEncounterCancelled || bEndingPlay || IsDead())
	{
		return 0.0f;
	}
	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (Applied > 0.0f && !IsDead() && !bHalfHealthTriggered &&
		GetCurrentHealth() <= GetMaxHealth() * 0.5f)
	{
		bHalfHealthTriggered = true;
		bFirstSpecialPending = true;
		AShooterSamProjectCharacter* DamageTarget = IsValid(EventInstigator)
			? Cast<AShooterSamProjectCharacter>(EventInstigator->GetPawn()) : nullptr;
		UE_LOG(LogTemp, Log, TEXT("[Boss] Half health: interrupting current action for first special attack"));
		if (!StartSpecialAttack(true, DamageTarget))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Boss] Immediate half-health special could not start; check target and montage settings"));
		}
	}
	return Applied;
}

float AShooterBossEnemy::GetOrdinaryIntervalMultiplier() const
{
	return bEnraged && FMath::IsFinite(EnrageAttackIntervalMultiplier)
		? FMath::Clamp(EnrageAttackIntervalMultiplier, 0.01f, 1.0f) : 1.0f;
}

void AShooterBossEnemy::ApplyEnrage()
{
	if (!bCombatReady || bEncounterCancelled || bEndingPlay || IsDead() || bEnraged)
	{
		return;
	}
	bEnraged = true;
	const float Multiplier = FMath::IsFinite(EnrageMoveSpeedMultiplier)
		? FMath::Max(1.0f, EnrageMoveSpeedMultiplier) : 1.2f;
	GetCharacterMovement()->MaxWalkSpeed *= Multiplier;
	UE_LOG(LogTemp, Log, TEXT("[Boss] Enrage applied once: move speed=%.1f interval multiplier=%.2f"),
		GetCharacterMovement()->MaxWalkSpeed, GetOrdinaryIntervalMultiplier());
}

bool AShooterBossEnemy::HasValidShootSettings(bool bSpecial) const
{
	const float Duration = bSpecial ? SpecialShootDuration : ShootDuration;
	const float Interval = bSpecial ? SpecialShootInterval : ShootInterval;
	const float Range = bSpecial ? SpecialShootRange : ShootRange;
	const float Damage = bSpecial ? SpecialShootDamage : ShootDamage;
	return IsValid(ShootMontage.Get()) && GetMesh()->DoesSocketExist(ShootMuzzleSocketName) &&
		FMath::IsFinite(Duration) && Duration > 0.0f &&
		FMath::IsFinite(Interval) && Interval > 0.0f &&
		FMath::IsFinite(Range) && Range > 0.0f &&
		FMath::IsFinite(Damage) && Damage > 0.0f;
}

bool AShooterBossEnemy::TryStartSpecialAttack()
{
	return StartSpecialAttack(false);
}

void AShooterBossEnemy::InterruptForFirstSpecialAttack()
{
	AAIController* AI = Cast<AAIController>(GetController());
	UBehaviorTreeComponent* Tree = AI ? Cast<UBehaviorTreeComponent>(AI->GetBrainComponent()) : nullptr;
	if (Tree && Tree->IsRunning())
	{
		InterruptedBehaviorTree = Tree;
		// 先结束旧任务，让它的 OnTaskFinished 清理完毕，再启动咆哮。
		Tree->StopTree(EBTStopMode::Forced);
	}
	if (AI)
	{
		AI->StopMovement();
	}
	GetCharacterMovement()->StopMovementImmediately();

	// 立即撤销旧攻击的命中资格和开火定时器，同时保留各招独立冷却。
	// 必须在咆哮开始前清空状态，旧蒙太奇稍后到达的回调就会被忽略。
	HandleAttackEnd(ActiveAttackMontage.Get(), true);
	HandleShootEnd(ActiveShootMontage.Get(), true);
	GetWorld()->GetTimerManager().ClearTimer(ShootFireTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(ShootDurationTimerHandle);
	NextAttackTime = 0.0;
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->StopAllMontages(0.0f);
	}
}

void AShooterBossEnemy::RestartBehaviorTreeAfterSpecial()
{
	if (!InterruptedBehaviorTree.IsValid() || IsAttackBusy())
	{
		return;
	}
	UBehaviorTreeComponent* Tree = InterruptedBehaviorTree.Get();
	AAIController* AI = Cast<AAIController>(GetController());
	if (!bCombatReady || bEncounterCancelled || bEndingPlay || IsDead() || !AI || AI->GetBrainComponent() != Tree)
	{
		InterruptedBehaviorTree.Reset();
		return;
	}
	if (!Tree->IsRunning())
	{
		// 入场蓝图通过 RunBehaviorTree 启动 AI，不一定填写 Boss 的 BehaviorTree 属性。
		// StopTree 会保留组件的 TreeStartInfo，RestartLogic 可恢复实际被中断的树。
		Tree->RestartLogic();
	}
	InterruptedBehaviorTree.Reset();
	if (Tree->IsRunning())
	{
		UE_LOG(LogTemp, Log, TEXT("[Boss] First special finished; behavior tree resumed: %s"),
			*GetNameSafe(Tree->GetRootTree()));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Boss] First special finished, but behavior tree did not restart"));
	}
}

bool AShooterBossEnemy::StartSpecialAttack(bool bInterruptCurrentAction, AShooterSamProjectCharacter* DamageTarget)
{
	UWorld* World = GetWorld();
	if (!World || !bCombatReady || bEncounterCancelled || bEndingPlay || IsDead() || bSpecialActive || (!bInterruptCurrentAction && IsAttackBusy()) || !bHalfHealthTriggered ||
		(!bFirstSpecialPending && GetCurrentHealth() > GetMaxHealth() * 0.5f) ||
		(!bInterruptCurrentAction && World->GetTimeSeconds() < NextSpecialReadyTime) ||
		!IsValid(RoarMontage.Get()) || RoarMontage == ShootMontage || !HasValidShootSettings(true))
	{
		return false;
	}
	AAIController* AI = Cast<AAIController>(GetController());
	const UBlackboardComponent* Blackboard = AI ? AI->GetBlackboardComponent() : nullptr;
	AShooterSamProjectCharacter* Target = Blackboard
		? Cast<AShooterSamProjectCharacter>(Blackboard->GetValueAsObject(TEXT("TargetActor"))) : nullptr;
	const auto IsValidTarget = [World](AShooterSamProjectCharacter* Candidate)
	{
		return IsValid(Candidate) && Candidate->IsAlive && Candidate->IsPlayerControlled() &&
			Candidate->GetWorld() == World;
	};
	if (bInterruptCurrentAction)
	{
		// 优先对伤害来源反击；感知尚未发现玩家时也能触发半血阶段。
		if (IsValidTarget(DamageTarget))
		{
			Target = DamageTarget;
		}
		else if (!IsValidTarget(Target))
		{
			Target = Cast<AShooterSamProjectCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
		}
	}
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (!AI || !IsValidTarget(Target) || !FMath::IsFinite(GetDistanceTo(Target)) || !AnimInstance ||
		(!bInterruptCurrentAction && (GetDistanceTo(Target) > SpecialShootRange || AnimInstance->IsAnyMontagePlaying())))
	{
		return false;
	}
	// 一次普通攻击成功开始后才允许下一次抽签，避免行为树每帧重试提高概率。
	if (!bFirstSpecialPending)
	{
		if (bSpecialSelectionRolled)
		{
			return false;
		}
		bSpecialSelectionRolled = true;
		const float Chance = FMath::IsFinite(SpecialAttackChance)
			? FMath::Clamp(SpecialAttackChance, 0.0f, 1.0f) : 0.25f;
		if (FMath::FRand() >= Chance)
		{
			return false;
		}
	}
	if (bInterruptCurrentAction)
	{
		InterruptForFirstSpecialAttack();
	}
	UAnimMontage* Montage = RoarMontage.Get();
	if (AnimInstance->Montage_Play(Montage, 1.0f) <= 0.0f)
	{
		return false;
	}
	FAnimMontageInstance* Instance = AnimInstance->GetActiveInstanceForMontage(Montage);
	if (!Instance)
	{
		AnimInstance->Montage_Stop(0.2f, Montage);
		return false;
	}
	bSpecialActive = true;
	bRoaring = true;
	bSpecialStopping = false;
	bSpecialShootStopped = false;
	ActiveRoarMontage = Montage;
	RoarInstanceId = Instance->GetInstanceID();
	SpecialTarget = Target;
	AI->StopMovement();
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->StopMovementImmediately();
	bSavedUseControllerRotationYaw = bUseControllerRotationYaw;
	bSavedOrientRotationToMovement = Movement->bOrientRotationToMovement;
	bSavedUseControllerDesiredRotation = Movement->bUseControllerDesiredRotation;
	bSpecialRotationSaved = true;
	bUseControllerRotationYaw = false;
	Movement->bOrientRotationToMovement = false;
	Movement->bUseControllerDesiredRotation = false;
	ApplyEnrage();
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AShooterBossEnemy::HandleRoarEnd);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
	UE_LOG(LogTemp, Log, TEXT("[Boss] Special roar started"));
	return true;
}

void AShooterBossEnemy::HandleRoarEnd(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bCombatReady || bEncounterCancelled || bEndingPlay || IsDead() || !bSpecialActive || !bRoaring || Montage != ActiveRoarMontage.Get())
	{
		return;
	}
	bRoaring = false;
	ActiveRoarMontage = nullptr;
	RoarInstanceId = INDEX_NONE;
	// 只接受完整结束。死亡、任务中止或动画被替换都不能进入扫射。
	if (bInterrupted || bSpecialStopping || IsDead())
	{
		FinishSpecialAttack();
		return;
	}
	// UE 在派发结束回调后才删除 MontageInstances 中的旧实例。
	// 保持特殊攻击忙碌，由下一次 Tick 启动扫射，避免 IsAnyMontagePlaying 拦截。
	UE_LOG(LogTemp, Log, TEXT("[Boss] Special roar completed; waiting for next tick to start shooting"));
}

void AShooterBossEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bCombatReady || bEncounterCancelled || bEndingPlay || IsDead())
	{
		return;
	}
	RestartBehaviorTreeAfterSpecial();
	if (!bSpecialActive)
	{
		return;
	}
	AShooterSamProjectCharacter* Target = SpecialTarget.Get();
	if (IsDead() || !IsValid(GetController()) || !IsValid(Target) || !Target->IsAlive ||
		!Target->IsPlayerControlled() || Target->GetWorld() != GetWorld())
	{
		StopSpecialAttack();
		return;
	}
	// 咆哮已正常结束、扫射尚未启动：此时旧动画结束事件已经派发完毕。
	// 死亡/目标失效检查在前面，任务中止也会清除此特殊攻击状态。
	if (!bRoaring && !bShooting)
	{
		if (bSpecialStopping || !StartShoot(true))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Boss] Special shooting could not start after roar"));
			FinishSpecialAttack();
		}
		return;
	}
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	const int32 ExpectedInstance = bRoaring ? RoarInstanceId : ShootInstanceId;
	if (!AnimInstance || !AnimInstance->GetMontageInstanceForID(ExpectedInstance))
	{
		StopSpecialAttack();
		return;
	}
	// 咆哮保持朝向，扫射时才按角速度追踪；普通射击行为保持原样。
	if (!bRoaring && bShooting && !bShootStopping && FMath::IsFinite(SpecialTurnSpeed))
	{
		const FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (!Direction.IsNearlyZero())
		{
			FRotator Rotation = GetActorRotation();
			Rotation.Yaw = FMath::FixedTurn(Rotation.Yaw, Direction.Rotation().Yaw,
				FMath::Max(0.0f, SpecialTurnSpeed) * DeltaSeconds);
			SetActorRotation(Rotation);
		}
	}
}

void AShooterBossEnemy::MarkSpecialShootStopped()
{
	if (bSpecialActive && bShooting && !bSpecialShootStopped)
	{
		bSpecialShootStopped = true;
		const float Cooldown = FMath::IsFinite(SpecialCooldown) ? FMath::Max(0.0f, SpecialCooldown) : 20.0f;
		NextSpecialReadyTime = GetWorld()->GetTimeSeconds() + Cooldown;
		UE_LOG(LogTemp, Log, TEXT("[Boss] Special firing stopped; cooldown=%.2f"), Cooldown);
	}
}

void AShooterBossEnemy::RestoreSpecialRotation()
{
	if (bSpecialRotationSaved)
	{
		bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
		GetCharacterMovement()->bOrientRotationToMovement = bSavedOrientRotationToMovement;
		GetCharacterMovement()->bUseControllerDesiredRotation = bSavedUseControllerDesiredRotation;
		bSpecialRotationSaved = false;
	}
}

void AShooterBossEnemy::FinishSpecialAttack()
{
	bSpecialActive = false;
	bRoaring = false;
	bSpecialStopping = false;
	ActiveRoarMontage = nullptr;
	RoarInstanceId = INDEX_NONE;
	SpecialTarget.Reset();
	RestoreSpecialRotation();
}

void AShooterBossEnemy::StopSpecialAttack()
{
	if (!bSpecialActive)
	{
		return;
	}
	bSpecialStopping = true;
	if (bRoaring)
	{
		UAnimMontage* Montage = ActiveRoarMontage.Get();
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		FAnimMontageInstance* Instance = AnimInstance ? AnimInstance->GetMontageInstanceForID(RoarInstanceId) : nullptr;
		const bool bOwnsMontage = Instance && Instance->Montage == Montage;
		if (bOwnsMontage)
		{
			// 保持忙碌直到旧实例完全结束，避免旧回调误接到下一次咆哮。
			if (!Instance->IsStopped())
			{
				AnimInstance->Montage_Stop(0.2f, Montage);
			}
		}
		else
		{
			FinishSpecialAttack();
		}
	}
	else if (bShooting)
	{
		StopShoot();
	}
	else
	{
		FinishSpecialAttack();
	}
}

bool AShooterBossEnemy::IsLightAttackBusy() const
{
	return IsMeleeAttackBusy();
}

bool AShooterBossEnemy::TryStartLightAttack()
{
	return TryStartMeleeAttack(EBossMeleeAttack::Light);
}

bool AShooterBossEnemy::IsMeleeAttackBusy() const
{
	const UWorld* World = GetWorld();
	return bAttackPlaying || (World && World->GetTimeSeconds() < NextAttackTime);
}

AShooterBossEnemy::FMeleeAttackSettings 
AShooterBossEnemy::GetMeleeAttackSettings(EBossMeleeAttack AttackType) const
{
	switch (AttackType)
	{
	case EBossMeleeAttack::Light:
		return{
			LightAttackMontage.Get(),
			LightAttackDamage,
			LightAttackRange,
			LightAttackRecoveryTime,
			LightAttackCooldown,
			NextLightReadyTime,
			FName(TEXT("BossLightHit"))
		};
		
	case EBossMeleeAttack::Heavy:
		return{
			HeavyAttackMontage.Get(),
			HeavyAttackDamage,
			HeavyAttackRange,
			HeavyAttackRecoveryTime,
			HeavyAttackCooldown,
			NextHeavyReadyTime,
			FName(TEXT("BossHeavyHit"))
		};
		
	case EBossMeleeAttack::Stomp:
		return{
			StompAttackMontage.Get(),
			StompAttackDamage,
			StompAttackRange,
			StompAttackRecoveryTime,
			StompAttackCooldown,
			NextStompReadyTime,
			FName(TEXT("BossStompHit"))
		};
		
	case EBossMeleeAttack::GroundSmash:
		return{
			GroundSmashMontage.Get(),
			GroundSmashDamage,
			GroundSmashRange,
			GroundSmashRecoveryTime,
			GroundSmashCooldown,
			NextGroundSmashReadyTime,
		FName(TEXT("BossGroundSmashHit"))
	};
		
	case EBossMeleeAttack::Charge:
		return{
			ChargeMontage.Get(),
			ChargeDamage,
			ChargeRange,
			ChargeRecoveryTime,
			ChargeCooldown,
			NextChargeReadyTime,
			FName(TEXT("BossChargeHit"))
		};
		
	default:
		return{};
	}
}

bool AShooterBossEnemy::IsMeleeAttackAvailable(
	EBossMeleeAttack AttackType,
	float Distance, 
	double Now
) const
{
	const FMeleeAttackSettings Settings =
		GetMeleeAttackSettings(AttackType);
	
	return 
		Settings.Montage != nullptr &&
			FMath::IsFinite(Distance) &&
			FMath::IsFinite(Settings.Range) &&
			Settings.Range > 0.0f &&
			Distance <= Settings.Range &&
			FMath::IsFinite(Settings.Damage) &&
			Settings.Damage > 0.0f &&
			FMath::IsFinite(Settings.Recovery) &&
			FMath::IsFinite(Settings.Cooldown) &&
			Now >= Settings.ReadyTime;
}

bool AShooterBossEnemy::TryStartMeleeAttack(EBossMeleeAttack AttackType)
{
	if (!bCombatReady || bEncounterCancelled || bEndingPlay || IsDead() || IsAttackBusy() || !GetWorld())
	{
		return false;
	}

	const AAIController* AI = Cast<AAIController>(GetController());
	const UBlackboardComponent* Blackboard = AI ? AI->GetBlackboardComponent() : nullptr;
	
	AShooterSamProjectCharacter* Target = Blackboard
		? Cast<AShooterSamProjectCharacter>(Blackboard->GetValueAsObject(TEXT("TargetActor")))
		: nullptr;
	
	if (!IsValid(Target) || !Target->IsAlive || !Target->IsPlayerControlled() ||
		Target->GetWorld() != GetWorld())
	{
		return false;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	// Do not interrupt the entrance or another attack.
	if (!AnimInstance || AnimInstance->IsAnyMontagePlaying())
	{
		return false;
	}

	const float Distance = GetDistanceTo(Target);
	const double Now = GetWorld()->GetTimeSeconds();
	
	double LightWeight = 0.0;
	double HeavyWeight = 0.0;
	double StompWeight = 0.0;
	double GroundSmashEffectiveWeight = 0.0;
	double ChargeEffectiveWeight = 0.0;
	
	const bool bRandomSelection = AttackType == EBossMeleeAttack::Random;
	if (bRandomSelection)
	{
		const float RepeatMultiplier = FMath::IsFinite(RepeatAttackWeightMultiplier)
			? FMath::Clamp(RepeatAttackWeightMultiplier, 0.01f, 1.0f) : 0.25f;
		auto EligibleWeight = [&](EBossMeleeAttack Type, float Weight) -> double
		{
			if (!IsMeleeAttackAvailable(Type, Distance, Now) || !FMath::IsFinite(Weight) || Weight <= 0.0f)
			{
				return 0.0;
			}
			return static_cast<double>(Weight) *
				(bHasPreviousAttack && ActiveAttackType == Type ? RepeatMultiplier : 1.0f);
		};
		
		LightWeight = EligibleWeight(
			EBossMeleeAttack::Light, LightAttackWeight	
		);
		
		HeavyWeight = EligibleWeight(
			EBossMeleeAttack::Heavy, HeavyAttackWeight	
		);
		
		StompWeight = EligibleWeight(
			EBossMeleeAttack::Stomp, StompAttackWeight	
		);
		
		GroundSmashEffectiveWeight = EligibleWeight(
			EBossMeleeAttack::GroundSmash, GroundSmashWeight	
		);
		
		ChargeEffectiveWeight = EligibleWeight(
			EBossMeleeAttack::Charge, ChargeWeight	
		);
		
		const double TotalWeight = LightWeight + HeavyWeight + StompWeight + 
			GroundSmashEffectiveWeight + ChargeEffectiveWeight;
		
		//无可用招式，本次不进攻
		if (TotalWeight <= 0.0)
		{
			return false;
		}
		
		//只在准备i开始攻击时抽取一次
		const double Roll = FMath::FRand() * TotalWeight;
		
		if (ChargeEffectiveWeight > 0.0 &&
			Roll >= LightWeight + HeavyWeight + StompWeight + GroundSmashEffectiveWeight	
		)
		{
			AttackType = EBossMeleeAttack::Charge;
		}
		else if (GroundSmashEffectiveWeight > 0.0 &&
			Roll >= LightWeight + HeavyWeight + StompWeight	
		)
		{
			AttackType = EBossMeleeAttack::GroundSmash;
		}
		else if (StompWeight > 0.0 && 
			Roll >= LightWeight + HeavyWeight
		)
		{
			AttackType = EBossMeleeAttack::Stomp;
		}
		else if (HeavyWeight > 0.0 &&
			Roll >= LightWeight	
		)
		{
			AttackType = EBossMeleeAttack::Heavy;
		}
		else
		{
				AttackType = EBossMeleeAttack::Light;
		}
	}
	
	if (!IsMeleeAttackAvailable(AttackType, Distance, Now))
	{
		return false;
	}
	
	const FMeleeAttackSettings Settings = 
		GetMeleeAttackSettings(AttackType);
	UAnimMontage* Montage = Settings.Montage;
	
	AnimInstance->OnPlayMontageNotifyBegin.AddUniqueDynamic(this, &AShooterBossEnemy::HandleAttackNotify);
	if (AnimInstance->Montage_Play(Montage, 1.0f) <= 0.0f)
	{
		return false;
	}
	FAnimMontageInstance* Instance = AnimInstance->GetActiveInstanceForMontage(Montage);
	if (!Instance)
	{
		AnimInstance->Montage_Stop(0.1f, Montage);
		return false;
	}

	// 固定本次攻击的招式和配置
	ActiveAttackType = AttackType;
	ActiveAttackMontage = Montage;

	ExpectedHitNotify = Settings.HitNotify;
	ActiveDamage = Settings.Damage;
	ActiveRange = Settings.Range;
	ActiveRecoveryTime = FMath::Max(0.0f, Settings.Recovery) * GetOrdinaryIntervalMultiplier();
	ActiveCooldown = FMath::Max(0.0f, Settings.Cooldown) * GetOrdinaryIntervalMultiplier();
	bSpecialSelectionRolled = false;
	
	bHasPreviousAttack = true;
	AttackInstanceId = Instance->GetInstanceID();
	AttackTarget = Target;
	bAttackPlaying = true;
	bHitHandled = false;

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &AShooterBossEnemy::HandleAttackEnd);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
	
	const TCHAR* AttackLabel = TEXT("Light");

	switch (AttackType)
	{
	case EBossMeleeAttack::Heavy:
		AttackLabel = TEXT("Heavy");
		break;

	case EBossMeleeAttack::Stomp:
		AttackLabel = TEXT("Stomp");
		break;
		
	case EBossMeleeAttack::GroundSmash:
		AttackLabel = TEXT("GroundSmash");
		break;
		
	case EBossMeleeAttack::Charge:
		AttackLabel = TEXT("Charge");
		break;

	default:
		break;
	}

	if (bRandomSelection)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[Boss] Selected %s: lightWeight=%.2f heavyWeight=%.2f stompWeight=%.2f groundSmashWeight=%.2f chargeWeight=%.2f distance=%.1f"),
				AttackLabel,
				LightWeight,
				HeavyWeight,
				StompWeight,
				GroundSmashEffectiveWeight,
				ChargeEffectiveWeight,
				Distance
			);
		}

	UE_LOG(LogTemp, Log,
		TEXT("[Boss] %s attack started"), AttackLabel);
	
	return true;
}

void AShooterBossEnemy::HandleAttackNotify(FName NotifyName, const FBranchingPointNotifyPayload& Payload)
{
	if (!bCombatReady || bEncounterCancelled || bEndingPlay || IsDead() || !bAttackPlaying || bHitHandled || NotifyName != ExpectedHitNotify ||
		Payload.MontageInstanceID != AttackInstanceId)
	{
		return;
	}

	// A missed attack is also consumed; duplicate notifications must never retry damage.
	bHitHandled = true;
	const TCHAR* AttackLabel = TEXT("Light");
	switch (ActiveAttackType)
	{
	case EBossMeleeAttack::Heavy:
		AttackLabel = TEXT("Heavy");
		break;
		
	case EBossMeleeAttack::Stomp:
		AttackLabel = TEXT("Stomp");
		break;
		
	case EBossMeleeAttack::GroundSmash:
		AttackLabel = TEXT("GroundSmash");
		break;
		
	case EBossMeleeAttack::Charge:
		AttackLabel = TEXT("Charge");
		break;
		
	default:
		break;
	}
	
	UE_LOG(LogTemp, Log, TEXT("[Boss] %s received"), *NotifyName.ToString());
	
	UWorld* World = GetWorld();
	AShooterSamProjectCharacter* Target = AttackTarget.Get();
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	FAnimMontageInstance* Instance = AnimInstance
		? AnimInstance->GetActiveInstanceForMontage(ActiveAttackMontage) : nullptr;
	// Interrupted blend-out, death and invalid targets cannot inflict damage.
	if (!Instance || Instance->GetInstanceID() != AttackInstanceId ||
		!World || !IsValid(GetController()) || !IsValid(Target) ||
		!Target->IsAlive || !Target->IsPlayerControlled() || Target->GetWorld() != World)
	{
		return;
	}

	const float Distance = GetDistanceTo(Target);
	if (Distance > ActiveRange)
	{
		UE_LOG(LogTemp, Log, TEXT("[Boss] %s attack missed: distance=%.1f range=%.1f"),
			AttackLabel, Distance, ActiveRange);
		return;
	}
	const FVector Direction = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), Direction) < 0.5f)
	{
		UE_LOG(LogTemp, Log, TEXT("[Boss] %s attack missed: target outside forward 120-degree arc"), AttackLabel);
		return;
	}
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, GetActorLocation(), Target->GetActorLocation(),
		ECC_Visibility, QueryParams) && Hit.GetActor() != Target)
	{
		UE_LOG(LogTemp, Log, TEXT("[Boss] %s attack blocked by %s"), AttackLabel, *GetNameSafe(Hit.GetActor()));
		return;
	}

	const float AppliedDamage = UGameplayStatics::ApplyDamage(
		Target, ActiveDamage, GetController(), this, UDamageType::StaticClass());
	UE_LOG(LogTemp, Log, TEXT("[Boss] %s attack hit %s: damage=%.1f distance=%.1f range=%.1f"),
		AttackLabel, *GetNameSafe(Target), AppliedDamage, Distance, ActiveRange);
}

void AShooterBossEnemy::HandleAttackEnd(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bAttackPlaying || Montage != ActiveAttackMontage)
	{
		return;
	}
	bAttackPlaying = false;
	AttackInstanceId = INDEX_NONE;
	AttackTarget.Reset();
	ActiveAttackMontage = nullptr;
	if (const UWorld* World = GetWorld())
	{
		NextAttackTime = World->GetTimeSeconds() + ActiveRecoveryTime;
		
		const double ReadyTime = 
			World->GetTimeSeconds() + ActiveCooldown;

		switch (ActiveAttackType)
		{
		case EBossMeleeAttack::Light:
			NextLightReadyTime = ReadyTime;
			break;
			
		case EBossMeleeAttack::Heavy:
			NextHeavyReadyTime = ReadyTime;
			break;
			
		case EBossMeleeAttack::Stomp:
			NextStompReadyTime = ReadyTime;
			break;
			
		case EBossMeleeAttack::GroundSmash:
			NextGroundSmashReadyTime = ReadyTime;
			break;
			
		case EBossMeleeAttack::Charge:
			NextChargeReadyTime = ReadyTime;
			break;
			
		default:
			break;
		}
		
	}
	
	const TCHAR* AttackLabel = TEXT("Light");

	switch (ActiveAttackType)
	{
	case EBossMeleeAttack::Heavy:
		AttackLabel = TEXT("Heavy");
		break;

	case EBossMeleeAttack::Stomp:
		AttackLabel = TEXT("Stomp");
		break;
		
	case EBossMeleeAttack::GroundSmash:
		AttackLabel = TEXT("GroundSmash");
		break;
		
	case EBossMeleeAttack::Charge:
		AttackLabel = TEXT("Charge");
		break;

	default:
		break;
	}
	
	UE_LOG(LogTemp, Log,
	TEXT("[Boss] %s attack ended. Interrupted=%s"),
	AttackLabel,
	bInterrupted ? TEXT("true") : TEXT("false"));
}

void AShooterBossEnemy::StopShoot()
{
	MarkSpecialShootStopped();
	//先停止计时，避免淡出期间继续开火
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ShootFireTimerHandle);
		World->GetTimerManager().ClearTimer(ShootDurationTimerHandle);
	}
	
	if (!bShooting)
	{
		return;
	}
	
	bShootStopping = true;
	
	UAnimMontage* Montage =ActiveShootMontage.Get();
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	
	FAnimMontageInstance* Instance = AnimInstance
		? AnimInstance->GetMontageInstanceForID(ShootInstanceId)
		: nullptr;
	
	if (Montage && Instance && Instance->Montage == Montage)
	{
		//已经在淡出，就等待结束回调，避免重复停止
		if (!Instance->IsStopped())
		{
			AnimInstance->Montage_Stop(0.2f, Montage);
		}
		
		return;
	}
	
	//本轮动画实例已经不存在，直接完成状态清理
	HandleShootEnd(Montage, true);
}

void AShooterBossEnemy::HandleShootBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bShooting || Montage != ActiveShootMontage.Get())
	{
		return;
	}
	
	MarkSpecialShootStopped();
	bShootStopping = true;
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ShootFireTimerHandle);
		World->GetTimerManager().ClearTimer(ShootDurationTimerHandle);
	}
}

void AShooterBossEnemy::HandleShootEnd(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bShooting || Montage != ActiveShootMontage.Get())
	{
		return;
	}
	
	MarkSpecialShootStopped();
	const bool bWasSpecial = bSpecialActive;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ShootFireTimerHandle);
		World->GetTimerManager().ClearTimer(ShootDurationTimerHandle);
		
		const double Now = World->GetTimeSeconds();
		
		const float Recovery = ActiveShootRecovery;
		const float Cooldown = ActiveShootCooldown;
		
		//公共恢复时间：限制所有攻击
		NextAttackTime = FMath::Max(
			NextAttackTime,
			Now + Recovery
		);
		
		//射击独立冷却：只限制下一轮射击
		if (!bWasSpecial)
		{
			NextShootReadyTime = Now + Cooldown;
		}
	}
	
	bShooting = false;
	bShootStopping = false;
	ShootTarget.Reset();
	ActiveShootMontage = nullptr;
	ShootInstanceId = INDEX_NONE;
	ShootEndTime = 0.0;
	
	if (bWasSpecial)
	{
		FinishSpecialAttack();
	}
	UE_LOG(LogTemp, Log, TEXT("[Boss] %s attack ended"), bWasSpecial ? TEXT("Special shoot") : TEXT("Shoot"));
}

bool AShooterBossEnemy::TryStartShoot()
{
	return StartShoot(false);
}

bool AShooterBossEnemy::IsDistanceInOrdinaryShootRange(float Distance) const
{
	return FMath::IsFinite(Distance) && FMath::IsFinite(ShootMinRange) &&
		FMath::IsFinite(ShootRange) && ShootMinRange >= 0.0f &&
		Distance > ShootMinRange && Distance <= ShootRange;
}

bool AShooterBossEnemy::CanStartOrdinaryShoot() const
{
	const UWorld* World = GetWorld();
	if (!World || !bCombatReady || bEncounterCancelled || bEndingPlay || IsDead() || IsAttackBusy() ||
		World->GetTimeSeconds() < NextShootReadyTime || !HasValidShootSettings(false))
	{
		return false;
	}
	const AAIController* AI = Cast<AAIController>(GetController());
	const UBlackboardComponent* Blackboard = AI ? AI->GetBlackboardComponent() : nullptr;
	const AShooterSamProjectCharacter* Target = Blackboard
		? Cast<AShooterSamProjectCharacter>(Blackboard->GetValueAsObject(TEXT("TargetActor"))) : nullptr;
	const UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	return IsValid(Target) && Target->IsAlive && Target->IsPlayerControlled() &&
		Target->GetWorld() == World && IsDistanceInOrdinaryShootRange(GetDistanceTo(Target)) &&
		AnimInstance && !AnimInstance->IsAnyMontagePlaying() && AI->LineOfSightTo(Target);
}

bool AShooterBossEnemy::StartShoot(bool bSpecial)
{
	UWorld* World = GetWorld();
	if (!bSpecial && !CanStartOrdinaryShoot())
	{
		return false;
	}
	
	if (!World || !bCombatReady || bEncounterCancelled || bEndingPlay || IsDead() || (bSpecial
		? (!bSpecialActive || bRoaring || bShooting || IsMeleeAttackBusy()) : IsAttackBusy()))
	{
		return false;
	}
	
	//检查射击资源和参数
	if (!HasValidShootSettings(bSpecial))
	{
		return false;
	}
	
	if (!GetMesh()->DoesSocketExist(ShootMuzzleSocketName))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Boss] Shoot socket missing: %s"),
			*ShootMuzzleSocketName.ToString()
		);

		return false;
	}
	
	const double Now = World->GetTimeSeconds();
	
	if (!bSpecial && Now < NextShootReadyTime)
	{
		return false;
	}
	
	//从黑板取得本轮目标
	AAIController* AI = Cast<AAIController>(GetController());
	const UBlackboardComponent* BlackBoard = AI
		? AI->GetBlackboardComponent()
		: nullptr;
	
	AShooterSamProjectCharacter* Target = BlackBoard
		? Cast<AShooterSamProjectCharacter>(BlackBoard->GetValueAsObject(TEXT("TargetActor")))
		: nullptr;
	if (bSpecial)
	{
		Target = SpecialTarget.Get();
	}
	
	if (!AI || !IsValid(Target) || 
		!Target->IsAlive ||
		!Target->IsPlayerControlled() ||
		Target->GetWorld() != World
	)
	{
		return false;
	}
	
	const float Distance = GetDistanceTo(Target);
	
	// 咆哮后目标暂时跑出射程时，特殊扫射仍持续，范围外不造成伤害。
	if (!FMath::IsFinite(Distance) || (!bSpecial && Distance > ShootRange))
	{
		return false;
	}
	
	//保留当前正在播放的入场或其他动作
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	
	if (!AnimInstance || AnimInstance->IsAnyMontagePlaying())
	{
		return false;
	}
	
	UAnimMontage* Montage = ShootMontage.Get();
	
	if (AnimInstance->Montage_Play(Montage, 1.0f) <= 0.0f)
	{
		return false;
	}
	
	FAnimMontageInstance* Instance = 
		AnimInstance->GetActiveInstanceForMontage(Montage);
	
	if (!Instance)
	{
		AnimInstance->Montage_Stop(0.2f, Montage);
		return false;
	}
	
	//射击期间原地站定
	AI->StopMovement();
	GetCharacterMovement()->StopMovementImmediately();
	
	const float Multiplier = bSpecial ? 1.0f : GetOrdinaryIntervalMultiplier();
	const float Duration = FMath::Max(bSpecial ? SpecialShootDuration : ShootDuration, 0.01f);
	const float Interval = FMath::Max((bSpecial ? SpecialShootInterval : ShootInterval) * Multiplier, 0.01f);
	ActiveShootDamage = bSpecial ? SpecialShootDamage : ShootDamage;
	ActiveShootRange = bSpecial ? SpecialShootRange : ShootRange;
	const float Recovery = bSpecial ? SpecialRecoveryTime : ShootRecoveryTime;
	ActiveShootRecovery = FMath::IsFinite(Recovery) ? FMath::Max(0.0f, Recovery) * Multiplier : 0.0f;
	ActiveShootCooldown = FMath::IsFinite(ShootCooldown) ? FMath::Max(0.0f, ShootCooldown) * Multiplier : 0.0f;
	if (bSpecial)
	{
		bFirstSpecialPending = false;
	}
	else
	{
		bSpecialSelectionRolled = false;
	}
	
	//记录本轮状态
	ShootTarget = Target;
	ActiveShootMontage = Montage;
	ShootInstanceId = Instance->GetInstanceID();
	ShootEndTime = Now + Duration;
	bShooting = true;
	bShootStopping = false;
	
	//绑定淡出与结束回调
	FOnMontageBlendingOutStarted BlendOutDelegate;
	BlendOutDelegate.BindUObject(
		this,
		&AShooterBossEnemy::HandleShootBlendingOut
	);
	
	AnimInstance->Montage_SetBlendingOutDelegate(
		BlendOutDelegate, Montage
	);
	
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(
		this,
		&AShooterBossEnemy::HandleShootEnd
	);
	AnimInstance->Montage_SetEndDelegate(
		EndDelegate, Montage	
	);
	
	//到达持续时间后，停止本轮射击
	World->GetTimerManager().SetTimer(
		ShootDurationTimerHandle,
		this,
		&AShooterBossEnemy::StopShoot,
		Duration,
		false
	);
	
	//按间隔持续开火
	World->GetTimerManager().SetTimer(
		ShootFireTimerHandle,
		this,
		&AShooterBossEnemy::FireShootRound,
		Interval,
		true
	);
	
	UE_LOG(LogTemp, Log,
		TEXT("[Boss] %s started: duration=%.2f interval=%.2f"),
		bSpecial ? TEXT("Special shoot") : TEXT("Shoot"), Duration, Interval
	);
	
	//第一发立即执行，后续由定时器触发
	FireShootRound();
	
	return true;
}

void AShooterBossEnemy::FireShootRound()
{
	if (!bShooting || bShootStopping)
	{
		return;
	}
	
	UWorld* World = GetWorld();
	AShooterSamProjectCharacter* Target = ShootTarget.Get();
	
	//每发重新检查，避免死亡后继续射击
	if (!World || !bCombatReady || bEncounterCancelled || bEndingPlay || IsDead() || !IsValid(GetController()) ||
		!IsValid(Target) || !Target->IsAlive ||
		!Target->IsPlayerControlled() ||
		Target->GetWorld() != World
	)
	{
		StopShoot();
		return;
	}
	
	// 玩家贴近时结束普通射击，收招后交回原有近战；特殊扫射不受这个下界限制。
	if (!bSpecialActive && !IsDistanceInOrdinaryShootRange(GetDistanceTo(Target)))
	{
		StopShoot();
		return;
	}

	//即使两个定时器在同一帧触发，也不能在截止时间补一发
	if (World->GetTimeSeconds() >= ShootEndTime)
	{
		StopShoot();
		return;
	}
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	
	FAnimMontageInstance* Instance = AnimInstance
		? AnimInstance->GetActiveInstanceForMontage(
			ActiveShootMontage.Get())
		: nullptr;
	
	if (!Instance || Instance->GetInstanceID() != ShootInstanceId)
	{
		StopShoot();
		return;
	}
	
	if (!GetMesh()->DoesSocketExist(ShootMuzzleSocketName))
	{
		StopShoot();
		return;
	}
	
	const FVector Start = 
		GetMesh()->GetSocketLocation(ShootMuzzleSocketName);
	
	const FVector ToTarget = Target->GetActorLocation() - Start;
	const float Distance = ToTarget.Size();
	
	if (!FMath::IsFinite(Distance) ||
		Distance <= KINDA_SMALL_NUMBER ||
		Distance > ActiveShootRange)
	{
		if (!bSpecialActive)
		{
			StopShoot();
		}
		return;
	}
	
	//玩家绕到背后时，不允许直接向背后开枪
	const FVector FacingTarget = 
		(Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	
	if (FVector::DotProduct(
		GetActorForwardVector().GetSafeNormal2D(),
		FacingTarget) < 0.5f)
	{
		return;
	}
	
	const FVector Direction = ToTarget / Distance;
	const FVector End = Start + Direction * ActiveShootRange;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FHitResult Hit;

	// 项目中 ECC_GameTraceChannel1 对应 Bullet
	const bool bHit = World->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_GameTraceChannel1,
		QueryParams
	);

	// 第一处阻挡物不是目标时，本发不对目标造成伤害
	if (!bHit || Hit.GetActor() != Target)
	{
		UE_LOG(LogTemp, Log,
			TEXT("[Boss] Shoot missed or blocked by %s"),
			*GetNameSafe(Hit.GetActor()));

		return;
	}

	const float AppliedDamage = UGameplayStatics::ApplyDamage(
		Target,
		ActiveShootDamage,
		GetController(),
		this,
		UDamageType::StaticClass()
	);

	UE_LOG(LogTemp, Log,
		TEXT("[Boss] Shoot hit %s: damage=%.1f"),
		*GetNameSafe(Target),
		AppliedDamage);

	if (!IsValid(Target) || !Target->IsAlive)
	{
		StopShoot();
	}
}
