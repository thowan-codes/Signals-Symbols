#include "Setup/VotVModContentCreator.h"

#include "VotVSetupOptions.h"
#include "AssetToolsModule.h"
#include "Engine/Blueprint.h"
#include "Engine/PrimaryAssetLabel.h"
#include "Factories/BlueprintFactory.h"
#include "Factories/DataAssetFactory.h"
#include "GameFramework/Actor.h"
#include "IAssetTools.h"
#include "UObject/UObjectGlobals.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"

void FVotVModContentCreator::CreateOrUpdate(
    const UVotVSetupOptions& SetupOptions,
    TArray<FString>& Results
)
{
    if (!SetupOptions.bAssignedExportFolder)
    {
        Results.Add(TEXT(
            "Skipped mod content: no Mod Chunk Directory was selected."
        ));

        return;
    }

    FString ModDirectory = SetupOptions.ModChunkDirectory.Path;
    ModDirectory.RemoveFromEnd(TEXT("/"));

    if (ModDirectory.IsEmpty() ||
        ModDirectory.EndsWith(TEXT("/MODNAMEHERE"), ESearchCase::IgnoreCase))
    {
        Results.Add(TEXT(
            "Skipped mod content: choose a real Mod Chunk Directory first."
        ));

        return;
    }

    FAssetToolsModule& AssetToolsModule =
        FModuleManager::LoadModuleChecked<FAssetToolsModule>(
            TEXT("AssetTools")
        );

    IAssetTools& AssetTools = AssetToolsModule.Get();

    UBlueprint* ModActor = nullptr;

    if (SetupOptions.bCreateModActor)
    {
        const FString ModActorPath = FString::Printf(
            TEXT("%s/ModActor.ModActor"),
            *ModDirectory
        );

        UObject* ExistingModActor = StaticLoadObject(
            UObject::StaticClass(),
            nullptr,
            *ModActorPath,
            nullptr,
            LOAD_NoWarn
        );

        if (ExistingModActor == nullptr)
        {
            UBlueprintFactory* BlueprintFactory =
                NewObject<UBlueprintFactory>();

            BlueprintFactory->ParentClass = AActor::StaticClass();

            ModActor = Cast<UBlueprint>(
                AssetTools.CreateAsset(
                    TEXT("ModActor"),
                    ModDirectory,
                    UBlueprint::StaticClass(),
                    BlueprintFactory
                )
            );

            Results.Add(ModActor != nullptr
                ? TEXT("Created ModActor.")
                : TEXT("Failed to create ModActor."));
        }
        else
        {
            ModActor = Cast<UBlueprint>(ExistingModActor);

            if (ModActor != nullptr)
            {
                Results.Add(TEXT("ModActor already exists; updating selected variables."));
            }
            else
            {
                Results.Add(TEXT(
                    "Could not use ModActor: another asset uses that name."
                ));
            }
        }
    }
    
    if (SetupOptions.bCreateModActor &&
        SetupOptions.bCreateModActorVars &&
        ModActor != nullptr)
    {
        const FEdGraphPinType StringPinType(
            UEdGraphSchema_K2::PC_String,
            NAME_None,
            nullptr,
            EPinContainerType::None,
            false,
            FEdGraphTerminalType()
        );

        const FEdGraphPinType StringArrayPinType(
            UEdGraphSchema_K2::PC_String,
            NAME_None,
            nullptr,
            EPinContainerType::Array,
            false,
            FEdGraphTerminalType()
        );

        const FEdGraphPinType BooleanPinType(
            UEdGraphSchema_K2::PC_Boolean,
            NAME_None,
            nullptr,
            EPinContainerType::None,
            false,
            FEdGraphTerminalType()
        );
        
        bool bVariablesChanged = false;

        const FText ModInfoCategory =
            FText::FromString(TEXT("VotV Mod"));
        
        auto AddVariableIfMissing =
            [&](const FName VariableName,
                const FEdGraphPinType& VariableType,
                const FString& DefaultValue,
                const FText& Category)
        {
            const int32 ExistingIndex =
                FBlueprintEditorUtils::FindNewVariableIndex(
                    ModActor,
                    VariableName
                );

            if (ExistingIndex != INDEX_NONE)
            {
                Results.Add(FString::Printf(
                    TEXT("%s already exists; left it unchanged."),
                    *VariableName.ToString()
                ));

                return;
            }

            const bool bAdded = FBlueprintEditorUtils::AddMemberVariable(
                ModActor,
                VariableName,
                VariableType,
                DefaultValue
            );

            if (bAdded)
            {
                FBlueprintEditorUtils::SetBlueprintVariableCategory(
                    ModActor,
                    VariableName,
                    nullptr,
                    Category,
                    true
                );
                
                bVariablesChanged = true;

                Results.Add(FString::Printf(
                    TEXT("Created %s."),
                    *VariableName.ToString()
                ));
            }
            else
            {
                Results.Add(FString::Printf(
                    TEXT("Failed to create %s."),
                    *VariableName.ToString()
                ));
            }
        };

        if (SetupOptions.bCreateModAuthorString)
        {
            AddVariableIfMissing(
                FName(TEXT("modAuthor")),
                StringPinType,
                TEXT(""),
                ModInfoCategory
            );
        }

        if (SetupOptions.bCreateModDescriptionString)
        {
            AddVariableIfMissing(
                FName(TEXT("modDescription")),
                StringPinType,
                TEXT(""),
                ModInfoCategory
            );
        }

        if (SetupOptions.bCreateModVersionString)
        {
            AddVariableIfMissing(
                FName(TEXT("modVersion")),
                StringPinType,
                TEXT(""),
                ModInfoCategory
            );
        }

        if (SetupOptions.bCreateModButtonsStringArray)
        {
            AddVariableIfMissing(
                FName(TEXT("modButtons")),
                StringArrayPinType,
                TEXT(""),
                ModInfoCategory
            );
        }

        if (SetupOptions.bCreateDebugModeBoolean)
        {
            AddVariableIfMissing(
                FName(TEXT("debugMode")),
                BooleanPinType,
                TEXT("false"),
                ModInfoCategory
            );
        }

        if (bVariablesChanged)
        {
            FKismetEditorUtilities::CompileBlueprint(ModActor);
            ModActor->MarkPackageDirty();

            Results.Add(TEXT("Compiled ModActor after creating variables."));
        }
    }

    const FString LabelName = TEXT("ModChunkLabel");

    const FString LabelPath = FString::Printf(
        TEXT("%s/%s.%s"),
        *ModDirectory,
        *LabelName,
        *LabelName
    );

    UPrimaryAssetLabel* PrimaryAssetLabel =
        Cast<UPrimaryAssetLabel>(
            StaticLoadObject(
                UObject::StaticClass(),
                nullptr,
                *LabelPath,
                nullptr,
                LOAD_NoWarn
            )
        );

    if (PrimaryAssetLabel == nullptr)
    {
        UDataAssetFactory* LabelFactory =
            NewObject<UDataAssetFactory>();

        LabelFactory->DataAssetClass =
            UPrimaryAssetLabel::StaticClass();

        PrimaryAssetLabel = Cast<UPrimaryAssetLabel>(
            AssetTools.CreateAsset(
                LabelName,
                ModDirectory,
                UPrimaryAssetLabel::StaticClass(),
                LabelFactory
            )
        );

        if (PrimaryAssetLabel == nullptr)
        {
            Results.Add(TEXT("Failed to create ModChunkLabel."));

            return;
        }

        Results.Add(TEXT("Created ModChunkLabel."));
    }

    PrimaryAssetLabel->Modify();

    PrimaryAssetLabel->Rules.ChunkId = SetupOptions.ModChunkId;

    PrimaryAssetLabel->Rules.bApplyRecursively =
        !SetupOptions.bDisableApplyRecursively;

    PrimaryAssetLabel->bLabelAssetsInMyDirectory =
        SetupOptions.bEnableLabelAssets;

    PrimaryAssetLabel->MarkPackageDirty();

    Results.Add(FString::Printf(
        TEXT("Configured ModChunkLabel with Chunk ID %d."),
        SetupOptions.ModChunkId
    ));
}