#include "RegionGuardSpawner.h"

#include "Components/SceneComponent.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "ShooterEnemyBase.h"
#include "ShooterMeleeEnemy.h"
#include "ShooterRangedEnemy.h"
#include "ShooterSamProjectGameMode.h"

ARegionGuardSpawner::ARegionGuardSpawner()
{
    PrimaryActorTick.bCanEverTick = false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot")));
}

int32 ARegionGuardSpawner::CalculateRangedCount(int32 TotalCount, int32 RangedCountOverride)
{
    if (RangedCountOverride >= 0)
    {
        return FMath::Clamp(RangedCountOverride, 0, FMath::Max(TotalCount, 0));
    }
    // 远程占 40%，四舍五入，其余给近战；用 int64 防止乘法溢出。
    return TotalCount > 0 ? static_cast<int32>((static_cast<int64>(TotalCount) * 4 + 5) / 10) : 0;
}

void ARegionGuardSpawner::BeginPlay()
{
    Super::BeginPlay();
    ShooterGameMode = GetWorld()->GetAuthGameMode<AShooterSamProjectGameMode>();
    if (!IsValid(ShooterGameMode))
    {
        UE_LOG(LogTemp, Error, TEXT("Guard spawner %s: ShooterGameMode missing."), *GetName());
        return;
    }

    ShooterGameMode->OnRegionUnlocked.AddUniqueDynamic(this, &ARegionGuardSpawner::HandleRegionUnlocked);
    ShooterGameMode->OnGameOver.AddUniqueDynamic(this, &ARegionGuardSpawner::StopSpawning);
    // 下一帧触发，让场景、GameMode 和出生点先完成 BeginPlay。
    InitialTriggerTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ARegionGuardSpawner::TriggerInitialSpawn);
}

void ARegionGuardSpawner::TriggerInitialSpawn()
{
    if (SpawnTrigger == ERegionGuardSpawnTrigger::BeginPlay ||
        (SpawnTrigger == ERegionGuardSpawnTrigger::RegionUnlocked &&
         IsValid(ShooterGameMode) && ShooterGameMode->IsRegionUnlocked(RegionId)))
    {
        TriggerSpawn();
    }
}

void ARegionGuardSpawner::HandleRegionUnlocked(FName UnlockedRegionId)
{
    if (SpawnTrigger == ERegionGuardSpawnTrigger::RegionUnlocked &&
        !RegionId.IsNone() && RegionId == UnlockedRegionId)
    {
        TriggerSpawn();
    }
}

bool ARegionGuardSpawner::ValidateConfiguration() const
{
    if (GuardCount < 1 || RangedGuardCount < -1 || RangedGuardCount > GuardCount ||
        SpawnPoints.Num() < GuardCount || MaxSpawnAttempts < 1 ||
        !FMath::IsFinite(RetryInterval) || RetryInterval < 0.1f ||
        (SpawnTrigger == ERegionGuardSpawnTrigger::RegionUnlocked && RegionId.IsNone()))
    {
        UE_LOG(LogTemp, Error, TEXT("Guard spawner %s: invalid count, points, retry settings or RegionId. RangedGuardCount must be -1 or between 0 and GuardCount."), *GetName());
        return false;
    }
    const int32 RangedCount = CalculateRangedCount(GuardCount, RangedGuardCount);
    const auto ValidClass = [](UClass* Class, UClass* RequiredBase)
    {
        return IsValid(Class) && Class->IsChildOf(RequiredBase) && !Class->HasAnyClassFlags(CLASS_Abstract);
    };
    if ((GuardCount > RangedCount && !ValidClass(MeleeGuardClass.Get(), AShooterMeleeEnemy::StaticClass())) ||
        (RangedCount > 0 && !ValidClass(RangedGuardClass.Get(), AShooterRangedEnemy::StaticClass())))
    {
        UE_LOG(LogTemp, Error, TEXT("Guard spawner %s: assign valid melee/ranged guard classes."), *GetName());
        return false;
    }
    TSet<ATargetPoint*> UsedPoints;
    for (int32 Index = 0; Index < GuardCount; ++Index)
    {
        ATargetPoint* Point = SpawnPoints[Index];
        if (!IsValid(Point) || Point->GetWorld() != GetWorld() || UsedPoints.Contains(Point))
        {
            UE_LOG(LogTemp, Error, TEXT("Guard spawner %s: invalid or duplicate SpawnPoints[%d]."), *GetName(), Index);
            return false;
        }
        UsedPoints.Add(Point);
    }
    return true;
}

bool ARegionGuardSpawner::TriggerSpawn()
{
    if (bTriggered || bStopped || !HasActorBegunPlay() || !IsValid(ShooterGameMode) ||
        ShooterGameMode->WaveState == EWaveState::GameOver || !ValidateConfiguration())
    {
        return false;
    }

    bTriggered = true;
    const int32 RangedCount = CalculateRangedCount(GuardCount, RangedGuardCount);
    Attempts.Init(0, GuardCount);
    for (int32 Index = 0; Index < GuardCount; ++Index)
    {
        PlannedClasses.Add(Index < RangedCount ? RangedGuardClass : MeleeGuardClass);
        // 出生点只提供位置和方向，避免编辑器中的缩放改变角色大小。
        PlannedTransforms.Add(FTransform(SpawnPoints[Index]->GetActorRotation(), SpawnPoints[Index]->GetActorLocation()));
        PendingSlots.Add(Index);
    }
    for (int32 Index = PlannedClasses.Num() - 1; Index > 0; --Index)
    {
        PlannedClasses.Swap(Index, FMath::RandRange(0, Index));
    }
    UE_LOG(LogTemp, Log, TEXT("Guards triggered: spawner=%s region=%s total=%d melee=%d ranged=%d"),
        *GetName(), *RegionId.ToString(), GuardCount, GuardCount - RangedCount, RangedCount);

    GetWorldTimerManager().SetTimer(RetryTimer, this, &ARegionGuardSpawner::TrySpawnPending, RetryInterval, true);
    TrySpawnPending();
    return true;
}

void ARegionGuardSpawner::TrySpawnPending()
{
    if (bStopped || !IsValid(ShooterGameMode) || ShooterGameMode->WaveState == EWaveState::GameOver)
    {
        StopSpawning();
        return;
    }
    for (int32 PendingIndex = PendingSlots.Num() - 1; PendingIndex >= 0; --PendingIndex)
    {
        const int32 Slot = PendingSlots[PendingIndex];
        ++Attempts[Slot];
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
        AShooterEnemyBase* Guard = GetWorld()->SpawnActor<AShooterEnemyBase>(
            PlannedClasses[Slot], PlannedTransforms[Slot], Params);

        if (IsValid(Guard) && !Guard->IsDead())
        {
            LivingGuards.Add(Guard);
            Guard->OnEnemyDied.AddUniqueDynamic(this, &ARegionGuardSpawner::HandleGuardDied);
            Guard->OnDestroyed.AddUniqueDynamic(this, &ARegionGuardSpawner::HandleGuardDestroyed);
            ++SpawnedCount;
            PendingSlots.RemoveAtSwap(PendingIndex);
            // 兼容蓝图 Auto Possess AI 仅设置为 Placed in World 的情况。
            if (!IsValid(Guard->GetController()))
            {
                Guard->SpawnDefaultController();
            }
            UE_LOG(LogTemp, Log, TEXT("Guard spawned: region=%s slot=%d enemy=%s spawned=%d/%d alive=%d"),
                *RegionId.ToString(), Slot, *GetNameSafe(Guard), SpawnedCount, GuardCount, LivingGuards.Num());
        }
        else
        {
            if (IsValid(Guard))
            {
                Guard->Destroy();
            }
            if (Attempts[Slot] >= MaxSpawnAttempts)
            {
                bHadSpawnFailure = true;
                PendingSlots.RemoveAtSwap(PendingIndex);
                UE_LOG(LogTemp, Error, TEXT("Guard spawn failed: region=%s slot=%d after %d attempts. Check spawn collision."),
                    *RegionId.ToString(), Slot, Attempts[Slot]);
            }
        }
        if (bStopped)
        {
            return;
        }
    }
    if (PendingSlots.IsEmpty())
    {
        bSpawnFinished = true;
        GetWorldTimerManager().ClearTimer(RetryTimer);
        CheckCleared();
    }
}

void ARegionGuardSpawner::HandleGuardDied(AShooterEnemyBase* Guard)
{
    if (LivingGuards.Remove(Guard) == 0)
    {
        return;
    }
    Guard->OnEnemyDied.RemoveDynamic(this, &ARegionGuardSpawner::HandleGuardDied);
    Guard->OnDestroyed.RemoveDynamic(this, &ARegionGuardSpawner::HandleGuardDestroyed);
    ++DefeatedCount;
    UE_LOG(LogTemp, Log, TEXT("Guard defeated: region=%s enemy=%s defeated=%d/%d alive=%d"),
        *RegionId.ToString(), *GetNameSafe(Guard), DefeatedCount, GuardCount, LivingGuards.Num());
    CheckCleared();
}

void ARegionGuardSpawner::HandleGuardDestroyed(AActor* Guard)
{
    if (LivingGuards.Remove(Cast<AShooterEnemyBase>(Guard)) > 0)
    {
        // 被外部直接销毁不等于击杀，不能误广播清理成功。
        bHadSpawnFailure = true;
        UE_LOG(LogTemp, Warning, TEXT("Guard removed without death: region=%s enemy=%s"),
            *RegionId.ToString(), *GetNameSafe(Guard));
    }
}

void ARegionGuardSpawner::CheckCleared()
{
    if (!bStopped && !bCleared && bSpawnFinished && !bHadSpawnFailure &&
        SpawnedCount == GuardCount && DefeatedCount == GuardCount && LivingGuards.IsEmpty())
    {
        bCleared = true;
        UE_LOG(LogTemp, Log, TEXT("Guards cleared: region=%s total=%d"), *RegionId.ToString(), GuardCount);
        OnGuardsCleared.Broadcast();
    }
}

void ARegionGuardSpawner::StopSpawning()
{
    bStopped = true;
    GetWorldTimerManager().ClearTimer(RetryTimer);
    GetWorldTimerManager().ClearTimer(InitialTriggerTimer);
}

void ARegionGuardSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopSpawning();
    if (IsValid(ShooterGameMode))
    {
        ShooterGameMode->OnRegionUnlocked.RemoveDynamic(this, &ARegionGuardSpawner::HandleRegionUnlocked);
        ShooterGameMode->OnGameOver.RemoveDynamic(this, &ARegionGuardSpawner::StopSpawning);
    }
    for (AShooterEnemyBase* Guard : LivingGuards)
    {
        if (IsValid(Guard))
        {
            Guard->OnEnemyDied.RemoveDynamic(this, &ARegionGuardSpawner::HandleGuardDied);
            Guard->OnDestroyed.RemoveDynamic(this, &ARegionGuardSpawner::HandleGuardDestroyed);
        }
    }
    Super::EndPlay(EndPlayReason);
}
