// Copyright Epic Games, Inc. All Rights Reserved.

#include "StatCheckPerformanceCollector.h"
#include "StatCheckPerformanceEvaluator.h"

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStatCheckPerformanceCollectorTest,
	"StatCheck.Performance.Collector.CollectsSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStatCheckPerformanceCollectorTest::RunTest(const FString& Parameters)
{
	const FStatCheckPerformanceSnapshot Snapshot = FStatCheckPerformanceCollector::CollectSnapshot();

	TestTrue(TEXT("Collected FPS should not be negative."), Snapshot.Fps >= 0.0f);
	TestTrue(TEXT("Collected GameThread time should not be negative."), Snapshot.GameThreadMs >= 0.0f);
	TestTrue(TEXT("Collected DrawCalls should not be negative."), Snapshot.DrawCalls >= 0);
	TestTrue(TEXT("Collected FrameTime should not be negative."), Snapshot.FrameTimeMs >= 0.0f);
	TestTrue(TEXT("Collected average FrameTime should not be negative."), Snapshot.AverageFrameTimeMs >= 0.0f);
	TestTrue(TEXT("Collected minimum FrameTime should not be negative."), Snapshot.MinFrameTimeMs >= 0.0f);
	TestTrue(TEXT("Collected maximum FrameTime should not be negative."), Snapshot.MaxFrameTimeMs >= 0.0f);
	TestEqual(TEXT("Default collection should skip memory usage."), Snapshot.UsedPhysicalMemoryMB, 0ull);

	FStatCheckPerformanceCollectOptions Options;
	Options.bCollectMemory = true;
	const FStatCheckPerformanceSnapshot SnapshotWithMemory =
		FStatCheckPerformanceCollector::CollectSnapshot(Options);
	TestTrue(
		TEXT("Collected memory usage should be available when enabled."),
		SnapshotWithMemory.UsedPhysicalMemoryMB > 0ull);

	const EStatCheckPerformanceState State = FStatCheckPerformanceEvaluator::Evaluate(Snapshot);
	TestTrue(
		TEXT("Collected snapshot should classify into a known state."),
		State == EStatCheckPerformanceState::Good ||
		State == EStatCheckPerformanceState::Warning ||
		State == EStatCheckPerformanceState::Critical);

	return true;
}

#endif
