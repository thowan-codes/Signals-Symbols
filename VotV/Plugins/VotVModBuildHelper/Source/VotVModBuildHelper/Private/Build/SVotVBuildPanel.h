#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UVotVBuildOptions;

DECLARE_DELEGATE(FVotVOpenSetupDelegate);
DECLARE_DELEGATE(FVotVRunBuildDelegate);

class SVotVBuildPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SVotVBuildPanel) {}
	SLATE_ARGUMENT(UVotVBuildOptions*, BuildOptions)
	SLATE_EVENT(FVotVOpenSetupDelegate, OnOpenSetup)
	SLATE_EVENT(FVotVRunBuildDelegate, OnRunBuild)
SLATE_END_ARGS()

void Construct(const FArguments& InArgs);
};