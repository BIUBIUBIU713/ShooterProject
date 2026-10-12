#include "BossEncounterSwitch.h"

#include "ShooterBossEnemy.h"
#include "ShooterSamProjectCharacter.h"
#include "ShooterSamProjectGameMode.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

ABossEncounterSwitch::ABossEncounterSwitch()
{
	PrimaryActorTick.bCanEverTick = false;
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	SetRootComponent(InteractionSphere);
	InteractionSphere->InitSphereRadius(180.0f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionSphere->SetGenerateOverlapEvents(true);
	InteractionSphere->SetCanEverAffectNavigation(false);
	SwitchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwitchMesh"));
	SwitchMesh->SetupAttachment(InteractionSphere);
	SwitchMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	SwitchMesh->SetGenerateOverlapEvents(false);
	// 可在蓝图替换外观；复用引擎模型作为可见的开关底座。
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	SwitchMesh->SetStaticMesh(Mesh.Object);
	SwitchMesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.8f));
}

bool ABossEncounterSwitch::CanInteract(AShooterSamProjectCharacter* Player) const
{
	const AShooterSamProjectGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<AShooterSamProjectGameMode>() : nullptr;
	return IsValid(GameMode) && GameMode->WaveState != EWaveState::GameOver &&
		IsValid(Player) && Player->IsAlive && Player->IsPlayerControlled() &&
		Player->GetWorld() == GetWorld() &&
		InteractionSphere->IsOverlappingComponent(Player->GetCapsuleComponent());
}

FText ABossEncounterSwitch::GetInteractionText() const
{
	const AShooterSamProjectGameMode* GameMode = GetWorld()
		? GetWorld()->GetAuthGameMode<AShooterSamProjectGameMode>() : nullptr;
	if (!IsValid(GameMode) || GameMode->WaveState == EWaveState::GameOver)
		return NSLOCTEXT("BossSwitch", "Unavailable", "Boss 战开关暂不可用");
	if (GameMode->IsBossDefeated())
		return NSLOCTEXT("BossSwitch", "Defeated", "Boss 已击败，可以前往撤离楼解锁入口");
	if (GameMode->GetBossEncounterState() == EBossEncounterState::InProgress)
		return NSLOCTEXT("BossSwitch", "InProgress", "Boss 战已启动");
	if (GameMode->GetBossEncounterState() == EBossEncounterState::Cancelled)
		return NSLOCTEXT("BossSwitch", "Cancelled", "Boss 战已终止");
	if (!GameMode->IsUpgradeStationUnlocked())
		return NSLOCTEXT("BossSwitch", "Locked", "需要先解锁增益站");
	if (!IsValid(TargetBoss) || TargetBoss->GetWorld() != GetWorld() || !TargetBoss->CanStartEncounter())
		return NSLOCTEXT("BossSwitch", "NotReady", "Boss 暂不可用");
	return NSLOCTEXT("BossSwitch", "Start", "[E] 启动 Boss 战");
}

bool ABossEncounterSwitch::TryInteract(AShooterSamProjectCharacter* Player)
{
	if (!CanInteract(Player)) return false;
	AShooterSamProjectGameMode* GameMode = GetWorld()->GetAuthGameMode<AShooterSamProjectGameMode>();
	return GameMode && GameMode->TryStartBossEncounter(Player, TargetBoss);
}
