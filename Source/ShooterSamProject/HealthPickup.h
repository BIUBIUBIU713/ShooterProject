// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include  "ShooterInteractable.h"

#include "HealthPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class SHOOTERSAMPROJECT_API AHealthPickup : public AActor, public IShooterInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHealthPickup();
	
	virtual bool CanInteract(AShooterSamProjectCharacter* Player) const override;
	
	virtual FText GetInteractionText() const override;
	
	virtual bool TryInteract(AShooterSamProjectCharacter* Player) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	//玩家进入此范围后，可以选择该补给物
	UPROPERTY(VisibleAnywhere, Category = "Supply")
	TObjectPtr<USphereComponent> PickupSphere;
	
	UPROPERTY(VisibleAnywhere, Category = "Supply")
	TObjectPtr<UStaticMeshComponent> PickupMesh;
	
	//初始可提供的治疗量
	UPROPERTY(EditAnywhere, Category = "Supply", meta = (ClampMin = "0.1"))
	float HealingAmount = 50.0f;
	
	//尚未被使用的治疗量
	UPROPERTY(VisibleInstanceOnly, Category = "Supply")
	float RemainingHealing = 0.0f;

public:	
	bool bConsumed = false;

};
