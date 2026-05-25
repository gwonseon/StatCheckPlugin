// Copyright Epic Games, Inc. All Rights Reserved.

#include "StatCheckPerformanceEvaluator.h"

EStatCheckPerformanceState FStatCheckPerformanceEvaluator::Evaluate(const FStatCheckPerformanceSnapshot& Snapshot)
{
	const EStatCheckPerformanceState FpsState = EvaluateFps(Snapshot.Fps);
	const EStatCheckPerformanceState GameThreadState = EvaluateGameThreadMs(Snapshot.GameThreadMs);

	return SelectMoreSevere(FpsState, GameThreadState);
}

EStatCheckPerformanceState FStatCheckPerformanceEvaluator::EvaluateFps(float Fps)
{
	if (Fps < CriticalFpsThreshold)
	{
		return EStatCheckPerformanceState::Critical;
	}

	if (Fps < GoodFpsThreshold)
	{
		return EStatCheckPerformanceState::Warning;
	}

	return EStatCheckPerformanceState::Good;
}

EStatCheckPerformanceState FStatCheckPerformanceEvaluator::EvaluateGameThreadMs(float GameThreadMs)
{
	if (GameThreadMs > CriticalGameThreadMsThreshold)
	{
		return EStatCheckPerformanceState::Critical;
	}

	if (GameThreadMs > GoodGameThreadMsThreshold)
	{
		return EStatCheckPerformanceState::Warning;
	}

	return EStatCheckPerformanceState::Good;
}

const TCHAR* FStatCheckPerformanceEvaluator::ToString(EStatCheckPerformanceState State)
{
	switch (State)
	{
	case EStatCheckPerformanceState::Good:
		return TEXT("Good");
	case EStatCheckPerformanceState::Warning:
		return TEXT("Warning");
	case EStatCheckPerformanceState::Critical:
		return TEXT("Critical");
	default:
		return TEXT("Unknown");
	}
}

EStatCheckPerformanceState FStatCheckPerformanceEvaluator::SelectMoreSevere(
	EStatCheckPerformanceState Left,
	EStatCheckPerformanceState Right)
{
	return static_cast<uint8>(Left) >= static_cast<uint8>(Right) ? Left : Right;
}
