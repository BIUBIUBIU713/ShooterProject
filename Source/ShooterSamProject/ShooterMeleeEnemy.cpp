#include "ShooterMeleeEnemy.h"

#include "ShooterSamProjectCharacter.h"
#include "ShooterEnemyAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AShooterMeleeEnemy::AShooterMeleeEnemy()
{
    PrimaryActorTick.bCanEverTick = false;

    AIControllerClass = AShooterEnemyAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    bUseControllerRotationYaw = false;

    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->MaxWalkSpeed = 400.0f;
    Movement->bOrientRotationToMovement = false;
    Movement->bUseControllerDesiredRotation = true;
    Movement->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
}

void AShooterMeleeEnemy::BeginPlay()
{
    MeleeDamage = FMath::IsFinite(MeleeDamage)
        ? FMath::Max(MeleeDamage, 1.0f)
        : 20.0f;

    MeleeAttackRange = FMath::IsFinite(MeleeAttackRange)
        ? FMath::Max(MeleeAttackRange, 1.0f)
        : 150.0f;

    Super::BeginPlay();
}

bool AShooterMeleeEnemy::IsMeleeAttackReady() const
{
    const UWorld* World = GetWorld();

    return !IsDead()
        && GetCurrentHealth() > 0.0f
        && IsValid(World)
        && World->GetTimeSeconds() >= NextAllowedAttackTime;
}

void AShooterMeleeEnemy::ReserveMeleeCooldown(float Duration)
{
    const UWorld* World = GetWorld();

    if (!IsValid(World) ||
        !FMath::IsFinite(Duration) ||
        Duration < 0.0f)
    {
        return;
    }

    const double CooldownEnd =
        static_cast<double>(World->GetTimeSeconds()) + Duration;

    NextAllowedAttackTime =
        FMath::Max(NextAllowedAttackTime, CooldownEnd);
}

bool AShooterMeleeEnemy::TryApplyMeleeHit(
    AShooterSamProjectCharacter* Target,
    float DamageMultiplier)
{
    UWorld* World = GetWorld();

    if (IsDead() ||
        GetCurrentHealth() <= 0.0f ||
        !IsValid(World) ||
        !IsValid(GetController()) ||
        !IsValid(Target) ||
        !Target->IsAlive ||
        !Target->IsPlayerControlled() ||
        Target->GetWorld() != World ||
        !FMath::IsFinite(DamageMultiplier) ||
        DamageMultiplier <= 0.0f)
    {
        return false;
    }

    const float FinalDamage = MeleeDamage * DamageMultiplier;

    if (!FMath::IsFinite(FinalDamage) || FinalDamage <= 0.0f)
    {
        return false;
    }

    // 到命中时刻重新检查，玩家可以通过移动躲开
    if (GetDistanceTo(Target) > MeleeAttackRange)
    {
        return false;
    }

    const FVector TargetDirection =
        (Target->GetActorLocation() - GetActorLocation())
        .GetSafeNormal2D();

    const FVector ForwardDirection =
        GetActorForwardVector().GetSafeNormal2D();

    // 前方左右各 60 度
    if (FVector::DotProduct(
        ForwardDirection, TargetDirection) < 0.5f)
    {
        return false;
    }

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    FHitResult Hit;

    const bool bBlocked = World->LineTraceSingleByChannel(
        Hit,
        GetActorLocation(),
        Target->GetActorLocation(),
        ECC_Visibility,
        QueryParams
    );

    if (bBlocked && Hit.GetActor() != Target)
    {
        return false;
    }

    const float AppliedDamage = UGameplayStatics::ApplyDamage(
        Target,
        FinalDamage,
        GetController(),
        this,
        UDamageType::StaticClass()
    );

    UE_LOG(
        LogTemp,
        Log,
        TEXT("%s melee hit %s: multiplier=%.2f, damage=%.1f"),
        *GetName(),
        *GetNameSafe(Target),
        DamageMultiplier,
        AppliedDamage
    );

    return AppliedDamage > 0.0f;
}