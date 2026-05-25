// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Templates/SharedPointer.h"

class FSpawnTabArgs;
class SDockTab;

class FStatCheckEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedRef<SDockTab> SpawnStatCheckTab(const FSpawnTabArgs& SpawnTabArgs);
	void RegisterMenus();
	void OpenStatCheckTab();
};
