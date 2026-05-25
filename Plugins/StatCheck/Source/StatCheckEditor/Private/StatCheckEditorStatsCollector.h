// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FStatCheckSelectedActorCostSnapshot
{
	bool bHasActor = false;
	FString ActorName;
	int32 ComponentCount = 0;
	int32 PrimitiveComponentCount = 0;
	int32 StaticMeshComponentCount = 0;
	int32 MaterialSlotCount = 0;
	int32 ApproxTriangleCount = 0;
	float BoundsRadius = 0.0f;
};

struct FStatCheckWorldCostSnapshot
{
	bool bAvailable = false;
	int32 TotalActorCount = 0;
	int32 HiddenActorCount = 0;
	int32 TickEnabledActorCount = 0;
	int32 PrimitiveComponentCount = 0;
	int32 StaticMeshComponentCount = 0;
	int32 SkeletalMeshComponentCount = 0;
	int32 LightComponentCount = 0;
	int32 MaterialSlotCount = 0;
	int64 ApproxTriangleCount = 0;
};

struct FStatCheckEditorStatsSnapshot
{
	bool bVisibleActorCountAvailable = false;
	int32 VisibleActorCount = 0;
	FStatCheckWorldCostSnapshot World;
	FStatCheckSelectedActorCostSnapshot SelectedActor;
};

struct FStatCheckActorTickReportRow
{
	FString ActorName;
	FString ClassName;
	bool bHidden = false;
	float TickInterval = 0.0f;
	FVector Location = FVector::ZeroVector;
};

struct FStatCheckComponentTypeReportRow
{
	FString TypeName;
	int32 Count = 0;
};

struct FStatCheckSelectedActorComponentReportRow
{
	FString ActorName;
	FString ComponentName;
	FString ClassName;
	bool bRegistered = false;
	bool bActive = false;
	bool bPrimitive = false;
	bool bVisible = false;
	int32 Mobility = 0;
	int32 CollisionEnabled = 0;
	int32 MaterialSlotCount = 0;
	int32 ApproxTriangleCount = 0;
	float BoundsRadius = 0.0f;
};

struct FStatCheckHeavyActorReportRow
{
	FString ActorName;
	FString ClassName;
	bool bHidden = false;
	bool bTickEnabled = false;
	int32 ComponentCount = 0;
	int32 PrimitiveComponentCount = 0;
	int32 StaticMeshComponentCount = 0;
	int32 SkeletalMeshComponentCount = 0;
	int32 LightComponentCount = 0;
	int32 MaterialSlotCount = 0;
	int64 ApproxTriangleCount = 0;
	double CostScore = 0.0;
};

class FStatCheckEditorStatsCollector
{
public:
	static FStatCheckEditorStatsSnapshot CollectSnapshot(
		bool bCollectVisibleActors,
		bool bCollectSelectedActorCost);
	static FStatCheckEditorStatsSnapshot CollectOptimizationSnapshot();
	static TArray<FStatCheckActorTickReportRow> CollectActorTickReport();
	static TArray<FStatCheckComponentTypeReportRow> CollectComponentTypeReport();
	static TArray<FStatCheckSelectedActorComponentReportRow> CollectSelectedActorComponentReport();
	static TArray<FStatCheckHeavyActorReportRow> CollectHeavyActorReport(int32 MaxRows);
};
