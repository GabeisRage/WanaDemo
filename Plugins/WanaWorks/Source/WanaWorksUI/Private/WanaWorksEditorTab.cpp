#include "WanaWorksEditorTab.h"

#include "WanaWorksCoreModule.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Text/SMultiLineEditableText.h"
#include "Widgets/Input/SButton.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "WanaWorksPanel"

void SWanaWorksPanel::Construct(const FArguments& InArgs)
{
    LogText = TEXT("WanaWorks ready.\n");

    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(12.0f)
        [
            SNew(SVerticalBox)

            // Header
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 4)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("WanaWorksTitle", "WanaWorks"))
                .Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 2)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("WanaWorksVersion", "v0.1.0"))
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 0, 0, 8)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("WanaWorksTagline", "Powering Your Creativity."))
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 4)
            [
                SNew(SSeparator)
            ]

            // Buttons
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 8)
            [
                SNew(SHorizontalBox)

                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(2)
                [
                    SNew(SButton)
                    .HAlign(HAlign_Center)
                    .ContentPadding(FMargin(8, 4))
                    .Text(LOCTEXT("AnalyzeButton", "Analyze"))
                    .OnClicked(this, &SWanaWorksPanel::OnAnalyzeClicked)
                ]

                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(2)
                [
                    SNew(SButton)
                    .HAlign(HAlign_Center)
                    .ContentPadding(FMargin(8, 4))
                    .Text(LOCTEXT("EnhanceButton", "Enhance"))
                    .OnClicked(this, &SWanaWorksPanel::OnEnhanceClicked)
                ]

                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(2)
                [
                    SNew(SButton)
                    .HAlign(HAlign_Center)
                    .ContentPadding(FMargin(8, 4))
                    .Text(LOCTEXT("TestButton", "Test"))
                    .OnClicked(this, &SWanaWorksPanel::OnTestClicked)
                ]

                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .Padding(2)
                [
                    SNew(SButton)
                    .HAlign(HAlign_Center)
                    .ContentPadding(FMargin(8, 4))
                    .Text(LOCTEXT("BuildButton", "Build"))
                    .OnClicked(this, &SWanaWorksPanel::OnBuildClicked)
                ]
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 4)
            [
                SNew(SSeparator)
            ]

            // Output log
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 4, 0, 4)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("OutputLogLabel", "Output Log"))
            ]

            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(0, 4)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
                .Padding(6.0f)
                [
                    SNew(SScrollBox)
                    + SScrollBox::Slot()
                    [
                        SAssignNew(OutputLog, SMultiLineEditableText)
                        .IsReadOnly(true)
                        .Text(FText::FromString(LogText))
                    ]
                ]
            ]
        ]
    ];
}

void SWanaWorksPanel::AppendLog(const FString& Line)
{
    LogText += Line;
    if (!Line.EndsWith(TEXT("\n")))
    {
        LogText += TEXT("\n");
    }
    if (OutputLog.IsValid())
    {
        OutputLog->SetText(FText::FromString(LogText));
    }
    FWanaLogger::Log(Line);
}

FReply SWanaWorksPanel::OnAnalyzeClicked()
{
    AppendLog(TEXT("Analyzing project..."));
    return FReply::Handled();
}

FReply SWanaWorksPanel::OnEnhanceClicked()
{
    AppendLog(TEXT("Enhancing assets..."));
    return FReply::Handled();
}

FReply SWanaWorksPanel::OnTestClicked()
{
    AppendLog(TEXT("Running test pass..."));
    return FReply::Handled();
}

FReply SWanaWorksPanel::OnBuildClicked()
{
    AppendLog(TEXT("Building output..."));
    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
