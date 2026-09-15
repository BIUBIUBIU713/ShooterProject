// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterInteractable.h"

#include "RegionDoor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USphereComponent;

UCLASS()
class SHOOTERSAMPROJECT_API ARegionDoor : public AActor, public IShooterInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARegionDoor();
	
	virtual void Tick(float DeltaTime) override;
	virtual bool CanInteract(AShooterSamProjectCharacter* Player)const override;
	virtual FText GetInteractionText() const override;
	virtual bool TryInteract(AShooterSamProjectCharacter* Player) override;

protected:
	//门Actor的整体位置基准
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;
	
	//门的外观与阻挡碰撞
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> DoorMesh;
	
	//玩家靠近后，允许发现这个交互目标
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionSphere;
	
	//暂定测试价格，之后可以在关卡实例上进行调整
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door" , meta = (ClampMin = "0"))
	int32 UnlockCost = 200;
	
	//本局运行时状态，不允许在编辑器中直接修改
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Door")
	bool bUnlocked = false;
	
	//总共下称多少厘米， 应大于门露出地面的高度
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Opening", meta = (ClampMin = "1.0"))
	float SinkDistance = 350.0f;
	
	//每秒下沉多少厘米
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door|Opening", meta = (ClampMin = "1.0"))
	float SinkSpeed = 150.0f;
	
	//本次动画还剩多少距离
	float RemainingSinkDistance = 0.0f;

};
