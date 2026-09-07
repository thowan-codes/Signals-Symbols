#include "Build/VotVBuildRunner.h"

#include "VotVBuildOptions.h"
#include "VotVSetupOptions.h"

#include "Async/Async.h"
#include "Dom/JsonObject.h"
#include "Engine/Blueprint.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "IUATHelperModule.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/App.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UnrealEdMisc.h"

namespace
{
    struct FVotVBuildPlan
    {
        FString ModName;
        FString Version;
        FString SemanticVersion;
        FString BuildType;
        FString DestinationDirectory;
        FString ArchiveDirectory;
        FString PakFileName;
        FString ManifestFileName;
        FString Author;
        FString Description;
        FString WebsiteUrl;
        FString SupportedGameVersion;
        TArray<FString> Dependencies;
        int32 ChunkId = INDEX_NONE;
        bool bWriteManifest = true;
        bool bManifestIncludeModName = true;
        bool bManifestIncludeVersion = true;
        bool bManifestIncludeSupportedGameVersion = true;
        bool bManifestIncludePakFileName = true;
        bool bManifestIncludeTimestamp = true;
        EVotVManifestTemplate ManifestTemplate = EVotVManifestTemplate::Custom;
        EVotVManifestVersionStyle ManifestVersionStyle =
            EVotVManifestVersionStyle::MajorMinorPatch;
        EVotVBuildType BuildConfiguration = EVotVBuildType::Development;
    };

    FString ReplaceToken(
        FString Value,
        const TCHAR* Token,
        const FString& Replacement
    )
    {
        Value.ReplaceInline(Token, *Replacement, ESearchCase::CaseSensitive);
        return Value;
    }

    FString ResolveVersion(const UVotVBuildOptions& Options)
    {
        const FString BuildType =
            Options.BuildType == EVotVBuildType::Development
            ? TEXT("Development")
            : TEXT("Release");
        const FString BuildId = Options.bEnableBuildIds
            ? FString::FromInt(
                Options.BuildType == EVotVBuildType::Development
                ? Options.DevelopmentBuildId
                : Options.ReleaseBuildId
            )
            : TEXT("");

        FString Result = Options.VersionFormat;
        Result = ReplaceToken(
            Result,
            TEXT("{majorVersion}"),
            FString::FromInt(Options.VersionMajor)
        );
        Result = ReplaceToken(
            Result,
            TEXT("{minorVersion}"),
            FString::FromInt(Options.VersionMinor)
        );
        Result = ReplaceToken(
            Result,
            TEXT("{patchVersion}"),
            FString::FromInt(Options.VersionPatch)
        );
        Result = ReplaceToken(Result, TEXT("{batchType}"), BuildType);
        return ReplaceToken(Result, TEXT("{buildID}"), BuildId);
    }

    bool IsSafeFileName(const FString& Value)
    {
        return !Value.IsEmpty() &&
            Value.Equals(FPaths::MakeValidFileName(Value), ESearchCase::CaseSensitive);
    }

    bool CreateBuildPlan(
        UVotVBuildOptions& Options,
        const UVotVSetupOptions& SetupOptions,
        FVotVBuildPlan& OutPlan,
        FString& OutError
    )
    {
        Options.RefreshDerivedValues();

        OutPlan.ModName = Options.ModName.TrimStartAndEnd();
        if (!IsSafeFileName(OutPlan.ModName))
        {
            OutError = TEXT(
                "Mod Name is required and cannot contain characters that are invalid in a file name."
            );
            return false;
        }

        if (Options.ModActor.IsNull())
        {
            OutError = TEXT(
                "Select the ModActor Blueprint that should receive the version and debug-mode values."
            );
            return false;
        }

        if (Options.VersionFormat.Contains(TEXT("{modName}")) ||
            Options.VersionFormat.Contains(TEXT("{version}")) ||
            Options.VersionFormat.Contains(TEXT("{versionSimple}")) ||
            Options.VersionFormat.Contains(TEXT("{versionComplex}")))
        {
            OutError = TEXT(
                "Version Format can only use {majorVersion}, {minorVersion}, "
                "{patchVersion}, {batchType}, and {buildID}. "
                "Use {modName}, {versionSimple}, or {versionComplex} in "
                "Folder Build Name Template instead."
            );
            return false;
        }

        if (SetupOptions.ModChunkId < 1)
        {
            OutError = TEXT("Run Setup Project and choose a valid Mod Chunk ID first.");
            return false;
        }

        const FString DistributionFolder =
            Options.DistributionFolder.Path.TrimStartAndEnd();
        if (DistributionFolder.IsEmpty())
        {
            OutError = TEXT("Choose a Distribution Folder before building.");
            return false;
        }

        const FString FolderName = FPaths::GetCleanFilename(
            Options.BuildDestinationPreview
        );
        if (FolderName.IsEmpty())
        {
            OutError = TEXT("The Folder Build Name Template produced an empty folder name.");
            return false;
        }

        OutPlan.Version = ResolveVersion(Options);
        OutPlan.SemanticVersion = FString::Printf(
            TEXT("%d.%d.%d"),
            Options.VersionMajor,
            Options.VersionMinor,
            Options.VersionPatch
        );
        OutPlan.BuildType = Options.BuildType == EVotVBuildType::Development
            ? TEXT("Development")
            : TEXT("Release");
        OutPlan.BuildConfiguration = Options.BuildType;
        OutPlan.DestinationDirectory = Options.BuildDestinationPreview;
        OutPlan.ArchiveDirectory = FPaths::Combine(
            FPaths::ProjectIntermediateDir(),
            TEXT("VotVModBuildHelper"),
            TEXT("PackageArchive"),
            FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"))
        );
        OutPlan.PakFileName = Options.BuildOutputPakFile;
        OutPlan.ManifestFileName = Options.ManifestTemplate ==
            EVotVManifestTemplate::Thunderstore
            ? TEXT("manifest")
            : FPaths::MakeValidFileName(Options.ManifestName.TrimStartAndEnd());
        if (OutPlan.ManifestFileName.IsEmpty())
        {
            OutPlan.ManifestFileName = TEXT("build");
        }
        OutPlan.ManifestFileName += TEXT(".json");
        OutPlan.ChunkId = SetupOptions.ModChunkId;
        OutPlan.Author = Options.ManifestAuthor.TrimStartAndEnd();
        OutPlan.Description = Options.ManifestDescription.TrimStartAndEnd();
        OutPlan.WebsiteUrl = Options.WebsiteUrl.TrimStartAndEnd();
        OutPlan.SupportedGameVersion = Options.SupportedGameVersion;
        OutPlan.Dependencies = Options.ManifestDependencies;
        OutPlan.bManifestIncludeModName = Options.bManifestIncludeModName;
        OutPlan.bManifestIncludeVersion = Options.bManifestIncludeVersion;
        OutPlan.bManifestIncludeSupportedGameVersion =
            Options.bManifestIncludeSupportedGameVersion;
        OutPlan.bManifestIncludePakFileName = Options.bManifestIncludePakFileName;
        OutPlan.bManifestIncludeTimestamp = Options.bManifestIncludeTimestamp;
        OutPlan.ManifestTemplate = Options.ManifestTemplate;
        OutPlan.ManifestVersionStyle = Options.ManifestVersionStyle;

        if (OutPlan.ManifestTemplate == EVotVManifestTemplate::Thunderstore)
        {
            if (OutPlan.Author.IsEmpty() || OutPlan.WebsiteUrl.IsEmpty() ||
                OutPlan.Description.IsEmpty())
            {
                OutError = TEXT(
                    "Thunderstore requires Author, website_url, and Description in Manifest Options."
                );
                return false;
            }

            if (OutPlan.Description.Len() > 250)
            {
                OutError = TEXT(
                    "Thunderstore descriptions must be 250 characters or fewer."
                );
                return false;
            }
        }

        return true;
    }

    bool SetBlueprintDefaultValue(
        UBlueprint& Blueprint,
        const FName VariableName,
        const FString& Value,
        TArray<UBlueprint*>& OutChangedBlueprints,
        FString& OutWarning
    )
    {
        const int32 VariableIndex = FBlueprintEditorUtils::FindNewVariableIndex(
            &Blueprint,
            VariableName
        );
        if (VariableIndex == INDEX_NONE)
        {
            OutWarning += FString::Printf(
                TEXT("\n%s does not contain a %s variable; left it unchanged."),
                *Blueprint.GetName(),
                *VariableName.ToString()
            );
            return false;
        }

        if (Blueprint.NewVariables[VariableIndex].DefaultValue == Value)
        {
            return false;
        }

        Blueprint.Modify();
        Blueprint.NewVariables[VariableIndex].DefaultValue = Value;
        FBlueprintEditorUtils::MarkBlueprintAsModified(&Blueprint);
        OutChangedBlueprints.AddUnique(&Blueprint);
        return true;
    }

    bool UpdateBuildBlueprints(
        UVotVBuildOptions& Options,
        const FVotVBuildPlan& Plan,
        FString& OutError
    )
    {
        UBlueprint* ModActor = Options.ModActor.LoadSynchronous();
        if (ModActor == nullptr)
        {
            OutError = TEXT("The selected ModActor Blueprint could not be loaded.");
            return false;
        }

        TArray<UBlueprint*> ChangedBlueprints;
        FString Warnings;

        SetBlueprintDefaultValue(
            *ModActor,
            TEXT("modVersion"),
            Plan.Version,
            ChangedBlueprints,
            Warnings
        );
        const bool bDebugValue =
            Options.BuildType == EVotVBuildType::Development &&
            Options.bDebugModeEnabled;
        UBlueprint* DebugBlueprint = ModActor;
        FName DebugVariableName(TEXT("debugMode"));

        if (Options.bOverrideDebugModeInfo)
        {
            if (!Options.DebugModeActorOverride.IsNull())
            {
                DebugBlueprint = Options.DebugModeActorOverride.LoadSynchronous();
            }

            const FString OverrideVariable =
                Options.DebugModeVariableOverride.TrimStartAndEnd();
            if (!OverrideVariable.IsEmpty())
            {
                DebugVariableName = FName(*OverrideVariable);
            }
        }

        if (DebugBlueprint == nullptr)
        {
            OutError = TEXT("The Debug Mode Actor Override could not be loaded.");
            return false;
        }

        SetBlueprintDefaultValue(
            *DebugBlueprint,
            DebugVariableName,
            bDebugValue ? TEXT("true") : TEXT("false"),
            ChangedBlueprints,
            Warnings
        );

        for (UBlueprint* Blueprint : ChangedBlueprints)
        {
            FKismetEditorUtilities::CompileBlueprint(Blueprint);
            Blueprint->MarkPackageDirty();
        }

        if (!FEditorFileUtils::SaveDirtyPackages(true, true, true))
        {
            OutError = TEXT("Build canceled because saving modified project content was canceled or failed.");
            return false;
        }

        if (!Warnings.IsEmpty())
        {
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(
                TEXT("The build will continue, but some optional Blueprint values could not be updated:") +
                Warnings
            ));
        }

        return true;
    }

    bool WriteManifest(
        const FVotVBuildPlan& Plan,
        FString& OutError
    )
    {
        TSharedRef<FJsonObject> Manifest = MakeShared<FJsonObject>();
        const FString ManifestVersion =
            Plan.ManifestVersionStyle == EVotVManifestVersionStyle::MajorMinorPatch
            ? Plan.SemanticVersion
            : Plan.Version;

        if (Plan.ManifestTemplate == EVotVManifestTemplate::Thunderstore)
        {
            Manifest->SetStringField(TEXT("name"), Plan.ModName);
            Manifest->SetStringField(TEXT("version_number"), ManifestVersion);
            Manifest->SetStringField(TEXT("author"), Plan.Author);
            Manifest->SetStringField(TEXT("website_url"), Plan.WebsiteUrl);
            Manifest->SetStringField(TEXT("description"), Plan.Description);

            TArray<TSharedPtr<FJsonValue>> Dependencies;
            for (const FString& Dependency : Plan.Dependencies)
            {
                Dependencies.Add(MakeShared<FJsonValueString>(Dependency));
            }
            Manifest->SetArrayField(TEXT("dependencies"), Dependencies);
        }
        else
        {
            Manifest->SetStringField(TEXT("template"), TEXT("custom"));
            Manifest->SetStringField(TEXT("build_type"), Plan.BuildType);
            Manifest->SetNumberField(TEXT("chunk_id"), Plan.ChunkId);

            if (Plan.bManifestIncludeModName)
            {
                Manifest->SetStringField(TEXT("mod_name"), Plan.ModName);
            }
            if (Plan.bManifestIncludeVersion)
            {
                Manifest->SetStringField(TEXT("version"), Plan.Version);
            }
            if (Plan.bManifestIncludeSupportedGameVersion &&
                !Plan.SupportedGameVersion.IsEmpty())
            {
                Manifest->SetStringField(
                    TEXT("supported_game_version"),
                    Plan.SupportedGameVersion
                );
            }
            if (Plan.bManifestIncludePakFileName)
            {
                Manifest->SetStringField(TEXT("pak_file_name"), Plan.PakFileName);
            }
            if (!Plan.Author.IsEmpty())
            {
                Manifest->SetStringField(TEXT("author"), Plan.Author);
            }
            if (!Plan.WebsiteUrl.IsEmpty())
            {
                Manifest->SetStringField(TEXT("website_url"), Plan.WebsiteUrl);
            }
            if (!Plan.Description.IsEmpty())
            {
                Manifest->SetStringField(TEXT("description"), Plan.Description);
            }
            if (Plan.Dependencies.Num() > 0)
            {
                TArray<TSharedPtr<FJsonValue>> Dependencies;
                for (const FString& Dependency : Plan.Dependencies)
                {
                    Dependencies.Add(MakeShared<FJsonValueString>(Dependency));
                }
                Manifest->SetArrayField(TEXT("dependencies"), Dependencies);
            }
            if (Plan.bManifestIncludeTimestamp)
            {
                Manifest->SetStringField(
                    TEXT("created_timestamp"),
                    FDateTime::UtcNow().ToIso8601()
                );
            }
        }

        FString Json;
        const TSharedRef<TJsonWriter<>> Writer =
            TJsonWriterFactory<>::Create(&Json);
        if (!FJsonSerializer::Serialize(Manifest, Writer) ||
            !FFileHelper::SaveStringToFile(
                Json,
                *FPaths::Combine(Plan.DestinationDirectory, Plan.ManifestFileName)
            ))
        {
            OutError = TEXT("The .pak was built, but the manifest JSON could not be written.");
            return false;
        }

        return true;
    }

    bool FinishBuild(
        const FVotVBuildPlan& Plan,
        const FString& Result,
        FString& OutMessage
    )
    {
        if (!Result.Equals(TEXT("Completed"), ESearchCase::CaseSensitive))
        {
            return false;
        }

        const FString ExpectedPakName = FString::Printf(
            TEXT("pakchunk%d-WindowsNoEditor.pak"),
            Plan.ChunkId
        );
        TArray<FString> FoundPaks;
        IFileManager::Get().FindFilesRecursive(
            FoundPaks,
            *Plan.ArchiveDirectory,
            *ExpectedPakName,
            true,
            false
        );

        if (FoundPaks.Num() != 1)
        {
            OutMessage = FString::Printf(
                TEXT("Packaging succeeded, but the build helper could not find exactly one %s under:\n%s\n\nCheck the Packaging Results and the ModChunkLabel Chunk ID."),
                *ExpectedPakName,
                *Plan.ArchiveDirectory
            );
            return false;
        }

        if (!IFileManager::Get().MakeDirectory(
            *Plan.DestinationDirectory,
            true
        ))
        {
            OutMessage =
                TEXT("Packaging succeeded, but the final distribution folder could not be created:\n") +
                Plan.DestinationDirectory;
            return false;
        }

        const FString FinalPakPath = FPaths::Combine(
            Plan.DestinationDirectory,
            Plan.PakFileName
        );
        if (IFileManager::Get().Copy(
            *FinalPakPath,
            *FoundPaks[0],
            true,
            true
        ) != COPY_OK)
        {
            OutMessage =
                TEXT("Packaging succeeded, but the generated .pak could not be copied to:\n") +
                FinalPakPath;
            return false;
        }

        FString ManifestError;
        const bool bManifestWritten = WriteManifest(
            Plan,
            ManifestError
        );
        OutMessage = bManifestWritten
            ? FString::Printf(
                TEXT("Build complete.\n\n.pak: %s\nManifest: %s"),
                *FinalPakPath,
                *FPaths::Combine(Plan.DestinationDirectory, Plan.ManifestFileName)
            )
            : FString::Printf(
                TEXT("Build completed, but there was a follow-up problem:\n%s\n\n.pak: %s"),
                *ManifestError,
                *FinalPakPath
            );
        return true;
    }
}

bool FVotVBuildRunner::Start(
    UVotVBuildOptions& BuildOptions,
    const UVotVSetupOptions& SetupOptions,
    FString& OutError
)
{
    FVotVBuildPlan Plan;
    if (!CreateBuildPlan(BuildOptions, SetupOptions, Plan, OutError))
    {
        return false;
    }

    if (!UpdateBuildBlueprints(BuildOptions, Plan, OutError))
    {
        return false;
    }

    BuildOptions.SaveConfig();
    IFileManager::Get().MakeDirectory(*Plan.ArchiveDirectory, true);

    const FString ProjectPath = FPaths::ConvertRelativePathToFull(
        FPaths::GetProjectFilePath()
    );
    const FString InstalledEngineArgument = FApp::IsEngineInstalled()
        ? TEXT(" -installed")
        : TEXT("");
    const FString CommandLine = FString::Printf(
        TEXT("-ScriptsForProject=\"%s\" BuildCookRun -nop4%s -project=\"%s\" -build -cook -stage -archive -archivedirectory=\"%s\" -package -pak -platform=Win64 -clientconfig=Development -ue4exe=\"%s\" -utf8output"),
        *ProjectPath,
        *InstalledEngineArgument,
        *ProjectPath,
        *Plan.ArchiveDirectory,
        *FUnrealEdMisc::Get().GetExecutableForCommandlets()
    );

    const TWeakObjectPtr<UVotVBuildOptions> WeakBuildOptions(&BuildOptions);

    IUATHelperModule::Get().CreateUatTask(
        CommandLine,
        FText::FromString(TEXT("Windows (64-bit)")),
        FText::FromString(TEXT("Packaging VotV mod")),
        FText::FromString(TEXT("Package Mod")),
        nullptr,
        [Plan, WeakBuildOptions](FString Result, double)
        {
            AsyncTask(ENamedThreads::GameThread, [Plan, Result, WeakBuildOptions]()
            {
                FString CompletionMessage;
                const bool bPakCopied = FinishBuild(
                    Plan,
                    Result,
                    CompletionMessage
                );

                if (bPakCopied && WeakBuildOptions.IsValid())
                {
                    UVotVBuildOptions* CompletedBuildOptions =
                        WeakBuildOptions.Get();
                    if (CompletedBuildOptions->bEnableBuildIds)
                    {
                        if (Plan.BuildConfiguration == EVotVBuildType::Development)
                        {
                            ++CompletedBuildOptions->DevelopmentBuildId;
                        }
                        else
                        {
                            ++CompletedBuildOptions->ReleaseBuildId;
                        }

                        CompletedBuildOptions->RefreshDerivedValues();
                        CompletedBuildOptions->SaveConfig();
                        CompletionMessage += TEXT(
                            "\n\nThe successful build advanced the active Build ID."
                        );
                    }
                }

                if (!CompletionMessage.IsEmpty())
                {
                    FMessageDialog::Open(
                        EAppMsgType::Ok,
                        FText::FromString(CompletionMessage)
                    );
                }
            });
        },
        Plan.DestinationDirectory
    );

    return true;
}
