// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterInteractable.h"

#include "PowerGenerator.generated.h"

class USphereComponent;
class USphereComponnet;
class UStaticMeshComponent;

UCLASS()
class SHOOTERSAMPROJECT_API APowerGenerator : public AActor, public IShooterInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APowerGenerator();
	
	virtual bool CanInteract(AShooterSamProjectCharacter* Player) const override;
	virtual FText GetInteractionText() const override;
	virtual bool TryInteract(AShooterSamProjectCharacter* Player) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionSphere;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> GeneratorMesh;


};
