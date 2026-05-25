// Copyright Epic Games, Inc. All Rights Reserved.

#include "StatCheckPerformanceEvaluator.h"

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStatCheckPerformanceEvaluatorTest,
	"StatCheck.Performance.Evaluator.ClassifiesSnapshots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStatCheckPerformanceEvaluatorTest::RunTest(const FString& Parameters)
{
	const FStatCheckPerformanceSnapshot GoodSnapshot{ 72.0f, 12.4f, 820 };
	TestEqual(
		TEXT("High FPS and low GameThread should be Good."),
		static_cast<int32>(FStatCheckPerformanceEvaluator::Evaluate(GoodSnapshot)),
		static_cast<int32>(EStatCheckPerformanceState::Good));

	const FStatCheckPerformanceSnapshot WarningByFpsSnapshot{ 45.0f, 12.4f, 820 };
	TestEqual(
		TEXT("Middle FPS should be Warning."),
		static_cast<int32>(FStatCheckPerformanceEvaluator::Evaluate(WarningByFpsSnapshot)),
		static_cast<int32>(EStatCheckPerformanceState::Warning));

	const FStatCheckPerformanceSnapshot WarningByGameThreadSnapshot{ 72.0f, 20.0f, 820 };
	TestEqual(
		TEXT("Middle GameThread time should be Warning."),
		static_cast<int32>(FStatCheckPerformanceEvaluator::Evaluate(WarningByGameThreadSnapshot)),
		static_cast<int32>(EStatCheckPerformanceState::Warning));

	const FStatCheckPerformanceSnapshot CriticalByFpsSnapshot{ 24.0f, 12.4f, 820 };
	TestEqual(
		TEXT("Low FPS should be Critical."),
		static_cast<int32>(FStatCheckPerformanceEvaluator::Evaluate(CriticalByFpsSnapshot)),
		static_cast<int32>(EStatCheckPerformanceState::Critical));

	const FStatCheckPerformanceSnapshot CriticalByGameThreadSnapshot{ 72.0f, 40.0f, 820 };
	TestEqual(
		TEXT("High GameThread time should be Critical."),
		static_cast<int32>(FStatCheckPerformanceEvaluator::Evaluate(CriticalByGameThreadSnapshot)),
		static_cast<int32>(EStatCheckPerformanceState::Critical));

	TestEqual(
		TEXT("State text should be stable for the UI layer."),
		FString(FStatCheckPerformanceEvaluator::ToString(EStatCheckPerformanceState::Warning)),
		FString(TEXT("Warning")));

	return true;
}

#endif
