// Copyright Epic Games, Inc. All Rights Reserved.

#include "StatCheckPerformanceCollector.h"

#include "HAL/PlatformMemory.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/App.h"
#include "RHIStats.h"

namespace StatCheckPerformanceCollector
{
	static uint64 FrameSampleCount = 0;
	static double TotalFrameTimeMs = 0.0;
	static float MinFrameTimeMs = 0.0f;
	static float MaxFrameTimeMs = 0.0f;

	static void AddFrameTimeSample(float FrameTimeMs)
	{
		++FrameSampleCount;
		TotalFrameTimeMs += FrameTimeMs;

		if (FrameSampleCount == 1)
		{
			MinFrameTimeMs = FrameTimeMs;
			MaxFrameTimeMs = FrameTimeMs;
			return;
		}

		MinFrameTimeMs = FMath::Min(MinFrameTimeMs, FrameTimeMs);
		MaxFrameTimeMs = FMath::Max(MaxFrameTimeMs, FrameTimeMs);
	}
}

FStatCheckPerformanceSnapshot FStatCheckPerformanceCollector::CollectSnapshot()
{
	const FStatCheckPerformanceCollectOptions Options;
	return CollectSnapshot(Options);
}

FStatCheckPerformanceSnapshot FStatCheckPerformanceCollector::CollectSnapshot(
	const FStatCheckPerformanceCollectOptions& Options)
{
	const double DeltaTimeSeconds = FMath::Max(FApp::GetDeltaTime(), 0.0);
	const float FrameTimeMs = static_cast<float>(DeltaTimeSeconds * 1000.0);

	StatCheckPerformanceCollector::AddFrameTimeSample(FrameTimeMs);

	FStatCheckPerformanceSnapshot Snapshot;
	Snapshot.Fps = DeltaTimeSeconds > UE_SMALL_NUMBER ? static_cast<float>(1.0 / DeltaTimeSeconds) : 0.0f;
	Snapshot.GameThreadMs = FrameTimeMs;
	Snapshot.DrawCalls = FMath::Max(GNumDrawCallsRHI[0], 0);
	Snapshot.FrameTimeMs = FrameTimeMs;
	Snapshot.AverageFrameTimeMs = StatCheckPerformanceCollector::FrameSampleCount > 0
		? static_cast<float>(
			StatCheckPerformanceCollector::TotalFrameTimeMs /
			static_cast<double>(StatCheckPerformanceCollector::FrameSampleCount))
		: 0.0f;
	Snapshot.MinFrameTimeMs = StatCheckPerformanceCollector::MinFrameTimeMs;
	Snapshot.MaxFrameTimeMs = StatCheckPerformanceCollector::MaxFrameTimeMs;

	if (Options.bCollectMemory)
	{
		const FPlatformMemoryStats MemoryStats = FPlatformMemory::GetStats();
		Snapshot.UsedPhysicalMemoryMB = MemoryStats.UsedPhysical / (1024ull * 1024ull);
	}

	return Snapshot;
}
