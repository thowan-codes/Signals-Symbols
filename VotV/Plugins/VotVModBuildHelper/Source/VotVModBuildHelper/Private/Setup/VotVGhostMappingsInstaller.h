#pragma once

#include "CoreMinimal.h"

class UVotVSetupOptions;

DECLARE_DELEGATE_TwoParams(
    FVotVGhostMappingsCompleteDelegate,
    bool,
    const FString&
);

/** Downloads a ghost-mappings GitHub archive and merges its Content folder into the project. */
class FVotVGhostMappingsInstaller
{
public:
    /** Calls OnComplete once the download and content merge have either succeeded or failed. */
    static void DownloadAndInstall(
        const UVotVSetupOptions& SetupOptions,
        FVotVGhostMappingsCompleteDelegate OnComplete
    );
};
