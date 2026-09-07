// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "UObject/StrongObjectPtr.h"
#include "VotVSetupOptions.h"
#include "VotVBuildOptions.h"

class FToolBarBuilder;
class FMenuBuilder;

class FVotVModBuildHelperModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();
	
private:

	void RegisterMenus();

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);
	FReply OpenSetupWindow();
	
	void RunSetup();
	void RunBuild();
	void FinishSetup(TArray<FString> Results, bool bRestartRequired);
	void ApplyProjectSettings(
	TArray<FString>& Results,
	bool& bRestartRequired
);

private:
	TSharedPtr<class FUICommandList> PluginCommands;
	TStrongObjectPtr<UVotVSetupOptions> SetupOptions;
	TStrongObjectPtr<UVotVBuildOptions> BuildOptions;
};
