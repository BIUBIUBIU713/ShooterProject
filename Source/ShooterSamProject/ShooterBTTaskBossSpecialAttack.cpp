#include "ShooterBTTaskBossSpecialAttack.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "ShooterBossEnemy.h"

UShooterBTTaskBossSpecialAttack::UShooterBTTaskBossSpecialAttack()
{
	NodeName = TEXT("Boss Special Attack");
	bCreateNodeInstance = true;
	bNotifyTick = true;
	bNotifyTaskFinished = true;
}

EBTNodeResult::Type UShooterBTTaskBossSpecialAttack::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	CachedBoss.Reset();
	AAIController* AI = OwnerComp.GetAIOwner();
	AShooterBossEnemy* Boss = AI ? Cast<AShooterBossEnemy>(AI->GetPawn()) : nullptr;
	if (!IsValid(Boss) || Boss->IsDead() || !Boss->TryStartSpecialAttack())
	{
		return EBTNodeResult::Failed;
	}
	CachedBoss = Boss;
	return EBTNodeResult::InProgress;
}

void UShooterBTTaskBossSpecialAttack::TickTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AShooterBossEnemy* Boss = CachedBoss.Get();
	if (!IsValid(Boss) || Boss->IsDead())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	// 完整等待咆哮、10 秒扫射、动画淡出和公共收招。
	if (!Boss->IsAttackBusy())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

void UShooterBTTaskBossSpecialAttack::OnTaskFinished(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	if (TaskResult != EBTNodeResult::Succeeded && CachedBoss.IsValid())
	{
		CachedBoss->StopSpecialAttack();
	}
	CachedBoss.Reset();
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
