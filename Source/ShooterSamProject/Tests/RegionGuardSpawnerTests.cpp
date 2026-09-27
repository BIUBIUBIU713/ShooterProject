#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RegionGuardSpawner.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRegionGuardCompositionTest,
    "ShooterSam.RegionGuards.Composition", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRegionGuardCompositionTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Invalid negative total"), ARegionGuardSpawner::CalculateRangedCount(-1), 0);
    TestEqual(TEXT("Empty group"), ARegionGuardSpawner::CalculateRangedCount(0), 0);
    TestEqual(TEXT("One guard is melee"), ARegionGuardSpawner::CalculateRangedCount(1), 0);
    TestEqual(TEXT("Two guards split evenly"), ARegionGuardSpawner::CalculateRangedCount(2), 1);
    TestEqual(TEXT("Five guards include two ranged"), ARegionGuardSpawner::CalculateRangedCount(5), 2);
    TestEqual(TEXT("Ten guards include four ranged"), ARegionGuardSpawner::CalculateRangedCount(10), 4);
    TestEqual(TEXT("Barracks has four ranged and two melee"), ARegionGuardSpawner::CalculateRangedCount(6, 4), 4);
    TestEqual(TEXT("Default six guards still include two ranged"), ARegionGuardSpawner::CalculateRangedCount(6, -1), 2);
    TestEqual(TEXT("All melee"), ARegionGuardSpawner::CalculateRangedCount(6, 0), 0);
    TestEqual(TEXT("All ranged"), ARegionGuardSpawner::CalculateRangedCount(6, 6), 6);
    for (int32 Total = 1; Total <= 100; ++Total)
    {
        const int32 Ranged = ARegionGuardSpawner::CalculateRangedCount(Total);
        TestTrue(TEXT("Ranged slots fit total"), Ranged >= 0 && Ranged <= Total);
        TestTrue(TEXT("Nearest integer to 40 percent"), FMath::Abs(Ranged - Total * 0.4) <= 0.5);
    }
    TestEqual(TEXT("Large counts do not overflow"), ARegionGuardSpawner::CalculateRangedCount(MAX_int32), 858993459);
    return true;
}

#endif
