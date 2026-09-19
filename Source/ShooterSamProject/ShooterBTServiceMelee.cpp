#include "ShooterBTServiceMelee.h"
#include "AIController.h"

UShooterBTServiceMelee::UShooterBTServiceMelee(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Focus Melee Target");

	bNotifyTick = false;
	bTickIntervals = false;

	FocusPriority = EAIFocusPriority::Gameplay;
}