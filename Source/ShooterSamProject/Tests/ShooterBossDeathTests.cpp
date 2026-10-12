#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShooterBossEnemy.h"
#include "ShooterSamProjectCharacter.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShooterBossDeathTest,
    "ShooterSam.Boss.Death.InterruptionAndPriority",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShooterBossDeathTest::RunTest(const FString& Parameters)
{
    UClass* BossClass = LoadClass<AShooterBossEnemy>(nullptr,
        TEXT("/Game/MyStuff/BluePrints/Enemies/BP_ShooterBossEnemy.BP_ShooterBossEnemy_C"));
    if (!TestNotNull(TEXT("Existing Boss blueprint loads"), BossClass)) return false;
    UClass* PlayerClass = LoadClass<AShooterSamProjectCharacter>(nullptr,
        TEXT("/Game/MyStuff/Character/BP_ShooterSamCharacter.BP_ShooterSamCharacter_C"));
    if (!TestNotNull(TEXT("Existing player blueprint loads"), PlayerClass)) return false;
    const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(true).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    FURL URL;
    URL.AddOption(TEXT("game=/Script/Engine.GameModeBase"));
    // APawn rejects all damage without an authoritative game mode.
    TestTrue(TEXT("Test world has authoritative game mode"), World->SetGameMode(URL));
    APlayerController* PlayerController = World->SpawnActor<APlayerController>();
    FActorSpawnParameters PlayerSpawn;
    PlayerSpawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AShooterSamProjectCharacter* Player = World->SpawnActor<AShooterSamProjectCharacter>(PlayerClass,
        FVector(200, 0, 0), FRotator::ZeroRotator, PlayerSpawn);
    if (!TestNotNull(TEXT("Player spawned"), Player) || !TestNotNull(TEXT("Player controller spawned"), PlayerController))
    {
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
        return false;
    }
    // UE 5.7's IsPlayerControlled checks PlayerState, not just controller type.
    PlayerController->PlayerState = World->SpawnActor<APlayerState>();
    PlayerController->Possess(Player);
    TestTrue(TEXT("Test killer is recognized as a player"), Player->IsPlayerControlled());
    const FDamageEvent DamageEvent;

    // Use the real blueprint, skeleton and montages, without starting its entrance event.
    for (int32 Phase = 0; Phase < 6; ++Phase)
    {
        FActorSpawnParameters Spawn;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AShooterBossEnemy* Boss = World->SpawnActor<AShooterBossEnemy>(BossClass, FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
        if (!TestNotNull(TEXT("Boss spawned"), Boss)) continue;
        Boss->CurrentHealth = Boss->MaxHealth;
        // This fixture starts directly in combat; encounter tests exercise the entrance.
        Boss->bCombatReady = true;
        Boss->SetCanBeDamaged(true);
        Boss->GetMesh()->InitAnim(true);
        UAnimInstance* Anim = Boss->GetMesh()->GetAnimInstance();
        if (!TestNotNull(TEXT("Boss animation instance exists"), Anim)) continue;
        if (!TestNotNull(TEXT("Death montage inherited by blueprint"), Boss->DeathMontage.Get())) continue;
        AAIController* AI = Cast<AAIController>(Boss->GetController());
        if (!AI) { Boss->SpawnDefaultController(); AI = Cast<AAIController>(Boss->GetController()); }
        if (!TestNotNull(TEXT("Boss AI controller exists"), AI)) continue;
        TestTrue(TEXT("Real Boss behavior tree starts"), AI->RunBehaviorTree(LoadObject<UBehaviorTree>(nullptr,
            TEXT("/Game/MyStuff/EnemyBT/BT_ShooterBoss.BT_ShooterBoss"))));
        UBehaviorTreeComponent* Tree = Cast<UBehaviorTreeComponent>(AI->GetBrainComponent());
        if (!TestNotNull(TEXT("Actual behavior tree component exists"), Tree)) continue;
        TestTrue(TEXT("Behavior tree running before death"), Tree->IsRunning());
        Boss->InterruptedBehaviorTree = Tree;
        UAnimMontage* OldMontage = Phase == 0 ? Boss->RoarMontage.Get()
            : Phase == 1 ? Boss->LightAttackMontage.Get() : Boss->ShootMontage.Get();
        if (Phase < 4)
        {
            TestTrue(TEXT("Previous attack montage plays"), Anim->Montage_Play(OldMontage) > 0.0f);
            Boss->bSpecialActive = Phase == 0 || Phase == 3;
            Boss->bRoaring = Phase == 0;
            Boss->bAttackPlaying = Phase == 1;
            Boss->bShooting = Phase == 2 || Phase == 3;
            Boss->ActiveRoarMontage = Phase == 0 ? OldMontage : nullptr;
            Boss->ActiveAttackMontage = Phase == 1 ? OldMontage : nullptr;
            Boss->ActiveShootMontage = Phase >= 2 ? OldMontage : nullptr;
            Boss->bFirstSpecialPending = true;
            World->GetTimerManager().SetTimer(Boss->ShootFireTimerHandle, Boss, &AShooterBossEnemy::FireShootRound, 0.1f, true);
            World->GetTimerManager().SetTimer(Boss->ShootDurationTimerHandle, Boss, &AShooterBossEnemy::StopShoot, 10.0f, false);
        }
        if (Phase == 5)
        {
            Boss->TakeDamage(Boss->MaxHealth * 0.55f, DamageEvent, PlayerController, Player);
            TestTrue(TEXT("Surviving half-health hit still immediately starts roar"), Boss->bRoaring);
            TestTrue(TEXT("Permanent enrage still applied"), Boss->bEnraged);
            TestFalse(TEXT("First special interrupts the actual tree"), Tree->IsRunning());
        }
        const int32 CoinsBefore = Player->GetCoins();
        Boss->TakeDamage(Boss->MaxHealth + 1.0f, DamageEvent, PlayerController, Player);
        TestTrue(TEXT("Lethal damage marks Boss dead"), Boss->IsDead());
        TestEqual(TEXT("Only surviving half-health damage triggers special"), Boss->bHalfHealthTriggered, Phase == 5);
        TestFalse(TEXT("Pending half-health sequence cleared"), Boss->bFirstSpecialPending);
        TestFalse(TEXT("All attack states revoked"), Boss->bShooting || Boss->bRoaring || Boss->bSpecialActive || Boss->bAttackPlaying);
        TestFalse(TEXT("Firing timer removed"), World->GetTimerManager().TimerExists(Boss->ShootFireTimerHandle));
        TestFalse(TEXT("Duration timer removed"), World->GetTimerManager().TimerExists(Boss->ShootDurationTimerHandle));
        TestFalse(TEXT("Interrupted tree cannot resume"), Boss->InterruptedBehaviorTree.IsValid());
        TestFalse(TEXT("Behavior tree stopped"), Tree->IsRunning());
        TestFalse(TEXT("Collision disabled"), Boss->GetActorEnableCollision());
        TestTrue(TEXT("Controller detached"), Boss->GetController() == nullptr);
        TestEqual(TEXT("Movement disabled"), Boss->GetCharacterMovement()->MovementMode.GetValue(), MOVE_None);
        TestEqual(TEXT("Reward granted once"), Player->GetCoins(), CoinsBefore + Boss->KillRewardCoins);
        FAnimMontageInstance* Death = Anim->GetActiveInstanceForMontage(Boss->DeathMontage);
        if (TestNotNull(TEXT("Death montage immediately replaces attack"), Death))
        {
            TestFalse(TEXT("Death holds final pose"), Death->bEnableAutoBlendOut);
            const int32 DeathId = Death->GetInstanceID();
            Boss->HandleRoarEnd(OldMontage, false);
            Boss->HandleShootEnd(OldMontage, false);
            Boss->HandleAttackEnd(OldMontage, false);
            Boss->FireShootRound();
            Boss->Tick(0.1f);
            TestEqual(TEXT("Repeated damage rejected"), Boss->TakeDamage(100, DamageEvent, PlayerController, Player), 0.0f);
            TestEqual(TEXT("No duplicate reward"), Player->GetCoins(), CoinsBefore + Boss->KillRewardCoins);
            FAnimMontageInstance* CurrentDeath = Anim->GetActiveInstanceForMontage(Boss->DeathMontage);
            if (TestNotNull(TEXT("Death instance survives old callbacks"), CurrentDeath))
                TestEqual(TEXT("Old callbacks cannot replace death"), CurrentDeath->GetInstanceID(), DeathId);
            Boss->GetMesh()->TickAnimation(Boss->DeathMontage->GetPlayLength() + 0.1f, false);
            CurrentDeath = Anim->GetActiveInstanceForMontage(Boss->DeathMontage);
            if (TestNotNull(TEXT("Death instance holds beyond clip length"), CurrentDeath))
                TestTrue(TEXT("Death reaches final frame"), FMath::IsNearlyEqual(CurrentDeath->GetPosition(), Boss->DeathMontage->GetPlayLength(), 0.001f));
        }
        TestTrue(TEXT("Lifetime covers animation plus hold"), Boss->GetLifeSpan() >= Boss->DeathMontage->GetPlayLength() + 0.49f);
        TestFalse(TEXT("Dead Boss cannot melee"), Boss->TryStartLightAttack());
        TestFalse(TEXT("Dead Boss cannot shoot"), Boss->TryStartShoot());
        TestFalse(TEXT("Dead Boss cannot start special"), Boss->TryStartSpecialAttack());
        Boss->Destroy();
    }
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
#endif
