// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterInteractable.h"

#include "PowerPartPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class SHOOTERSAMPROJECT_API APowerPartPickup : public AActor, public IShooterInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APowerPartPickup();
	
	virtual  bool CanInteract(AShooterSamProjectCharacter* Player) const override;
	virtual FText GetInteractionText() const override;
	virtual bool TryInteract(AShooterSamProjectCharacter* Player) override;

protected:
	//玩家进入这个范围后，可以发现交互目标
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> PickupSphere;
	
	//道具外观，不参与碰撞
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PickupMesh;
	
public:	
	//防止该实例重复处理拾取
	bool bCollected = false;

};
