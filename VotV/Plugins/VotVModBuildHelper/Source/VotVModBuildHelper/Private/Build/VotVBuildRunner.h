#pragma once

#include "CoreMinimal.h"

class UVotVBuildOptions;
class UVotVSetupOptions;

/**
 * Owns the non-UI side of a mod build: validation, Automation Tool packaging,
 * and moving the generated chunk pak into the user-selected distribution folder.
 */
class FVotVBuildRunner
{
public:
    static bool Start(
        UVotVBuildOptions& BuildOptions,
        const UVotVSetupOptions& SetupOptions,
        FString& OutError
    );
};
