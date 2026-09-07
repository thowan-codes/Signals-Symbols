// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "VotVModBuildHelperStyle.h"

class FVotVModBuildHelperCommands : public TCommands<FVotVModBuildHelperCommands>
{
public:

	FVotVModBuildHelperCommands()
		: TCommands<FVotVModBuildHelperCommands>(TEXT("VotVModBuildHelper"), NSLOCTEXT("Contexts", "VotVModBuildHelper", "VotVModBuildHelper Plugin"), NAME_None, FVotVModBuildHelperStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};