// Copyright Epic Games, Inc. All Rights Reserved.

#include "StatCheckEditorStatsCollector.h"

#include "Algo/Sort.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "LevelEditorViewport.h"
#include "SceneView.h"

namespace StatCheckEditorStatsCollector
{
	static UWorld* GetEditorWorld()
	{
		if (GCurrentLevelEditingViewportClient != nullptr && GCurrentLevelEditingViewportClient->GetWorld() != nullptr)
		{
			return GCurrentLevelEditingViewportClient->GetWorld();
		}

		return GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
	}

	static int32 CountMaterialSlots(const UStaticMeshComponent& StaticMeshComponent)
	{
		int32 MaterialSlotCount = 0;
		if (const UStaticMesh* StaticMesh = StaticMeshComponent.GetStaticMesh())
		{
			MaterialSlotCount = StaticMesh->GetStaticMaterials().Num();
		}

		return FMath::Max(MaterialSlotCount, StaticMeshComponent.GetNumMaterials());
	}

	static int32 CountApproxTriangles(const UStaticMeshComponent& StaticMeshComponent)
	{
		const UStaticMesh* StaticMesh = StaticMeshComponent.GetStaticMesh();
		if (StaticMesh == nullptr)
		{
			return 0;
		}

		return StaticMesh->GetNumTriangles(0);
	}

	static FStatCheckHeavyActorReportRow MakeHeavyActorRow(AActor& Actor)
	{
		FStatCheckHeavyActorReportRow Row;
		Row.ActorName = Actor.GetActorLabel();
		Row.ClassName = Actor.GetClass() != nullptr ? Actor.GetClass()->GetName() : FString();
		Row.bHidden = Actor.IsHiddenEd() || Actor.IsTemporarilyHiddenInEditor();
		Row.bTickEnabled = Actor.IsActorTickEnabled();

		TArray<UActorComponent*> Components;
		Actor.GetComponents(Components);
		Row.ComponentCount = Components.Num();

		for (UActorComponent* Component : Components)
		{
			if (Component == nullptr)
			{
				continue;
			}

			if (Cast<UPrimitiveComponent>(Component) != nullptr)
			{
				++Row.PrimitiveComponentCount;
			}

			if (const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Component))
			{
				++Row.StaticMeshComponentCount;
				Row.MaterialSlotCount += CountMaterialSlots(*StaticMeshComponent);
				Row.ApproxTriangleCount += CountApproxTriangles(*StaticMeshComponent);
			}

			if (Cast<USkeletalMeshComponent>(Component) != nullptr)
			{
				++Row.SkeletalMeshComponentCount;
			}

			if (Cast<ULightComponent>(Component) != nullptr)
			{
				++Row.LightComponentCount;
			}
		}

		Row.CostScore =
			static_cast<double>(Row.ComponentCount) +
			static_cast<double>(Row.PrimitiveComponentCount * 2) +
			static_cast<double>(Row.StaticMeshComponentCount * 5) +
			static_cast<double>(Row.SkeletalMeshComponentCount * 8) +
			static_cast<double>(Row.LightComponentCount * 6) +
			static_cast<double>(Row.MaterialSlotCount) +
			static_cast<double>(Row.ApproxTriangleCount) / 1000.0 +
			(Row.bTickEnabled ? 10.0 : 0.0);

		return Row;
	}

	static FStatCheckSelectedActorCostSnapshot CollectSelectedActorCost()
	{
		FStatCheckSelectedActorCostSnapshot Snapshot;

		if (GEditor == nullptr)
		{
			return Snapshot;
		}

		AActor* SelectedActor = nullptr;
		for (FSelectionIterator It = GEditor->GetSelectedActorIterator(); It; ++It)
		{
			SelectedActor = Cast<AActor>(*It);
			if (SelectedActor != nullptr)
			{
				break;
			}
		}

		if (SelectedActor == nullptr)
		{
			return Snapshot;
		}

		TArray<UActorComponent*> Components;
		SelectedActor->GetComponents(Components);

		Snapshot.bHasActor = true;
		Snapshot.ActorName = SelectedActor->GetActorLabel();
		Snapshot.ComponentCount = Components.Num();

		for (UActorComponent* Component : Components)
		{
			if (const UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
			{
				++Snapshot.PrimitiveComponentCount;
				Snapshot.BoundsRadius = FMath::Max(Snapshot.BoundsRadius, PrimitiveComponent->Bounds.SphereRadius);
			}

			if (const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Component))
			{
				++Snapshot.StaticMeshComponentCount;
				Snapshot.MaterialSlotCount += CountMaterialSlots(*StaticMeshComponent);
				Snapshot.ApproxTriangleCount += CountApproxTriangles(*StaticMeshComponent);
			}
		}

		return Snapshot;
	}

	static FStatCheckWorldCostSnapshot CollectWorldCost()
	{
		FStatCheckWorldCostSnapshot Snapshot;

		UWorld* World = GetEditorWorld();
		if (World == nullptr)
		{
			return Snapshot;
		}

		Snapshot.bAvailable = true;

		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor == nullptr)
			{
				continue;
			}

			++Snapshot.TotalActorCount;

			if (Actor->IsHiddenEd() || Actor->IsTemporarilyHiddenInEditor())
			{
				++Snapshot.HiddenActorCount;
			}

			if (Actor->IsActorTickEnabled())
			{
				++Snapshot.TickEnabledActorCount;
			}

			TArray<UActorComponent*> Components;
			Actor->GetComponents(Components);

			for (UActorComponent* Component : Components)
			{
				if (Component == nullptr)
				{
					continue;
				}

				if (Cast<UPrimitiveComponent>(Component) != nullptr)
				{
					++Snapshot.PrimitiveComponentCount;
				}

				if (const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Component))
				{
					++Snapshot.StaticMeshComponentCount;
					Snapshot.MaterialSlotCount += CountMaterialSlots(*StaticMeshComponent);
					Snapshot.ApproxTriangleCount += CountApproxTriangles(*StaticMeshComponent);
				}

				if (Cast<USkeletalMeshComponent>(Component) != nullptr)
				{
					++Snapshot.SkeletalMeshComponentCount;
				}

				if (Cast<ULightComponent>(Component) != nullptr)
				{
					++Snapshot.LightComponentCount;
				}
			}
		}

		return Snapshot;
	}

	static int32 CountVisibleActorsInFocusedViewport()
	{
		if (GCurrentLevelEditingViewportClient == nullptr || GCurrentLevelEditingViewportClient->Viewport == nullptr)
		{
			return 0;
		}

		UWorld* World = GetEditorWorld();
		if (World == nullptr || World->Scene == nullptr)
		{
			return 0;
		}

		FSceneViewFamilyContext ViewFamily(
			FSceneViewFamily::ConstructionValues(
				GCurrentLevelEditingViewportClient->Viewport,
				World->Scene,
				GCurrentLevelEditingViewportClient->EngineShowFlags)
			.SetRealtimeUpdate(false));

		FSceneView* SceneView = GCurrentLevelEditingViewportClient->CalcSceneView(&ViewFamily);
		if (SceneView == nullptr)
		{
			return 0;
		}

		int32 VisibleActorCount = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor == nullptr || Actor->IsHiddenEd() || Actor->IsTemporarilyHiddenInEditor())
			{
				continue;
			}

			const FBox Bounds = Actor->GetComponentsBoundingBox(true);
			if (!Bounds.IsValid)
			{
				continue;
			}

			const FVector Center = Bounds.GetCenter();
			const FVector Extent = Bounds.GetExtent();
			if (SceneView->ViewFrustum.IntersectBox(Center, Extent))
			{
				++VisibleActorCount;
			}
		}

		return VisibleActorCount;
	}
}

FStatCheckEditorStatsSnapshot FStatCheckEditorStatsCollector::CollectSnapshot(
	bool bCollectVisibleActors,
	bool bCollectSelectedActorCost)
{
	FStatCheckEditorStatsSnapshot Snapshot;

	if (bCollectVisibleActors)
	{
		Snapshot.VisibleActorCount = StatCheckEditorStatsCollector::CountVisibleActorsInFocusedViewport();
		Snapshot.bVisibleActorCountAvailable = true;
	}

	if (bCollectSelectedActorCost)
	{
		Snapshot.SelectedActor = StatCheckEditorStatsCollector::CollectSelectedActorCost();
	}

	return Snapshot;
}

FStatCheckEditorStatsSnapshot FStatCheckEditorStatsCollector::CollectOptimizationSnapshot()
{
	FStatCheckEditorStatsSnapshot Snapshot;

	Snapshot.VisibleActorCount = StatCheckEditorStatsCollector::CountVisibleActorsInFocusedViewport();
	Snapshot.bVisibleActorCountAvailable = true;
	Snapshot.World = StatCheckEditorStatsCollector::CollectWorldCost();
	Snapshot.SelectedActor = StatCheckEditorStatsCollector::CollectSelectedActorCost();

	return Snapshot;
}

TArray<FStatCheckActorTickReportRow> FStatCheckEditorStatsCollector::CollectActorTickReport()
{
	TArray<FStatCheckActorTickReportRow> Rows;

	UWorld* World = StatCheckEditorStatsCollector::GetEditorWorld();
	if (World == nullptr)
	{
		return Rows;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor == nullptr || !Actor->IsActorTickEnabled())
		{
			continue;
		}

		FStatCheckActorTickReportRow Row;
		Row.ActorName = Actor->GetActorLabel();
		Row.ClassName = Actor->GetClass() != nullptr ? Actor->GetClass()->GetName() : FString();
		Row.bHidden = Actor->IsHiddenEd() || Actor->IsTemporarilyHiddenInEditor();
		Row.TickInterval = Actor->PrimaryActorTick.TickInterval;
		Row.Location = Actor->GetActorLocation();
		Rows.Add(Row);
	}

	Algo::Sort(Rows, [](const FStatCheckActorTickReportRow& Left, const FStatCheckActorTickReportRow& Right)
	{
		return Left.ActorName < Right.ActorName;
	});

	return Rows;
}

TArray<FStatCheckComponentTypeReportRow> FStatCheckEditorStatsCollector::CollectComponentTypeReport()
{
	TArray<FStatCheckComponentTypeReportRow> Rows;
	TMap<FString, int32> CountsByType;

	UWorld* World = StatCheckEditorStatsCollector::GetEditorWorld();
	if (World == nullptr)
	{
		return Rows;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor == nullptr)
		{
			continue;
		}

		TArray<UActorComponent*> Components;
		Actor->GetComponents(Components);

		for (UActorComponent* Component : Components)
		{
			if (Component == nullptr || Component->GetClass() == nullptr)
			{
				continue;
			}

			++CountsByType.FindOrAdd(Component->GetClass()->GetName());
		}
	}

	for (const TPair<FString, int32>& Pair : CountsByType)
	{
		FStatCheckComponentTypeReportRow Row;
		Row.TypeName = Pair.Key;
		Row.Count = Pair.Value;
		Rows.Add(Row);
	}

	Algo::Sort(Rows, [](const FStatCheckComponentTypeReportRow& Left, const FStatCheckComponentTypeReportRow& Right)
	{
		if (Left.Count == Right.Count)
		{
			return Left.TypeName < Right.TypeName;
		}

		return Left.Count > Right.Count;
	});

	return Rows;
}

TArray<FStatCheckSelectedActorComponentReportRow> FStatCheckEditorStatsCollector::CollectSelectedActorComponentReport()
{
	TArray<FStatCheckSelectedActorComponentReportRow> Rows;

	if (GEditor == nullptr)
	{
		return Rows;
	}

	AActor* SelectedActor = nullptr;
	for (FSelectionIterator It = GEditor->GetSelectedActorIterator(); It; ++It)
	{
		SelectedActor = Cast<AActor>(*It);
		if (SelectedActor != nullptr)
		{
			break;
		}
	}

	if (SelectedActor == nullptr)
	{
		return Rows;
	}

	TArray<UActorComponent*> Components;
	SelectedActor->GetComponents(Components);

	for (UActorComponent* Component : Components)
	{
		if (Component == nullptr)
		{
			continue;
		}

		FStatCheckSelectedActorComponentReportRow Row;
		Row.ActorName = SelectedActor->GetActorLabel();
		Row.ComponentName = Component->GetName();
		Row.ClassName = Component->GetClass() != nullptr ? Component->GetClass()->GetName() : FString();
		Row.bRegistered = Component->IsRegistered();
		Row.bActive = Component->IsActive();

		if (const UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
		{
			Row.bPrimitive = true;
			Row.bVisible = PrimitiveComponent->IsVisible();
			Row.Mobility = static_cast<int32>(PrimitiveComponent->Mobility.GetValue());
			Row.CollisionEnabled = static_cast<int32>(PrimitiveComponent->GetCollisionEnabled());
			Row.BoundsRadius = PrimitiveComponent->Bounds.SphereRadius;
		}

		if (const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Component))
		{
			Row.MaterialSlotCount = StatCheckEditorStatsCollector::CountMaterialSlots(*StaticMeshComponent);
			Row.ApproxTriangleCount = StatCheckEditorStatsCollector::CountApproxTriangles(*StaticMeshComponent);
		}

		Rows.Add(Row);
	}

	Algo::Sort(Rows, [](const FStatCheckSelectedActorComponentReportRow& Left, const FStatCheckSelectedActorComponentReportRow& Right)
	{
		return Left.ComponentName < Right.ComponentName;
	});

	return Rows;
}

TArray<FStatCheckHeavyActorReportRow> FStatCheckEditorStatsCollector::CollectHeavyActorReport(int32 MaxRows)
{
	TArray<FStatCheckHeavyActorReportRow> Rows;

	UWorld* World = StatCheckEditorStatsCollector::GetEditorWorld();
	if (World == nullptr || MaxRows <= 0)
	{
		return Rows;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor == nullptr)
		{
			continue;
		}

		Rows.Add(StatCheckEditorStatsCollector::MakeHeavyActorRow(*Actor));
	}

	Algo::Sort(Rows, [](const FStatCheckHeavyActorReportRow& Left, const FStatCheckHeavyActorReportRow& Right)
	{
		if (FMath::IsNearlyEqual(Left.CostScore, Right.CostScore))
		{
			return Left.ActorName < Right.ActorName;
		}

		return Left.CostScore > Right.CostScore;
	});

	if (Rows.Num() > MaxRows)
	{
		Rows.SetNum(MaxRows);
	}

	return Rows;
}
