#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "RegionGuardSpawner.generated.h"

class AShooterEnemyBase;
class AShooterSamProjectGameMode;
class ATargetPoint;

UENUM(BlueprintType)
enum class ERegionGuardSpawnTrigger : uint8
{
    RegionUnlocked UMETA(DisplayName = "Region Unlocked"),
    BeginPlay UMETA(DisplayName = "Begin Play"),
    Manual UMETA(DisplayName = "Manual")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRegionGuardsCleared);

// 独立于 WaveManager：不登记回合敌人，也不影响回合结束条件。
UCLASS()
class SHOOTERSAMPROJECT_API ARegionGuardSpawner : public AActor
{
    GENERATED_BODY()

public:
    ARegionGuardSpawner();

    // true 表示接受本次生成请求；每局每个生成器只接受一次。
    UFUNCTION(BlueprintCallable, Category = "Guards")
    bool TriggerSpawn();

    UPROPERTY(BlueprintAssignable, Category = "Guards|Events")
    FOnRegionGuardsCleared OnGuardsCleared;

    static int32 CalculateRangedCount(int32 TotalCount, int32 RangedCountOverride = -1);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // 与区域门的 DoorGroupId 一致，单扇门也需要填写标识。
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Guards")
    FName RegionId = NAME_None;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Guards")
    ERegionGuardSpawnTrigger SpawnTrigger = ERegionGuardSpawnTrigger::RegionUnlocked;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Guards", meta = (ClampMin = "1"))
    int32 GuardCount = 5;

    // -1 沿用远程 40%；0 到 GuardCount 指定远程数量，其余为近战。
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Guards", meta = (ClampMin = "-1"))
    int32 RangedGuardCount = -1;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Guards|Classes")
    TSubclassOf<AShooterEnemyBase> MeleeGuardClass;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Guards|Classes")
    TSubclassOf<AShooterEnemyBase> RangedGuardClass;

    // 每名守卫一个独立 TargetPoint；其位置是角色胶囊中心，不是脚底。
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Guards|Spawn")
    TArray<TObjectPtr<ATargetPoint>> SpawnPoints;

    UPROPERTY(EditInstanceOnly, Category = "Guards|Spawn", meta = (ClampMin = "0.1"))
    float RetryInterval = 0.5f;

    UPROPERTY(EditInstanceOnly, Category = "Guards|Spawn", meta = (ClampMin = "1"))
    int32 MaxSpawnAttempts = 20;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Guards|Runtime")
    bool bTriggered = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Guards|Runtime")
    bool bSpawnFinished = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Guards|Runtime")
    bool bHadSpawnFailure = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Guards|Runtime")
    bool bCleared = false;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Guards|Runtime")
    int32 SpawnedCount = 0;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Guards|Runtime")
    int32 DefeatedCount = 0;

    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Guards|Runtime")
    TArray<TObjectPtr<AShooterEnemyBase>> LivingGuards;

private:
    UPROPERTY()
    TObjectPtr<AShooterSamProjectGameMode> ShooterGameMode;

    UPROPERTY()
    TArray<TSubclassOf<AShooterEnemyBase>> PlannedClasses;

    TArray<FTransform> PlannedTransforms;
    TArray<int32> PendingSlots;
    TArray<int32> Attempts;
    FTimerHandle RetryTimer;
    FTimerHandle InitialTriggerTimer;
    bool bStopped = false;

    bool ValidateConfiguration() const;
    void TrySpawnPending();
    void TriggerInitialSpawn();
    void CheckCleared();

    UFUNCTION()
    void HandleRegionUnlocked(FName UnlockedRegionId);

    UFUNCTION()
    void StopSpawning();

    UFUNCTION()
    void HandleGuardDied(AShooterEnemyBase* Guard);

    UFUNCTION()
    void HandleGuardDestroyed(AActor* Guard);
};
