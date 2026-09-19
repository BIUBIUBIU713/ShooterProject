#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "ShooterBTTaskMeleeAttack.generated.h"

class UAnimSequence;
class UAnimInstance;
class UAnimMontage;
class AShooterMeleeEnemy;
class AShooterSamProjectCharacter;

// 行为树中每一种攻击的配置
USTRUCT(BlueprintType)
struct FShooterMeleeAttackOption
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Attack")
    FName AttackName = TEXT("Attack");

    UPROPERTY(EditAnywhere, Category = "Attack")
    TObjectPtr<UAnimSequence> Animation = nullptr;

    UPROPERTY(EditAnywhere, Category = "Attack",
        meta = (ClampMin = "0.01"))
    float DamageMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Attack",
        meta = (ClampMin = "0.1"))
    float PlayRate = 1.0f;

    // 原动画时间轴上的秒数
    UPROPERTY(EditAnywhere, Category = "Attack",
        meta = (ClampMin = "0.0"))
    float HitTime = 0.3f;

    // 动画结束后的恢复时间
    UPROPERTY(EditAnywhere, Category = "Attack",
        meta = (ClampMin = "0.0"))
    float RecoveryTime = 0.5f;

    // 0 表示不参与随机选择
    UPROPERTY(EditAnywhere, Category = "Attack",
        meta = (ClampMin = "0.0"))
    float Weight = 1.0f;

    // 动作播放期间锁住身体朝向，适合重攻击
    UPROPERTY(EditAnywhere, Category = "Attack")
    bool bLockFacing = false;
};

UCLASS()
class SHOOTERSAMPROJECT_API UShooterBTTaskMeleeAttack
    : public UBTTask_BlackboardBase
{
    GENERATED_BODY()

public:
    UShooterBTTaskMeleeAttack();

protected:
    virtual EBTNodeResult::Type ExecuteTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory) override;

    virtual void TickTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory,
        float DeltaSeconds) override;

    virtual void OnTaskFinished(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory,
        EBTNodeResult::Type TaskResult) override;

    UPROPERTY(EditAnywhere, Category = "Melee",
        meta = (TitleProperty = "AttackName"))
    TArray<FShooterMeleeAttackOption> Attacks;

    UPROPERTY(EditAnywhere, Category = "Melee")
    FName SlotName = TEXT("DefaultSlot");

    UPROPERTY(EditAnywhere, Category = "Melee")
    bool bAvoidImmediateRepeat = true;

private:
    bool IsOptionValid(const FShooterMeleeAttackOption& Option) const;

    int32 SelectAttackIndex() const;

    void HandleMontageEnded(
        UAnimMontage* Montage,
        bool bInterrupted);

    void LockFacing();
    void RestoreFacing();

    UPROPERTY()
    TWeakObjectPtr<AShooterMeleeEnemy> CachedEnemy;

    UPROPERTY()
    TWeakObjectPtr<AShooterSamProjectCharacter> CachedTarget;

    UPROPERTY()
    TWeakObjectPtr<UAnimInstance> CachedAnimInstance;

    UPROPERTY()
    TObjectPtr<UAnimMontage> ActiveMontage;

    // 本次攻击使用配置副本，中途不会重新随机
    UPROPERTY()
    FShooterMeleeAttackOption ActiveAttack;

    int32 SelectedAttackIndex = INDEX_NONE;
    int32 LastAttackIndex = INDEX_NONE;

    bool bTaskRunning = false;
    bool bAnimationStarted = false;
    bool bAnimationEnded = false;
    bool bAnimationInterrupted = false;
    bool bHitChecked = false;

    bool bFacingLocked = false;
    bool bSavedControllerYaw = false;
    bool bSavedDesiredRotation = false;
    bool bSavedOrientToMovement = false;

    float FacingWaitTime = 0.0f;
};