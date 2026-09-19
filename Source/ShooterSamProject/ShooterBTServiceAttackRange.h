// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"

#include "ShooterBTServiceAttackRange.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERSAMPROJECT_API UShooterBTServiceAttackRange : public UBTService
{
	GENERATED_BODY()
	

public:
	UShooterBTServiceAttackRange();
	
	// 判断给定距离是否处于攻击范围内
	static bool IsDistanceInAttackRange(
		float Distance,
		float AttackRange
	);
	
protected:
	virtual void TickNode(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds
	) override;
	
	//当前行为树节点使用的攻击距离，单位为厘米
	UPROPERTY(
		EditAnywhere,
		Category = "Attack",
		meta = (ClampMin = "0.0")
	)
	float AttackRange = 800.0f;
};
