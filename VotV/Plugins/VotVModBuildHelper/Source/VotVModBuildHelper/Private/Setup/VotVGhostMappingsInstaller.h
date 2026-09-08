#pragma once

#include "CoreMinimal.h"

class UVotVSetupOptions;

DECLARE_DELEGATE_TwoParams(
    FVotVGhostMappingsCompleteDelegate,
    bool,
    const FString&
);

/** Incrementally updates cached ghost mappings and can repair existing mapping compatibility. */
class FVotVGhostMappingsInstaller
{
public:
    /** Calls OnComplete once the update or local compatibility repair has succeeded or failed. */
    static void DownloadAndInstall(
        const UVotVSetupOptions& SetupOptions,
        FVotVGhostMappingsCompleteDelegate OnComplete
    );
};
