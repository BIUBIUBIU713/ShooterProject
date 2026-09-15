// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterInteractable.h"
#include "EnergyPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class SHOOTERSAMPROJECT_API AEnergyPickup : public AActor, public IShooterInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEnergyPickup();
	
	virtual bool CanInteract(AShooterSamProjectCharacter* Player)const override;
	
	virtual FText GetInteractionText() const override;
	
	virtual bool TryInteract(AShooterSamProjectCharacter* Player) override;

protected:
	//决定触碰拾取范围
	UPROPERTY(VisibleAnywhere, Category = "Supply")
	TObjectPtr<USphereComponent> PickupSphere;
	
	//补给物的外观
	UPROPERTY(VisibleAnywhere, Category = "Supply")
	TObjectPtr<UStaticMeshComponent> PickupMesh;
	
	virtual void BeginPlay() override;

	//每个补给物初始包含多少块电池
	UPROPERTY(
		EditDefaultsOnly,
		Category = "Supply",
		meta = (ClampMin = "1")
	)
	int32 BatteryAmount = 2;
	
	//实际尚未领取的数量
	UPROPERTY(VisibleInstanceOnly, Category = "Supply")
	int32 RemainingBatteries = 0;
	
private:
	//防止同一个补给物重复生效
	bool bConsumed = false;

};
