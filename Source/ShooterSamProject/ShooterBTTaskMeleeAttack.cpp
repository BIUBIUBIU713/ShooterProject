#include "ShooterBTTaskMeleeAttack.h"

#include "AIController.h"
#include "ShooterMeleeEnemy.h"
#include "ShooterSamProjectCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UShooterBTTaskMeleeAttack::UShooterBTTaskMeleeAttack()
{
    NodeName = TEXT("Random Melee Attack");

    // 每个 AI 独立保存动画、目标和上一次攻击
    bCreateNodeInstance = true;
    bNotifyTick = true;
    bNotifyTaskFinished = true;

    BlackboardKey.AddObjectFilter(
        this,
        GET_MEMBER_NAME_CHECKED(
            UShooterBTTaskMeleeAttack, BlackboardKey),
        AActor::StaticClass()
    );
}

bool UShooterBTTaskMeleeAttack::IsOptionValid(
    const FShooterMeleeAttackOption& Option) const
{
    if (!IsValid(Option.Animation.Get()) ||
        !FMath::IsFinite(Option.Weight) ||
        Option.Weight <= 0.0f ||
        !FMath::IsFinite(Option.DamageMultiplier) ||
        Option.DamageMultiplier <= 0.0f ||
        !FMath::IsFinite(Option.PlayRate) ||
        Option.PlayRate < 0.1f ||
        !FMath::IsFinite(Option.HitTime) ||
        Option.HitTime < 0.0f ||
        !FMath::IsFinite(Option.RecoveryTime) ||
        Option.RecoveryTime < 0.0f)
    {
        return false;
    }

    const float Length = Option.Animation->GetPlayLength();
    const float Duration = Length / Option.PlayRate;

    return FMath::IsFinite(Length)
        && Length > 0.0f
        && Option.HitTime < Length
        && FMath::IsFinite(Duration)
        && FMath::IsFinite(Duration + Option.RecoveryTime);
}

int32 UShooterBTTaskMeleeAttack::SelectAttackIndex() const
{
    TArray<int32> Candidates;

    for (int32 Index = 0; Index < Attacks.Num(); ++Index)
    {
        if (IsOptionValid(Attacks[Index]))
        {
            Candidates.Add(Index);
        }
    }

    // 至少有两种有效攻击时，才排除上一次攻击
    if (bAvoidImmediateRepeat && Candidates.Num() > 1)
    {
        Candidates.Remove(LastAttackIndex);
    }

    if (Candidates.IsEmpty())
    {
        return INDEX_NONE;
    }

    double TotalWeight = 0.0;

    for (const int32 Index : Candidates)
    {
        TotalWeight += Attacks[Index].Weight;
    }

    const double RandomValue = FMath::FRand() * TotalWeight;
    double AccumulatedWeight = 0.0;

    for (const int32 Index : Candidates)
    {
        AccumulatedWeight += Attacks[Index].Weight;

        if (RandomValue < AccumulatedWeight)
        {
            return Index;
        }
    }

    return Candidates.Last();
}

EBTNodeResult::Type UShooterBTTaskMeleeAttack::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory)
{
    CachedEnemy.Reset();
    CachedTarget.Reset();
    CachedAnimInstance.Reset();
    ActiveMontage = nullptr;

    bTaskRunning = false;
    bAnimationStarted = false;
    bAnimationEnded = false;
    bAnimationInterrupted = false;
    bHitChecked = false;
    bFacingLocked = false;
    FacingWaitTime = 0.0f;

    SelectedAttackIndex = SelectAttackIndex();

    if (SelectedAttackIndex == INDEX_NONE || SlotName.IsNone())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Random melee task: no valid attack configuration")
        );

        return EBTNodeResult::Failed;
    }

    AAIController* AIController = OwnerComp.GetAIOwner();
    UBlackboardComponent* BlackboardComp =
        OwnerComp.GetBlackboardComponent();

    if (!IsValid(AIController) || !IsValid(BlackboardComp))
    {
        return EBTNodeResult::Failed;
    }

    CachedEnemy = Cast<AShooterMeleeEnemy>(
        AIController->GetPawn()
    );

    CachedTarget = Cast<AShooterSamProjectCharacter>(
        BlackboardComp->GetValueAsObject(
            BlackboardKey.SelectedKeyName
        )
    );

    if (!CachedEnemy.IsValid() || !CachedTarget.IsValid())
    {
        return EBTNodeResult::Failed;
    }

    CachedAnimInstance =
        CachedEnemy->GetMesh()->GetAnimInstance();

    if (!CachedAnimInstance.IsValid())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Random melee task: enemy has no animation instance")
        );

        return EBTNodeResult::Failed;
    }

    ActiveAttack = Attacks[SelectedAttackIndex];
    bTaskRunning = true;

    AIController->StopMovement();

    return EBTNodeResult::InProgress;
}

void UShooterBTTaskMeleeAttack::TickTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory,
    float DeltaSeconds)
{
    AShooterMeleeEnemy* Enemy = CachedEnemy.Get();
    AShooterSamProjectCharacter* Target = CachedTarget.Get();
    UAnimInstance* AnimInstance = CachedAnimInstance.Get();
    AAIController* AIController = OwnerComp.GetAIOwner();

    if (!bTaskRunning ||
        !IsValid(Enemy) ||
        Enemy->IsDead() ||
        !IsValid(Target) ||
        !Target->IsAlive ||
        !Target->IsPlayerControlled() ||
        !IsValid(AnimInstance) ||
        !IsValid(AIController) ||
        AIController->GetPawn() != Enemy)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // 动作结束后等待恢复时间
    if (bAnimationEnded)
    {
        if (bAnimationInterrupted)
        {
            FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
            return;
        }

        if (Enemy->IsMeleeAttackReady())
        {
            FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        }

        return;
    }

    if (!bAnimationStarted)
    {
        // 动作尚未开始时，目标离开就返回追击
        if (Enemy->GetDistanceTo(Target) >
                Enemy->GetMeleeAttackRange() ||
            !AIController->LineOfSightTo(Target))
        {
            FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
            return;
        }

        if (!Enemy->IsMeleeAttackReady())
        {
            return;
        }

        // 先朝向玩家，再开始动作和锁定方向
        const FVector TargetDirection =
            (Target->GetActorLocation() - Enemy->GetActorLocation())
            .GetSafeNormal2D();

        const float FacingDot = FVector::DotProduct(
            Enemy->GetActorForwardVector().GetSafeNormal2D(),
            TargetDirection
        );

        if (FacingDot < 0.5f)
        {
            FacingWaitTime += DeltaSeconds;

            if (FacingWaitTime >= 2.0f)
            {
                FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
            }

            return;
        }

        ActiveMontage =
            AnimInstance->PlaySlotAnimationAsDynamicMontage(
                ActiveAttack.Animation.Get(),
                SlotName,
                0.1f,
                0.1f,
                ActiveAttack.PlayRate,
                1,
                0.0f
            );

        if (!IsValid(ActiveMontage.Get()) ||
            !AnimInstance->Montage_IsActive(ActiveMontage.Get()))
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("%s failed to play melee animation"),
                *Enemy->GetName()
            );

            FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
            return;
        }

        FOnMontageEnded EndDelegate;
        EndDelegate.BindUObject(
            this,
            &UShooterBTTaskMeleeAttack::HandleMontageEnded
        );

        AnimInstance->Montage_SetEndDelegate(
            EndDelegate,
            ActiveMontage.Get()
        );

        bAnimationStarted = true;
        LastAttackIndex = SelectedAttackIndex;

        // 先预留整个动作和恢复阶段。
        // 即使动作被中断，也不能立即重新攻击。
        const float Duration =
            ActiveMontage->GetPlayLength() / ActiveAttack.PlayRate;

        Enemy->ReserveMeleeCooldown(
            Duration + ActiveAttack.RecoveryTime
        );

        if (ActiveAttack.bLockFacing)
        {
            LockFacing();
        }

        UE_LOG(
            LogTemp,
            Log,
            TEXT("%s started attack %s: multiplier=%.2f"),
            *Enemy->GetName(),
            *ActiveAttack.AttackName.ToString(),
            ActiveAttack.DamageMultiplier
        );

        return;
    }

    // 蒙太奇已不再活跃时，不补发伤害。
    // 正常结束或中断结果由结束回调记录。
    if (!AnimInstance->Montage_IsActive(ActiveMontage.Get()) ||
        !AnimInstance->Montage_IsPlaying(ActiveMontage.Get()))
    {
        return;
    }

    const float Position =
        AnimInstance->Montage_GetPosition(ActiveMontage.Get());

    if (!bHitChecked && Position >= ActiveAttack.HitTime)
    {
        // 命中和挥空都只检查一次
        bHitChecked = true;

        Enemy->TryApplyMeleeHit(
            Target,
            ActiveAttack.DamageMultiplier
        );

        // 伤害可能触发玩家死亡和行为树中断
        return;
    }
}

void UShooterBTTaskMeleeAttack::HandleMontageEnded(
    UAnimMontage* Montage,
    bool bInterrupted)
{
    if (!bTaskRunning || Montage != ActiveMontage.Get())
    {
        return;
    }

    bAnimationEnded = true;
    bAnimationInterrupted = bInterrupted;

    // 用实际结束时刻补足恢复时间
    if (AShooterMeleeEnemy* Enemy = CachedEnemy.Get())
    {
        Enemy->ReserveMeleeCooldown(ActiveAttack.RecoveryTime);
    }

    RestoreFacing();
}

void UShooterBTTaskMeleeAttack::LockFacing()
{
    AShooterMeleeEnemy* Enemy = CachedEnemy.Get();

    if (!IsValid(Enemy) || bFacingLocked)
    {
        return;
    }

    UCharacterMovementComponent* Movement =
        Enemy->GetCharacterMovement();

    if (!IsValid(Movement))
    {
        return;
    }

    bSavedControllerYaw = Enemy->bUseControllerRotationYaw;
    bSavedDesiredRotation = Movement->bUseControllerDesiredRotation;
    bSavedOrientToMovement = Movement->bOrientRotationToMovement;

    Enemy->bUseControllerRotationYaw = false;
    Movement->bUseControllerDesiredRotation = false;
    Movement->bOrientRotationToMovement = false;

    bFacingLocked = true;
}

void UShooterBTTaskMeleeAttack::RestoreFacing()
{
    if (!bFacingLocked)
    {
        return;
    }

    if (AShooterMeleeEnemy* Enemy = CachedEnemy.Get())
    {
        Enemy->bUseControllerRotationYaw = bSavedControllerYaw;

        if (UCharacterMovementComponent* Movement =
            Enemy->GetCharacterMovement())
        {
            Movement->bUseControllerDesiredRotation =
                bSavedDesiredRotation;

            Movement->bOrientRotationToMovement =
                bSavedOrientToMovement;
        }
    }

    bFacingLocked = false;
}

void UShooterBTTaskMeleeAttack::OnTaskFinished(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory,
    EBTNodeResult::Type TaskResult)
{
    // 先关闭任务，防止停止蒙太奇产生的回调继续处理攻击
    bTaskRunning = false;
    
    // 动画结束回调已处理恢复时间。
    // 只有动作尚未结束就被中止时，才在这里补充冷却。
    if (bAnimationStarted && !bAnimationEnded)
    {
        if (AShooterMeleeEnemy* Enemy = CachedEnemy.Get())
        {
            Enemy->ReserveMeleeCooldown(ActiveAttack.RecoveryTime);
        }
    }

    RestoreFacing();

    if (UAnimInstance* AnimInstance = CachedAnimInstance.Get())
    {
        if (IsValid(ActiveMontage.Get()))
        {
            AnimInstance->Montage_Stop(0.1f, ActiveMontage.Get());
        }
    }

    ActiveMontage = nullptr;
    CachedEnemy.Reset();
    CachedTarget.Reset();
    CachedAnimInstance.Reset();

    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}