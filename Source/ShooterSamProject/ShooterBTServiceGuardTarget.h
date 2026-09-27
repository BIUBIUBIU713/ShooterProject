#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "ShooterBTServiceGuardTarget.generated.h"

class APawn;

// 替换守卫树中的 Update Player Target，不与原服务同时使用。
UCLASS(meta = (DisplayName = "Update Guard Target"))
class SHOOTERSAMPROJECT_API UShooterBTServiceGuardTarget : public UBTService
{
    GENERATED_BODY()

public:
    UShooterBTServiceGuardTarget();

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    // 均相对出生岗位计算，以厘米为单位。
    UPROPERTY(EditAnywhere, Category = "Guard", meta = (ClampMin = "1.0", Units = "cm"))
    float AcquireRadius = 1200.0f;

    UPROPERTY(EditAnywhere, Category = "Guard", meta = (ClampMin = "1.0", Units = "cm"))
    float LeashRadius = 1800.0f;

    UPROPERTY(EditAnywhere, Category = "Guard", meta = (ClampMin = "1.0", Units = "cm"))
    float HomeAcceptanceRadius = 150.0f;

private:
    TWeakObjectPtr<APawn> HomePawn;
    FVector HomeLocation = FVector::ZeroVector;
    bool bReturning = false;
    bool bLoggedMissingKeys = false;
};
