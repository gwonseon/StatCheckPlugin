// Copyright Epic Games, Inc. All Rights Reserved.

#include "StatCheckEditorStatsCollector.h"

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStatCheckEditorStatsCollectorTest,
	"StatCheck.Editor.Stats.Collector.IsSafe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStatCheckEditorStatsCollectorTest::RunTest(const FString& Parameters)
{
	const FStatCheckEditorStatsSnapshot DisabledSnapshot =
		FStatCheckEditorStatsCollector::CollectSnapshot(false, false);

	TestFalse(
		TEXT("Visible actor count should be unavailable when collection is disabled."),
		DisabledSnapshot.bVisibleActorCountAvailable);

	const FStatCheckEditorStatsSnapshot EnabledSnapshot =
		FStatCheckEditorStatsCollector::CollectSnapshot(true, true);

	TestTrue(TEXT("Visible actor count should not be negative."), EnabledSnapshot.VisibleActorCount >= 0);
	TestTrue(
		TEXT("Selected actor component count should not be negative."),
		EnabledSnapshot.SelectedActor.ComponentCount >= 0);

	const FStatCheckEditorStatsSnapshot OptimizationSnapshot =
		FStatCheckEditorStatsCollector::CollectOptimizationSnapshot();

	TestTrue(
		TEXT("Optimization snapshot visible actor count should be available."),
		OptimizationSnapshot.bVisibleActorCountAvailable);
	TestTrue(
		TEXT("Optimization snapshot total actor count should not be negative."),
		OptimizationSnapshot.World.TotalActorCount >= 0);
	TestTrue(
		TEXT("Optimization snapshot primitive component count should not be negative."),
		OptimizationSnapshot.World.PrimitiveComponentCount >= 0);

	const TArray<FStatCheckActorTickReportRow> ActorTickRows =
		FStatCheckEditorStatsCollector::CollectActorTickReport();
	const TArray<FStatCheckComponentTypeReportRow> ComponentRows =
		FStatCheckEditorStatsCollector::CollectComponentTypeReport();
	const TArray<FStatCheckSelectedActorComponentReportRow> SelectedActorRows =
		FStatCheckEditorStatsCollector::CollectSelectedActorComponentReport();
	const TArray<FStatCheckHeavyActorReportRow> HeavyActorRows =
		FStatCheckEditorStatsCollector::CollectHeavyActorReport(50);

	TestTrue(TEXT("Actor tick report row count should not be negative."), ActorTickRows.Num() >= 0);
	TestTrue(TEXT("Component report row count should not be negative."), ComponentRows.Num() >= 0);
	TestTrue(TEXT("Selected actor report row count should not be negative."), SelectedActorRows.Num() >= 0);
	TestTrue(TEXT("Heavy actor report row count should not be negative."), HeavyActorRows.Num() >= 0);

	return true;
}

#endif
