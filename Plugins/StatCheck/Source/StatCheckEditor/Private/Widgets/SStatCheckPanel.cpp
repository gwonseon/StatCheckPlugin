// Copyright Epic Games, Inc. All Rights Reserved.

#include "Widgets/SStatCheckPanel.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "StatCheckPerformanceCollector.h"
#include "StatCheckPerformanceEvaluator.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SStatCheckPanel"

namespace StatCheckPanel
{
	static constexpr float RefreshIntervalSeconds = 1.0f;
	static constexpr double RecordingDurationSeconds = 10.0;

	static FString BoolToCsvValue(bool bValue)
	{
		return bValue ? TEXT("true") : TEXT("false");
	}

	static FString EscapeCsvString(const FString& Value)
	{
		FString EscapedValue = Value;
		EscapedValue.ReplaceInline(TEXT("\""), TEXT("\"\""));
		return FString::Printf(TEXT("\"%s\""), *EscapedValue);
	}

	static TSharedRef<SWidget> MakeStatRow(
		const FText& Label,
		TSharedPtr<STextBlock>& OutValueTextBlock,
		const FText& InitialValue,
		const FText& Unit)
	{
		return SNew(SBorder)
			.Padding(FMargin(10.0f, 8.0f))
			.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
			[
				SNew(SGridPanel)

				+ SGridPanel::Slot(0, 0)
				.Padding(0.0f, 0.0f, 12.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(Label)
				]

				+ SGridPanel::Slot(1, 0)
				.Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SAssignNew(OutValueTextBlock, STextBlock)
					.Text(InitialValue)
					.Font(FAppStyle::GetFontStyle("NormalFontBold"))
					.AutoWrapText(true)
				]

				+ SGridPanel::Slot(2, 0)
				[
					SNew(STextBlock)
					.Text(Unit)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			];
	}

	static TSharedRef<SWidget> MakeToggle(
		const FText& Label,
		SStatCheckPanel* Panel,
		SStatCheckPanel::EDisplayStat DisplayStat)
	{
		return SNew(SCheckBox)
			.IsChecked(Panel, &SStatCheckPanel::IsDisplayStatChecked, DisplayStat)
			.OnCheckStateChanged(Panel, &SStatCheckPanel::OnDisplayStatChanged, DisplayStat)
			[
				SNew(STextBlock)
				.Text(Label)
			];
	}
}

void SStatCheckPanel::Construct(const FArguments& InArgs)
{
	CurrentSnapshot = InArgs._InitialSnapshot;

	ChildSlot
	[
		SNew(SBorder)
		.Padding(16.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Title", "StatCheck"))
					.Font(FAppStyle::GetFontStyle("HeadingMedium"))
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.ToolTipText(LOCTEXT("HelpButtonTooltip", "Show StatCheck usage help."))
					.OnClicked(this, &SStatCheckPanel::OnHelpClicked)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("HelpButtonText", "?"))
						.Font(FAppStyle::GetFontStyle("NormalFontBold"))
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 4.0f, 0.0f, 12.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Subtitle", "Capture optimization logs on demand without live tracking."))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				SNew(SSeparator)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				SNew(SBorder)
				.Padding(10.0f)
				.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
				[
					SNew(SGridPanel)

					+ SGridPanel::Slot(0, 0)
					.Padding(0.0f, 0.0f, 12.0f, 6.0f)
					[
						StatCheckPanel::MakeToggle(LOCTEXT("ToggleFps", "FPS"), this, EDisplayStat::Fps)
					]

					+ SGridPanel::Slot(1, 0)
					.Padding(0.0f, 0.0f, 12.0f, 6.0f)
					[
						StatCheckPanel::MakeToggle(LOCTEXT("ToggleFrameTime", "FrameTime"), this, EDisplayStat::FrameTime)
					]

					+ SGridPanel::Slot(2, 0)
					.Padding(0.0f, 0.0f, 12.0f, 6.0f)
					[
						StatCheckPanel::MakeToggle(LOCTEXT("ToggleGameThread", "GameThread"), this, EDisplayStat::GameThread)
					]

					+ SGridPanel::Slot(3, 0)
					.Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						StatCheckPanel::MakeToggle(LOCTEXT("ToggleDrawCalls", "DrawCall"), this, EDisplayStat::DrawCalls)
					]

					+ SGridPanel::Slot(0, 1)
					.Padding(0.0f, 0.0f, 12.0f, 0.0f)
					[
						StatCheckPanel::MakeToggle(LOCTEXT("ToggleFrameAverage", "Frame Avg"), this, EDisplayStat::FrameAverage)
					]

					+ SGridPanel::Slot(1, 1)
					.Padding(0.0f, 0.0f, 12.0f, 0.0f)
					[
						StatCheckPanel::MakeToggle(LOCTEXT("ToggleMemory", "Memory"), this, EDisplayStat::Memory)
					]

					+ SGridPanel::Slot(2, 1)
					.Padding(0.0f, 0.0f, 12.0f, 0.0f)
					[
						StatCheckPanel::MakeToggle(LOCTEXT("ToggleVisibleActors", "Visible Actors"), this, EDisplayStat::VisibleActors)
					]

					+ SGridPanel::Slot(3, 1)
					[
						StatCheckPanel::MakeToggle(LOCTEXT("ToggleSelectedActorCost", "Selected Actor"), this, EDisplayStat::SelectedActorCost)
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				SNew(SBorder)
				.Padding(10.0f)
				.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.0f, 0.0f, 12.0f, 0.0f)
					[
						SNew(SButton)
						.OnClicked(this, &SStatCheckPanel::OnRefreshNowClicked)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("SaveSnapshotLogButton", "Save Snapshot Log"))
						]
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.0f, 0.0f, 12.0f, 0.0f)
					[
						SNew(SButton)
						.IsEnabled(this, &SStatCheckPanel::IsRecordLogButtonEnabled)
						.OnClicked(this, &SStatCheckPanel::OnRecordLogClicked)
						[
							SNew(STextBlock)
							.Text(this, &SStatCheckPanel::GetRecordLogButtonText)
						]
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.0f, 0.0f, 12.0f, 0.0f)
					[
						SNew(SButton)
						.OnClicked(this, &SStatCheckPanel::OnDeleteLogsClicked)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("DeleteLogsButton", "Delete Logs"))
						]
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SAssignNew(RecordingStatusText, STextBlock)
						.Text(MakeRecordingStatusText())
						.AutoWrapText(true)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				SNew(SBorder)
				.Padding(10.0f)
				.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
				[
					SNew(SGridPanel)

					+ SGridPanel::Slot(0, 0)
					.Padding(0.0f, 0.0f, 12.0f, 6.0f)
					[
						SNew(SButton)
						.OnClicked(this, &SStatCheckPanel::OnSaveActorTickReportClicked)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ActorTickReportButton", "Actor Tick Report"))
						]
					]

					+ SGridPanel::Slot(1, 0)
					.Padding(0.0f, 0.0f, 12.0f, 6.0f)
					[
						SNew(SButton)
						.OnClicked(this, &SStatCheckPanel::OnSaveComponentReportClicked)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ComponentReportButton", "Component Report"))
						]
					]

					+ SGridPanel::Slot(2, 0)
					.Padding(0.0f, 0.0f, 12.0f, 6.0f)
					[
						SNew(SButton)
						.OnClicked(this, &SStatCheckPanel::OnSaveSelectedActorReportClicked)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("SelectedActorReportButton", "Selected Actor Report"))
						]
					]

					+ SGridPanel::Slot(3, 0)
					.Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SNew(SButton)
						.OnClicked(this, &SStatCheckPanel::OnSaveHeavyActorReportClicked)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("HeavyActorReportButton", "Top Actors Report"))
						]
					]

					+ SGridPanel::Slot(0, 1)
					.Padding(0.0f, 0.0f, 12.0f, 0.0f)
					[
						SNew(SButton)
						.OnClicked(this, &SStatCheckPanel::OnSetComparisonBaselineClicked)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("SetBaselineButton", "Set Baseline"))
						]
					]

					+ SGridPanel::Slot(1, 1)
					.Padding(0.0f, 0.0f, 12.0f, 0.0f)
					[
						SNew(SButton)
						.OnClicked(this, &SStatCheckPanel::OnSaveComparisonReportClicked)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("CompareBaselineButton", "Compare Baseline"))
						]
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SAssignNew(FpsRow, SBox)
				.Visibility(this, &SStatCheckPanel::GetStatVisibility, EDisplayStat::Fps)
				[
					StatCheckPanel::MakeStatRow(
						LOCTEXT("FpsLabel", "FPS"),
						FpsValueText,
						FText::AsNumber(CurrentSnapshot.Fps),
						LOCTEXT("FpsUnit", "frames/sec"))
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SAssignNew(FrameTimeRow, SBox)
				.Visibility(this, &SStatCheckPanel::GetStatVisibility, EDisplayStat::FrameTime)
				[
					StatCheckPanel::MakeStatRow(
						LOCTEXT("FrameTimeLabel", "FrameTime"),
						FrameTimeValueText,
						FText::AsNumber(CurrentSnapshot.FrameTimeMs),
						LOCTEXT("FrameTimeUnit", "ms"))
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SAssignNew(GameThreadRow, SBox)
				.Visibility(this, &SStatCheckPanel::GetStatVisibility, EDisplayStat::GameThread)
				[
					StatCheckPanel::MakeStatRow(
						LOCTEXT("GameThreadLabel", "GameThread"),
						GameThreadValueText,
						FText::AsNumber(CurrentSnapshot.GameThreadMs),
						LOCTEXT("GameThreadUnit", "ms"))
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SAssignNew(DrawCallRow, SBox)
				.Visibility(this, &SStatCheckPanel::GetStatVisibility, EDisplayStat::DrawCalls)
				[
					StatCheckPanel::MakeStatRow(
						LOCTEXT("DrawCallLabel", "DrawCall"),
						DrawCallValueText,
						FText::AsNumber(CurrentSnapshot.DrawCalls),
						LOCTEXT("DrawCallUnit", "calls"))
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SAssignNew(FrameAverageRow, SBox)
				.Visibility(this, &SStatCheckPanel::GetStatVisibility, EDisplayStat::FrameAverage)
				[
					StatCheckPanel::MakeStatRow(
						LOCTEXT("FrameAverageLabel", "Frame Avg"),
						FrameAverageValueText,
						MakeFrameAverageText(),
						LOCTEXT("FrameAverageUnit", "avg/min/max ms"))
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SAssignNew(MemoryRow, SBox)
				.Visibility(this, &SStatCheckPanel::GetStatVisibility, EDisplayStat::Memory)
				[
					StatCheckPanel::MakeStatRow(
						LOCTEXT("MemoryLabel", "Memory"),
						MemoryValueText,
						FText::AsNumber(CurrentSnapshot.UsedPhysicalMemoryMB),
						LOCTEXT("MemoryUnit", "MB used"))
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SAssignNew(VisibleActorsRow, SBox)
				.Visibility(this, &SStatCheckPanel::GetStatVisibility, EDisplayStat::VisibleActors)
				[
					StatCheckPanel::MakeStatRow(
						LOCTEXT("VisibleActorsLabel", "Visible Actors"),
						VisibleActorsValueText,
						FText::AsNumber(CurrentEditorSnapshot.VisibleActorCount),
						LOCTEXT("VisibleActorsUnit", "actors"))
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				SAssignNew(SelectedActorCostRow, SBox)
				.Visibility(this, &SStatCheckPanel::GetStatVisibility, EDisplayStat::SelectedActorCost)
				[
					StatCheckPanel::MakeStatRow(
						LOCTEXT("SelectedActorCostLabel", "Selected Actor"),
						SelectedActorCostValueText,
						MakeSelectedActorCostText(),
						LOCTEXT("SelectedActorCostUnit", "proxy"))
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				SAssignNew(WorldCostRow, SBox)
				[
					StatCheckPanel::MakeStatRow(
						LOCTEXT("WorldCostLabel", "World Cost"),
						WorldCostValueText,
						MakeWorldCostText(),
						LOCTEXT("WorldCostUnit", "scene"))
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				SNew(SBorder)
				.Padding(10.0f)
				.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
				[
					SAssignNew(StateText, STextBlock)
					.Text(MakeStateText())
					.Font(FAppStyle::GetFontStyle("NormalFontBold"))
				]
			]
		]
	];

	RefreshSnapshotText();
}

void SStatCheckPanel::SetSnapshot(const FStatCheckPerformanceSnapshot& InSnapshot)
{
	CurrentSnapshot = InSnapshot;
	RefreshSnapshotText();
}

EActiveTimerReturnType SStatCheckPanel::HandleSnapshotRefresh(double InCurrentTime, float InDeltaTime)
{
	if (!bIsRecordingLog)
	{
		ActiveTimerHandle.Reset();
		return EActiveTimerReturnType::Stop;
	}

	RefreshSnapshotAndMaybeRecord(InCurrentTime, InDeltaTime, true);

	if (!bIsRecordingLog)
	{
		ActiveTimerHandle.Reset();
		return EActiveTimerReturnType::Stop;
	}

	return EActiveTimerReturnType::Continue;
}

void SStatCheckPanel::EnsureActiveTimerRegistered()
{
	if (ActiveTimerHandle.IsValid())
	{
		return;
	}

	ActiveTimerHandle = RegisterActiveTimer(
		StatCheckPanel::RefreshIntervalSeconds,
		FWidgetActiveTimerDelegate::CreateSP(this, &SStatCheckPanel::HandleSnapshotRefresh));
}

void SStatCheckPanel::RefreshSnapshotAndMaybeRecord(
	double InCurrentTime,
	float InDeltaTime,
	bool bAllowRecordingSample)
{
	const double RefreshStartSeconds = FPlatformTime::Seconds();

	const double PerformanceCollectStartSeconds = FPlatformTime::Seconds();
	const FStatCheckPerformanceSnapshot Snapshot = CollectPerformanceSnapshot();
	LastPerformanceCollectMs =
		(FPlatformTime::Seconds() - PerformanceCollectStartSeconds) * 1000.0;

	const double SnapshotRefreshStartSeconds = FPlatformTime::Seconds();
	SetSnapshot(Snapshot);
	LastSnapshotRefreshMs =
		(FPlatformTime::Seconds() - SnapshotRefreshStartSeconds) * 1000.0;

	const double EditorStatsStartSeconds = FPlatformTime::Seconds();
	CurrentEditorSnapshot = FStatCheckEditorStatsCollector::CollectOptimizationSnapshot();
	bLastEditorStatsRefreshed = true;
	RefreshSnapshotText();
	LastEditorStatsMs =
		(FPlatformTime::Seconds() - EditorStatsStartSeconds) * 1000.0;

	LastTotalRefreshMs =
		(FPlatformTime::Seconds() - RefreshStartSeconds) * 1000.0;

	if (bAllowRecordingSample)
	{
		TickRecordingLog(
			InDeltaTime,
			LastPerformanceCollectMs,
			LastSnapshotRefreshMs,
			LastEditorStatsMs,
			LastTotalRefreshMs,
			bLastEditorStatsRefreshed);
	}
}

FStatCheckPerformanceSnapshot SStatCheckPanel::CollectPerformanceSnapshot() const
{
	FStatCheckPerformanceCollectOptions Options;
	Options.bCollectMemory = true;
	return FStatCheckPerformanceCollector::CollectSnapshot(Options);
}

void SStatCheckPanel::RefreshSnapshotText()
{
	if (FpsValueText.IsValid() && bShowFps)
	{
		FpsValueText->SetText(FText::AsNumber(CurrentSnapshot.Fps));
	}

	if (FrameTimeValueText.IsValid() && bShowFrameTime)
	{
		FrameTimeValueText->SetText(FText::AsNumber(CurrentSnapshot.FrameTimeMs));
	}

	if (GameThreadValueText.IsValid() && bShowGameThread)
	{
		GameThreadValueText->SetText(FText::AsNumber(CurrentSnapshot.GameThreadMs));
	}

	if (DrawCallValueText.IsValid() && bShowDrawCalls)
	{
		DrawCallValueText->SetText(FText::AsNumber(CurrentSnapshot.DrawCalls));
	}

	if (FrameAverageValueText.IsValid() && bShowFrameAverage)
	{
		FrameAverageValueText->SetText(MakeFrameAverageText());
	}

	if (MemoryValueText.IsValid() && bShowMemory)
	{
		MemoryValueText->SetText(FText::AsNumber(CurrentSnapshot.UsedPhysicalMemoryMB));
	}

	if (VisibleActorsValueText.IsValid() && bShowVisibleActors)
	{
		VisibleActorsValueText->SetText(
			CurrentEditorSnapshot.bVisibleActorCountAvailable
				? FText::AsNumber(CurrentEditorSnapshot.VisibleActorCount)
				: LOCTEXT("VisibleActorsUnavailable", "Off"));
	}

	if (SelectedActorCostValueText.IsValid() && bShowSelectedActorCost)
	{
		SelectedActorCostValueText->SetText(MakeSelectedActorCostText());
	}

	if (WorldCostValueText.IsValid())
	{
		WorldCostValueText->SetText(MakeWorldCostText());
	}

	if (StateText.IsValid())
	{
		StateText->SetText(MakeStateText());
	}
}

FText SStatCheckPanel::MakeStateText() const
{
	const EStatCheckPerformanceState CurrentState =
		FStatCheckPerformanceEvaluator::Evaluate(CurrentSnapshot);

	return FText::Format(
		LOCTEXT("StatePreviewFormat", "State: {0}"),
		FText::FromString(FStatCheckPerformanceEvaluator::ToString(CurrentState)));
}

FText SStatCheckPanel::MakeFrameAverageText() const
{
	return FText::Format(
		LOCTEXT("FrameAverageFormat", "{0} / {1} / {2}"),
		FText::AsNumber(CurrentSnapshot.AverageFrameTimeMs),
		FText::AsNumber(CurrentSnapshot.MinFrameTimeMs),
		FText::AsNumber(CurrentSnapshot.MaxFrameTimeMs));
}

FText SStatCheckPanel::MakeSelectedActorCostText() const
{
	if (!CurrentEditorSnapshot.SelectedActor.bHasActor)
	{
		return LOCTEXT("SelectedActorNone", "None");
	}

	return FText::Format(
		LOCTEXT(
			"SelectedActorCostFormat",
			"{0} | C:{1} P:{2} Mesh:{3} Mat:{4} Tri:{5} Radius:{6}"),
		FText::FromString(CurrentEditorSnapshot.SelectedActor.ActorName),
		FText::AsNumber(CurrentEditorSnapshot.SelectedActor.ComponentCount),
		FText::AsNumber(CurrentEditorSnapshot.SelectedActor.PrimitiveComponentCount),
		FText::AsNumber(CurrentEditorSnapshot.SelectedActor.StaticMeshComponentCount),
		FText::AsNumber(CurrentEditorSnapshot.SelectedActor.MaterialSlotCount),
		FText::AsNumber(CurrentEditorSnapshot.SelectedActor.ApproxTriangleCount),
		FText::AsNumber(CurrentEditorSnapshot.SelectedActor.BoundsRadius));
}

FText SStatCheckPanel::MakeWorldCostText() const
{
	if (!CurrentEditorSnapshot.World.bAvailable)
	{
		return LOCTEXT("WorldCostUnavailable", "Not captured");
	}

	return FText::Format(
		LOCTEXT(
			"WorldCostFormat",
			"Actors:{0} Hidden:{1} Tick:{2} Prim:{3} SM:{4} SK:{5} Light:{6} Mat:{7} Tri:{8}"),
		FText::AsNumber(CurrentEditorSnapshot.World.TotalActorCount),
		FText::AsNumber(CurrentEditorSnapshot.World.HiddenActorCount),
		FText::AsNumber(CurrentEditorSnapshot.World.TickEnabledActorCount),
		FText::AsNumber(CurrentEditorSnapshot.World.PrimitiveComponentCount),
		FText::AsNumber(CurrentEditorSnapshot.World.StaticMeshComponentCount),
		FText::AsNumber(CurrentEditorSnapshot.World.SkeletalMeshComponentCount),
		FText::AsNumber(CurrentEditorSnapshot.World.LightComponentCount),
		FText::AsNumber(CurrentEditorSnapshot.World.MaterialSlotCount),
		FText::AsNumber(CurrentEditorSnapshot.World.ApproxTriangleCount));
}

FText SStatCheckPanel::MakeHelpText() const
{
	return LOCTEXT(
		"StatCheckHelpText",
		"StatCheck 사용 방법\n"
		"\n"
		"목적\n"
		"- StatCheck는 Unreal Editor에서 최적화 후보를 찾기 위한 버튼 기반 진단 플러그인입니다.\n"
		"- 실시간 추적을 기본으로 하지 않으며, 버튼을 눌렀을 때만 수집합니다.\n"
		"\n"
		"기본 사용\n"
		"1. Window > StatCheck를 엽니다.\n"
		"2. Save Snapshot Log를 눌러 현재 순간의 FPS, DrawCall, Actor/Component 정보를 저장합니다.\n"
		"3. Record 10s Log를 누르면 10초 동안 1초 간격으로 CSV 로그를 저장합니다.\n"
		"4. Delete Logs를 누르면 저장된 StatCheck 로그를 삭제합니다.\n"
		"\n"
		"추가 리포트\n"
		"- Actor Tick Report: Tick이 켜진 Actor 목록을 저장합니다.\n"
		"- Component Report: Component 타입별 개수를 저장합니다.\n"
		"- Selected Actor Report: 선택한 Actor의 Component 상세 정보를 저장합니다.\n"
		"- Top Actors Report: 무거운 Actor 후보 상위 50개를 저장합니다.\n"
		"- Set Baseline / Compare Baseline: 최적화 전후 Snapshot 차이를 저장합니다.\n"
		"\n"
		"저장 위치\n"
		"- 현재 프로젝트의 Saved/StatCheck/Logs 폴더에 CSV로 저장됩니다.\n"
		"\n"
		"외부 프로젝트 사용\n"
		"- Unreal Editor를 닫고, 이 플러그인의 Plugins/StatCheck 폴더를 대상 프로젝트의 Plugins/StatCheck 위치로 복사합니다.\n"
		"- 대상 프로젝트를 열고 플러그인을 활성화한 뒤 빌드합니다.\n"
		"- 자세한 내용은 Plugins/StatCheck/사용설명.md를 확인하세요.");
}

FReply SStatCheckPanel::OnHelpClicked()
{
	FMessageDialog::Open(EAppMsgType::Ok, MakeHelpText(), LOCTEXT("StatCheckHelpTitle", "StatCheck Help"));
	return FReply::Handled();
}

EVisibility SStatCheckPanel::GetStatVisibility(EDisplayStat DisplayStat) const
{
	return IsDisplayStatEnabled(DisplayStat) ? EVisibility::Visible : EVisibility::Collapsed;
}

ECheckBoxState SStatCheckPanel::IsDisplayStatChecked(EDisplayStat DisplayStat) const
{
	return IsDisplayStatEnabled(DisplayStat) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SStatCheckPanel::OnDisplayStatChanged(ECheckBoxState NewState, EDisplayStat DisplayStat)
{
	SetDisplayStatEnabled(DisplayStat, NewState == ECheckBoxState::Checked);
	RefreshRowVisibility();
	RefreshSnapshotText();
}

FReply SStatCheckPanel::OnRefreshNowClicked()
{
	RefreshSnapshotAndMaybeRecord(FPlatformTime::Seconds(), 0.0f, false);
	TArray<FString> Rows;
	Rows.Add(MakeLogHeader());
	Rows.Add(MakeLogRow(
		0,
		0.0,
		0.0f,
		LastPerformanceCollectMs,
		LastSnapshotRefreshMs,
		LastEditorStatsMs,
		LastTotalRefreshMs,
		bLastEditorStatsRefreshed));
	SaveRowsToLogFile(TEXT("StatCheckSnapshot"), Rows);
	return FReply::Handled();
}

FReply SStatCheckPanel::OnRecordLogClicked()
{
	StartRecordingLog();
	return FReply::Handled();
}

FReply SStatCheckPanel::OnDeleteLogsClicked()
{
	const FString LogDirectory = GetLogDirectory();
	IFileManager::Get().DeleteDirectory(*LogDirectory, false, true);
	IFileManager::Get().MakeDirectory(*LogDirectory, true);

	LastSavedRecordingPath.Empty();
	LastLogStatusMessage = FString::Printf(TEXT("Deleted logs: %s"), *LogDirectory);

	if (RecordingStatusText.IsValid())
	{
		RecordingStatusText->SetText(MakeRecordingStatusText());
	}

	return FReply::Handled();
}

FReply SStatCheckPanel::OnSaveActorTickReportClicked()
{
	TArray<FString> Rows;
	Rows.Add(TEXT("actor_name,class_name,hidden,tick_interval,location_x,location_y,location_z"));

	for (const FStatCheckActorTickReportRow& Row : FStatCheckEditorStatsCollector::CollectActorTickReport())
	{
		Rows.Add(FString::Printf(
			TEXT("%s,%s,%s,%.3f,%.3f,%.3f,%.3f"),
			*StatCheckPanel::EscapeCsvString(Row.ActorName),
			*StatCheckPanel::EscapeCsvString(Row.ClassName),
			*StatCheckPanel::BoolToCsvValue(Row.bHidden),
			Row.TickInterval,
			Row.Location.X,
			Row.Location.Y,
			Row.Location.Z));
	}

	SaveRowsToLogFile(TEXT("StatCheckActorTickReport"), Rows);
	return FReply::Handled();
}

FReply SStatCheckPanel::OnSaveComponentReportClicked()
{
	TArray<FString> Rows;
	Rows.Add(TEXT("component_type,count"));

	for (const FStatCheckComponentTypeReportRow& Row : FStatCheckEditorStatsCollector::CollectComponentTypeReport())
	{
		Rows.Add(FString::Printf(
			TEXT("%s,%d"),
			*StatCheckPanel::EscapeCsvString(Row.TypeName),
			Row.Count));
	}

	SaveRowsToLogFile(TEXT("StatCheckComponentReport"), Rows);
	return FReply::Handled();
}

FReply SStatCheckPanel::OnSaveSelectedActorReportClicked()
{
	TArray<FString> Rows;
	Rows.Add(TEXT("actor_name,component_name,class_name,registered,active,primitive,visible,mobility,collision_enabled,material_slots,approx_triangles,bounds_radius"));

	for (const FStatCheckSelectedActorComponentReportRow& Row :
		FStatCheckEditorStatsCollector::CollectSelectedActorComponentReport())
	{
		Rows.Add(FString::Printf(
			TEXT("%s,%s,%s,%s,%s,%s,%s,%d,%d,%d,%d,%.3f"),
			*StatCheckPanel::EscapeCsvString(Row.ActorName),
			*StatCheckPanel::EscapeCsvString(Row.ComponentName),
			*StatCheckPanel::EscapeCsvString(Row.ClassName),
			*StatCheckPanel::BoolToCsvValue(Row.bRegistered),
			*StatCheckPanel::BoolToCsvValue(Row.bActive),
			*StatCheckPanel::BoolToCsvValue(Row.bPrimitive),
			*StatCheckPanel::BoolToCsvValue(Row.bVisible),
			Row.Mobility,
			Row.CollisionEnabled,
			Row.MaterialSlotCount,
			Row.ApproxTriangleCount,
			Row.BoundsRadius));
	}

	SaveRowsToLogFile(TEXT("StatCheckSelectedActorReport"), Rows);
	return FReply::Handled();
}

FReply SStatCheckPanel::OnSaveHeavyActorReportClicked()
{
	TArray<FString> Rows;
	Rows.Add(TEXT("rank,actor_name,class_name,hidden,tick_enabled,component_count,primitive_components,static_mesh_components,skeletal_mesh_components,light_components,material_slots,approx_triangles,cost_score"));

	int32 Rank = 1;
	for (const FStatCheckHeavyActorReportRow& Row : FStatCheckEditorStatsCollector::CollectHeavyActorReport(50))
	{
		Rows.Add(FString::Printf(
			TEXT("%d,%s,%s,%s,%s,%d,%d,%d,%d,%d,%d,%lld,%.3f"),
			Rank++,
			*StatCheckPanel::EscapeCsvString(Row.ActorName),
			*StatCheckPanel::EscapeCsvString(Row.ClassName),
			*StatCheckPanel::BoolToCsvValue(Row.bHidden),
			*StatCheckPanel::BoolToCsvValue(Row.bTickEnabled),
			Row.ComponentCount,
			Row.PrimitiveComponentCount,
			Row.StaticMeshComponentCount,
			Row.SkeletalMeshComponentCount,
			Row.LightComponentCount,
			Row.MaterialSlotCount,
			Row.ApproxTriangleCount,
			Row.CostScore));
	}

	SaveRowsToLogFile(TEXT("StatCheckTopActorsReport"), Rows);
	return FReply::Handled();
}

FReply SStatCheckPanel::OnSetComparisonBaselineClicked()
{
	RefreshSnapshotAndMaybeRecord(FPlatformTime::Seconds(), 0.0f, false);

	ComparisonBaselineSnapshot = CurrentSnapshot;
	ComparisonBaselineEditorSnapshot = CurrentEditorSnapshot;
	bHasComparisonBaseline = true;
	LastSavedRecordingPath.Empty();
	LastLogStatusMessage = TEXT("Baseline captured.");

	if (RecordingStatusText.IsValid())
	{
		RecordingStatusText->SetText(MakeRecordingStatusText());
	}

	return FReply::Handled();
}

FReply SStatCheckPanel::OnSaveComparisonReportClicked()
{
	if (!bHasComparisonBaseline)
	{
		LastSavedRecordingPath.Empty();
		LastLogStatusMessage = TEXT("No baseline. Press Set Baseline first.");

		if (RecordingStatusText.IsValid())
		{
			RecordingStatusText->SetText(MakeRecordingStatusText());
		}

		return FReply::Handled();
	}

	RefreshSnapshotAndMaybeRecord(FPlatformTime::Seconds(), 0.0f, false);

	TArray<FString> Rows;
	Rows.Add(TEXT("metric,baseline,current,delta"));
	AddComparisonRow(Rows, TEXT("fps"), ComparisonBaselineSnapshot.Fps, CurrentSnapshot.Fps);
	AddComparisonRow(Rows, TEXT("frame_time_ms"), ComparisonBaselineSnapshot.FrameTimeMs, CurrentSnapshot.FrameTimeMs);
	AddComparisonRow(Rows, TEXT("game_thread_ms"), ComparisonBaselineSnapshot.GameThreadMs, CurrentSnapshot.GameThreadMs);
	AddComparisonRow(Rows, TEXT("draw_calls"), ComparisonBaselineSnapshot.DrawCalls, CurrentSnapshot.DrawCalls);
	AddComparisonRow(Rows, TEXT("memory_mb"), ComparisonBaselineSnapshot.UsedPhysicalMemoryMB, CurrentSnapshot.UsedPhysicalMemoryMB);
	AddComparisonRow(Rows, TEXT("visible_actor_count"), ComparisonBaselineEditorSnapshot.VisibleActorCount, CurrentEditorSnapshot.VisibleActorCount);
	AddComparisonRow(Rows, TEXT("total_actor_count"), ComparisonBaselineEditorSnapshot.World.TotalActorCount, CurrentEditorSnapshot.World.TotalActorCount);
	AddComparisonRow(Rows, TEXT("hidden_actor_count"), ComparisonBaselineEditorSnapshot.World.HiddenActorCount, CurrentEditorSnapshot.World.HiddenActorCount);
	AddComparisonRow(Rows, TEXT("tick_enabled_actor_count"), ComparisonBaselineEditorSnapshot.World.TickEnabledActorCount, CurrentEditorSnapshot.World.TickEnabledActorCount);
	AddComparisonRow(Rows, TEXT("primitive_component_count"), ComparisonBaselineEditorSnapshot.World.PrimitiveComponentCount, CurrentEditorSnapshot.World.PrimitiveComponentCount);
	AddComparisonRow(Rows, TEXT("static_mesh_component_count"), ComparisonBaselineEditorSnapshot.World.StaticMeshComponentCount, CurrentEditorSnapshot.World.StaticMeshComponentCount);
	AddComparisonRow(Rows, TEXT("skeletal_mesh_component_count"), ComparisonBaselineEditorSnapshot.World.SkeletalMeshComponentCount, CurrentEditorSnapshot.World.SkeletalMeshComponentCount);
	AddComparisonRow(Rows, TEXT("light_component_count"), ComparisonBaselineEditorSnapshot.World.LightComponentCount, CurrentEditorSnapshot.World.LightComponentCount);
	AddComparisonRow(Rows, TEXT("material_slot_count"), ComparisonBaselineEditorSnapshot.World.MaterialSlotCount, CurrentEditorSnapshot.World.MaterialSlotCount);
	AddComparisonRow(Rows, TEXT("approx_triangle_count"), ComparisonBaselineEditorSnapshot.World.ApproxTriangleCount, CurrentEditorSnapshot.World.ApproxTriangleCount);

	SaveRowsToLogFile(TEXT("StatCheckComparisonReport"), Rows);
	return FReply::Handled();
}

bool SStatCheckPanel::IsRecordLogButtonEnabled() const
{
	return !bIsRecordingLog;
}

FText SStatCheckPanel::GetRecordLogButtonText() const
{
	return bIsRecordingLog
		? LOCTEXT("RecordLogButtonRecording", "Recording...")
		: LOCTEXT("RecordLogButtonIdle", "Record 10s Log");
}

FText SStatCheckPanel::MakeRecordingStatusText() const
{
	if (bIsRecordingLog)
	{
		const double ElapsedSeconds = FPlatformTime::Seconds() - RecordingStartTime;
		const int32 RemainingSeconds = FMath::Max(
			0,
			FMath::CeilToInt(StatCheckPanel::RecordingDurationSeconds - ElapsedSeconds));

		return FText::Format(
			LOCTEXT("RecordingStatusActive", "Recording... {0}s remaining"),
			FText::AsNumber(RemainingSeconds));
	}

	if (!LastSavedRecordingPath.IsEmpty())
	{
		return FText::Format(
			LOCTEXT("RecordingStatusSaved", "Saved: {0}"),
			FText::FromString(LastSavedRecordingPath));
	}

	if (!LastLogStatusMessage.IsEmpty())
	{
		return FText::FromString(LastLogStatusMessage);
	}

	return LOCTEXT("RecordingStatusIdle", "Ready to save optimization logs.");
}

FString SStatCheckPanel::GetLogDirectory() const
{
	return FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("StatCheck"),
		TEXT("Logs"));
}

FString SStatCheckPanel::MakeLogHeader() const
{
	return TEXT(
		"sample,elapsed_sec,active_timer_delta_sec,fps,frame_time_ms,game_thread_ms,draw_calls,frame_avg_ms,frame_min_ms,frame_max_ms,memory_mb,visible_actor_available,visible_actor_count,world_available,total_actor_count,hidden_actor_count,tick_enabled_actor_count,primitive_component_count,static_mesh_component_count,skeletal_mesh_component_count,light_component_count,total_material_slots,approx_triangle_count,selected_actor_has_actor,selected_actor_name,selected_actor_components,selected_actor_primitives,selected_actor_static_meshes,selected_actor_material_slots,selected_actor_triangles,selected_actor_bounds_radius,performance_collect_ms,snapshot_refresh_ms,editor_stats_ms,total_refresh_ms,editor_stats_refreshed");
}

FString SStatCheckPanel::MakeLogRow(
	int32 SampleIndex,
	double ElapsedSeconds,
	float InDeltaTime,
	double PerformanceCollectMs,
	double SnapshotRefreshMs,
	double EditorStatsMs,
	double TotalRefreshMs,
	bool bEditorStatsRefreshed) const
{
	return FString::Printf(
		TEXT("%d,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%.3f,%.3f,%.3f,%llu,%s,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%lld,%s,%s,%d,%d,%d,%d,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%s"),
		SampleIndex,
		ElapsedSeconds,
		static_cast<double>(InDeltaTime),
		CurrentSnapshot.Fps,
		CurrentSnapshot.FrameTimeMs,
		CurrentSnapshot.GameThreadMs,
		CurrentSnapshot.DrawCalls,
		CurrentSnapshot.AverageFrameTimeMs,
		CurrentSnapshot.MinFrameTimeMs,
		CurrentSnapshot.MaxFrameTimeMs,
		CurrentSnapshot.UsedPhysicalMemoryMB,
		*StatCheckPanel::BoolToCsvValue(CurrentEditorSnapshot.bVisibleActorCountAvailable),
		CurrentEditorSnapshot.VisibleActorCount,
		*StatCheckPanel::BoolToCsvValue(CurrentEditorSnapshot.World.bAvailable),
		CurrentEditorSnapshot.World.TotalActorCount,
		CurrentEditorSnapshot.World.HiddenActorCount,
		CurrentEditorSnapshot.World.TickEnabledActorCount,
		CurrentEditorSnapshot.World.PrimitiveComponentCount,
		CurrentEditorSnapshot.World.StaticMeshComponentCount,
		CurrentEditorSnapshot.World.SkeletalMeshComponentCount,
		CurrentEditorSnapshot.World.LightComponentCount,
		CurrentEditorSnapshot.World.MaterialSlotCount,
		CurrentEditorSnapshot.World.ApproxTriangleCount,
		*StatCheckPanel::BoolToCsvValue(CurrentEditorSnapshot.SelectedActor.bHasActor),
		*StatCheckPanel::EscapeCsvString(CurrentEditorSnapshot.SelectedActor.ActorName),
		CurrentEditorSnapshot.SelectedActor.ComponentCount,
		CurrentEditorSnapshot.SelectedActor.PrimitiveComponentCount,
		CurrentEditorSnapshot.SelectedActor.StaticMeshComponentCount,
		CurrentEditorSnapshot.SelectedActor.MaterialSlotCount,
		CurrentEditorSnapshot.SelectedActor.ApproxTriangleCount,
		CurrentEditorSnapshot.SelectedActor.BoundsRadius,
		PerformanceCollectMs,
		SnapshotRefreshMs,
		EditorStatsMs,
		TotalRefreshMs,
		*StatCheckPanel::BoolToCsvValue(bEditorStatsRefreshed));
}

bool SStatCheckPanel::SaveRowsToLogFile(const FString& FilePrefix, const TArray<FString>& Rows)
{
	const FString LogDirectory = GetLogDirectory();
	IFileManager::Get().MakeDirectory(*LogDirectory, true);

	const FString FileName = FString::Printf(
		TEXT("%s_%s.csv"),
		*FilePrefix,
		*FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S")));
	LastSavedRecordingPath = FPaths::Combine(LogDirectory, FileName);

	const bool bSaved = FFileHelper::SaveStringArrayToFile(Rows, *LastSavedRecordingPath);
	LastLogStatusMessage = bSaved
		? FString::Printf(TEXT("Saved: %s"), *LastSavedRecordingPath)
		: FString::Printf(TEXT("Failed to save: %s"), *LastSavedRecordingPath);

	if (RecordingStatusText.IsValid())
	{
		RecordingStatusText->SetText(MakeRecordingStatusText());
	}

	return bSaved;
}

void SStatCheckPanel::AddComparisonRow(
	TArray<FString>& Rows,
	const FString& MetricName,
	double BaselineValue,
	double CurrentValue) const
{
	Rows.Add(FString::Printf(
		TEXT("%s,%.3f,%.3f,%.3f"),
		*StatCheckPanel::EscapeCsvString(MetricName),
		BaselineValue,
		CurrentValue,
		CurrentValue - BaselineValue));
}

void SStatCheckPanel::StartRecordingLog()
{
	if (bIsRecordingLog)
	{
		return;
	}

	bIsRecordingLog = true;
	RecordingStartTime = FPlatformTime::Seconds();
	LastSavedRecordingPath.Empty();
	LastLogStatusMessage.Empty();
	RecordingRows.Reset();
	RecordingRows.Add(MakeLogHeader());

	if (RecordingStatusText.IsValid())
	{
		RecordingStatusText->SetText(MakeRecordingStatusText());
	}

	RefreshSnapshotAndMaybeRecord(FPlatformTime::Seconds(), 0.0f, true);
	EnsureActiveTimerRegistered();
}

void SStatCheckPanel::TickRecordingLog(
	float InDeltaTime,
	double PerformanceCollectMs,
	double SnapshotRefreshMs,
	double EditorStatsMs,
	double TotalRefreshMs,
	bool bEditorStatsRefreshed)
{
	if (!bIsRecordingLog)
	{
		return;
	}

	const double ElapsedSeconds = FPlatformTime::Seconds() - RecordingStartTime;
	AddRecordingSample(
		ElapsedSeconds,
		InDeltaTime,
		PerformanceCollectMs,
		SnapshotRefreshMs,
		EditorStatsMs,
		TotalRefreshMs,
		bEditorStatsRefreshed);

	if (RecordingStatusText.IsValid())
	{
		RecordingStatusText->SetText(MakeRecordingStatusText());
	}

	if (ElapsedSeconds >= StatCheckPanel::RecordingDurationSeconds)
	{
		FinishRecordingLog();
	}
}

void SStatCheckPanel::FinishRecordingLog()
{
	if (!bIsRecordingLog)
	{
		return;
	}

	bIsRecordingLog = false;
	SaveRowsToLogFile(TEXT("StatCheckProfile"), RecordingRows);
}

void SStatCheckPanel::AddRecordingSample(
	double ElapsedSeconds,
	float InDeltaTime,
	double PerformanceCollectMs,
	double SnapshotRefreshMs,
	double EditorStatsMs,
	double TotalRefreshMs,
	bool bEditorStatsRefreshed)
{
	const int32 SampleIndex = FMath::Max(0, RecordingRows.Num() - 1);

	RecordingRows.Add(MakeLogRow(
		SampleIndex,
		ElapsedSeconds,
		InDeltaTime,
		PerformanceCollectMs,
		SnapshotRefreshMs,
		EditorStatsMs,
		TotalRefreshMs,
		bEditorStatsRefreshed));
}

bool SStatCheckPanel::IsDisplayStatEnabled(EDisplayStat DisplayStat) const
{
	switch (DisplayStat)
	{
	case EDisplayStat::Fps:
		return bShowFps;
	case EDisplayStat::FrameTime:
		return bShowFrameTime;
	case EDisplayStat::GameThread:
		return bShowGameThread;
	case EDisplayStat::DrawCalls:
		return bShowDrawCalls;
	case EDisplayStat::FrameAverage:
		return bShowFrameAverage;
	case EDisplayStat::Memory:
		return bShowMemory;
	case EDisplayStat::VisibleActors:
		return bShowVisibleActors;
	case EDisplayStat::SelectedActorCost:
		return bShowSelectedActorCost;
	default:
		return false;
	}
}

void SStatCheckPanel::SetDisplayStatEnabled(EDisplayStat DisplayStat, bool bEnabled)
{
	switch (DisplayStat)
	{
	case EDisplayStat::Fps:
		bShowFps = bEnabled;
		break;
	case EDisplayStat::FrameTime:
		bShowFrameTime = bEnabled;
		break;
	case EDisplayStat::GameThread:
		bShowGameThread = bEnabled;
		break;
	case EDisplayStat::DrawCalls:
		bShowDrawCalls = bEnabled;
		break;
	case EDisplayStat::FrameAverage:
		bShowFrameAverage = bEnabled;
		break;
	case EDisplayStat::Memory:
		bShowMemory = bEnabled;
		break;
	case EDisplayStat::VisibleActors:
		bShowVisibleActors = bEnabled;
		break;
	case EDisplayStat::SelectedActorCost:
		bShowSelectedActorCost = bEnabled;
		break;
	default:
		break;
	}
}

void SStatCheckPanel::RefreshRowVisibility()
{
	if (FpsRow.IsValid())
	{
		FpsRow->SetVisibility(GetStatVisibility(EDisplayStat::Fps));
	}

	if (FrameTimeRow.IsValid())
	{
		FrameTimeRow->SetVisibility(GetStatVisibility(EDisplayStat::FrameTime));
	}

	if (GameThreadRow.IsValid())
	{
		GameThreadRow->SetVisibility(GetStatVisibility(EDisplayStat::GameThread));
	}

	if (DrawCallRow.IsValid())
	{
		DrawCallRow->SetVisibility(GetStatVisibility(EDisplayStat::DrawCalls));
	}

	if (FrameAverageRow.IsValid())
	{
		FrameAverageRow->SetVisibility(GetStatVisibility(EDisplayStat::FrameAverage));
	}

	if (MemoryRow.IsValid())
	{
		MemoryRow->SetVisibility(GetStatVisibility(EDisplayStat::Memory));
	}

	if (VisibleActorsRow.IsValid())
	{
		VisibleActorsRow->SetVisibility(GetStatVisibility(EDisplayStat::VisibleActors));
	}

	if (SelectedActorCostRow.IsValid())
	{
		SelectedActorCostRow->SetVisibility(GetStatVisibility(EDisplayStat::SelectedActorCost));
	}
}

#undef LOCTEXT_NAMESPACE
