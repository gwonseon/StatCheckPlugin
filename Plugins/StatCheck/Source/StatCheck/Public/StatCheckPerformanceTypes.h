// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

enum class EStatCheckPerformanceState : uint8
{
	Good,
	Warning,
	Critical
};

struct STATCHECK_API FStatCheckPerformanceSnapshot
{
	float Fps = 0.0f;
	float GameThreadMs = 0.0f;
	int32 DrawCalls = 0;
	float FrameTimeMs = 0.0f;
	float AverageFrameTimeMs = 0.0f;
	float MinFrameTimeMs = 0.0f;
	float MaxFrameTimeMs = 0.0f;
	uint64 UsedPhysicalMemoryMB = 0;
};
