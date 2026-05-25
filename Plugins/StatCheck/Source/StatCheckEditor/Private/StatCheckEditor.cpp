// Copyright Epic Games, Inc. All Rights Reserved.

#include "StatCheckEditor.h"

#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "StatCheckPerformanceCollector.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "Widgets/SStatCheckPanel.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FStatCheckEditorModule"

namespace StatCheckEditor
{
	static const FName TabName(TEXT("StatCheck"));

	static FStatCheckPerformanceSnapshot MakePreviewSnapshot()
	{
		return FStatCheckPerformanceCollector::CollectSnapshot();
	}
}

void FStatCheckEditorModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		StatCheckEditor::TabName,
		FOnSpawnTab::CreateRaw(this, &FStatCheckEditorModule::SpawnStatCheckTab))
		.SetDisplayName(LOCTEXT("StatCheckTabTitle", "StatCheck"))
		.SetTooltipText(LOCTEXT("StatCheckTabTooltip", "Open the StatCheck performance monitor."))
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.GameSettings"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FStatCheckEditorModule::RegisterMenus));
}

void FStatCheckEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(StatCheckEditor::TabName);
}

TSharedRef<SDockTab> FStatCheckEditorModule::SpawnStatCheckTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SStatCheckPanel)
			.InitialSnapshot(StatCheckEditor::MakePreviewSnapshot())
		];
}

void FStatCheckEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	FToolMenuSection& Section = WindowMenu->FindOrAddSection("WindowLayout");

	Section.AddMenuEntry(
		"OpenStatCheckWindow",
		LOCTEXT("OpenStatCheckWindowLabel", "StatCheck"),
		LOCTEXT("OpenStatCheckWindowTooltip", "Open the StatCheck performance monitor window."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.GameSettings"),
		FUIAction(FExecuteAction::CreateRaw(this, &FStatCheckEditorModule::OpenStatCheckTab)));
}

void FStatCheckEditorModule::OpenStatCheckTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(StatCheckEditor::TabName);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FStatCheckEditorModule, StatCheckEditor)
