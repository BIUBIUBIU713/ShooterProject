// Fill out your copyright notice in the Description page of Project Settings.
//只保存出生点的位置，方向和类型，不在这里生成敌人
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawnPoint.generated.h"

class USceneComponent;
class UArrowComponent;

//敌人出生点类型
UENUM(BlueprintType)
enum class EEnemySpawnPointType : uint8
{
	MainBattlefield UMETA(DisplayName = "Main Battlerfield"),
	Outer UMETA(DisplayName = "Outer")
};

UCLASS()
class SHOOTERSAMPROJECT_API AEnemySpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEnemySpawnPoint();
	
	//获取敌人生成时使用的位置和旋转
	UFUNCTION(BlueprintPure, Category = "Enemy Spawn Point")
	FTransform GetSpawnTransform() const;
	
	//当前出生点是否允许使用
	UFUNCTION(BlueprintPure, Category = "Enemy Spawn Point")
	bool IsSpawnEnabled() const;
	
	//是否为主战场出生点
	UFUNCTION(BlueprintPure, Category = "Enemy Spawn point")
	bool IsMainBattlefield() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	//根组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	//在编辑器中显示生成方向
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> SpawnArrow;
	
	//主战场或外围出生点
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	EEnemySpawnPointType SpawnPointType = EEnemySpawnPointType::Outer;
	
	//是否允许波次系统使用这个出生点
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	bool bSpawnEnabled = true;

};
