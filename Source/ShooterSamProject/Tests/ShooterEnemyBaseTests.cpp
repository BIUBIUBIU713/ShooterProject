#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShooterEnemyBase.h"
#include "UObject/UnrealType.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShooterEnemyBaseIsAbstractTest,
	"ShooterSam.Enemy.Base.IsAbstract",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

bool FShooterEnemyBaseIsAbstractTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("ShooterEnemyBase should be an abstract enemy template"),
		AShooterEnemyBase::StaticClass()->HasAnyClassFlags(CLASS_Abstract)
	);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShooterEnemyBaseDefaultMaxHealthTest,
	"ShooterSam.Enemy.Base.DefaultMaxHealth",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

bool FShooterEnemyBaseDefaultMaxHealthTest::RunTest(const FString& Parameters)
{
	const UClass* EnemyClass = AShooterEnemyBase::StaticClass();
	
	const FFloatProperty* MaxHealthProperty = FindFProperty<FFloatProperty>(EnemyClass, TEXT("MaxHealth"));
	
	if (!TestNotNull(TEXT("Enemy base exposes MaxHealth to the UE reflection system"), MaxHealthProperty))
	{
		return false;
	}
	
	const AShooterEnemyBase* DefaultEnemy = GetDefault<AShooterEnemyBase>();
	
	const float DefaultMaxHealth = MaxHealthProperty->GetPropertyValue_InContainer(DefaultEnemy);
	
	TestEqual(
		TEXT("Enemy base starts with 100 maximum health"),
		DefaultMaxHealth,
		100.0f
	);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShooterEnemyBaseDefaultCurrentHealthTest,
	"ShooterSam.Enemy.Base.DefaultCurrentHealth",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

bool FShooterEnemyBaseDefaultCurrentHealthTest::RunTest(const FString& Parameters)
{
	const UClass* EnemyClass = AShooterEnemyBase::StaticClass();
	
	const FFloatProperty* CurrentHealthProperty = FindFProperty<FFloatProperty>(EnemyClass,TEXT("CurrentHealth"));
	
	if (!TestNotNull(
		TEXT("Enemy base exposes CurrentHealth to the UE reflection system"),
		CurrentHealthProperty
	))
	{
		return false;
	}
	
	const FFloatProperty* MaxHealthProperty = FindFProperty<FFloatProperty>(EnemyClass, TEXT("MaxHealth"));
	
	if (!TestNotNull(
		TEXT("MaxHealth is available for comparison"),
		MaxHealthProperty
	))
	{
		return false;
	}
	
	const AShooterEnemyBase* DefaultEnemy = GetDefault<AShooterEnemyBase>();
	
	const float DefaultCurrentHealth = CurrentHealthProperty->GetPropertyValue_InContainer(DefaultEnemy);
	
	const float DefaultMaxHealth = MaxHealthProperty->GetPropertyValue_InContainer(DefaultEnemy);
	
	TestEqual(
		TEXT("Current health starts to maximum health"),
		DefaultCurrentHealth,
		DefaultMaxHealth
	);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShooterEnemyBaseGetCurrentHealthTest,
	"ShooterSam.Enemy.Base.GetCurrentHealth",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

bool FShooterEnemyBaseGetCurrentHealthTest::RunTest(
	const FString& Parameters)
{
	UFunction* GetCurrentHealthFunction =
		AShooterEnemyBase::StaticClass()->FindFunctionByName(
			TEXT("GetCurrentHealth")
		);

	if (!TestNotNull(
		TEXT("Enemy base exposes GetCurrentHealth as a UFUNCTION"),
		GetCurrentHealthFunction))
	{
		return false;
	}

	struct FGetCurrentHealthParameters
	{
		float ReturnValue = 0.0f;
	};

	FGetCurrentHealthParameters FunctionParameters;

	AShooterEnemyBase* DefaultEnemy =
		GetMutableDefault<AShooterEnemyBase>();

	DefaultEnemy->ProcessEvent(
		GetCurrentHealthFunction,
		&FunctionParameters
	);

	TestEqual(
		TEXT("GetCurrentHealth returns the current health value"),
		FunctionParameters.ReturnValue,
		100.0f
	);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShooterEnemyBaseGetMaxHealthTest,
	"ShooterSam.Enemy.Base.GetMaxHealth",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

bool FShooterEnemyBaseGetMaxHealthTest::RunTest(const FString& Parameters)
{
	UFunction* GetMaxHealthFunction = AShooterEnemyBase::StaticClass()->FindFunctionByName(TEXT("GetMaxHealth"));
	
	if (!TestNotNull(
		TEXT("Enemy base exposes GetMaxHealth as a UFUNCTION"),
		GetMaxHealthFunction
	))
	{
		return false;
	}
	
	struct FGetMaxHealthParameters
	{
		float ReturnValue = 0.0f;
	};
	
	FGetMaxHealthParameters FunctionParameters;
	
	AShooterEnemyBase* DefaultEnemy = GetMutableDefault<AShooterEnemyBase>();
	
	DefaultEnemy->ProcessEvent(
		GetMaxHealthFunction,
		&FunctionParameters
	);
	
	TestEqual(
		TEXT("GetMaxHealth returns the maximum health value"),
		FunctionParameters.ReturnValue,
			100.0f
		);
		
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
		FShooterEnemyBaseDefaultAliveStateTest,
		"ShooterSam.Enemy.Base.Death.DefaultAlive",
		EAutomationTestFlags::EditorContext |
		EAutomationTestFlags::EngineFilter
)

bool FShooterEnemyBaseDefaultAliveStateTest::RunTest(const FString& Parameters)
{
	const FBoolProperty* IsDeadProperty = FindFProperty<FBoolProperty>(
		AShooterEnemyBase::StaticClass(),
		TEXT("bIsDead")
	);
	
	if (!TestNotNull(
		TEXT("Enemy base exposes bIsDead to the UE reflection system"),
		IsDeadProperty
	))
	{
		return false;
	}
	
	const AShooterEnemyBase* DefaultEnemy = GetDefault<AShooterEnemyBase>();
	
	const bool bDefaultIsDead = IsDeadProperty->GetPropertyValue_InContainer(DefaultEnemy);
	
	TestFalse(
		TEXT("Enemy base starts alive"),
		bDefaultIsDead
	);
	
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShooterEnemyBaseIsDeadGetterTest,
	"ShooterSam.Enemy.Base.Death.IsDeadGetter",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

bool FShooterEnemyBaseIsDeadGetterTest::RunTest(const FString& Parameters)
{
	UFunction* IsDeadFunction = 
		AShooterEnemyBase::StaticClass()->FindFunctionByName(
			TEXT("IsDead")	
		);
	
	if (!TestNotNull(
		TEXT("Enemy base exposes IsDead as a UFUNCTION"),
		IsDeadFunction
	))
	{
		return false;
	}
	
	struct FIsDeadParameters
	{
		bool ReturnValue = false;
	};
	
	FIsDeadParameters FunctionParameters;
	
	AShooterEnemyBase* DefaultEnemy = GetMutableDefault<AShooterEnemyBase>();
	
	DefaultEnemy->ProcessEvent(
		IsDeadFunction,
		&FunctionParameters
	);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShooterEnemyBaseDeathDelegateExistsTest,
	"ShooterSam.Enemy.Base.Death.DelegateExists",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

bool FShooterEnemyBaseDeathDelegateExistsTest::RunTest(const FString& Parameters)
{
	const FMulticastDelegateProperty* DeathDelegateProperty =
		FindFProperty<FMulticastDelegateProperty>(
			AShooterEnemyBase::StaticClass(),
			TEXT("OnEnemyDied")
		);
	
	TestNotNull(
		TEXT("Enemy base exposes the OnEnemyDied multicast delegate"),
		DeathDelegateProperty
	);
	
	return true;
}

#endif
