#include "Setup/VotVGhostMappingsInstaller.h"

#include "VotVSetupOptions.h"

#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

namespace
{
    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveDownloadRequest;

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

    void Complete(
        FVotVGhostMappingsCompleteDelegate OnComplete,
        bool bSucceeded,
        const FString& Message
    )
    {
        ActiveDownloadRequest.Reset();
        OnComplete.ExecuteIfBound(bSucceeded, Message);
    }

    FString GetSystemExecutable(const TCHAR* FileName)
    {
        const FString Candidate = FPaths::Combine(
            FPlatformMisc::GetEnvironmentVariable(TEXT("WINDIR")),
            TEXT("System32"),
            FileName
        );
        return FPlatformFileManager::Get().GetPlatformFile().FileExists(*Candidate)
            ? Candidate
            : FString(FileName);
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
    if (ActiveDownloadRequest.IsValid())
    {
        Complete(
            OnComplete,
            false,
            TEXT("Skipped ghost mappings: a ghost-mappings download is already in progress.")
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

    const FString TemporaryDirectory = FPaths::Combine(
        FPaths::ProjectIntermediateDir(),
        TEXT("VotVModBuildHelper"),
        TEXT("GhostMappings"),
        FGuid::NewGuid().ToString(EGuidFormats::Digits)
    );
    const FString ArchivePath = FPaths::Combine(
        TemporaryDirectory,
        TEXT("ghost-mappings.zip")
    );
    const FString ExtractDirectory = FPaths::Combine(
        TemporaryDirectory,
        TEXT("Extracted")
    );
    const FString DownloadUrl = FString::Printf(
        TEXT("https://codeload.github.com/%s/%s/zip/refs/heads/main"),
        *Owner,
        *Repository
    );

    IPlatformFile& PlatformFile =
        FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.CreateDirectoryTree(*TemporaryDirectory))
    {
        Complete(
            OnComplete,
            false,
            TEXT("Skipped ghost mappings: could not create a temporary download folder.")
        );
        return;
    }

    ActiveDownloadRequest = FHttpModule::Get().CreateRequest();
    ActiveDownloadRequest->SetURL(DownloadUrl);
    ActiveDownloadRequest->SetVerb(TEXT("GET"));
    ActiveDownloadRequest->OnProcessRequestComplete().BindLambda(
        [OnComplete, Repository, TemporaryDirectory, ArchivePath, ExtractDirectory](
            FHttpRequestPtr,
            FHttpResponsePtr Response,
            bool bWasSuccessful
        )
        {
            IPlatformFile& CallbackPlatformFile =
                FPlatformFileManager::Get().GetPlatformFile();
            const int32 HttpCode = Response.IsValid()
                ? Response->GetResponseCode()
                : 0;
            if (!bWasSuccessful || !Response.IsValid() || HttpCode != 200)
            {
                Complete(
                    OnComplete,
                    false,
                    FString::Printf(
                        TEXT("Skipped ghost mappings: GitHub download failed (HTTP %d)."),
                        HttpCode
                    )
                );
                return;
            }

            if (!FFileHelper::SaveArrayToFile(
                Response->GetContent(),
                *ArchivePath
            ))
            {
                Complete(
                    OnComplete,
                    false,
                    TEXT("Skipped ghost mappings: GitHub download completed, but the ZIP could not be saved.")
                );
                return;
            }

            if (!CallbackPlatformFile.CreateDirectoryTree(*ExtractDirectory))
            {
                Complete(
                    OnComplete,
                    false,
                    TEXT("Skipped ghost mappings: could not create a temporary extraction folder.")
                );
                return;
            }

            int32 TarExitCode = INDEX_NONE;
            FString TarOutput;
            FString TarError;
            const FString TarArguments = FString::Printf(
                TEXT("-xf \"%s\" -C \"%s\""),
                *ArchivePath,
                *ExtractDirectory
            );
            const bool bTarStarted = FPlatformProcess::ExecProcess(
                *GetSystemExecutable(TEXT("tar.exe")),
                *TarArguments,
                &TarExitCode,
                &TarOutput,
                &TarError
            );

            const FString SourceContentDirectory = FPaths::Combine(
                ExtractDirectory,
                Repository + TEXT("-main"),
                TEXT("Content")
            );
            if (!bTarStarted || TarExitCode != 0 ||
                !CallbackPlatformFile.DirectoryExists(*SourceContentDirectory))
            {
                Complete(
                    OnComplete,
                    false,
                    FString::Printf(
                        TEXT("Skipped ghost mappings: the downloaded ZIP could not be extracted (tar started: %s, exit code: %d). %s"),
                        bTarStarted ? TEXT("yes") : TEXT("no"),
                        TarExitCode,
                        *TarError.Right(1000)
                    )
                );
                return;
            }

            if (!CallbackPlatformFile.CopyDirectoryTree(
                *FPaths::ProjectContentDir(),
                *SourceContentDirectory,
                true
            ))
            {
                Complete(
                    OnComplete,
                    false,
                    TEXT("Skipped ghost mappings: downloaded the archive, but could not merge its Content folder into this project.")
                );
                return;
            }

            CallbackPlatformFile.DeleteDirectoryRecursively(*TemporaryDirectory);
            Complete(
                OnComplete,
                true,
                FString::Printf(
                    TEXT("Downloaded and updated ghost mappings from %s. Restart required."),
                    *Repository
                )
            );
        }
    );

    if (!ActiveDownloadRequest->ProcessRequest())
    {
        Complete(
            OnComplete,
            false,
            TEXT("Skipped ghost mappings: Unreal could not start the GitHub HTTP request.")
        );
    }
#endif
}
