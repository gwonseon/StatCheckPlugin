// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"

#if WITH_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStatCheckModuleLoadsTest,
	"StatCheck.Module.Loads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStatCheckModuleLoadsTest::RunTest(const FString& Parameters)
{
	const FName ModuleName(TEXT("StatCheck"));
	FModuleManager& ModuleManager = FModuleManager::Get();

	IModuleInterface* Module = ModuleManager.GetModule(ModuleName);

	if (Module == nullptr)
	{
		Module = ModuleManager.LoadModule(ModuleName);
	}

	TestNotNull(TEXT("StatCheck module should be available."), Module);
	TestTrue(TEXT("StatCheck module should be loaded."), ModuleManager.IsModuleLoaded(ModuleName));

	return true;
}

#endif
