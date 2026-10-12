// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterBTTaskBossLightAttack.h"

#include "ShooterBossEnemy.h"
#include "AIController.h"
#include "Animation/AnimMontage.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

UShooterBTTaskBossLightAttack::UShooterBTTaskBossLightAttack()
{
	NodeName = TEXT("Boss Melee Attack");
	
	//每个AI使用自己的任务实例，避免共享攻击状态
	bCreateNodeInstance = true;
	bNotifyTick = true;
	bNotifyTaskFinished = true;
}

EBTNodeResult::Type UShooterBTTaskBossLightAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	
	AShooterBossEnemy* Boss = AI
		? Cast<AShooterBossEnemy>(AI->GetPawn())
		: nullptr;
	
	if (!IsValid(Boss) || Boss->IsDead())
	{
		return EBTNodeResult::Failed;
	}
	
	//取消寻路，兵消除剩余移动速度
	AI->StopMovement();
	Boss->GetCharacterMovement()->StopMovementImmediately();
	
	if (!Boss->TryStartMeleeAttack(AttackType))
	{
		return EBTNodeResult::Failed;
	}
	
	CachedBoss = Boss;
	CachedMontage = Boss->GetCurrentMontage();
	
	//保持任务运行，阻止行为树提前执行后面的节点
	return EBTNodeResult::InProgress;
}

void UShooterBTTaskBossLightAttack::TickTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	AShooterBossEnemy* Boss = CachedBoss.Get();

	if (!IsValid(Boss) || Boss->IsDead())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 同时等待动画播放和额外恢复时间
	if (!Boss->IsMeleeAttackBusy())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

void UShooterBTTaskBossLightAttack::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	//任务失败或被中止时，停止本任务启动的攻击蒙太奇
	if (TaskResult != EBTNodeResult::Succeeded &&
		CachedBoss.IsValid() && CachedMontage.IsValid()
	)
	{
		CachedBoss->StopAnimMontage(CachedMontage.Get());
	}
	
	CachedBoss.Reset();
	CachedMontage.Reset();
	
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
