#include "ShooterEnemyAIController.h"

#include "ShooterEnemyBase.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

AShooterEnemyAIController::AShooterEnemyAIController()
{
	UAIPerceptionComponent* EnemyPerception = 
		CreateDefaultSubobject<UAIPerceptionComponent>(
			TEXT("EnemyPerception")	
		);
	
	SetPerceptionComponent(*EnemyPerception);
	
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(
		TEXT("SightConfig")	
	);
	
	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 1800.0f;
	SightConfig->PeripheralVisionAngleDegrees = 60.0f;
	
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	
	EnemyPerception->ConfigureSense(*SightConfig);
	EnemyPerception->SetDominantSense(
		SightConfig->GetSenseImplementation()	
	);
	
}

void AShooterEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	ControlledEnemy = Cast<AShooterEnemyBase>(GetPawn());
	
	if (!IsValid(ControlledEnemy))
	{
		return;
	}
	
	UBehaviorTree* EnemyBehaviorTree =
		ControlledEnemy->GetBehaviorTree();
	
	if (!IsValid(EnemyBehaviorTree))
	{
		return;
	}
	
	if (!RunBehaviorTree(EnemyBehaviorTree))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Failed to start behavior tree %s for enemy %s"),
			*GetNameSafe(EnemyBehaviorTree),
			*GetNameSafe(ControlledEnemy.Get())
		);
	}
	
}

void AShooterEnemyAIController::OnUnPossess()
{
	Super::OnUnPossess();
	
	ControlledEnemy = nullptr;
}

