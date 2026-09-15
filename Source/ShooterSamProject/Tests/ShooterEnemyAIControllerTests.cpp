#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShooterEnemyAIController.h"
#include "ShooterEnemyBase.h"
#include "UObject/UnrealType.h"

#include "ShooterRangedEnemy.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShooterEnemyAIControllerControlledEnemyTypeTest,
	"ShooterSam.Enemy.AIController.ControlledEnemyType",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

bool FShooterEnemyAIControllerControlledEnemyTypeTest::RunTest(
	const FString& Parameters)
{
	const FObjectProperty* ControlledEnemyProperty =
		FindFProperty<FObjectProperty>(
			AShooterEnemyAIController::StaticClass(),
			TEXT("ControlledEnemy")
		);

	if (!TestNotNull(
		TEXT("AI controller exposes a ControlledEnemy object property"),
		ControlledEnemyProperty))
	{
		return false;
	}

	TestTrue(
		TEXT("ControlledEnemy only stores ShooterEnemyBase objects"),
		ControlledEnemyProperty->PropertyClass ==
			AShooterEnemyBase::StaticClass()
	);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShooterEnemyAIControlledCachesEnemyTest,
	"ShooterSam.Enemy.AIControlled.CachesControlledEnemy",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

bool FShooterEnemyAIControlledCachesEnemyTest::RunTest(
	const FString& Parameters)
{
	UWorld* TestWorld = nullptr;
	
	if (GEngine)
	{
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			if (WorldContext.WorldType == EWorldType::Editor)
			{
				TestWorld = WorldContext.World();
				break;
			}
		}
	}
	
	if (!TestNotNull(
		TEXT("An editor world exists for the possession test"),
		TestWorld
	))
	{
		return false;
	}
	
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	AShooterEnemyAIController* TestController = 
		TestWorld->SpawnActor<AShooterEnemyAIController>(
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParameters
		);
	
	AShooterRangedEnemy* TestEnemy = 
		TestWorld->SpawnActor<AShooterRangedEnemy>(
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParameters
		);
	
	if (!TestNotNull(
		TEXT("A test AI controller can be spawned"),
		TestController) || 
		!TestNotNull(
		TEXT("A test ranged enemy can be spawned"),
		TestEnemy
		))
	{
		if (TestEnemy)
		{
			TestEnemy->Destroy();
		}
		
		if (TestController)
		{
			TestController->Destroy();
		}
		
		return false;
	}
	
	TestController->Possess(TestEnemy);
	
	const FObjectProperty* ControlledEnemyProperty = 
		FindFProperty<FObjectProperty>(
			AShooterEnemyAIController::StaticClass(),
			TEXT("ControlledEnemy")
		);
	
	AShooterEnemyBase* CachedEnemy = nullptr;
	
	if (ControlledEnemyProperty)
	{
		CachedEnemy = Cast<AShooterEnemyBase>(
			ControlledEnemyProperty->GetObjectPropertyValue_InContainer(TestController)	
		);
	}
	
	TestController->UnPossess();
	TestEnemy->Destroy();
	TestController->Destroy();
	
	TestTrue(
		TEXT("OnPossess caches the controlled ShooterEnemyBase"),
		CachedEnemy == TestEnemy
	);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShooterEnemyAIControllerClearsEnemyTest,
	"ShooterSam.Enemy.AIController.ClearsControlledEnemyOnUnPossess",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

bool FShooterEnemyAIControllerClearsEnemyTest::RunTest(
	const FString& Parameters)
{
	UWorld* TestWorld = nullptr;
	
	if (GEngine)
	{
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			if (WorldContext.WorldType == EWorldType::Editor)
			{
				TestWorld = WorldContext.World();
				break;
			}
		}
	}
	
	if (!TestNotNull(
		TEXT("An editor world exists for the possession test"),
		TestWorld
	))
	{
		return false;
	}
	
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	AShooterEnemyAIController* TestController = 
		TestWorld->SpawnActor<AShooterEnemyAIController>(
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParameters
		);
	
	AShooterRangedEnemy* TestEnemy = 
		TestWorld->SpawnActor<AShooterRangedEnemy>(
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParameters
		);
	
	if (!TestNotNull(
		TEXT("A test AI controller can be spawned"),
		TestController) || 
		!TestNotNull(
		TEXT("A test ranged enemy can be spawned"),
		TestEnemy
		))
	{
		if (TestEnemy)
		{
			TestEnemy->Destroy();
		}
		
		if (TestController)
		{
			TestController->Destroy();
		}
		
		return false;
	}
	
	TestController->Possess(TestEnemy);
	
	const FObjectProperty* ControlledEnemyProperty = 
		FindFProperty<FObjectProperty>(
			AShooterEnemyAIController::StaticClass(),
			TEXT("ControlledEnemy")
		);
	
	if (!TestNotNull(
		TEXT("ControlledEnemy property exists"),
		ControlledEnemyProperty
	))
	{
		TestController->UnPossess();
		TestEnemy->Destroy();
		TestController->Destroy();
		return false;
	}
	
	//先确认控制器确实缓存过敌人，避免空指针造成假通过
	UObject* CachedBeforeUnPossess = 
		ControlledEnemyProperty->GetObjectPropertyValue_InContainer(
			TestController	
		);
	
	const bool bCachedCorrectEnemy = TestTrue(
		TEXT("Enemy is cached before UnPossess"),
		CachedBeforeUnPossess == TestEnemy
	);
	
	//执行我们要测试的操作
	TestController->UnPossess();
	
	//解除控制后，重新读取属性
	UObject* CachedAfterUnPossess = 
		ControlledEnemyProperty->GetObjectPropertyValue_InContainer(
			TestController	
		);
	
	const bool bCacheWasCleared = TestTrue(
		TEXT("UnPossess clears ControlledEnemy"),
		CachedAfterUnPossess == nullptr
	);
	
	//检查完成后再清理测试对象
	TestEnemy->Destroy();
	TestController->Destroy();
	
	return bCachedCorrectEnemy && bCacheWasCleared;
	
}

#endif