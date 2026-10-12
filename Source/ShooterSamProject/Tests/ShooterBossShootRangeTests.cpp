#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "ShooterBossEnemy.h"
#include "ShooterSamProjectCharacter.h"
#include "ShooterBTDecoratorBossShootReady.h"
#include "ShooterBTTaskBossShoot.h"
#include "ShooterBTTaskBossSpecialAttack.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShooterBossShootRangeTest,
    "ShooterSam.Boss.Shoot.RangeAndSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShooterBossShootRangeTest::RunTest(const FString& Parameters)
{
    UClass* BossClass = LoadClass<AShooterBossEnemy>(nullptr,
        TEXT("/Game/MyStuff/BluePrints/Enemies/BP_ShooterBossEnemy.BP_ShooterBossEnemy_C"));
    UClass* PlayerClass = LoadClass<AShooterSamProjectCharacter>(nullptr,
        TEXT("/Game/MyStuff/Character/BP_ShooterSamCharacter.BP_ShooterSamCharacter_C"));
    UBehaviorTree* Tree = LoadObject<UBehaviorTree>(nullptr, TEXT("/Game/MyStuff/EnemyBT/BT_ShooterBoss.BT_ShooterBoss"));
    if (!TestNotNull(TEXT("Boss blueprint"), BossClass) || !TestNotNull(TEXT("Player blueprint"), PlayerClass) ||
        !TestNotNull(TEXT("Boss tree"), Tree) || !TestNotNull(TEXT("Tree root"), Tree->RootNode.Get())) return false;
    UBTCompositeNode* Selector = Cast<UBTCompositeNode>(Tree->RootNode->GetChildNode(0));
    if (TestNotNull(TEXT("Attack selector"), Selector) && TestEqual(TEXT("Independent special, shoot and melee branches"), Selector->GetChildrenNum(), 3))
    {
        TestTrue(TEXT("Special retains first priority"), Selector->GetChildNode(0)->IsA<UShooterBTTaskBossSpecialAttack>());
        TestTrue(TEXT("Shoot is before chase, not after MoveTo"), Selector->GetChildNode(1)->IsA<UShooterBTTaskBossShoot>());
        const auto& Decorators = Selector->Children[1].Decorators;
        if (TestEqual(TEXT("Shoot readiness observer exists"), Decorators.Num(), 1))
        {
            TestTrue(TEXT("Boss-only readiness decorator"), Decorators[0]->IsA<UShooterBTDecoratorBossShootReady>());
            TestEqual(TEXT("Only interrupts lower priority branches"), Decorators[0]->GetFlowAbortMode(), EBTFlowAbortMode::LowerPriority);
        }
    }
    const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(true).ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    FURL URL;
    URL.AddOption(TEXT("game=/Script/Engine.GameModeBase"));
    World->SetGameMode(URL);
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    APlayerController* PC = World->SpawnActor<APlayerController>();
    AShooterSamProjectCharacter* Player = World->SpawnActor<AShooterSamProjectCharacter>(PlayerClass, FVector(2000,0,0), FRotator::ZeroRotator, Spawn);
    AShooterBossEnemy* Boss = World->SpawnActor<AShooterBossEnemy>(BossClass, FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
    if (TestNotNull(TEXT("Player spawned"), Player) && TestNotNull(TEXT("Boss spawned"), Boss) && TestNotNull(TEXT("Controller spawned"), PC))
    {
        PC->PlayerState = World->SpawnActor<APlayerState>();
        PC->Possess(Player);
        // This fixture starts directly in combat; encounter tests exercise the entrance.
        Boss->bCombatReady = true;
        Boss->SetCanBeDamaged(true);
        Boss->GetMesh()->InitAnim(true);
        AAIController* AI = Cast<AAIController>(Boss->GetController());
        if (!AI) { Boss->SpawnDefaultController(); AI = Cast<AAIController>(Boss->GetController()); }
        if (TestNotNull(TEXT("Boss controller"), AI))
        {
            AI->RunBehaviorTree(Tree);
            AI->GetBlackboardComponent()->SetValueAsObject(TEXT("TargetActor"), Player);
            TestFalse(TEXT("500cm remains melee"), Boss->IsDistanceInOrdinaryShootRange(500));
            TestTrue(TEXT("Just beyond melee can shoot"), Boss->IsDistanceInOrdinaryShootRange(501));
            TestTrue(TEXT("3000cm included"), Boss->IsDistanceInOrdinaryShootRange(3000));
            TestFalse(TEXT("Beyond maximum rejected"), Boss->IsDistanceInOrdinaryShootRange(3001));
            TestTrue(TEXT("Can shoot at 2000cm before chasing"), Boss->CanStartOrdinaryShoot());
            Boss->bAttackPlaying = true;
            TestFalse(TEXT("Cannot interrupt ongoing melee"), Boss->CanStartOrdinaryShoot());
            Boss->bAttackPlaying = false;
            Boss->NextShootReadyTime = World->GetTimeSeconds() + 5;
            TestFalse(TEXT("Cooldown respected"), Boss->CanStartOrdinaryShoot());
            Boss->NextShootReadyTime = 0;
            TestTrue(TEXT("Ordinary shot actually starts at distance"), Boss->TryStartShoot());
            TestTrue(TEXT("Shooting remains busy"), Boss->bShooting);
            Player->SetActorLocation(FVector(400,0,0));
            Boss->FireShootRound();
            TestTrue(TEXT("Close target stops ordinary firing"), Boss->bShootStopping || !Boss->bShooting);
            TestFalse(TEXT("Close target clears firing timer"), World->GetTimerManager().TimerExists(Boss->ShootFireTimerHandle));
            // Reset only the test fixture, then check the existing special path at the same close distance.
            Boss->ClearCombatForDeathOrEndPlay();
            Boss->GetMesh()->TickAnimation(0.3f, false);
            Boss->NextAttackTime = 0;
            Boss->bSpecialActive = true;
            Boss->bRoaring = false;
            Boss->bSpecialStopping = false;
            Boss->SpecialTarget = Player;
            TestTrue(TEXT("Special sweep still starts within melee range"), Boss->StartShoot(true));
            Boss->ClearCombatForDeathOrEndPlay();
        }
    }
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
#endif
