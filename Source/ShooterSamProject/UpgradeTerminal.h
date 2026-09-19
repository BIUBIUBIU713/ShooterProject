// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterInteractable.h"
#include "ShooterSamProjectGameMode.h"

#include "UpgradeTerminal.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class SHOOTERSAMPROJECT_API AUpgradeTerminal : public AActor, public IShooterInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AUpgradeTerminal();
	
	virtual bool CanInteract(AShooterSamProjectCharacter* Player) const override;
	virtual FText GetInteractionText() const override;
	virtual bool TryInteract(AShooterSamProjectCharacter* Player) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionSphere;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TerminalMesh;
	
	//同一个类可以制作不同增益终端
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrades")
	EPlayerUpgradeType UpgradeType = EPlayerUpgradeType::Reload;

public:	
	FText GetUpgradeName() const;
	
	FText GetPurchaseResultText(EUpgradePurchaseResult Result) const;

};
