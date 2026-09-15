#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ShooterInteractable.generated.h"

class AShooterSamProjectCharacter;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UShooterInteractable : public UInterface
{
	GENERATED_BODY()
};

class SHOOTERSAMPROJECT_API IShooterInteractable
{
	GENERATED_BODY()

public:
	// 当前玩家是否处于可以交互的位置与状态
	virtual bool CanInteract(
		AShooterSamProjectCharacter* Player
	) const = 0;

	// 当前交互目标显示的提示
	virtual FText GetInteractionText() const = 0;

	// 执行交互，返回是否实际生效
	virtual bool TryInteract(
		AShooterSamProjectCharacter* Player
	) = 0;
};