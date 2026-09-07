// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "VotVSetupOptions.generated.h"


/**
 * Temporary visual-model data for the Setup Project window.
 * These values do not change the Unreal project yet.
 */
UCLASS(Config=EditorPerProjectUserSettings)
class VOTVMODBUILDHELPER_API UVotVSetupOptions : public UObject
{
	GENERATED_BODY()
	UVotVSetupOptions()
	{
		ModChunkDirectory.Path = TEXT("/Game/Mods/MODNAMEHERE");
	}

public:
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Project Settings",
		meta = (
			DisplayName = "Ensure project name is VotV",
			ToolTip = "The mod loader expects the Unreal project to be named VotV."
		)
	)
	bool bEnsureProjectName = true;

	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Project Settings",
		meta = (
			DisplayName = "Enable Project Chunk ID assignments",
			ToolTip = "Enables Chunk ID assignments in Editor Preferences."
		)
	)
	bool bEnableChunkIdAssignments = true;
	
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Project Settings",
		meta = (
			DisplayName = "Enable Project Generate Chunks Setting",
			ToolTip = "Must be enabled for limited sections of the project to be packaged."
		)
	)
	bool bEnableGenerateChunks = true;
	
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Project Settings",
		meta = (
			DisplayName = "Disable Project Share Material Shader Code",
			ToolTip = "Disable Material Shader Code for VotV."
			)
		)
	bool bDisableMaterialShaderCode = true;
	
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Project Settings",
		meta = (
			DisplayName = "Wish to specify data chunk to export?",
			ToolTip = "Do you wish to specify which folder should be exported as part of your mod?"
		)
	)
	bool bAssignedExportFolder = true;
	
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Project Settings",
		meta = (
			DisplayName = "Mod Chunk Directory:",
			EditCondition = "bAssignedExportFolder",
			ContentDir,
			LongPackageName,
			Tooltip = "Mod Chunk to Export to VotV."
		)
	)
	FDirectoryPath ModChunkDirectory;
	
	UPROPERTY(
	Config,
	EditAnywhere,
	Category = "Project Settings",
	meta = (
		DisplayName = "Mod Chunk ID",
		EditCondition = "bAssignedExportFolder",
		ClampMin = "1",
		UIMin = "1",
		ToolTip = "The Chunk ID assigned to this mod's Primary Asset Label."
	)
)
	int32 ModChunkId = 2000;
	
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Project Settings",
		meta = (
			DisplayName = "Enable Label Assets in My Directory",
			EditCondition = "bAssignedExportFolder",
			ToolTip = "Enable Label Assets in My Directory."
			)
		)
	bool bEnableLabelAssets = true;
	
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Project Settings",
		meta = (
			DisplayName = "Disable Apply Recursively",
			EditCondition = "bAssignedExportFolder",
			ToolTip = "Disable Apply Recursively."
			)
		)
	bool bDisableApplyRecursively = true;
	
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Project Settings",
		meta = (
			DisplayName = "Fix Sound Cooking",
			Tooltip = "By default, sounds do not work out of the box. This option updates DefaultEngine.ini with the appropriate fixes."
		)
	)
	bool bFixSoundCooking = true;
	
	UPROPERTY(
		Config,
	EditAnywhere,
	Category = "Mod Setup",
	meta = (
		DisplayName = "Import Ghost Mappings",
		Tooltip = "Ghost Mappings provided by modestimpala / VotV_ghostmap")
	)
	bool bImportGhostMappings = true;

	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Mod Setup",
		meta = (
			DisplayName = "Ghost Mappings GitHub Repository",
			EditCondition = "bImportGhostMappings",
			ToolTip = "GitHub repository in owner/repository form. Its main branch must contain a Content folder. Matching ghost-mapping files in this project will be updated."
		)
	)
	FString GhostMappingsRepository = TEXT("modestimpala/VotV_ghostmap");
	
	UPROPERTY(
		Config,
	EditAnywhere,
	Category = "Mod Setup",
	meta = (
		DisplayName = "Create modActor in the Mod Chunk to Export",
		ToolTip = "ModActor is the core of your mod. It is the element the mod loader is looking for and initializes on level startup."
	)
)
	bool bCreateModActor = true;
	
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Mod Actor Setup",
		meta = (
			DisplayName = "Create default variables in modActor",
			EditCondition = "bCreateModActor",
			Tooltip = "Create default variables in modActor."
		)
	)
	bool bCreateModActorVars = true;
	
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Mod Actor Setup",
		meta = (
			DisplayName = "Create modAuthor String",
			EditCondition = "bCreateModActor && bCreateModActorVars",
			ToolTip = "Create default variables ModAuthor String in created modActor."
		)
	)
	bool bCreateModAuthorString = true;
	
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Mod Actor Setup",
		meta = (
			DisplayName = "Create modDescription String",
			EditCondition = "bCreateModActor && bCreateModActorVars",
			ToolTip = "Create default variable modDescription String in created modActor."
		)
	)
	bool bCreateModDescriptionString = true;
	
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Mod Actor Setup",
		meta = (
			DisplayName = "Create modVersion String",
			EditCondition = "bCreateModActor && bCreateModActorVars",
			ToolTip = "Create default variable modVersion String in created modActor."
		)
	)
	bool bCreateModVersionString = true;
	
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Mod Actor Setup",
		meta = (
			DisplayName = "Create modButtons String Array",
			EditCondition = "bCreateModActor && bCreateModActorVars",
			ToolTip = "Create default variable modButtons String Array.")
		)
	bool bCreateModButtonsStringArray = true;
	
	UPROPERTY(
		Config,
		EditAnywhere,
		Category = "Mod Actor Setup",
		meta = (
			DisplayName = "Create debugMode Boolean",
			EditCondition = "bCreateModActor && bCreateModActorVars",
			ToolTip = "Create default variable debugMode Boolean in created modActor.")
			)
	bool bCreateDebugModeBoolean = true;
	

	
};
