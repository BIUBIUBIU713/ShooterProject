// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "ShooterSamProjectCharacter.h"

#include "shootAI.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERSAMPROJECT_API AshootAI : public AAIController
{
	GENERATED_BODY()
	
	virtual void BeginPlay() override;
	
	
public:
	virtual void Tick(float DeltaTime) override;
	
	APawn* PlayPawn;
	
	UPROPERTY(EditAnywhere)
	float MoveInstance = 200.0f;
	
	UPROPERTY(EditAnywhere)
	UBehaviorTree* EnemyAIBehaviorTree;
	
	AShooterSamProjectCharacter* PlayerCharacter;
	AShooterSamProjectCharacter* MyCharacter;
	
	void StartBehaviorTree(AShooterSamProjectCharacter* Player);
};
