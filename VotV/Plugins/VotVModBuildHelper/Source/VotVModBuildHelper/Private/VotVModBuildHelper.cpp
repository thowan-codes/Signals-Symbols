// Copyright Epic Games, Inc. All Rights Reserved.

#include "VotVModBuildHelper.h"
#include "VotVModBuildHelperStyle.h"
#include "VotVModBuildHelperCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SWindow.h"
#include "Styling/CoreStyle.h"
#include "ToolMenus.h"
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Misc/App.h"
#include "Misc/MessageDialog.h"
#include "Settings/EditorExperimentalSettings.h"
#include "Settings/ProjectPackagingSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "UnrealEdMisc.h"
#include "Misc/Paths.h"
#include "Setup/VotVSetupWindow.h"
#include "Setup/VotVModContentCreator.h"
#include "Setup/VotVGhostMappingsInstaller.h"
#include "Build/SVotVBuildPanel.h"
#include "Build/VotVBuildRunner.h"


static const FName VotVModBuildHelperTabName("VotVModBuildHelper");

#define LOCTEXT_NAMESPACE "FVotVModBuildHelperModule"
DEFINE_LOG_CATEGORY_STATIC(LogVotVModBuildHelper, Log, All);

void FVotVModBuildHelperModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	UE_LOG(LogVotVModBuildHelper, Log, TEXT("VotV Mod Build Helper loaded."));
	
	FVotVModBuildHelperStyle::Initialize();
	FVotVModBuildHelperStyle::ReloadTextures();

	FVotVModBuildHelperCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FVotVModBuildHelperCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FVotVModBuildHelperModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FVotVModBuildHelperModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(VotVModBuildHelperTabName, FOnSpawnTab::CreateRaw(this, &FVotVModBuildHelperModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FVotVModBuildHelperTabTitle", "VotVModBuildHelper"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
	
	
}

void FVotVModBuildHelperModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FVotVModBuildHelperStyle::Shutdown();

	FVotVModBuildHelperCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(VotVModBuildHelperTabName);
}

TSharedRef<SDockTab> FVotVModBuildHelperModule::OnSpawnPluginTab(
	const FSpawnTabArgs& SpawnTabArgs
)
{
	if (!BuildOptions.IsValid())
	{
		BuildOptions = TStrongObjectPtr<UVotVBuildOptions>(
			GetMutableDefault<UVotVBuildOptions>()
		);
	}

	BuildOptions->RefreshDerivedValues();

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SVotVBuildPanel)
			.BuildOptions(BuildOptions.Get())
			.OnOpenSetup(
				FVotVOpenSetupDelegate::CreateLambda([this]()
				{
					OpenSetupWindow();
				})
			)
			.OnRunBuild(
				FVotVRunBuildDelegate::CreateRaw(
					this,
					&FVotVModBuildHelperModule::RunBuild
				)
			)
		];
}

void FVotVModBuildHelperModule::RunBuild()
{
	if (!BuildOptions.IsValid())
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(TEXT(
				"Build options are not available. Reopen the VotV Mod Build Helper tab."
			))
		);
		return;
	}

	if (!SetupOptions.IsValid())
	{
		SetupOptions = TStrongObjectPtr<UVotVSetupOptions>(
			GetMutableDefault<UVotVSetupOptions>()
		);
	}

	FString BuildError;
	if (!FVotVBuildRunner::Start(
		*BuildOptions,
		*SetupOptions,
		BuildError
	))
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(BuildError));
	}
}

FReply FVotVModBuildHelperModule::OpenSetupWindow()
{
	if (!SetupOptions.IsValid())
	{
		SetupOptions = TStrongObjectPtr<UVotVSetupOptions>(
			GetMutableDefault<UVotVSetupOptions>()
		);
	}

	FVotVSetupWindow::Open(
		SetupOptions.Get(),
		FVotVRunSetupDelegate::CreateRaw(
			this,
			&FVotVModBuildHelperModule::RunSetup
		)
	);

	return FReply::Handled();
}

void FVotVModBuildHelperModule::RunSetup()
{
	if (!SetupOptions.IsValid())
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(TEXT(
				"Setup options are not available. Reopen the Setup Project window."
			))
		);
		return;
	}

	TArray<FString> Results;
	bool bRestartRequired = false;

	SetupOptions->SaveConfig();
	
	ApplyProjectSettings(Results, bRestartRequired);

	FVotVModContentCreator::CreateOrUpdate(
		*SetupOptions,
		Results
	);

	// Downloading is asynchronous.  Wait to show the final result and restart
	// prompt until the archive has either been merged or reported as failed.
	if (SetupOptions->bImportGhostMappings ||
		SetupOptions->bRemoveUnsupportedAudioDeviceMapping)
	{
		FVotVGhostMappingsInstaller::DownloadAndInstall(
			*SetupOptions,
			FVotVGhostMappingsCompleteDelegate::CreateLambda(
				[this, Results, bRestartRequired](
					bool bGhostMappingsImported,
					const FString& GhostMappingsResult
				) mutable
				{
					Results.Add(GhostMappingsResult);
					FinishSetup(
						MoveTemp(Results),
						bRestartRequired || bGhostMappingsImported
					);
				}
			)
		);
		return;
	}

	FinishSetup(MoveTemp(Results), bRestartRequired);
}

void FVotVModBuildHelperModule::FinishSetup(
	TArray<FString> Results,
	bool bRestartRequired
)
{
	const FString Summary = FString::Join(Results, TEXT("\n"));

	if (bRestartRequired)
	{
		const FString RestartPrompt = Summary +
			TEXT("\n\nUnreal Editor needs to restart before these changes can fully take effect.\n\nRestart now?");

		const EAppReturnType::Type Answer = FMessageDialog::Open(
			EAppMsgType::YesNo,
			FText::FromString(RestartPrompt)
		);

		if (Answer == EAppReturnType::Yes)
		{
			FUnrealEdMisc::Get().RestartEditor(false);
		}
	}
	else
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString(Summary)
		);
	}
}

void FVotVModBuildHelperModule::ApplyProjectSettings(
	TArray<FString>& Results,
	bool& bRestartRequired
)
{
    // Project name: verify, but do not attempt an automatic rename.
    if (SetupOptions->bEnsureProjectName)
    {
        const FString ProjectName = FApp::GetProjectName();

        if (ProjectName.Equals(TEXT("VotV"), ESearchCase::CaseSensitive))
        {
            Results.Add(TEXT("Project name: VotV (correct)."));
        }
        else
        {
            Results.Add(FString::Printf(
                TEXT("Project name: %s. Please rename project to VotV."),
                *ProjectName
            ));
        }
    }

    // Editor Preferences: Allow Chunk ID Assignments.
    if (SetupOptions->bEnableChunkIdAssignments)
    {
        UEditorExperimentalSettings* EditorPreferences =
            GetMutableDefault<UEditorExperimentalSettings>();

        if (!EditorPreferences->bContextMenuChunkAssignments)
        {
            EditorPreferences->bContextMenuChunkAssignments = true;
            EditorPreferences->SaveConfig();

            Results.Add(TEXT("Enabled Allow Chunk ID Assignments."));
        }
        else
        {
            Results.Add(TEXT("Allow Chunk ID Assignments was already enabled."));
        }
    }

    UProjectPackagingSettings* PackagingSettings =
        GetMutableDefault<UProjectPackagingSettings>();

    bool bPackagingSettingsChanged = false;

    if (SetupOptions->bEnableGenerateChunks)
    {
        if (!PackagingSettings->bGenerateChunks)
        {
            PackagingSettings->bGenerateChunks = true;
            bPackagingSettingsChanged = true;

            Results.Add(TEXT("Enabled Generate Chunks."));
        }
        else
        {
            Results.Add(TEXT("Generate Chunks was already enabled."));
        }
    }

    if (SetupOptions->bDisableMaterialShaderCode)
    {
        if (PackagingSettings->bShareMaterialShaderCode)
        {
            PackagingSettings->bShareMaterialShaderCode = false;
            bPackagingSettingsChanged = true;

            Results.Add(TEXT("Disabled Share Material Shader Code."));
        }
        else
        {
            Results.Add(TEXT("Share Material Shader Code was already disabled."));
        }
    }

	// Add the user-selected mod directory to Additional Asset Directories to Cook.
	if (SetupOptions->bAssignedExportFolder &&
	!SetupOptions->ModChunkDirectory.Path.IsEmpty())
	{
		const FString DirectoryToCook = SetupOptions->ModChunkDirectory.Path;
		
		// Remove the old broad /Game entry created by earlier versions of setup.
		// Keep it only if the user intentionally selected /Game.
		if (!DirectoryToCook.Equals(TEXT("/Game"), ESearchCase::IgnoreCase))
		{
			const int32 RemovedDirectories =
				PackagingSettings->DirectoriesToAlwaysCook.RemoveAll(
					[](const FDirectoryPath& Directory)
					{
						return Directory.Path.Equals(
							TEXT("/Game"),
							ESearchCase::IgnoreCase
						);
					}
				);

			if (RemovedDirectories > 0)
			{
				bPackagingSettingsChanged = true;

				Results.Add(TEXT(
					"Removed the previous /Game cooking directory."
				));
			}
		}

		const bool bDirectoryAlreadyAdded =
			PackagingSettings->DirectoriesToAlwaysCook.ContainsByPredicate(
				[&DirectoryToCook](const FDirectoryPath& Directory)
				{
					return Directory.Path.Equals(
						DirectoryToCook,
						ESearchCase::IgnoreCase
					);
				}
			);

		if (!bDirectoryAlreadyAdded)
		{
			FDirectoryPath DirectoryPath;
			DirectoryPath.Path = DirectoryToCook;

			PackagingSettings->DirectoriesToAlwaysCook.Add(DirectoryPath);
			bPackagingSettingsChanged = true;

			Results.Add(FString::Printf(
				TEXT("Added %s to Additional Asset Directories to Cook."),
				*DirectoryToCook
			));
		}
		else
		{
			Results.Add(FString::Printf(
				TEXT("%s was already in Additional Asset Directories to Cook."),
				*DirectoryToCook
			));
		}
	}

    

	if (bPackagingSettingsChanged)
	{
		PackagingSettings->UpdateDefaultConfigFile();
	}

    if (SetupOptions->bAssignedExportFolder)
    {
        Results.Add(FString::Printf(
            TEXT("Mod chunk directory saved for the next step: %s"),
            *SetupOptions->ModChunkDirectory.Path
        ));
    }

	if (SetupOptions->bFixSoundCooking)
	{
		const TCHAR* SoundSettingsSection =
			TEXT("/Script/WindowsTargetPlatform.WindowsTargetSettings");

		const FString DefaultEngineIni = FPaths::Combine(
			FPaths::ProjectConfigDir(),
			TEXT("DefaultEngine.ini")
		);

		// Load the source project config file so we write directly to it.
		GConfig->LoadFile(DefaultEngineIni);

		bool bSoundSettingsChanged = false;

		auto SetSoundSettingToZero = [&](const TCHAR* SettingName)
		{
			float CurrentValue = 0.0f;

			const bool bSettingExists = GConfig->GetFloat(
				SoundSettingsSection,
				SettingName,
				CurrentValue,
				DefaultEngineIni
			);

			if (!bSettingExists || !FMath::IsNearlyZero(CurrentValue))
			{
				GConfig->SetFloat(
					SoundSettingsSection,
					SettingName,
					0.0f,
					DefaultEngineIni
				);

				bSoundSettingsChanged = true;
			}
		};

		SetSoundSettingToZero(TEXT("MaxSampleRate"));
		SetSoundSettingToZero(TEXT("HighSampleRate"));
		SetSoundSettingToZero(TEXT("MedSampleRate"));
		SetSoundSettingToZero(TEXT("LowSampleRate"));
		SetSoundSettingToZero(TEXT("MinSampleRate"));
		SetSoundSettingToZero(TEXT("CompressionQualityModifier"));
		SetSoundSettingToZero(TEXT("AutoStreamingThreshold"));

		if (bSoundSettingsChanged)
		{
			GConfig->Flush(false, DefaultEngineIni);

			bRestartRequired = true;

			Results.Add(FString::Printf(
				TEXT("Applied VotV sound cooking settings to: %s"),
				*DefaultEngineIni
			));
		}
		else
		{
			Results.Add(TEXT("VotV sound cooking settings were already correct."));
		}
	}
}

void FVotVModBuildHelperModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(VotVModBuildHelperTabName);
}

void FVotVModBuildHelperModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FVotVModBuildHelperCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FVotVModBuildHelperCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVotVModBuildHelperModule, VotVModBuildHelper)
