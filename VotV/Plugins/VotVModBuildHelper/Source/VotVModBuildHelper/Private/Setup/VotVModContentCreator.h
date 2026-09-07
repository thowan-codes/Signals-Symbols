#pragma once

#include "CoreMinimal.h"

class UVotVSetupOptions;

class FVotVModContentCreator
{
public:
	static void CreateOrUpdate(
		const UVotVSetupOptions& SetupOptions,
		TArray<FString>& Results
	);
};