#include "VotVBuildOptions.h"

#include "Misc/Paths.h"

namespace
{
    FString ReplaceBuildToken(
        FString Value,
        const TCHAR* Token,
        const FString& Replacement
    )
    {
        Value.ReplaceInline(Token, *Replacement, ESearchCase::CaseSensitive);
        return Value;
    }
}

void UVotVBuildOptions::RefreshDerivedValues()
{
    if (ManifestTemplate == EVotVManifestTemplate::Thunderstore)
    {
        ManifestName = TEXT("manifest");
        ManifestVersionStyle = EVotVManifestVersionStyle::MajorMinorPatch;
    }

    // Early versions used the folder template as the Version Format default,
    // then appended the same build details a second time in the folder name.
    // Correct those exact legacy defaults without changing custom user formats.
    const FString LegacyFolderTemplate =
        TEXT("{modName}-{version}-{batchType}-build{buildID}");
    if (VersionFormat == LegacyFolderTemplate)
    {
        VersionFormat =
            TEXT("{majorVersion}.{minorVersion}.{patchVersion}-{batchType}-build{buildID}");
    }
    if (FolderBuildNameTemplate == LegacyFolderTemplate)
    {
        FolderBuildNameTemplate = TEXT("{modName}-{versionComplex}");
    }

    const FString TrimmedModName = ModName.TrimStartAndEnd();
    const FString SafeModName = FPaths::MakeValidFileName(TrimmedModName);
    const FString BatchType = BuildType == EVotVBuildType::Development
        ? TEXT("Development")
        : TEXT("Release");
    const FString BuildId = bEnableBuildIds
        ? FString::FromInt(BuildType == EVotVBuildType::Development
            ? DevelopmentBuildId
            : ReleaseBuildId)
        : TEXT("");

    FString Version = VersionFormat;
    Version = ReplaceBuildToken(
        Version,
        TEXT("{majorVersion}"),
        FString::FromInt(VersionMajor)
    );
    Version = ReplaceBuildToken(
        Version,
        TEXT("{minorVersion}"),
        FString::FromInt(VersionMinor)
    );
    Version = ReplaceBuildToken(
        Version,
        TEXT("{patchVersion}"),
        FString::FromInt(VersionPatch)
    );
    Version = ReplaceBuildToken(Version, TEXT("{batchType}"), BatchType);
    Version = ReplaceBuildToken(Version, TEXT("{buildID}"), BuildId);

    const FString SimpleVersion = FString::Printf(
        TEXT("%d.%d.%d"),
        VersionMajor,
        VersionMinor,
        VersionPatch
    );

    FString FolderName = FolderBuildNameTemplate;
    FolderName = ReplaceBuildToken(FolderName, TEXT("{modName}"), SafeModName);
    FolderName = ReplaceBuildToken(
        FolderName,
        TEXT("{versionSimple}"),
        SimpleVersion
    );
    FolderName = ReplaceBuildToken(
        FolderName,
        TEXT("{versionComplex}"),
        Version
    );
    // Keep older saved templates working.
    FolderName = ReplaceBuildToken(FolderName, TEXT("{version}"), Version);
    FolderName = ReplaceBuildToken(FolderName, TEXT("{batchType}"), BatchType);
    FolderName = ReplaceBuildToken(FolderName, TEXT("{buildID}"), BuildId);
    FolderName = FPaths::MakeValidFileName(FolderName);

    FString DistributionPath = DistributionFolder.Path.TrimStartAndEnd();
    if (!DistributionPath.IsEmpty())
    {
        DistributionPath = FPaths::ConvertRelativePathToFull(
            FPaths::ProjectDir(),
            DistributionPath
        );

        BuildDestinationPreview = FPaths::Combine(
            DistributionPath,
            FolderName
        );
    }
    else
    {
        BuildDestinationPreview.Empty();
    }

    BuildOutputPakFile = SafeModName.IsEmpty()
        ? FString()
        : SafeModName + TEXT(".pak");
}

#if WITH_EDITOR
void UVotVBuildOptions::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent
)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    RefreshDerivedValues();
}
#endif
