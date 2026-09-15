// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterEnemyBase.h"

#include "BehaviorTree/BehaviorTree.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ShooterSamProjectCharacter.h"
#include "GameFramework/Controller.h"

// Sets default values
AShooterEnemyBase::AShooterEnemyBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	CurrentHealth = MaxHealth;

}

float AShooterEnemyBase::GetCurrentHealth() const
{
	return CurrentHealth;
}

float AShooterEnemyBase::GetMaxHealth() const
{
	return MaxHealth;
}

bool AShooterEnemyBase::IsDead() const
{
	return bIsDead;
}

UBehaviorTree* AShooterEnemyBase::GetBehaviorTree() const
{
	return BehaviorTree.Get();
}

float AShooterEnemyBase::TakeDamage(
	float DamageAmount,
	const FDamageEvent& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser
)
{
	//已死亡或无效伤害， 不再继续处理
	if (bIsDead || !FMath::IsFinite(DamageAmount) || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}
	
	const float AppliedDamage = Super::TakeDamage(
		DamageAmount,
		DamageEvent,
		EventInstigator,
		DamageCauser
	);
	
	if (bIsDead || !FMath::IsFinite(AppliedDamage) || AppliedDamage <= 0.0f)
	{
		return 0.0f;
	}
	
	CurrentHealth = FMath::Max(
		CurrentHealth - AppliedDamage,
		0.0f
	);
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s took %.1f damage, health: %.1f / %.1f"),
		*GetName(),
		AppliedDamage,
		CurrentHealth,
		MaxHealth
	);
	
	if (CurrentHealth <= 0.0f)
	{
		HandleDeath(EventInstigator);
	}
	
	return AppliedDamage;
}

// Called when the game starts or when spawned
void AShooterEnemyBase::BeginPlay()
{
	MaxHealth = FMath::Max(MaxHealth, 1.0f);
	CurrentHealth = MaxHealth;
	bIsDead = false;
	
	Super::BeginPlay();
}

void AShooterEnemyBase::HandleDeath(AController* KillerController)
{
	if (bIsDead)
	{
		return;
	}
	
	//必须先标记死亡，再执行清理和广播
	bIsDead = true;
	CurrentHealth = 0.0f;
	SetCanBeDamaged(false);
	
	AAIController* AIController = Cast<AAIController>(GetController());
	
	if (IsValid(AIController))
	{
		UBehaviorTreeComponent* BehaviorTreeComp = 
			Cast<UBehaviorTreeComponent>(AIController->GetBrainComponent());
		
		if (IsValid(BehaviorTreeComp))
		{
			BehaviorTreeComp->StopTree(
				EBTStopMode::Forced	
			);
		}
		
		AIController->StopMovement();
	}
	
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	
	//当前没有死亡动画或布娃娃，死亡后不再阻挡其他角色
	SetActorEnableCollision(false);
	
	//与控制器解除关系，走UE的Pawn销毁前处理流程
	DetachFromControllerPendingDestroy();
	
	//先安排清理，再通知外部系统
	SetLifeSpan(FMath::Max(DeathLifeSpan, 0.1f));
	
	UE_LOG(
		LogTemp,
		Log,
		TEXT("%s died; broadcasting OnEnemyDied"),
		*GetName()
	);
	
	AShooterSamProjectCharacter* KillerPlayer = nullptr;
	
	if (IsValid(KillerController))
	{
		KillerPlayer = Cast<AShooterSamProjectCharacter>(
			KillerController->GetPawn()	
		);
	}
	
	if (IsValid(KillerPlayer) && KillerPlayer->IsPlayerControlled())
	{
		const int32 GrantedCoins = KillerPlayer->AddCoins(
			FMath::Max(KillRewardCoins, 0)	
		);
		
		UE_LOG(
			LogTemp,
			Log,
			TEXT("%s kill reward: player = %s, coins = %d"),
			*GetName(),
			*GetNameSafe(KillerPlayer),
			GrantedCoins
		);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("%s died without a player killer; no coin reward"),
			*GetName()
		);
	}
	
	OnEnemyDied.Broadcast(this);
}

// Called every frame
void AShooterEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AShooterEnemyBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

