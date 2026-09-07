#include "Setup/VotVSetupWindow.h"

#include "VotVSetupOptions.h"
#include "Framework/Application/SlateApplication.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SWindow.h"

namespace
{
    TWeakPtr<SWindow> ActiveSetupWindow;
}

void FVotVSetupWindow::Open(
    UVotVSetupOptions* SetupOptions,
    FVotVRunSetupDelegate OnRunSetup
)
{
    check(SetupOptions != nullptr);

    if (TSharedPtr<SWindow> ExistingWindow = ActiveSetupWindow.Pin())
    {
        ExistingWindow->BringToFront(true);
        return;
    }

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

    TSharedRef<IDetailsView> SetupDetailsView =
        PropertyEditorModule.CreateDetailView(DetailsViewArgs);

    SetupDetailsView->SetObject(SetupOptions);

    TSharedRef<SWindow> SetupWindow = SNew(SWindow)
        .Title(FText::FromString(TEXT("Setup Project")))
        .ClientSize(FVector2D(850.0f, 650.0f))
        .SupportsMinimize(false)
        .SupportsMaximize(false)
        [
            SNew(SBorder)
            .Padding(FMargin(12.0f))
            [
                SNew(SVerticalBox)

                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                [
                    SetupDetailsView
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Right)
                .Padding(0.0f, 12.0f, 0.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("Run Setup")))
                    .ToolTipText(FText::FromString(
                        TEXT("Apply the selected project settings.")
                    ))
                    .OnClicked_Lambda([OnRunSetup]()
                    {
                        // Close the setup dialog before applying settings. Several
                        // settings may open a modal prompt or restart the editor.
                        if (TSharedPtr<SWindow> ExistingWindow =
                            ActiveSetupWindow.Pin())
                        {
                            ExistingWindow->RequestDestroyWindow();
                        }

                        OnRunSetup.ExecuteIfBound();

                        return FReply::Handled();
                    })
                ]
            ]
        ];

    ActiveSetupWindow = SetupWindow;
    FSlateApplication::Get().AddWindow(SetupWindow);
}
