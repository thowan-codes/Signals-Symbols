// Copyright Epic Games, Inc. All Rights Reserved.

#include "VotVModBuildHelperCommands.h"

#define LOCTEXT_NAMESPACE "FVotVModBuildHelperModule"

void FVotVModBuildHelperCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "VotVModBuildHelper", "Bring up VotVModBuildHelper window", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
