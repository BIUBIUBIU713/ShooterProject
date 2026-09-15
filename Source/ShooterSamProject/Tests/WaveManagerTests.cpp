#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "WaveManager.h"

#include "Containers/Set.h"
#include "Math/RandomStream.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveManagerSpawnPointProgressionTest, "ShooterSam.WaveManager.SpawnPointCount.Progression", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWaveManagerSpawnPointProgressionTest::RunTest(const FString& Parameters)
{
	constexpr int32 AvailablePointCount = 5;
	
	TestEqual(TEXT("Wave 1 uses all available spawn points"), AWaveManager::CalculateSpawnPointCountForWave(1, AvailablePointCount), 5);
	
	TestEqual(TEXT("Wave 2 uses two spawn points"), AWaveManager::CalculateSpawnPointCountForWave(2, AvailablePointCount), 2);
	
	TestEqual(TEXT("Wave 3 still uses two spawn points"), AWaveManager::CalculateSpawnPointCountForWave(3, AvailablePointCount), 2);
	
	TestEqual(TEXT("Wave 4 uses three spawn points"), AWaveManager::CalculateSpawnPointCountForWave(4, AvailablePointCount), 3);
	
	TestEqual(TEXT("Wave 5 still uses three spawn points"), AWaveManager::CalculateSpawnPointCountForWave(5, AvailablePointCount),3);
	
	TestEqual(TEXT("Wave 6 uses four spawn points"), AWaveManager::CalculateSpawnPointCountForWave(6, AvailablePointCount), 4);
	
	TestEqual(TEXT("Wave 7 still uses four spawn points"), AWaveManager::CalculateSpawnPointCountForWave(7, AvailablePointCount), 4);
	
	TestEqual(TEXT("Wave 8 uses all available spawn points"), AWaveManager::CalculateSpawnPointCountForWave(8, AvailablePointCount), 5);
	
	TestEqual(TEXT("Later waves continue using all spawn points"), AWaveManager::CalculateSpawnPointCountForWave(20, AvailablePointCount), 5);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWaveManagerSpawnPointBoundaryTest,
	"ShooterSam.WaveManager.SpawnPointCount.Boundaries",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

bool FWaveManagerSpawnPointBoundaryTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Wave zero is invalid"),AWaveManager::CalculateSpawnPointCountForWave(0, 6),0);

	TestEqual(TEXT("Negative wave number is invalid"),AWaveManager::CalculateSpawnPointCountForWave(-1, 6),0);

	TestEqual(TEXT("No available spawn points returns zero"),AWaveManager::CalculateSpawnPointCountForWave(1, 0),0);

	TestEqual(TEXT("Requested count cannot exceed available points"),AWaveManager::CalculateSpawnPointCountForWave(6, 3),3);

	TestEqual(TEXT("Two-point wave clamps to one available point"),AWaveManager::CalculateSpawnPointCountForWave(2, 1),1);

	return true;
}

//出生点选择测试
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveManagerSpawnPointSelectionValidityTest, "ShooterSam.WaveManager.SpawnPointSelection.Validity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWaveManagerSpawnPointSelectionValidityTest::RunTest(const FString& Parameters)
{
	constexpr int32 RequestedCount = 3;
	constexpr int32 AvailableCount = 5;
	
	FRandomStream RandomStream(12345);
	
	const TArray<int32> SelectedIndices = AWaveManager::SelectUniqueSpawnPointIndices(RequestedCount, AvailableCount, RandomStream);
	
	TestEqual(TEXT("Returns requested number of indices"), SelectedIndices.Num(), RequestedCount);
	
	TSet<int32> UniqueIndices;
	
	for (int32 Index : SelectedIndices)
	{
		TestTrue(TEXT("Every selected index is in valid range"), Index >= 0 && Index < AvailableCount);
		UniqueIndices.Add(Index);
	}
	
	TestEqual(TEXT("Selected indices contain no duplicates"), UniqueIndices.Num(), SelectedIndices.Num());
	
	return true;
}

//边界测试
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveManagerSpawnPointSelectionBoundaryTest, "ShooterSam.WaveManager.SpawnPointSelection.Boundaries", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWaveManagerSpawnPointSelectionBoundaryTest::RunTest(const FString& Parameters)
{
	FRandomStream RandomStream(777);
	
	const TArray<int32> ZeroRequested = AWaveManager::SelectUniqueSpawnPointIndices(0, 5, RandomStream);
	
	TestTrue(TEXT("Zero requested count returns empty array"), ZeroRequested.IsEmpty());
	
	const TArray<int32> NegativeRequested = AWaveManager::SelectUniqueSpawnPointIndices(-1, 5, RandomStream);
	
	TestTrue(TEXT("Negative requested count returns empty array"), NegativeRequested.IsEmpty());
	
	const TArray<int32> NoAvailablePoints = AWaveManager::SelectUniqueSpawnPointIndices(3, 0, RandomStream);
	
	TestTrue(TEXT("Zero available points returns empty array"), NoAvailablePoints.IsEmpty());
	
	const TArray<int32> ClampedSelection = AWaveManager::SelectUniqueSpawnPointIndices(8, 5, RandomStream);
	
	TestEqual(TEXT("Requested count is clamped to available count"), ClampedSelection.Num(), 5);
	
	return true;
}

//固定种子测试
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveManagerSpawnPointSelectionDeterminismTest, "ShooterSam.WaveManager.SpawnPointSelection.DeterministicSeed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWaveManagerSpawnPointSelectionDeterminismTest::RunTest(const FString& Parameters)
{
	FRandomStream FirstRandomStream(13579);
	FRandomStream SecondRandomStream(13579);
	
	const TArray<int32> FirstSelection = AWaveManager::SelectUniqueSpawnPointIndices(3, 5, FirstRandomStream);
	
	const TArray<int32> SecondSelection = AWaveManager::SelectUniqueSpawnPointIndices(3, 5, SecondRandomStream);
	
	TestTrue(TEXT("Same seed produces the same selection"), FirstSelection == SecondSelection);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWaveManagerEnemyCountProgressionTest,
	"ShooterSam.WaveManager.EnemyCount.Progression",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

bool FWaveManagerEnemyCountProgressionTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Wave 1 starts with eight enemies"),
		AWaveManager::CalculateEnemyCountForWave(1),
		8
	);
	
	TestEqual(
		TEXT("Wave 2 adds four enemies"),
		AWaveManager::CalculateEnemyCountForWave(2),
		12
	);
	
	TestEqual(
		TEXT("Wave 5 continues early-wave growth"),
		AWaveManager::CalculateEnemyCountForWave(5),
		24
	);
	
	TestEqual(
		TEXT("Wave 14 is the final four-enemy growth wave"),
		AWaveManager::CalculateEnemyCountForWave(14),
		60
	);
	
	TestEqual(
		TEXT("Wave 15 switches to two-enemy growth"),
		AWaveManager::CalculateEnemyCountForWave(15),
		62
	);
	
	TestEqual(
		TEXT("Wave 16 continues late-wave growth"),
		AWaveManager::CalculateEnemyCountForWave(16),
		64
	);
	
	TestEqual(
		TEXT("Wave 20 has seventy-two enemies"),
		AWaveManager::CalculateEnemyCountForWave(20),
		72
	);
	
	return true;
}

//添加非法波次测试
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWaveManagerEnemyCountBoundaryTest,
	"ShooterSam.WaveManager.EnemyCount.Boundaries",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

bool FWaveManagerEnemyCountBoundaryTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Wave zero has no enemies"),
		AWaveManager::CalculateEnemyCountForWave(0),
		0
	);
	
	TestEqual(
		TEXT("Negative wave number has no enemies"),
		AWaveManager::CalculateEnemyCountForWave(-1),
		0
	);
	
	return true;
}

//暂停生成测试
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWaveManagerSpawnFlowPauseTest,
	"ShooterSam.WaveManager.SpawnFlow.Pause",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

bool FWaveManagerSpawnFlowPauseTest::RunTest(const FString& Parameters)
{
	TestFalse(
		TEXT("Spawning continues below the alive limit"),
		AWaveManager::ShouldPauseEnemySpawning(8, 62, 8, 12)
	);
	
	TestTrue(
		TEXT("Spawning pauses when alive count reaches the limit"),
		AWaveManager::ShouldPauseEnemySpawning(12, 62, 12, 12)
	);
	
	TestTrue(
		TEXT("Spawning pauses when the wave target is reached"),
		AWaveManager::ShouldPauseEnemySpawning(62, 62, 8, 12)
	);
	
	TestTrue(
		TEXT("Spawning remains paused if target was exceeded"),
		AWaveManager::ShouldPauseEnemySpawning(63, 62, 8, 12)
	);
	
	return true;
}

//恢复生成测试
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWaveManagerSpawnFlowResumeTest,
	"ShooterSam.WaveManager.SpawnFlow.Resume",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

bool FWaveManagerSpawnFlowResumeTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("Spawning resumes when alive count reaches eight"),
		AWaveManager::ShouldResumeEnemySpawning(12, 62, 8, 8)
	);
	
	TestFalse(
		TEXT("Spawning stays paused above the resume threshold"),
		AWaveManager::ShouldResumeEnemySpawning(12, 62, 9, 8)
	);
	
	TestFalse(
		TEXT("Spawning cannot resume after wave target is reached"),
		AWaveManager::ShouldResumeEnemySpawning(62, 62, 0, 8)
	);
	
	return true;
}

//波次完成预测
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWaveManagerSpawnFlowCompletionTest,
	"ShooterSam.WaveManager.SpawnFlow.Completion",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::EngineFilter
)

bool FWaveManagerSpawnFlowCompletionTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("Wave completes after target spawned and no enemies remain"),
		AWaveManager::IsEnemyWaveComplete(62, 62, 0)
	);
	
	TestFalse(
		TEXT("Wave does not complete before all enemies are spawned"),
		AWaveManager::IsEnemyWaveComplete(60, 62, 0)
	);
	
	TestFalse(
		TEXT("Wave does not complete while an enemy is alive"),
		AWaveManager::IsEnemyWaveComplete(62, 62, 1)
	);
	
	TestTrue(
		TEXT("Wave can recover if spawned count accidentally exceeded target"),
		AWaveManager::IsEnemyWaveComplete(63, 62, 0)
	);
	
	
	return true;
}



#endif
