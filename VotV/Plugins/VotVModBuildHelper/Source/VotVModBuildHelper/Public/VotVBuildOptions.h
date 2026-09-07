#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "VotVBuildOptions.generated.h"


UENUM()
enum class EVotVBuildType : uint8
{
    Development UMETA(DisplayName = "Development"),
    Release UMETA(DisplayName = "Release")
};

UENUM()
enum class EVotVManifestTemplate : uint8
{
    Custom UMETA(DisplayName = "Custom"),
    Thunderstore UMETA(DisplayName = "Thunderstore")
};

UENUM()
enum class EVotVManifestVersionStyle : uint8
{
    MajorMinorPatch UMETA(DisplayName = "Major.Minor.Patch"),
    VersionFormat UMETA(DisplayName = "Version Format")
};

UCLASS(Config=EditorPerProjectUserSettings)
class VOTVMODBUILDHELPER_API UVotVBuildOptions : public UObject
{
    GENERATED_BODY()

public:
        UVotVBuildOptions()
        {
            DistributionFolder.Path = TEXT("../Dist");
        }

    /** Refreshes the read-only paths shown in the Build window. */
    void RefreshDerivedValues();

#if WITH_EDITOR
    virtual void PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent
    ) override;
#endif
    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Build Settings",
        meta = (
            DisplayName = "Mod Name",
            ToolTip = "The name used for the final .pak file."
        )
    )
    FString ModName = TEXT("MyMod");

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Build Settings",
        meta = (
            DisplayName = "Mod Actor",
            ToolTip = "The ModActor Blueprint whose values will be updated during a build."
        )
    )
    TSoftObjectPtr<UBlueprint> ModActor;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Version",
        meta = (
            DisplayName = "Major",
            ClampMin = "0",
            UIMin = "0"
        )
    )
    int32 VersionMajor = 0;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Version",
        meta = (
            DisplayName = "Minor",
            ClampMin = "0",
            UIMin = "0"
        )
    )
    int32 VersionMinor = 1;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Version",
        meta = (
            DisplayName = "Patch",
            ClampMin = "0",
            UIMin = "0"
        )
    )
    int32 VersionPatch = 0;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Version",
        meta = (
            DisplayName = "Version Format",
            ToolTip = "Supports placeholders such as {majorVersion}, {minorVersion}, and {patchVersion}."
        )
    )
    FString VersionFormat =
        TEXT("{majorVersion}.{minorVersion}.{patchVersion}-{batchType}-build{buildID}");

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Build Settings",
        meta = (
            DisplayName = "Build Type",
            ToolTip = "Development uses the development build ID. Release uses the release build ID."
        )
    )
    EVotVBuildType BuildType = EVotVBuildType::Development;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Build IDs",
        meta = (
            DisplayName = "Enable Build IDs"
        )
    )
    bool bEnableBuildIds = true;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Build IDs",
        meta = (
            DisplayName = "Development Build ID",
            EditCondition = "bEnableBuildIds",
            ClampMin = "1",
            UIMin = "1"
        )
    )
    int32 DevelopmentBuildId = 1;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Build IDs",
        meta = (
            DisplayName = "Release Build ID",
            EditCondition = "bEnableBuildIds",
            ClampMin = "1",
            UIMin = "1"
        )
    )
    int32 ReleaseBuildId = 1;
    
    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Debug",
        meta = (
            DisplayName = "Debug Mode Enabled",
            EditCondition = "BuildType == EVotVBuildType::Development",
            ToolTip = "Enabled only for Development builds. Release builds will later force this off."
        )
    )
    bool bDebugModeEnabled = true;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Debug",
        meta = (
            DisplayName = "Override Debug Mode Actor and Variable",
            ToolTip = "Use a different Blueprint and variable instead of the selected ModActor's debugMode variable."
        )
    )
    bool bOverrideDebugModeInfo = false;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Debug",
        meta = (
            DisplayName = "Debug Mode Actor Override",
            EditCondition = "bOverrideDebugModeInfo",
            ToolTip = "Blueprint that contains the replacement debug-mode Boolean."
        )
    )
    TSoftObjectPtr<UBlueprint> DebugModeActorOverride;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Debug",
        meta = (
            DisplayName = "Debug Mode Variable Override",
            EditCondition = "bOverrideDebugModeInfo",
            ToolTip = "Name of the Boolean variable to update on the override Blueprint."
        )
    )
    FString DebugModeVariableOverride;
    
    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Output",
        meta = (
            DisplayName = "Distribution Folder",
            ToolTip = "The destination folder for built mod output. This should be outside the Unreal project."
        )
    )
    FDirectoryPath DistributionFolder;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Output",
        meta = (
            DisplayName = "Folder Build Name Template",
            ToolTip = "Supports {modName}, {versionSimple}, {versionComplex}, {batchType}, and {buildID}. {version} remains an alias for {versionComplex}."
        )
    )
    FString FolderBuildNameTemplate =
        TEXT("{modName}-{versionComplex}");

    UPROPERTY(
        VisibleAnywhere,
        Transient,
        Category = "Output",
        meta = (
            DisplayName = "Build Destination Preview",
            ToolTip = "The final folder that will contain the generated .pak and manifest."
        )
    )
    FString BuildDestinationPreview;

    UPROPERTY(
        VisibleAnywhere,
        Transient,
        Category = "Output",
        meta = (
            DisplayName = "Build Output .pak File",
            ToolTip = "The final .pak file name."
        )
    )
    FString BuildOutputPakFile;
    
    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Manifest Options",
        meta = (
            DisplayName = "Template",
            ToolTip = "Controls the intended manifest layout. Behavior will be implemented during the manifest-building step."
        )
    )
    EVotVManifestTemplate ManifestTemplate =
        EVotVManifestTemplate::Custom;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Manifest Options",
        meta = (
            DisplayName = ".json Name",
            EditCondition = "ManifestTemplate == EVotVManifestTemplate::Custom",
            ToolTip = "Name of the generated JSON file without its .json extension."
        )
    )
    FString ManifestName = TEXT("build");

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Manifest Options",
        meta = (
            DisplayName = "Author",
            ToolTip = "Written as author when provided. Required by Thunderstore."
        )
    )
    FString ManifestAuthor;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Manifest Options",
        meta = (
            DisplayName = "Description",
            ToolTip = "Written as description when provided. Required by Thunderstore, where it is limited to 250 characters."
        )
    )
    FString ManifestDescription;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Manifest Options",
        meta = (
            DisplayName = "Supported Game Version",
            EditCondition = "ManifestTemplate == EVotVManifestTemplate::Custom && bManifestIncludeSupportedGameVersion",
            ToolTip = "Optional VotV version written to the generated manifest. Leave blank when the build supports multiple versions."
        )
    )
    FString SupportedGameVersion;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Manifest Options",
        meta = (
            DisplayName = "website_url",
            ToolTip = "Written when provided. Required by Thunderstore."
        )
    )
    FString WebsiteUrl;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Manifest Options",
        meta = (
            DisplayName = "Dependencies",
            ToolTip = "Written when one or more dependency identifiers are provided."
        )
    )
    TArray<FString> ManifestDependencies;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Manifest Options",
        meta = (DisplayName = "Include Mod Name", EditCondition = "ManifestTemplate == EVotVManifestTemplate::Custom")
    )
    bool bManifestIncludeModName = true;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Manifest Options",
        meta = (DisplayName = "Include Version", EditCondition = "ManifestTemplate == EVotVManifestTemplate::Custom")
    )
    bool bManifestIncludeVersion = true;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Manifest Options",
        meta = (
            DisplayName = "Version Style",
            EditCondition = "ManifestTemplate == EVotVManifestTemplate::Custom",
            ToolTip = "Choose whether the manifest uses Major.Minor.Patch or the full Version Format value."
        )
    )
    EVotVManifestVersionStyle ManifestVersionStyle =
        EVotVManifestVersionStyle::MajorMinorPatch;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Manifest Options",
        meta = (DisplayName = "Include Supported Game Version", EditCondition = "ManifestTemplate == EVotVManifestTemplate::Custom")
    )
    bool bManifestIncludeSupportedGameVersion = true;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Manifest Options",
        meta = (DisplayName = "Include .pak File Name", EditCondition = "ManifestTemplate == EVotVManifestTemplate::Custom")
    )
    bool bManifestIncludePakFileName = true;

    UPROPERTY(
        Config,
        EditAnywhere,
        Category = "Manifest Options",
        meta = (DisplayName = "Include Created Timestamp", EditCondition = "ManifestTemplate == EVotVManifestTemplate::Custom")
    )
    bool bManifestIncludeTimestamp = true;

    
};
