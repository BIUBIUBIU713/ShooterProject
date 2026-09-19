#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_DefaultFocus.h"
#include "ShooterBTServiceMelee.generated.h"

UCLASS()
class SHOOTERSAMPROJECT_API UShooterBTServiceMelee
	: public UBTService_DefaultFocus
{
	GENERATED_BODY()

public:
	UShooterBTServiceMelee(
		const FObjectInitializer& ObjectInitializer =
			FObjectInitializer::Get()
	);
};