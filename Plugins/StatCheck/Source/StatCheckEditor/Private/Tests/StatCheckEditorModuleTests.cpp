// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"

#if WITH_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStatCheckEditorModuleLoadsTest,
	"StatCheck.Editor.Module.Loads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStatCheckEditorModuleLoadsTest::RunTest(const FString& Parameters)
{
	const FName ModuleName(TEXT("StatCheckEditor"));
	FModuleManager& ModuleManager = FModuleManager::Get();

	IModuleInterface* Module = ModuleManager.GetModule(ModuleName);

	if (Module == nullptr)
	{
		Module = ModuleManager.LoadModule(ModuleName);
	}

	TestNotNull(TEXT("StatCheckEditor module should be available."), Module);
	TestTrue(TEXT("StatCheckEditor module should be loaded."), ModuleManager.IsModuleLoaded(ModuleName));

	return true;
}

#endif
