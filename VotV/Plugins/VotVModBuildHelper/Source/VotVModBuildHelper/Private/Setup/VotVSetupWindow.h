#pragma once

#include "CoreMinimal.h"

class UVotVSetupOptions;

DECLARE_DELEGATE(FVotVRunSetupDelegate);

class FVotVSetupWindow
{
public:
	static void Open(
		UVotVSetupOptions* SetupOptions,
		FVotVRunSetupDelegate OnRunSetup
	);
};