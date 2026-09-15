// Fill out your copyright notice in the Description page of Project Settings.


#include "shootAI.h"

#include "BehaviorTree/BlackboardComponent.h"


void AshootAI::BeginPlay()
{
	Super::BeginPlay();
}

void AshootAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AshootAI::StartBehaviorTree(AShooterSamProjectCharacter* Player)
{
	if (EnemyAIBehaviorTree)
	{
		MyCharacter = Cast<AShooterSamProjectCharacter>(GetPawn());
		if (Player)
		{
			PlayerCharacter = Player;
		}
		RunBehaviorTree(EnemyAIBehaviorTree);
	}
	
	UBlackboardComponent* MyBlackBoard = GetBlackboardComponent();
	if (MyBlackBoard && PlayerCharacter)
	{
		MyBlackBoard->SetValueAsVector("PlayerLocation", PlayerCharacter->GetActorLocation());
		MyBlackBoard->SetValueAsVector("StartLocation",MyCharacter->GetActorLocation());
	}
}

/*
*PlayPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
if (PlayPawn)
{
if (LineOfSightTo(PlayPawn))
{
SetFocus(PlayPawn);
MoveToActor(PlayPawn, MoveInstance);
}
else
{
ClearFocus(EAIFocusPriority::Gameplay);
StopMovement();
}
}
*/
