// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterBTTaskBossShoot.h"

#include "ShooterBossEnemy.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UShooterBTTaskBossShoot::UShooterBTTaskBossShoot()
{
	NodeName = TEXT("Boss Shoot Attack");

	// 每个 AI 使用自己的任务实例，避免共享射击状态。
	bCreateNodeInstance = true;
	bNotifyTick = true;
	bNotifyTaskFinished = true;
}

EBTNodeResult::Type UShooterBTTaskBossShoot::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory
)
{
	CachedBoss.Reset();

	AAIController* AI = OwnerComp.GetAIOwner();
	AShooterBossEnemy* Boss = AI
		? Cast<AShooterBossEnemy>(AI->GetPawn())
		: nullptr;

	if (!IsValid(Boss) || Boss->IsDead())
	{
		return EBTNodeResult::Failed;
	}

	// TryStartShoot 内部负责停止移动和启动本轮射击。
	if (!Boss->TryStartShoot())
	{
		return EBTNodeResult::Failed;
	}

	// 只有成功启动，才记录本任务负责的 Boss。
	CachedBoss = Boss;
	return EBTNodeResult::InProgress;
}

void UShooterBTTaskBossShoot::TickTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds
)
{
	AShooterBossEnemy* Boss = CachedBoss.Get();
	if (!IsValid(Boss) || Boss->IsDead())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 等待射击、动画淡出和公共恢复时间全部结束。
	if (!Boss->IsAttackBusy())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

void UShooterBTTaskBossShoot::OnTaskFinished(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	EBTNodeResult::Type TaskResult
)
{
	// 任务失败或被中止时，清理定时器并停止射击动画。
	if (TaskResult != EBTNodeResult::Succeeded && CachedBoss.IsValid())
	{
		CachedBoss->StopShoot();
	}

	CachedBoss.Reset();
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

