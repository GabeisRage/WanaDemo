#include "WanaWorksUITabBuilder.h"

#include "WanaWorksCommandRegistry.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "WanaWorksUITabBuilder"

namespace
{
TSharedRef<SWidget> MakeSection(
    const FSlateFontInfo& SectionHeaderFont,
    const FText& Title,
    const TSharedRef<SWidget>& Content,
    const TOptional<FText>& Description = TOptional<FText>(),
    const TOptional<FText>& StatusText = TOptional<FText>(),
    bool bProminent = false);
TSharedRef<SWidget> MakeCommandButton(const FWanaWorksUITabBuilderArgs& Args, const FWanaCommandDefinition& Definition);
constexpr float WanaAISubsectionSpacing = 10.0f;
const FLinearColor SecondaryTextColor(0.78f, 0.82f, 0.88f, 1.0f);
const FLinearColor InfoPanelColor(0.08f, 0.11f, 0.15f, 0.75f);

FString NormalizeStatusLabel(const FString& StatusLabel)
{
    return StatusLabel.TrimStartAndEnd().ToUpper();
}

FString GetStatusLabelFromMessage(const FText& StatusText)
{
    const FString LowerStatus = StatusText.ToString().TrimStartAndEnd().ToLower();

    if (LowerStatus.Contains(TEXT("warning")) ||
        LowerStatus.Contains(TEXT("no actors")) ||
        LowerStatus.Contains(TEXT("missing")) ||
        LowerStatus.Contains(TEXT("could not")) ||
        LowerStatus.Contains(TEXT("failed")) ||
        LowerStatus.Contains(TEXT("invalid")))
    {
        return TEXT("WARNING");
    }

    if (LowerStatus.Contains(TEXT("already")) ||
        LowerStatus.Contains(TEXT("preserved")) ||
        LowerStatus.Contains(TEXT("safe")))
    {
        return TEXT("SAFE");
    }

    if (LowerStatus.Contains(TEXT("live")) || LowerStatus.Contains(TEXT("evaluat")))
    {
        return TEXT("LIVE");
    }

    if (LowerStatus.Contains(TEXT("ready")) ||
        LowerStatus.Contains(TEXT("applied")) ||
        LowerStatus.Contains(TEXT("ensured")) ||
        LowerStatus.Contains(TEXT("complete")) ||
        LowerStatus.Contains(TEXT("initialized")))
    {
        return TEXT("READY");
    }

    return TEXT("ACTIVE");
}

FLinearColor GetStatusBadgeColor(const FString& StatusLabel)
{
    const FString NormalizedStatus = NormalizeStatusLabel(StatusLabel);

    if (NormalizedStatus == TEXT("READY"))
    {
        return FLinearColor(0.19f, 0.45f, 0.25f, 0.9f);
    }

    if (NormalizedStatus == TEXT("LIVE"))
    {
        return FLinearColor(0.11f, 0.43f, 0.52f, 0.9f);
    }

    if (NormalizedStatus == TEXT("SAFE"))
    {
        return FLinearColor(0.13f, 0.36f, 0.40f, 0.9f);
    }

    if (NormalizedStatus == TEXT("WARNING"))
    {
        return FLinearColor(0.56f, 0.28f, 0.12f, 0.92f);
    }

    return FLinearColor(0.20f, 0.26f, 0.45f, 0.9f);
}

TSharedRef<SWidget> MakeStatusBadge(const FText& StatusText)
{
    const FString NormalizedStatus = NormalizeStatusLabel(StatusText.ToString());

    return SNew(SBorder)
        .Padding(FMargin(8.0f, 3.0f))
        .BorderBackgroundColor(GetStatusBadgeColor(NormalizedStatus))
        [
            SNew(STextBlock)
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
            .ColorAndOpacity(FLinearColor::White)
            .Text(FText::FromString(NormalizedStatus))
        ];
}

TSharedRef<SWidget> MakeReadOnlyInfoPanel(const FText& Title, TFunction<FText(void)> GetBodyText)
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 6.0f)
        [
            SNew(STextBlock)
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
            .ColorAndOpacity(SecondaryTextColor)
            .Text(Title)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SBorder)
            .Padding(10.0f)
            .BorderBackgroundColor(InfoPanelColor)
            [
                SNew(STextBlock)
                .AutoWrapText(true)
                .Text_Lambda([GetBodyText]()
                {
                    return GetBodyText ? GetBodyText() : FText::GetEmpty();
                })
            ]
        ];
}

TSharedRef<SWidget> MakeFixedWidthButton(const FText& ButtonText, TFunction<void(void)> OnPressed, float WidthOverride = 156.0f)
{
    return SNew(SBox)
        .WidthOverride(WidthOverride)
        .HeightOverride(30.0f)
        [
            SNew(SButton)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            .Text(ButtonText)
            .OnClicked_Lambda([OnPressed]()
            {
                if (OnPressed)
                {
                    OnPressed();
                }

                return FReply::Handled();
            })
        ];
}

TSharedRef<SWidget> MakeCharacterEnhancementSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksCharacterEnhancementSection", "Character Enhancement"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksCharacterEnhancementSummaryLabel", "Selected Actor Summary"),
                [GetCharacterEnhancementSummaryText = Args.GetCharacterEnhancementSummaryText]()
                {
                    return GetCharacterEnhancementSummaryText ? GetCharacterEnhancementSummaryText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksCharacterEnhancementChainLabel", "Live System Chain"),
                [GetCharacterEnhancementChainText = Args.GetCharacterEnhancementChainText]()
                {
                    return GetCharacterEnhancementChainText ? GetCharacterEnhancementChainText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksCharacterEnhancementWorkflowInfoLabel", "Workflow Guidance"),
                [GetCharacterEnhancementWorkflowText = Args.GetCharacterEnhancementWorkflowText]()
                {
                    return GetCharacterEnhancementWorkflowText ? GetCharacterEnhancementWorkflowText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 6.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("WanaWorksCharacterEnhancementWorkflowLabel", "Setup Workflow"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            SNew(SBox)
            .WidthOverride(260.0f)
            [
                SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(Args.EnhancementWorkflowOptions)
                .InitiallySelectedItem(Args.GetSelectedEnhancementWorkflowOption ? Args.GetSelectedEnhancementWorkflowOption() : nullptr)
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                {
                    return SNew(STextBlock)
                        .Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty());
                })
                .OnSelectionChanged_Lambda([OnEnhancementWorkflowOptionSelected = Args.OnEnhancementWorkflowOptionSelected](TSharedPtr<FString> SelectedItem, ESelectInfo::Type)
                {
                    if (OnEnhancementWorkflowOptionSelected)
                    {
                        OnEnhancementWorkflowOptionSelected(SelectedItem);
                    }
                })
                [
                    SNew(STextBlock)
                    .Text_Lambda([GetSelectedEnhancementWorkflowOption = Args.GetSelectedEnhancementWorkflowOption]()
                    {
                        const TSharedPtr<FString> SelectedOption = GetSelectedEnhancementWorkflowOption ? GetSelectedEnhancementWorkflowOption() : nullptr;
                        return SelectedOption.IsValid() ? FText::FromString(*SelectedOption) : LOCTEXT("WanaWorksCharacterEnhancementWorkflowDefault", "Use Original Character");
                    })
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 6.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("WanaWorksCharacterEnhancementPresetLabel", "Starter Preset"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SBox)
            .WidthOverride(220.0f)
            [
                SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(Args.EnhancementPresetOptions)
                .InitiallySelectedItem(Args.GetSelectedEnhancementPresetOption ? Args.GetSelectedEnhancementPresetOption() : nullptr)
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                {
                    return SNew(STextBlock)
                        .Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty());
                })
                .OnSelectionChanged_Lambda([OnEnhancementPresetOptionSelected = Args.OnEnhancementPresetOptionSelected](TSharedPtr<FString> SelectedItem, ESelectInfo::Type)
                {
                    if (OnEnhancementPresetOptionSelected)
                    {
                        OnEnhancementPresetOptionSelected(SelectedItem);
                    }
                })
                [
                    SNew(STextBlock)
                    .Text_Lambda([GetSelectedEnhancementPresetOption = Args.GetSelectedEnhancementPresetOption]()
                    {
                        const TSharedPtr<FString> SelectedOption = GetSelectedEnhancementPresetOption ? GetSelectedEnhancementPresetOption() : nullptr;
                        return SelectedOption.IsValid() ? FText::FromString(*SelectedOption) : LOCTEXT("WanaWorksCharacterEnhancementDefault", "Identity Only");
                    })
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 10.0f, 0.0f, 0.0f)
        [
            MakeFixedWidthButton(
                LOCTEXT("WanaWorksApplyCharacterEnhancementButton", "Apply WanaAI Enhancement"),
                [OnApplyCharacterEnhancement = Args.OnApplyCharacterEnhancement]()
                {
                    if (OnApplyCharacterEnhancement)
                    {
                        OnApplyCharacterEnhancement();
                    }
                },
                220.0f)
        ],
        LOCTEXT("WanaWorksCharacterEnhancementDescription", "Step 2. Choose how you want to enhance the current subject. Create Sandbox Duplicate is the safest testing path, and every option adds WanaAI on top of the existing setup instead of replacing it."),
        LOCTEXT("WanaWorksReadyStatus", "READY"),
        true);
}

TSharedRef<SWidget> MakeSubjectSetupSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksSubjectSetupSection", "Subject Setup"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksSubjectSetupSummaryLabel", "Selected Subject"),
                [GetSubjectSetupSummaryText = Args.GetSubjectSetupSummaryText]()
                {
                    return GetSubjectSetupSummaryText ? GetSubjectSetupSummaryText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksSubjectStackSummaryLabel", "Detected Stack"),
                [GetSubjectStackSummaryText = Args.GetSubjectStackSummaryText]()
                {
                    return GetSubjectStackSummaryText ? GetSubjectStackSummaryText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SWrapBox)
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksSubjectSetupApplyButton", "Apply WanaAI Enhancement"),
                    [OnApplyCharacterEnhancement = Args.OnApplyCharacterEnhancement]()
                    {
                        if (OnApplyCharacterEnhancement)
                        {
                            OnApplyCharacterEnhancement();
                        }
                    },
                    220.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksSubjectSetupAIControllerButton", "Create AI-Ready Controller"),
                    [OnCreateAIReadyController = Args.OnCreateAIReadyController]()
                    {
                        if (OnCreateAIReadyController)
                        {
                            OnCreateAIReadyController();
                        }
                    },
                    220.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksSubjectSetupConvertButton", "Convert to AI-Ready Subject"),
                    [OnConvertToAIReadySubject = Args.OnConvertToAIReadySubject]()
                    {
                        if (OnConvertToAIReadySubject)
                        {
                            OnConvertToAIReadySubject();
                        }
                    },
                    220.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksSubjectSetupSaveButton", "Save Progress"),
                    [OnSaveSubjectProgress = Args.OnSaveSubjectProgress]()
                    {
                        if (OnSaveSubjectProgress)
                        {
                            OnSaveSubjectProgress();
                        }
                    },
                    160.0f)
            ]
        ],
        LOCTEXT("WanaWorksSubjectSetupDescription", "Step 1. Start with the selected subject. WanaWorks enhances your existing Character BP, AI Controller, and Animation Blueprint stack without replacing it."),
        LOCTEXT("WanaWorksSubjectSetupStatus", "READY"),
        true);
}

TSharedRef<SWidget> MakeValidationWorkflowSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    const FWanaCommandDefinition* ClassifyDefinition = WanaWorksCommandRegistry::FindCommandDefinitionById(FName(TEXT("classify_selected")));

    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksValidationWorkflowSection", "Validation / Testing"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksValidationGuidedLabel", "Guided Starter Test"),
                [GetGuidedWorkflowSummaryText = Args.GetGuidedWorkflowSummaryText]()
                {
                    return GetGuidedWorkflowSummaryText ? GetGuidedWorkflowSummaryText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 12.0f)
        [
            MakeFixedWidthButton(
                LOCTEXT("WanaWorksApplyStarterAndTestButton", "Apply Starter + Test Target"),
                [OnApplyStarterAndTestTarget = Args.OnApplyStarterAndTestTarget]()
                {
                    if (OnApplyStarterAndTestTarget)
                    {
                        OnApplyStarterAndTestTarget();
                    }
                },
                220.0f)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksValidationLiveTestLabel", "Live Test"),
                [GetLiveTestSummaryText = Args.GetLiveTestSummaryText]()
            {
                return GetLiveTestSummaryText ? GetLiveTestSummaryText() : FText::GetEmpty();
            })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 12.0f)
        [
            MakeFixedWidthButton(
                LOCTEXT("WanaWorksEvaluateTargetButton", "Evaluate Target"),
                [OnEvaluateLiveTarget = Args.OnEvaluateLiveTarget]()
                {
                    if (OnEvaluateLiveTarget)
                    {
                        OnEvaluateLiveTarget();
                    }
                },
                180.0f)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksValidationSandboxLabel", "Test Sandbox"),
                [GetTestSandboxSummaryText = Args.GetTestSandboxSummaryText]()
                {
                    return GetTestSandboxSummaryText ? GetTestSandboxSummaryText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 12.0f)
        [
            SNew(SWrapBox)
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksUseSelectedObserverButton_Validation", "Use Selected as Observer"),
                    [OnUseSelectedAsSandboxObserver = Args.OnUseSelectedAsSandboxObserver]()
                    {
                        if (OnUseSelectedAsSandboxObserver)
                        {
                            OnUseSelectedAsSandboxObserver();
                        }
                    },
                    180.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksUseSelectedTargetButton_Validation", "Use Selected as Target"),
                    [OnUseSelectedAsSandboxTarget = Args.OnUseSelectedAsSandboxTarget]()
                    {
                        if (OnUseSelectedAsSandboxTarget)
                        {
                            OnUseSelectedAsSandboxTarget();
                        }
                    },
                    180.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksEvaluateSandboxPairButton_Validation", "Evaluate Current Pair"),
                    [OnEvaluateSandboxPair = Args.OnEvaluateSandboxPair]()
                    {
                        if (OnEvaluateSandboxPair)
                        {
                            OnEvaluateSandboxPair();
                        }
                    },
                    180.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksFocusObserverButton_Validation", "Focus Observer"),
                    [OnFocusSandboxObserver = Args.OnFocusSandboxObserver]()
                    {
                        if (OnFocusSandboxObserver)
                        {
                            OnFocusSandboxObserver();
                        }
                    },
                    160.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksFocusTargetButton_Validation", "Focus Target"),
                    [OnFocusSandboxTarget = Args.OnFocusSandboxTarget]()
                    {
                        if (OnFocusSandboxTarget)
                        {
                            OnFocusSandboxTarget();
                        }
                    },
                    160.0f)
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksValidationWITLabel", "WIT Environment Readiness"),
                [GetWITEnvironmentReadinessText = Args.GetWITEnvironmentReadinessText]()
                {
                    return GetWITEnvironmentReadinessText ? GetWITEnvironmentReadinessText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SWrapBox)
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                ClassifyDefinition
                    ? MakeCommandButton(Args, *ClassifyDefinition)
                    : MakeFixedWidthButton(FText::FromString(TEXT("Classify Selected")), []() {})
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksScanEnvironmentReadinessButton_Validation", "Scan Environment Readiness"),
                    [OnScanEnvironmentReadiness = Args.OnScanEnvironmentReadiness]()
                    {
                        if (OnScanEnvironmentReadiness)
                        {
                            OnScanEnvironmentReadiness();
                        }
                    },
                    210.0f)
            ]
        ],
        LOCTEXT("WanaWorksValidationWorkflowDescription", "Step 3. Validate the subject with guided tests, explicit sandbox pairs, and WIT readiness checks so movement and reactions are safe before deeper AI wiring."),
        LOCTEXT("WanaWorksValidationWorkflowStatus", "LIVE"));
}

TSharedRef<SWidget> MakeResultsWorkflowSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksResultsWorkflowSection", "Results"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksResultsEnhancementLabel", "Enhancement Results"),
                [GetEnhancementResultsText = Args.GetEnhancementResultsText]()
                {
                    return GetEnhancementResultsText ? GetEnhancementResultsText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksResultsBehaviorLabel", "Behavior Results"),
                [GetBehaviorResultsText = Args.GetBehaviorResultsText]()
                {
                    return GetBehaviorResultsText ? GetBehaviorResultsText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksResultsSavedProgressLabel", "Saved Progress"),
                [GetSavedSubjectProgressText = Args.GetSavedSubjectProgressText]()
                {
                    return GetSavedSubjectProgressText ? GetSavedSubjectProgressText() : FText::GetEmpty();
                })
        ],
        LOCTEXT("WanaWorksResultsWorkflowDescription", "Step 4. Review the latest enhancement state, current behavior status, and saved subject progress without digging through the output log."),
        LOCTEXT("WanaWorksResultsWorkflowStatus", "SAFE"));
}

TSharedRef<SWidget> MakeCommandButton(const FWanaWorksUITabBuilderArgs& Args, const FWanaCommandDefinition& Definition)
{
    return MakeFixedWidthButton(
        FText::FromString(Definition.DisplayLabel),
        [OnExecuteCommandText = Args.OnExecuteCommandText, CommandTextValue = FString(Definition.CommandText)]()
        {
            if (OnExecuteCommandText)
            {
                OnExecuteCommandText(CommandTextValue);
            }
        });
}

TSharedRef<SWidget> MakeSection(
    const FSlateFontInfo& SectionHeaderFont,
    const FText& Title,
    const TSharedRef<SWidget>& Content,
    const TOptional<FText>& Description,
    const TOptional<FText>& StatusText,
    bool bProminent)
{
    const FSlateFontInfo DescriptionFont = FCoreStyle::GetDefaultFontStyle("Regular", 9);
    const FSlateFontInfo EffectiveHeaderFont = bProminent ? FCoreStyle::GetDefaultFontStyle("Bold", 12) : SectionHeaderFont;
    TSharedRef<SVerticalBox> SectionLayout = SNew(SVerticalBox);

    SectionLayout->AddSlot()
    .AutoHeight()
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Font(EffectiveHeaderFont)
            .Text(Title)
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        [
            StatusText.IsSet()
                ? StaticCastSharedRef<SWidget>(MakeStatusBadge(StatusText.GetValue()))
                : StaticCastSharedRef<SWidget>(SNew(SSpacer).Size(FVector2D::ZeroVector))
        ]
    ];

    if (Description.IsSet())
    {
        SectionLayout->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 4.0f, 0.0f, 0.0f)
        [
            SNew(STextBlock)
            .Font(DescriptionFont)
            .AutoWrapText(true)
            .ColorAndOpacity(SecondaryTextColor)
            .Text(Description.GetValue())
        ];
    }

    SectionLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 8.0f, 0.0f, 10.0f)
    [
        SNew(SSeparator)
    ];

    SectionLayout->AddSlot()
    .AutoHeight()
    [
        Content
    ];

    return SNew(SBorder)
        .Padding(bProminent ? FMargin(16.0f, 14.0f) : FMargin(14.0f, 12.0f))
        [
            SectionLayout
        ];
}

TSharedRef<SWidget> MakeWrappedButtonSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont, const FText& Title, std::initializer_list<const TCHAR*> CommandIds, const TOptional<FText>& Description = TOptional<FText>())
{
    TSharedRef<SWrapBox> WrapBox = SNew(SWrapBox);

    for (const TCHAR* CommandId : CommandIds)
    {
        if (const FWanaCommandDefinition* Definition = WanaWorksCommandRegistry::FindCommandDefinitionById(FName(CommandId)))
        {
            WrapBox->AddSlot()
                .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
                [
                    MakeCommandButton(Args, *Definition)
                ];
        }
    }

    return MakeSection(SectionHeaderFont, Title, WrapBox, Description);
}

TSharedRef<SWidget> MakeWaySection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    const FWanaCommandDefinition* EnsureDefinition = WanaWorksCommandRegistry::FindCommandDefinitionById(FName(TEXT("ensure_relationship_profile")));
    const FWanaCommandDefinition* ShowDefinition = WanaWorksCommandRegistry::FindCommandDefinitionById(FName(TEXT("show_relationship_profile")));
    const FWanaCommandDefinition* ApplyDefinition = WanaWorksCommandRegistry::FindCommandDefinitionById(FName(TEXT("apply_relationship_state")));

    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksWAYSection", "WAY"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            SNew(SBorder)
            .Padding(10.0f)
            [
                SNew(STextBlock)
                .AutoWrapText(true)
                .Text_Lambda([GetRelationshipSummaryText = Args.GetRelationshipSummaryText]()
                {
                    return GetRelationshipSummaryText ? GetRelationshipSummaryText() : FText::GetEmpty();
                })
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 6.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("WanaWorksRelationshipStateLabel", "Relationship State For Target"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(Args.RelationshipStateOptions)
                .InitiallySelectedItem(Args.GetSelectedRelationshipStateOption ? Args.GetSelectedRelationshipStateOption() : nullptr)
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                {
                    return SNew(STextBlock)
                        .Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty());
                })
                .OnSelectionChanged_Lambda([OnRelationshipStateOptionSelected = Args.OnRelationshipStateOptionSelected](TSharedPtr<FString> SelectedItem, ESelectInfo::Type)
                {
                    if (OnRelationshipStateOptionSelected)
                    {
                        OnRelationshipStateOptionSelected(SelectedItem);
                    }
                })
                [
                    SNew(STextBlock)
                    .Text_Lambda([GetSelectedRelationshipStateOption = Args.GetSelectedRelationshipStateOption]()
                    {
                        const TSharedPtr<FString> SelectedOption = GetSelectedRelationshipStateOption ? GetSelectedRelationshipStateOption() : nullptr;
                        return SelectedOption.IsValid() ? FText::FromString(*SelectedOption) : LOCTEXT("WanaWorksRelationshipStateDefault", "Neutral");
                    })
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 10.0f, 0.0f, 0.0f)
        [
            SNew(SWrapBox)
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                EnsureDefinition
                    ? MakeCommandButton(Args, *EnsureDefinition)
                    : MakeFixedWidthButton(FText::FromString(TEXT("Ensure Relationship Profile")), []() {})
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                ShowDefinition
                    ? MakeCommandButton(Args, *ShowDefinition)
                    : MakeFixedWidthButton(FText::FromString(TEXT("Show Relationship Profile")), []() {})
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    FText::FromString(ApplyDefinition ? ApplyDefinition->DisplayLabel : TEXT("Apply Relationship State")),
                    [OnApplyRelationshipState = Args.OnApplyRelationshipState]()
                    {
                        if (OnApplyRelationshipState)
                        {
                            OnApplyRelationshipState();
                        }
                    })
            ]
        ],
        LOCTEXT("WanaWorksWAYDescription", "Controls how an AI interprets and relates to another actor over time."));
}

TSharedRef<SWidget> MakeIdentitySection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksIdentitySection", "Identity"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            SNew(SBorder)
            .Padding(10.0f)
            [
                SNew(STextBlock)
                .AutoWrapText(true)
                .Text_Lambda([GetIdentitySummaryText = Args.GetIdentitySummaryText]()
                {
                    return GetIdentitySummaryText ? GetIdentitySummaryText() : FText::GetEmpty();
                })
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 6.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("WanaWorksFactionTagLabel", "Faction Tag"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 10.0f)
            [
                SNew(SEditableTextBox)
                .HintText(LOCTEXT("WanaWorksFactionTagHint", "Enter faction tag"))
                .Text_Lambda([GetIdentityFactionTagText = Args.GetIdentityFactionTagText]()
                {
                    return GetIdentityFactionTagText ? GetIdentityFactionTagText() : FText::GetEmpty();
                })
                .OnTextChanged_Lambda([OnIdentityFactionTagTextChanged = Args.OnIdentityFactionTagTextChanged](const FText& NewText)
                {
                    if (OnIdentityFactionTagTextChanged)
                    {
                        OnIdentityFactionTagTextChanged(NewText);
                    }
                })
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 6.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("WanaWorksIdentitySeedLabel", "Default Relationship Seed"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(Args.RelationshipStateOptions)
                .InitiallySelectedItem(Args.GetSelectedIdentitySeedStateOption ? Args.GetSelectedIdentitySeedStateOption() : nullptr)
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                {
                    return SNew(STextBlock)
                        .Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty());
                })
                .OnSelectionChanged_Lambda([OnIdentitySeedStateOptionSelected = Args.OnIdentitySeedStateOptionSelected](TSharedPtr<FString> SelectedItem, ESelectInfo::Type)
                {
                    if (OnIdentitySeedStateOptionSelected)
                    {
                        OnIdentitySeedStateOptionSelected(SelectedItem);
                    }
                })
                [
                    SNew(STextBlock)
                    .Text_Lambda([GetSelectedIdentitySeedStateOption = Args.GetSelectedIdentitySeedStateOption]()
                    {
                        const TSharedPtr<FString> SelectedOption = GetSelectedIdentitySeedStateOption ? GetSelectedIdentitySeedStateOption() : nullptr;
                        return SelectedOption.IsValid() ? FText::FromString(*SelectedOption) : LOCTEXT("WanaWorksIdentitySeedDefault", "Neutral");
                    })
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 10.0f, 0.0f, 0.0f)
        [
            SNew(SWrapBox)
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksEnsureIdentityButton", "Ensure Identity Component"),
                    [OnEnsureIdentityComponent = Args.OnEnsureIdentityComponent]()
                    {
                        if (OnEnsureIdentityComponent)
                        {
                            OnEnsureIdentityComponent();
                        }
                    })
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksApplyIdentityButton", "Apply Identity"),
                    [OnApplyIdentity = Args.OnApplyIdentity]()
                    {
                        if (OnApplyIdentity)
                        {
                            OnApplyIdentity();
                        }
                    })
            ]
        ],
        LOCTEXT("WanaWorksIdentityDescription", "Defines how this actor is recognized by other systems, including faction and relationship seed data."));
}

TSharedRef<SWidget> MakeWanaAISection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksWanaAISection", "WanaAI"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing + 4.0f)
        [
            MakeSubjectSetupSection(Args, SectionHeaderFont)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeCharacterEnhancementSection(Args, SectionHeaderFont)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeValidationWorkflowSection(Args, SectionHeaderFont)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeResultsWorkflowSection(Args, SectionHeaderFont)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeWrappedButtonSection(
                Args,
                SectionHeaderFont,
                LOCTEXT("WanaWorksWAISection", "WAI"),
                {
                    TEXT("add_memory_test"),
                    TEXT("show_memory")
                },
                LOCTEXT("WanaWorksWAIDescription", "Controls the AI's internal memory, personality, and emotional state."))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeIdentitySection(Args, SectionHeaderFont)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeWaySection(Args, SectionHeaderFont)
        ],
        TOptional<FText>(),
        LOCTEXT("WanaWorksActiveStatus", "ACTIVE"));
}

TSharedRef<SWidget> MakeAdvancedCommandSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksAdvancedCommandSection", "Advanced Command"),
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(0.0f, 0.0f, 8.0f, 0.0f)
        [
            SNew(SEditableTextBox)
            .HintText(LOCTEXT("WanaWorksCommandHint", "Enter command"))
            .Text_Lambda([GetCommandText = Args.GetCommandText]()
            {
                return GetCommandText ? GetCommandText() : FText::GetEmpty();
            })
            .OnTextChanged_Lambda([OnCommandTextChanged = Args.OnCommandTextChanged](const FText& NewText)
            {
                if (OnCommandTextChanged)
                {
                    OnCommandTextChanged(NewText);
                }
            })
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0.0f, 0.0f, 8.0f, 0.0f)
        [
            SNew(SBox)
            .WidthOverride(120.0f)
            .HeightOverride(30.0f)
            [
                SNew(SButton)
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                .Text(LOCTEXT("WanaWorksRunButton", "Run"))
                .OnClicked_Lambda([OnRunCommand = Args.OnRunCommand]()
                {
                    if (OnRunCommand)
                    {
                        OnRunCommand();
                    }

                    return FReply::Handled();
                })
            ]
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(SBox)
            .WidthOverride(120.0f)
            .HeightOverride(30.0f)
            [
                SNew(SButton)
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                .Text(LOCTEXT("WanaWorksClearLogButton", "Clear Log"))
                .OnClicked_Lambda([OnClearLog = Args.OnClearLog]()
                {
                    if (OnClearLog)
                    {
                        OnClearLog();
                    }

                    return FReply::Handled();
                })
            ]
        ]);
}

TSharedRef<SWidget> MakeOutputSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksOutputSection", "Output"),
        SNew(SBorder)
        .Padding(10.0f)
        .BorderBackgroundColor(FLinearColor(0.05f, 0.08f, 0.11f, 0.85f))
        [
            SNew(SBox)
            .MinDesiredHeight(240.0f)
            [
                SNew(SMultiLineEditableTextBox)
                .IsReadOnly(true)
                .Text_Lambda([GetLogText = Args.GetLogText]()
                {
                    return GetLogText ? GetLogText() : FText::GetEmpty();
                })
            ]
        ],
        LOCTEXT("WanaWorksOutputDescription", "Latest action results, live test feedback, and structured observer-target summaries."));
}
}

namespace WanaWorksUITabBuilder
{
TSharedRef<SWidget> BuildTabContent(const FWanaWorksUITabBuilderArgs& Args)
{
    const FSlateFontInfo SectionHeaderFont = FCoreStyle::GetDefaultFontStyle("Bold", 11);
    const FSlateFontInfo PanelHeaderFont = FCoreStyle::GetDefaultFontStyle("Bold", 12);

    return SNew(SBorder)
        .Padding(16.0f)
        [
            SNew(SScrollBox)
            + SScrollBox::Slot()
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 12.0f)
                [
                    SNew(SBorder)
                    .Padding(FMargin(12.0f, 10.0f))
                    .BorderBackgroundColor(FLinearColor(0.08f, 0.12f, 0.18f, 0.92f))
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(0.0f, 0.0f, 8.0f, 0.0f)
                        [
                            SNew(SBorder)
                            .Padding(FMargin(8.0f, 4.0f))
                            .BorderBackgroundColor_Lambda([GetStatusText = Args.GetStatusText]()
                            {
                                return GetStatusBadgeColor(GetStatusLabelFromMessage(GetStatusText ? GetStatusText() : FText::GetEmpty()));
                            })
                            [
                                SNew(STextBlock)
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
                                .ColorAndOpacity(FLinearColor::White)
                                .Text_Lambda([GetStatusText = Args.GetStatusText]()
                                {
                                    return FText::FromString(GetStatusLabelFromMessage(GetStatusText ? GetStatusText() : FText::GetEmpty()));
                                })
                            ]
                        ]
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                            .Font(PanelHeaderFont)
                            .Text_Lambda([GetStatusText = Args.GetStatusText]()
                            {
                                return GetStatusText ? GetStatusText() : FText::GetEmpty();
                            })
                        ]
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 10.0f)
                [
                    MakeWrappedButtonSection(
                        Args,
                        SectionHeaderFont,
                        LOCTEXT("WanaWorksWeatherSection", "Weather"),
                        {
                            TEXT("weather_clear"),
                            TEXT("weather_overcast"),
                            TEXT("weather_storm")
                        })
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 10.0f)
                [
                    MakeWanaAISection(Args, SectionHeaderFont)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 12.0f)
                [
                    MakeWrappedButtonSection(
                        Args,
                        SectionHeaderFont,
                        LOCTEXT("WanaWorksSceneSection", "Scene"),
                        {
                            TEXT("spawn_cube"),
                            TEXT("list_selection")
                        })
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 12.0f)
                [
                    MakeAdvancedCommandSection(Args, SectionHeaderFont)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 2.0f, 0.0f, 12.0f)
                [
                    SNew(SSeparator)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    MakeOutputSection(Args, SectionHeaderFont)
                ]
            ]
        ];
}
}

#undef LOCTEXT_NAMESPACE
