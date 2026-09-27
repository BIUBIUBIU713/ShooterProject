#include "ShooterBTServiceGuardTarget.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Kismet/GameplayStatics.h"
#include "ShooterEnemyBase.h"
#include "ShooterSamProjectCharacter.h"

UShooterBTServiceGuardTarget::UShooterBTServiceGuardTarget()
{
    NodeName = TEXT("Update Guard Target");
    bCreateNodeInstance = true; // 每名守卫保存自己的岗位和返回状态。
    INIT_SERVICE_NODE_NOTIFY_FLAGS();
    Interval = 0.2f;
    RandomDeviation = 0.0f;
    bCallTickOnSearchStart = true;
}

void UShooterBTServiceGuardTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
    AAIController* AI = OwnerComp.GetAIOwner();
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    AShooterEnemyBase* Enemy = IsValid(AI) ? Cast<AShooterEnemyBase>(AI->GetPawn()) : nullptr;
    if (!IsValid(AI) || !IsValid(BB) || !IsValid(Enemy))
    {
        return;
    }

    const FName TargetKey(TEXT("TargetActor"));
    const FName HomeKey(TEXT("GuardHomeLocation"));
    const FName ReturnKey(TEXT("GuardReturning"));
    const FName SightKey(TEXT("HasLineOfSight"));
    const FName RangeKey(TEXT("IsInAttackRange"));

    const FBlackboard::FKey HomeId = BB->GetKeyID(HomeKey);
    const FBlackboard::FKey ReturnId = BB->GetKeyID(ReturnKey);
    if (HomeId == FBlackboard::InvalidKey || ReturnId == FBlackboard::InvalidKey ||
        BB->GetKeyType(HomeId) != UBlackboardKeyType_Vector::StaticClass() ||
        BB->GetKeyType(ReturnId) != UBlackboardKeyType_Bool::StaticClass())
    {
        BB->ClearValue(TargetKey);
        BB->SetValueAsBool(SightKey, false);
        BB->SetValueAsBool(RangeKey, false);
        if (!bLoggedMissingKeys)
        {
            UE_LOG(LogTemp, Error, TEXT("Guard tree %s needs GuardHomeLocation (Vector) and GuardReturning (Bool)."), *GetNameSafe(Enemy));
            bLoggedMissingKeys = true;
        }
        return;
    }
    if (HomePawn.Get() != Enemy)
    {
        HomePawn = Enemy;
        HomeLocation = Enemy->GetActorLocation();
        bReturning = false;
        BB->SetValueAsVector(HomeKey, HomeLocation);
    }

    const float SafeAcquire = FMath::IsFinite(AcquireRadius) ? FMath::Max(AcquireRadius, 1.0f) : 1200.0f;
    const float SafeLeash = FMath::IsFinite(LeashRadius) ? FMath::Max(LeashRadius, SafeAcquire) : FMath::Max(1800.0f, SafeAcquire);
    const float SafeHome = FMath::IsFinite(HomeAcceptanceRadius)
        ? FMath::Clamp(HomeAcceptanceRadius, 1.0f, SafeAcquire) : FMath::Min(150.0f, SafeAcquire);
    const float HomeDistance = FVector::Dist(Enemy->GetActorLocation(), HomeLocation);
    AActor* PreviousTarget = Cast<AActor>(BB->GetValueAsObject(TargetKey));
    AShooterSamProjectCharacter* Player = Cast<AShooterSamProjectCharacter>(UGameplayStatics::GetPlayerPawn(AI, 0));
    const bool bValidPlayer = IsValid(Player) && Player->IsAlive && Player->IsPlayerControlled() && !Enemy->IsDead();
    const bool bHasSight = bValidPlayer && AI->LineOfSightTo(Player);
    const float PlayerDistance = bValidPlayer ? FVector::Dist(Player->GetActorLocation(), HomeLocation) : TNumericLimits<float>::Max();
    const bool bWasReturning = bReturning;

    // 进入返回状态后，即使玩家再次靠近，也必须先回到岗位。
    if (bReturning && HomeDistance <= SafeHome)
    {
        bReturning = false;
    }
    if (HomeDistance > SafeLeash || (PreviousTarget && (!bValidPlayer || PlayerDistance > SafeLeash)))
    {
        bReturning = HomeDistance > SafeHome;
    }
    AShooterSamProjectCharacter* Target = nullptr;
    if (!bReturning && bValidPlayer && HomeDistance <= SafeLeash)
    {
        const bool bRetainTarget = PreviousTarget == Player && PlayerDistance <= SafeLeash;
        const bool bAcquireTarget = PlayerDistance <= SafeAcquire && bHasSight;
        if (bRetainTarget || bAcquireTarget)
        {
            Target = Player;
        }
    }
    if (!Target && HomeDistance > SafeHome)
    {
        bReturning = true;
    }

    // 成组更新，避免装饰器在键值只更新一半时中断行为树。
    BB->PauseObserverNotifications();
    BB->SetValueAsBool(ReturnKey, bReturning);
    if (Target)
    {
        BB->SetValueAsObject(TargetKey, Target);
    }
    else
    {
        BB->ClearValue(TargetKey);
        BB->SetValueAsBool(RangeKey, false);
        AI->ClearFocus(EAIFocusPriority::Gameplay);
    }
    BB->SetValueAsBool(SightKey, Target && bHasSight);
    BB->ResumeObserverNotifications(true);

    if (PreviousTarget != Target || bWasReturning != bReturning)
    {
        UE_LOG(LogTemp, Log, TEXT("Guard state: enemy=%s target=%s returning=%d homeDistance=%.0f"),
            *GetNameSafe(Enemy), *GetNameSafe(Target), bReturning, HomeDistance);
    }
}
