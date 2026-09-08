#include "Setup/VotVGhostMappingsInstaller.h"

#include "VotVSetupOptions.h"

#include "AssetRegistryModule.h"
#include "Async/Async.h"
#include "EdGraph/EdGraph.h"
#include "Engine/Blueprint.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformProcess.h"
#include "IAssetRegistry.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/Paths.h"
#include "Misc/ScopedSlowTask.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Notifications/SNotificationList.h"

namespace
{
    bool bInstallationInProgress = false;
    TSharedPtr<SNotificationItem> ProgressNotification;

    bool IsSafeGitHubSegment(const FString& Segment)
    {
        if (Segment.IsEmpty())
        {
            return false;
        }

        for (const TCHAR Character : Segment)
        {
            if (!FChar::IsAlnum(Character) && Character != TEXT('-') &&
                Character != TEXT('_') && Character != TEXT('.'))
            {
                return false;
            }
        }

        return true;
    }

    bool ParseRepository(
        const FString& Value,
        FString& OutOwner,
        FString& OutRepository
    )
    {
        TArray<FString> Parts;
        Value.TrimStartAndEnd().ParseIntoArray(Parts, TEXT("/"), true);

        if (Parts.Num() != 2 || !IsSafeGitHubSegment(Parts[0]) ||
            !IsSafeGitHubSegment(Parts[1]))
        {
            return false;
        }

        OutOwner = Parts[0];
        OutRepository = Parts[1];
        return true;
    }

    void ShowProgress(const FString& Message)
    {
        if (!ProgressNotification.IsValid())
        {
            FNotificationInfo Info(FText::FromString(Message));
            Info.bFireAndForget = false;
            Info.ExpireDuration = 0.0f;
            Info.FadeOutDuration = 0.25f;
            ProgressNotification =
                FSlateNotificationManager::Get().AddNotification(Info);
        }

        if (ProgressNotification.IsValid())
        {
            ProgressNotification->SetText(FText::FromString(Message));
        }
    }

    void Complete(
        FVotVGhostMappingsCompleteDelegate OnComplete,
        bool bSucceeded,
        const FString& Message
    )
    {
        bInstallationInProgress = false;

        if (ProgressNotification.IsValid())
        {
            ProgressNotification->SetText(FText::FromString(Message));
            ProgressNotification->SetCompletionState(
                bSucceeded
                    ? SNotificationItem::CS_Success
                    : SNotificationItem::CS_Fail
            );
            ProgressNotification->ExpireAndFadeout();
            ProgressNotification.Reset();
        }

        OnComplete.ExecuteIfBound(bSucceeded, Message);
    }

    bool RunGit(
        const FString& Arguments,
        FString& OutError,
        FString* OutOutput = nullptr
    )
    {
        int32 ExitCode = INDEX_NONE;
        FString Output;
        const bool bStarted = FPlatformProcess::ExecProcess(
            TEXT("git.exe"),
            *Arguments,
            &ExitCode,
            &Output,
            &OutError
        );

        if (!bStarted || ExitCode != 0)
        {
            OutError = FString::Printf(
                TEXT("git started: %s, exit code: %d. %s"),
                bStarted ? TEXT("yes") : TEXT("no"),
                ExitCode,
                *OutError.Right(1000)
            );
            return false;
        }

        if (OutOutput != nullptr)
        {
            *OutOutput = Output;
        }

        return true;
    }

    void FindAssetFiles(
        const FString& SourceContentDirectory,
        TArray<FString>& OutDestinationFiles
    )
    {
        TArray<FString> SourceFiles;
        IFileManager::Get().FindFilesRecursive(
            SourceFiles,
            *SourceContentDirectory,
            TEXT("*.uasset"),
            true,
            false,
            false
        );
        IFileManager::Get().FindFilesRecursive(
            SourceFiles,
            *SourceContentDirectory,
            TEXT("*.umap"),
            true,
            false,
            false
        );

        OutDestinationFiles.Reserve(SourceFiles.Num());
        for (const FString& SourceFile : SourceFiles)
        {
            FString RelativeFile = SourceFile;
            FPaths::MakePathRelativeTo(
                RelativeFile,
                *SourceContentDirectory
            );
            OutDestinationFiles.Add(FPaths::Combine(
                FPaths::ProjectContentDir(),
                RelativeFile
            ));
        }
    }

    int32 RemoveNamedGraphs(
        UBlueprint* Blueprint,
        const FName GraphName
    )
    {
        if (Blueprint == nullptr)
        {
            return 0;
        }

        TArray<UEdGraph*> GraphsToRemove;
        auto CollectNamedGraphs =
            [&GraphsToRemove, GraphName](const TArray<UEdGraph*>& Graphs)
            {
                for (UEdGraph* Graph : Graphs)
                {
                    if (Graph != nullptr && Graph->GetFName() == GraphName)
                    {
                        GraphsToRemove.AddUnique(Graph);
                    }
                }
            };

        CollectNamedGraphs(Blueprint->FunctionGraphs);
        CollectNamedGraphs(Blueprint->EventGraphs);
        CollectNamedGraphs(Blueprint->MacroGraphs);
        CollectNamedGraphs(Blueprint->DelegateSignatureGraphs);

        if (GraphsToRemove.Num() == 0)
        {
            return 0;
        }

        Blueprint->Modify();
        for (UEdGraph* Graph : GraphsToRemove)
        {
            FBlueprintEditorUtils::RemoveGraph(
                Blueprint,
                Graph,
                EGraphRemoveFlags::MarkTransient
            );
        }

        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
        FKismetEditorUtilities::CompileBlueprint(Blueprint);
        Blueprint->MarkPackageDirty();
        return GraphsToRemove.Num();
    }

    FString RemoveUnsupportedAudioDeviceMappings()
    {
        constexpr const TCHAR* AudioDeviceFunctionName = TEXT("getAudioDevices");
        const FName GraphName(AudioDeviceFunctionName);

        UBlueprint* CommunicationsInterface = Cast<UBlueprint>(
            StaticLoadObject(
                UBlueprint::StaticClass(),
                nullptr,
                TEXT("/Game/main/interfaces/int_coms.int_coms"),
                nullptr,
                LOAD_NoWarn
            )
        );
        UBlueprint* MainGameMode = Cast<UBlueprint>(
            StaticLoadObject(
                UBlueprint::StaticClass(),
                nullptr,
                TEXT("/Game/main/mainGamemode.mainGamemode"),
                nullptr,
                LOAD_NoWarn
            )
        );

        const int32 RemovedInterfaceGraphs = RemoveNamedGraphs(
            CommunicationsInterface,
            GraphName
        );
        const int32 RemovedGameModeGraphs = RemoveNamedGraphs(
            MainGameMode,
            GraphName
        );
        const int32 RemovedGraphs =
            RemovedInterfaceGraphs + RemovedGameModeGraphs;

        return RemovedGraphs > 0
            ? FString::Printf(
                TEXT(" Removed %d unsupported getAudioDevices mapping graph(s); mainGamemode pLog functions remain available."),
                RemovedGraphs
            )
            : TEXT("");
    }
}

void FVotVGhostMappingsInstaller::DownloadAndInstall(
    const UVotVSetupOptions& SetupOptions,
    FVotVGhostMappingsCompleteDelegate OnComplete
)
{
#if !PLATFORM_WINDOWS
    Complete(
        OnComplete,
        false,
        TEXT("Skipped ghost mappings: automatic download is currently supported on Windows only.")
    );
    return;
#else
    if (bInstallationInProgress)
    {
        Complete(
            OnComplete,
            false,
            TEXT("Skipped ghost mappings: a ghost-mappings download is already in progress.")
        );
        return;
    }

    const bool bUpdateGhostMappings = SetupOptions.bImportGhostMappings;
    const bool bRemoveUnsupportedAudioDeviceMapping =
        SetupOptions.bRemoveUnsupportedAudioDeviceMapping;

    // This repair is deliberately independent of downloading.  It lets an
    // already-imported project remove the unsupported mapping without network
    // access or another Git LFS transfer.
    if (!bUpdateGhostMappings)
    {
        bInstallationInProgress = true;
        ShowProgress(TEXT("Ghost mappings: applying compatibility settings..."));

        AsyncTask(ENamedThreads::GameThread,
            [OnComplete, bRemoveUnsupportedAudioDeviceMapping]()
            {
                const FString CompatibilityResult =
                    bRemoveUnsupportedAudioDeviceMapping
                    ? RemoveUnsupportedAudioDeviceMappings()
                    : TEXT("");

                Complete(
                    OnComplete,
                    true,
                    CompatibilityResult.IsEmpty()
                    ? TEXT("Ghost mappings were not downloaded or updated. Existing mappings were left unchanged.")
                    : FString::Printf(
                        TEXT("Ghost mappings were not downloaded or updated.%s Save All, then restart the editor."),
                        *CompatibilityResult
                    )
                );
            }
        );
        return;
    }

    FString Owner;
    FString Repository;
    if (!ParseRepository(SetupOptions.GhostMappingsRepository, Owner, Repository))
    {
        Complete(
            OnComplete,
            false,
            TEXT("Skipped ghost mappings: enter the GitHub repository as owner/repository (for example modestimpala/VotV_ghostmap).")
        );
        return;
    }

    // Saved is intentional: Intermediate may be deleted by a clean build, but
    // this clone is the cache that makes later setup runs incremental.
    const FString CacheDirectory = FPaths::Combine(
        FPaths::ProjectSavedDir(),
        TEXT("VotVModBuildHelper"),
        TEXT("GhostMappings"),
        Owner,
        Repository
    );
    const FString RepositoryUrl = FString::Printf(
        TEXT("https://github.com/%s/%s.git"),
        *Owner,
        *Repository
    );
    IPlatformFile& PlatformFile =
        FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.CreateDirectoryTree(*FPaths::GetPath(CacheDirectory)))
    {
        Complete(
            OnComplete,
            false,
            TEXT("Skipped ghost mappings: could not create the local Git cache folder.")
        );
        return;
    }

    bInstallationInProgress = true;
    ShowProgress(TEXT("Ghost mappings: checking Git LFS..."));

    Async(EAsyncExecution::ThreadPool,
        [OnComplete, Repository, RepositoryUrl, CacheDirectory, bRemoveUnsupportedAudioDeviceMapping]()
        {
            FString GitError;
            if (!RunGit(TEXT("lfs version"), GitError))
            {
                AsyncTask(ENamedThreads::GameThread,
                    [OnComplete, GitError]()
                    {
                        Complete(
                            OnComplete,
                            false,
                            FString::Printf(
                                TEXT("Skipped ghost mappings: Git LFS is required to download the real Unreal assets. Install Git for Windows with Git LFS, then retry. %s"),
                                *GitError
                            )
                        );
                    }
                );
                return;
            }

            IPlatformFile& BackgroundPlatformFile =
                FPlatformFileManager::Get().GetPlatformFile();
            bool bRepositoryChanged = !BackgroundPlatformFile.DirectoryExists(*CacheDirectory);

            if (bRepositoryChanged)
            {
                AsyncTask(ENamedThreads::GameThread, []()
                {
                    ShowProgress(TEXT("Ghost mappings: cloning the repository..."));
                });

                const FString CloneArguments = FString::Printf(
                    TEXT("clone --depth 1 \"%s\" \"%s\""),
                    *RepositoryUrl,
                    *CacheDirectory
                );
                if (!RunGit(CloneArguments, GitError))
                {
                    AsyncTask(ENamedThreads::GameThread,
                        [OnComplete, GitError]()
                        {
                            Complete(
                                OnComplete,
                                false,
                                FString::Printf(
                                    TEXT("Skipped ghost mappings: could not clone the GitHub repository. %s"),
                                    *GitError
                                )
                            );
                        }
                    );
                    return;
                }
            }
            else
            {
                AsyncTask(ENamedThreads::GameThread, []()
                {
                    ShowProgress(TEXT("Ghost mappings: checking for repository updates..."));
                });

                const FString FetchArguments = FString::Printf(
                    TEXT("-C \"%s\" fetch --quiet origin main"),
                    *CacheDirectory
                );
                if (!RunGit(FetchArguments, GitError))
                {
                    AsyncTask(ENamedThreads::GameThread,
                        [OnComplete, GitError]()
                        {
                            Complete(
                                OnComplete,
                                false,
                                FString::Printf(
                                    TEXT("Skipped ghost mappings: could not check the GitHub repository for updates. %s"),
                                    *GitError
                                )
                            );
                        }
                    );
                    return;
                }

                FString LocalRevision;
                FString RemoteRevision;
                const FString LocalRevisionArguments = FString::Printf(
                    TEXT("-C \"%s\" rev-parse HEAD"),
                    *CacheDirectory
                );
                const FString RemoteRevisionArguments = FString::Printf(
                    TEXT("-C \"%s\" rev-parse origin/main"),
                    *CacheDirectory
                );
                if (!RunGit(LocalRevisionArguments, GitError, &LocalRevision) ||
                    !RunGit(RemoteRevisionArguments, GitError, &RemoteRevision))
                {
                    AsyncTask(ENamedThreads::GameThread,
                        [OnComplete, GitError]()
                        {
                            Complete(
                                OnComplete,
                                false,
                                FString::Printf(
                                    TEXT("Skipped ghost mappings: could not read the cached repository revision. %s"),
                                    *GitError
                                )
                            );
                        }
                    );
                    return;
                }

                bRepositoryChanged =
                    !LocalRevision.TrimStartAndEnd().Equals(
                        RemoteRevision.TrimStartAndEnd(),
                        ESearchCase::CaseSensitive
                    );

                if (bRepositoryChanged)
                {
                    const FString PullArguments = FString::Printf(
                        TEXT("-C \"%s\" pull --ff-only origin main"),
                        *CacheDirectory
                    );
                    if (!RunGit(PullArguments, GitError))
                    {
                        AsyncTask(ENamedThreads::GameThread,
                            [OnComplete, GitError]()
                            {
                                Complete(
                                    OnComplete,
                                    false,
                                    FString::Printf(
                                        TEXT("Skipped ghost mappings: could not update the cached repository. %s"),
                                        *GitError
                                    )
                                );
                            }
                        );
                        return;
                    }
                }
            }

            if (bRepositoryChanged)
            {
                AsyncTask(ENamedThreads::GameThread, []()
                {
                    ShowProgress(TEXT("Ghost mappings: downloading changed Git LFS asset files..."));
                });

                const FString LfsArguments = FString::Printf(
                    TEXT("-C \"%s\" lfs pull"),
                    *CacheDirectory
                );
                if (!RunGit(LfsArguments, GitError))
                {
                    AsyncTask(ENamedThreads::GameThread,
                        [OnComplete, GitError]()
                        {
                            Complete(
                                OnComplete,
                                false,
                                FString::Printf(
                                    TEXT("Skipped ghost mappings: Git LFS could not download the Unreal assets. %s"),
                                    *GitError
                                )
                            );
                        }
                    );
                    return;
                }
            }

            const FString SourceContentDirectory = FPaths::Combine(
                CacheDirectory,
                TEXT("Content")
            );
            if (!BackgroundPlatformFile.DirectoryExists(*SourceContentDirectory))
            {
                AsyncTask(ENamedThreads::GameThread,
                    [OnComplete]()
                    {
                        Complete(
                            OnComplete,
                            false,
                            TEXT("Skipped ghost mappings: the cloned repository does not contain a Content folder.")
                        );
                    }
                );
                return;
            }

            TArray<FString> DestinationAssetFiles;
            if (bRepositoryChanged)
            {
                FindAssetFiles(SourceContentDirectory, DestinationAssetFiles);
                if (!BackgroundPlatformFile.CopyDirectoryTree(
                    *FPaths::ProjectContentDir(),
                    *SourceContentDirectory,
                    true
                ))
                {
                    AsyncTask(ENamedThreads::GameThread,
                        [OnComplete]()
                        {
                            Complete(
                                OnComplete,
                                false,
                                TEXT("Skipped ghost mappings: downloaded the assets, but could not merge their Content folder into this project.")
                            );
                        }
                    );
                    return;
                }
            }

            AsyncTask(ENamedThreads::GameThread,
                [OnComplete, Repository, bRepositoryChanged, bRemoveUnsupportedAudioDeviceMapping, DestinationAssetFiles = MoveTemp(DestinationAssetFiles)]() mutable
                {
                    if (bRepositoryChanged)
                    {
                        ShowProgress(TEXT("Ghost mappings: registering assets with Unreal..."));

                        FScopedSlowTask AssetScanTask(
                            1.0f,
                            FText::FromString(TEXT("Registering ghost mappings with the Asset Registry..."))
                        );
                        AssetScanTask.MakeDialog();
                        AssetScanTask.EnterProgressFrame();

                        FAssetRegistryModule& AssetRegistryModule =
                            FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
                                TEXT("AssetRegistry")
                            );
                        AssetRegistryModule.Get().ScanFilesSynchronous(
                            DestinationAssetFiles,
                            true
                        );
                    }

                    const FString CompatibilityResult =
                        bRemoveUnsupportedAudioDeviceMapping
                        ? RemoveUnsupportedAudioDeviceMappings()
                        : TEXT("");

                    Complete(
                        OnComplete,
                        true,
                        bRepositoryChanged
                        ? FString::Printf(
                            TEXT("Downloaded, imported, and registered %d ghost-mapping assets from %s.%s Restart required."),
                            DestinationAssetFiles.Num(),
                            *Repository,
                            *CompatibilityResult
                        )
                        : FString::Printf(
                            TEXT("Ghost mappings from %s are already up to date; no asset download was needed.%s"),
                            *Repository,
                            *CompatibilityResult
                        )
                    );
                }
            );
        }
    );
#endif
}
