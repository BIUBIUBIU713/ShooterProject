#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterInteractable.h"
#include "BossEncounterSwitch.generated.h"

class AShooterBossEnemy;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class SHOOTERSAMPROJECT_API ABossEncounterSwitch : public AActor, public IShooterInteractable
{
	GENERATED_BODY()
public:
	ABossEncounterSwitch();
	virtual bool CanInteract(AShooterSamProjectCharacter* Player) const override;
	virtual FText GetInteractionText() const override;
	virtual bool TryInteract(AShooterSamProjectCharacter* Player) override;

	// 指向关卡中由设计者摆放的 Boss，不生成第二个 Boss。
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Boss|Encounter")
	TObjectPtr<AShooterBossEnemy> TargetBoss;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SwitchMesh;
};
