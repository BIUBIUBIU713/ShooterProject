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
	
	//检查门是否处于可打开状态，以及动画参数是否有效
	bool CanStartOpening() const;
	
	//只负责打开门，不检查玩家距离，也不扣款
	void StartOpening();

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
	
	//相同标识的门一起打开：None表示独立门
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Door")
	FName DoorGroupId = NAME_None;
	
	//0 表示不限制回合： 15 表示第15回合开始允许解锁
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door", meta = (ClampMin = "0"))
	int32 RequiredWave = 0;
	
	//是否需要先恢复供电，才能解锁这扇门
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	bool bRequiresPower =false;
	
	//当前是满足供电需求
	bool IsPowerRequiredmenMet() const;
	
	//当前是否满足回合要求
	bool HasReachedRequiredWave() const;
	
	//本次动画还剩多少距离
	float RemainingSinkDistance = 0.0f;

};
