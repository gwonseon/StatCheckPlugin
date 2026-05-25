// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StatCheckEditorStatsCollector.h"
#include "StatCheckPerformanceTypes.h"
#include "Templates/SharedPointer.h"
#include "Widgets/SCompoundWidget.h"

enum class ECheckBoxState : uint8;
class STextBlock;

class SStatCheckPanel : public SCompoundWidget
{
public:
	enum class EDisplayStat : uint8
	{
		Fps,
		FrameTime,
		GameThread,
		DrawCalls,
		FrameAverage,
		Memory,
		VisibleActors,
		SelectedActorCost
	};

	SLATE_BEGIN_ARGS(SStatCheckPanel)
	{
	}
		SLATE_ARGUMENT(FStatCheckPerformanceSnapshot, InitialSnapshot)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetSnapshot(const FStatCheckPerformanceSnapshot& InSnapshot);
	ECheckBoxState IsDisplayStatChecked(EDisplayStat DisplayStat) const;
	void OnDisplayStatChanged(ECheckBoxState NewState, EDisplayStat DisplayStat);

private:
	EActiveTimerReturnType HandleSnapshotRefresh(double InCurrentTime, float InDeltaTime);
	void EnsureActiveTimerRegistered();
	void RefreshSnapshotAndMaybeRecord(double InCurrentTime, float InDeltaTime, bool bAllowRecordingSample);
	FStatCheckPerformanceSnapshot CollectPerformanceSnapshot() const;
	void RefreshSnapshotText();
	FText MakeStateText() const;
	FText MakeFrameAverageText() const;
	FText MakeSelectedActorCostText() const;
	FText MakeWorldCostText() const;
	FText MakeHelpText() const;
	FReply OnHelpClicked();
	FReply OnRefreshNowClicked();
	FReply OnRecordLogClicked();
	FReply OnDeleteLogsClicked();
	FReply OnSaveActorTickReportClicked();
	FReply OnSaveComponentReportClicked();
	FReply OnSaveSelectedActorReportClicked();
	FReply OnSaveHeavyActorReportClicked();
	FReply OnSetComparisonBaselineClicked();
	FReply OnSaveComparisonReportClicked();
	bool IsRecordLogButtonEnabled() const;
	FText GetRecordLogButtonText() const;
	FText MakeRecordingStatusText() const;
	FString GetLogDirectory() const;
	FString MakeLogHeader() const;
	FString MakeLogRow(
		int32 SampleIndex,
		double ElapsedSeconds,
		float InDeltaTime,
		double PerformanceCollectMs,
		double SnapshotRefreshMs,
		double EditorStatsMs,
		double TotalRefreshMs,
		bool bEditorStatsRefreshed) const;
	bool SaveRowsToLogFile(const FString& FilePrefix, const TArray<FString>& Rows);
	void AddComparisonRow(
		TArray<FString>& Rows,
		const FString& MetricName,
		double BaselineValue,
		double CurrentValue) const;
	void StartRecordingLog();
	void TickRecordingLog(
		float InDeltaTime,
		double PerformanceCollectMs,
		double SnapshotRefreshMs,
		double EditorStatsMs,
		double TotalRefreshMs,
		bool bEditorStatsRefreshed);
	void FinishRecordingLog();
	void AddRecordingSample(
		double ElapsedSeconds,
		float InDeltaTime,
		double PerformanceCollectMs,
		double SnapshotRefreshMs,
		double EditorStatsMs,
		double TotalRefreshMs,
		bool bEditorStatsRefreshed);
	EVisibility GetStatVisibility(EDisplayStat DisplayStat) const;
	bool IsDisplayStatEnabled(EDisplayStat DisplayStat) const;
	void SetDisplayStatEnabled(EDisplayStat DisplayStat, bool bEnabled);
	void RefreshRowVisibility();

	FStatCheckPerformanceSnapshot CurrentSnapshot;
	FStatCheckEditorStatsSnapshot CurrentEditorSnapshot;
	bool bShowFps = true;
	bool bShowFrameTime = true;
	bool bShowGameThread = true;
	bool bShowDrawCalls = true;
	bool bShowFrameAverage = true;
	bool bShowMemory = true;
	bool bShowVisibleActors = true;
	bool bShowSelectedActorCost = true;
	bool bIsRecordingLog = false;
	double RecordingStartTime = 0.0;
	FString LastSavedRecordingPath;
	FString LastLogStatusMessage;
	TArray<FString> RecordingRows;
	TWeakPtr<FActiveTimerHandle> ActiveTimerHandle;
	double LastPerformanceCollectMs = 0.0;
	double LastSnapshotRefreshMs = 0.0;
	double LastEditorStatsMs = 0.0;
	double LastTotalRefreshMs = 0.0;
	bool bLastEditorStatsRefreshed = false;
	bool bHasComparisonBaseline = false;
	FStatCheckPerformanceSnapshot ComparisonBaselineSnapshot;
	FStatCheckEditorStatsSnapshot ComparisonBaselineEditorSnapshot;
	TSharedPtr<SWidget> FpsRow;
	TSharedPtr<SWidget> FrameTimeRow;
	TSharedPtr<SWidget> GameThreadRow;
	TSharedPtr<SWidget> DrawCallRow;
	TSharedPtr<SWidget> FrameAverageRow;
	TSharedPtr<SWidget> MemoryRow;
	TSharedPtr<SWidget> VisibleActorsRow;
	TSharedPtr<SWidget> SelectedActorCostRow;
	TSharedPtr<SWidget> WorldCostRow;
	TSharedPtr<STextBlock> FpsValueText;
	TSharedPtr<STextBlock> FrameTimeValueText;
	TSharedPtr<STextBlock> GameThreadValueText;
	TSharedPtr<STextBlock> DrawCallValueText;
	TSharedPtr<STextBlock> FrameAverageValueText;
	TSharedPtr<STextBlock> MemoryValueText;
	TSharedPtr<STextBlock> VisibleActorsValueText;
	TSharedPtr<STextBlock> SelectedActorCostValueText;
	TSharedPtr<STextBlock> WorldCostValueText;
	TSharedPtr<STextBlock> StateText;
	TSharedPtr<STextBlock> RecordingStatusText;
};
