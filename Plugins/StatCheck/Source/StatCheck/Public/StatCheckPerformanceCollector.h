// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "StatCheckPerformanceTypes.h"

struct STATCHECK_API FStatCheckPerformanceCollectOptions
{
	bool bCollectMemory = false;
};

class STATCHECK_API FStatCheckPerformanceCollector
{
public:
	static FStatCheckPerformanceSnapshot CollectSnapshot();
	static FStatCheckPerformanceSnapshot CollectSnapshot(const FStatCheckPerformanceCollectOptions& Options);
};
