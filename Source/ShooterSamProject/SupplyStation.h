#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterInteractable.h"
#include "ShooterSamProjectCharacter.h"
#include "SupplyStation.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ESupplyProduct : uint8
{
	Battery UMETA(DisplayName = "Battery"),
	Healing UMETA(DisplayName = "Healing")
};

UCLASS()
class SHOOTERSAMPROJECT_API ASupplyStation
	: public AActor, public IShooterInteractable
{
	GENERATED_BODY()

public:
	ASupplyStation();

	virtual bool CanInteract(
		AShooterSamProjectCharacter* Player) const override;

	virtual FText GetInteractionText() const override;

	virtual bool TryInteract(
		AShooterSamProjectCharacter* Player) override;

	UFUNCTION(BlueprintCallable, Category = "Supply")
	ESupplyPurchaseResult TryPurchase(
		AShooterSamProjectCharacter* Player,
		ESupplyProduct Product);

	UFUNCTION(BlueprintPure, Category = "Supply")
	FText GetOfferText(
		AShooterSamProjectCharacter* Player,
		ESupplyProduct Product) const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Supply")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(VisibleAnywhere, Category = "Supply")
	TObjectPtr<UStaticMeshComponent> StationMesh;

	UPROPERTY(EditAnywhere, Category = "Supply|Battery",
		meta = (ClampMin = "1"))
	int32 BatteryAmount = 1;

	UPROPERTY(EditAnywhere, Category = "Supply|Battery",
		meta = (ClampMin = "1"))
	int32 BatteryPrice = 10;

	UPROPERTY(EditAnywhere, Category = "Supply|Healing",
		meta = (ClampMin = "0.1"))
	float HealingAmount = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Supply|Healing",
		meta = (ClampMin = "1"))
	int32 HealingPrice = 20;
};