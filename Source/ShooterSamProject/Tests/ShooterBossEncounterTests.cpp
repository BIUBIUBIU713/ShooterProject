#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "BossEncounterSwitch.h"
#include "RegionDoor.h"
#include "ShooterBossEnemy.h"
#include "ShooterSamProjectCharacter.h"
#include "ShooterSamProjectGameMode.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/LatentActionManager.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShooterBossEncounterTest,
    "ShooterSam.Boss.Encounter.ProgressionAndLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShooterBossEncounterTest::RunTest(const FString& Parameters)
{
    UClass* BossClass = LoadClass<AShooterBossEnemy>(nullptr,
        TEXT("/Game/MyStuff/BluePrints/Enemies/BP_ShooterBossEnemy.BP_ShooterBossEnemy_C"));
    UClass* PlayerClass = LoadClass<AShooterSamProjectCharacter>(nullptr,
        TEXT("/Game/MyStuff/Character/BP_ShooterSamCharacter.BP_ShooterSamCharacter_C"));
    UClass* SwitchClass = LoadClass<ABossEncounterSwitch>(nullptr,
        TEXT("/Game/MyStuff/BluePrints/Boss/BP_BossEncounterSwitch.BP_BossEncounterSwitch_C"));
    UBehaviorTree* BossTree = LoadObject<UBehaviorTree>(nullptr,
        TEXT("/Game/MyStuff/EnemyBT/BT_ShooterBoss.BT_ShooterBoss"));
    UAnimMontage* EntranceMontage = LoadObject<UAnimMontage>(nullptr,
        TEXT("/Game/SciFi_ToiletMech/Animation/AM_Boss_Entrance.AM_Boss_Entrance"));
    if (!TestNotNull(TEXT("Boss BP"), BossClass) || !TestNotNull(TEXT("Player BP"), PlayerClass) ||
        !TestNotNull(TEXT("Switch BP"), SwitchClass) || !TestNotNull(TEXT("Boss tree"), BossTree) ||
        !TestNotNull(TEXT("Entrance montage"), EntranceMontage)) return false;
    const int32 EntranceFrames = FMath::CeilToInt((3.f + EntranceMontage->GetPlayLength() /
        FMath::Max(EntranceMontage->RateScale, 0.01f) + 1.f) / 0.05f);

    // Success, game over during entrance, removal without death, game over during combat.
    for (int32 Scenario = 0; Scenario < 4; ++Scenario)
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false)
            .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(true).ShouldSimulatePhysics(false);
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        World->SetGameInstance(NewObject<UGameInstance>(GEngine));
        FURL URL;
        URL.AddOption(TEXT("game=/Game/MyStuff/BluePrints/BP_ShooterSamGameMode.BP_ShooterSamGameMode_C"));
        World->SetGameMode(URL);
        AShooterSamProjectGameMode* GM = World->GetAuthGameMode<AShooterSamProjectGameMode>();
        if (!TestNotNull(TEXT("Real game mode"), GM))
        {
            World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return false;
        }
        FActorSpawnParameters Spawn;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AShooterSamProjectCharacter* Player = World->SpawnActor<AShooterSamProjectCharacter>(PlayerClass, FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
        APlayerController* PC = World->SpawnActor<APlayerController>();
        PC->PlayerState = World->SpawnActor<APlayerState>();
        PC->Possess(Player);
        AShooterBossEnemy* Boss = World->SpawnActor<AShooterBossEnemy>(BossClass, FVector(1500, 0, 0), FRotator(0, 180, 0), Spawn);
        Boss->bWaitForEncounterSwitch = true;
        ABossEncounterSwitch* Switch = World->SpawnActor<ABossEncounterSwitch>(SwitchClass, FVector(100, 0, 0), FRotator::ZeroRotator, Spawn);
        Switch->TargetBoss = Boss;
        ARegionDoor* UpgradeDoor = World->SpawnActor<ARegionDoor>(ARegionDoor::StaticClass(), FVector(0, 100, 0), FRotator::ZeroRotator, Spawn);
        UpgradeDoor->DoorGroupId = TEXT("UpgradeStation");
        UpgradeDoor->UnlockCost = 0;
        ARegionDoor* Door = World->SpawnActor<ARegionDoor>(ARegionDoor::StaticClass(), FVector(0, -100, 0), FRotator::ZeroRotator, Spawn);
        Door->DoorGroupId = TEXT("ExtractionBuilding");
        Door->UnlockCost = 10;
        World->InitializeActorsForPlay(URL);
        World->BeginPlay();
        Player->GetCharacterMovement()->GravityScale = 0;
        Boss->GetCharacterMovement()->GravityScale = 0;
        Boss->GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Player->SetCanBeDamaged(false);
        Player->AddCoins(1000);
        const int32 InitialCoins = Player->GetCoins();
        Player->GetCapsuleComponent()->UpdateOverlaps();
        Switch->FindComponentByClass<USphereComponent>()->UpdateOverlaps();
        UpgradeDoor->InteractionSphere->UpdateOverlaps();
        Door->InteractionSphere->UpdateOverlaps();
        const FDamageEvent Damage;
        const auto Advance = [World](int32 Frames)
        {
            for (int32 Frame = 0; Frame < Frames; ++Frame)
            {
                // TimerManager and skeletal meshes only advance once per engine frame.
                ++GFrameCounter;
                World->Tick(LEVELTICK_All, 0.05f);
            }
        };
        TestFalse(TEXT("Waiting Boss has no automatic entrance"), Boss->bEntranceStarted);
        TestEqual(TEXT("Waiting Boss rejects lethal damage"), Boss->TakeDamage(100000, Damage, PC, Player), 0.0f);
        TestFalse(TEXT("Waiting Boss cannot complete entrance early"), Boss->FinishEncounterEntrance(BossTree));
        TestFalse(TEXT("Locked station rejects switch"), Switch->TryInteract(Player));
        TestFalse(TEXT("Extraction door locked before Boss"), Door->TryInteract(Player));
        TestEqual(TEXT("Rejected interaction takes no coins"), Player->GetCoins(), InitialCoins);
        GM->RecordRegionUnlocked(TEXT("ExtractionBuilding"));
        TestFalse(TEXT("Cannot bypass Boss via region recording"), GM->IsRegionUnlocked(TEXT("ExtractionBuilding")));
        TestTrue(TEXT("Existing upgrade door unlocks station"), UpgradeDoor->TryInteract(Player));
        TestTrue(TEXT("Station unlocked with no upgrade levels purchased"), GM->IsUpgradeStationUnlocked());
        TestTrue(TEXT("Switch starts registered Boss"), Switch->TryInteract(Player));
        AddInfo(FString::Printf(TEXT("Scenario %d: pending entrance actions=%d, actor tick=%d, mesh tick=%d"),
            Scenario, World->GetLatentActionManager().GetNumActionsForObject(Boss), Boss->IsActorTickEnabled(), Boss->GetMesh()->IsComponentTickEnabled()));
        TestFalse(TEXT("Repeated switch cannot start a second encounter"), Switch->TryInteract(Player));
        TestFalse(TEXT("Invalid tree cannot open damage"), Boss->FinishEncounterEntrance(nullptr));
        TestFalse(TEXT("Tree without root cannot open damage"), Boss->FinishEncounterEntrance(NewObject<UBehaviorTree>()));
        TestEqual(TEXT("Entrance rejects lethal damage"), Boss->TakeDamage(100000, Damage, PC, Player), 0.0f);
        TestFalse(TEXT("Entrance does not trigger half health"), Boss->bHalfHealthTriggered);
        if (Scenario == 1)
        {
            GM->SetGameOver();
            Advance(EntranceFrames);
            TestFalse(TEXT("Cancelled entrance never starts combat"), Boss->IsCombatReady());
            TestFalse(TEXT("Late completed callback rejected"), Boss->FinishEncounterEntrance(BossTree));
            TestFalse(TEXT("Cancelled encounter cannot unlock extraction"), GM->IsBossDefeated());
        }
        else if (Scenario == 2)
        {
            Boss->Destroy();
            TestEqual(TEXT("Removal is cancellation, not defeat"), GM->GetBossEncounterState(), EBossEncounterState::Cancelled);
            TestFalse(TEXT("Removal leaves extraction locked"), Door->CanStartOpening());
        }
        else
        {
            // Advance the actual BP Delay and montage, not a fabricated completion callback.
            Advance(EntranceFrames);
            UAnimInstance* DebugAnim = Boss->GetMesh()->GetAnimInstance();
            UAnimMontage* DebugMontage = DebugAnim ? DebugAnim->GetCurrentActiveMontage() : nullptr;
            AddInfo(FString::Printf(TEXT("After ticks: time=%.2f latent=%d montage=%s position=%.2f controller=%s"),
                World->GetTimeSeconds(), World->GetLatentActionManager().GetNumActionsForObject(Boss),
                *GetNameSafe(DebugMontage), DebugMontage ? DebugAnim->Montage_GetPosition(DebugMontage) : -1.f, *GetNameSafe(Boss->GetController())));
            TestTrue(TEXT("Actual entrance callback enables combat"), Boss->IsCombatReady());
            TestTrue(TEXT("Ordinary waves still start during Boss encounter"), GM->CurrentWave > 0);
            TestEqual(TEXT("Boss is not registered as a wave enemy"), GM->AliveEnemyCount, 0);
            AAIController* AI = Cast<AAIController>(Boss->GetController());
            UBehaviorTreeComponent* Tree = AI ? Cast<UBehaviorTreeComponent>(AI->GetBrainComponent()) : nullptr;
            TestTrue(TEXT("Entrance started the actual behavior tree"), Tree && Tree->IsRunning());
            if (Scenario == 3)
            {
                GM->SetGameOver();
                TestFalse(TEXT("Game over stops Boss tree"), Tree && Tree->IsRunning());
                TestFalse(TEXT("Game over cancels combat"), Boss->IsCombatReady());
                TestFalse(TEXT("Game over prevents a late entrance callback"), Boss->FinishEncounterEntrance(BossTree));
                TestEqual(TEXT("Game over blocks later damage"), Boss->TakeDamage(100000, Damage, PC, Player), 0.0f);
                TestFalse(TEXT("Game over does not count as Boss defeat"), GM->IsBossDefeated());
            }
            else
            {
                Boss->TakeDamage(Boss->GetMaxHealth() * 0.55f, Damage, PC, Player);
                TestTrue(TEXT("First half health still immediately interrupts"), Boss->bSpecialActive && Boss->bRoaring);
                TestTrue(TEXT("Special interruption does not restore invulnerability"), Boss->IsCombatReady() && Boss->CanBeDamaged());
                const int32 BeforeKill = Player->GetCoins();
                const int32 Reward = Boss->KillRewardCoins;
                Boss->TakeDamage(100000, Damage, PC, Player);
                TestTrue(TEXT("Confirmed Boss death unlocks eligibility"), GM->IsBossDefeated());
                TestFalse(TEXT("Death does not automatically unlock the door"), GM->IsRegionUnlocked(TEXT("ExtractionBuilding")));
                Boss->OnEnemyDied.Broadcast(Boss);
                Boss->TakeDamage(100000, Damage, PC, Player);
                TestEqual(TEXT("Boss reward is paid exactly once"), Player->GetCoins(), BeforeKill + Reward);
                TestFalse(TEXT("Death callback cannot restart combat"), Boss->FinishEncounterEntrance(BossTree));
                Player->SetActorLocation(FVector::ZeroVector);
                Door->InteractionSphere->UpdateOverlaps();
                TestTrue(TEXT("Player can now interact to unlock extraction"), Door->TryInteract(Player));
                TestTrue(TEXT("Extraction region recorded after interaction"), GM->IsRegionUnlocked(TEXT("ExtractionBuilding")));
                TestEqual(TEXT("Existing door price still paid once"), Player->GetCoins(), BeforeKill + Reward - 10);
                TestFalse(TEXT("Second interaction is rejected"), Door->TryInteract(Player));
                TestTrue(TEXT("Boss defeat itself does not end the run"), GM->WaveState != EWaveState::GameOver);
            }
        }
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    }
    return true;
}
#endif
