// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "StatCheckPerformanceTypes.h"

class STATCHECK_API FStatCheckPerformanceEvaluator
{
public:
	static constexpr float GoodFpsThreshold = 60.0f;
	static constexpr float CriticalFpsThreshold = 30.0f;
	static constexpr float GoodGameThreadMsThreshold = 16.6f;
	static constexpr float CriticalGameThreadMsThreshold = 33.3f;

	static EStatCheckPerformanceState Evaluate(const FStatCheckPerformanceSnapshot& Snapshot);
	static EStatCheckPerformanceState EvaluateFps(float Fps);
	static EStatCheckPerformanceState EvaluateGameThreadMs(float GameThreadMs);
	static const TCHAR* ToString(EStatCheckPerformanceState State);

private:
	static EStatCheckPerformanceState SelectMoreSevere(
		EStatCheckPerformanceState Left,
		EStatCheckPerformanceState Right);
};
