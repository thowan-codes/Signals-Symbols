#include "Build/SVotVBuildPanel.h"

#include "VotVBuildOptions.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"

void SVotVBuildPanel::Construct(const FArguments& InArgs)
{
    check(InArgs._BuildOptions != nullptr);

    FPropertyEditorModule& PropertyEditorModule =
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>(
            TEXT("PropertyEditor")
        );

    FDetailsViewArgs DetailsViewArgs;
    DetailsViewArgs.bAllowSearch = true;
    DetailsViewArgs.bHideSelectionTip = true;
    DetailsViewArgs.bLockable = false;
    DetailsViewArgs.bUpdatesFromSelection = false;
    DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

    TSharedRef<IDetailsView> BuildDetailsView =
        PropertyEditorModule.CreateDetailView(DetailsViewArgs);

    BuildDetailsView->SetObject(InArgs._BuildOptions);

    ChildSlot
    [
        SNew(SBorder)
        .Padding(FMargin(12.0f))
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            [
                BuildDetailsView
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .HAlign(HAlign_Right)
            .Padding(0.0f, 12.0f, 0.0f, 0.0f)
            [
                SNew(SHorizontalBox)

                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Setup Project")))
                    .ToolTipText(FText::FromString(
                        TEXT("Open the project setup options.")
                    ))
                    .OnClicked_Lambda([OnOpenSetup = InArgs._OnOpenSetup]()
                    {
                        OnOpenSetup.ExecuteIfBound();

                        return FReply::Handled();
                    })
                ]

                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(8.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Build Project")))
                    .ToolTipText(FText::FromString(
                        TEXT("Package the selected mod chunk and copy its .pak to the distribution folder.")
                    ))
                    .OnClicked_Lambda([OnRunBuild = InArgs._OnRunBuild]()
                    {
                        OnRunBuild.ExecuteIfBound();

                        return FReply::Handled();
                    })
                ]
            ]
        ]
    ];
}
