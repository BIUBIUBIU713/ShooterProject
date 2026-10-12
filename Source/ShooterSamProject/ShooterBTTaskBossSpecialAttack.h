#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ShooterBTTaskBossSpecialAttack.generated.h"

class AShooterBossEnemy;

// 放在普通追击/攻击分支之前；条件不满足时返回 Failed，让普通分支继续。
UCLASS()
class SHOOTERSAMPROJECT_API UShooterBTTaskBossSpecialAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UShooterBTTaskBossSpecialAttack();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
		EBTNodeResult::Type TaskResult) override;

private:
	UPROPERTY()
	TWeakObjectPtr<AShooterBossEnemy> CachedBoss;
};
