#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShooterBTServiceAttackRange.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShooterAttackDistanceRuleTest,
	"ShooterSam.Enemy.AttackRange.DistanceRule",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

bool FShooterAttackDistanceRuleTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("Distance inside attack range true"),
		UShooterBTServiceAttackRange::IsDistanceInAttackRange(
			500.0f,
			800.0f
		)
	);
	
	TestTrue(
		TEXT("Distance exactly at attack range returns true"),
		UShooterBTServiceAttackRange::IsDistanceInAttackRange(
			800.0f,
			800.0f
		)
	);
	
	TestFalse(
		TEXT("Distance outside attack range returns false"),
		UShooterBTServiceAttackRange::IsDistanceInAttackRange(
			900.0f,
			800.0f
		)
	);
	
	return true;
}

#endif
