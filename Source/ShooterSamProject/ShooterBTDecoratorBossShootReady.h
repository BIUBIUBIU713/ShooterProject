#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "ShooterBTDecoratorBossShootReady.generated.h"

// 只观察普通射击：进入射程且冷却结束时，允许从追赶切换到射击。
UCLASS()
class SHOOTERSAMPROJECT_API UShooterBTDecoratorBossShootReady : public UBTDecorator
{
	GENERATED_BODY()

public:
	UShooterBTDecoratorBossShootReady();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
