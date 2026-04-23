#include "WanaWorksUITabBuilder.h"

#include "AssetThumbnail.h"
#include "WanaWorksCommandRegistry.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SCompoundWidget.h"
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
TSharedRef<SWidget> MakeSandboxPreviewSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont);
constexpr float WanaAISubsectionSpacing = 10.0f;
const FLinearColor SecondaryTextColor(0.73f, 0.78f, 0.86f, 1.0f);
const FLinearColor TertiaryTextColor(0.49f, 0.56f, 0.67f, 1.0f);
const FLinearColor InfoPanelColor(0.06f, 0.08f, 0.13f, 0.88f);
const FLinearColor PreviewPanelColor(0.03f, 0.05f, 0.09f, 0.98f);
const FLinearColor StudioPanelColor(0.045f, 0.06f, 0.10f, 0.96f);
const FLinearColor StudioPanelRaisedColor(0.07f, 0.09f, 0.14f, 0.98f);
const FLinearColor StudioPanelElevatedColor(0.09f, 0.11f, 0.18f, 1.0f);
const FLinearColor StudioOutlineColor(0.16f, 0.20f, 0.31f, 0.95f);
const FLinearColor StudioDividerColor(0.11f, 0.14f, 0.22f, 0.95f);
const FLinearColor StudioAccentColor(0.43f, 0.26f, 0.84f, 1.0f);
const FLinearColor StudioAccentBlueColor(0.24f, 0.47f, 0.88f, 1.0f);
const FLinearColor StudioMutedButtonColor(0.07f, 0.09f, 0.14f, 1.0f);

bool IsLiveWorkspaceLabel(const FString& WorkspaceLabel)
{
    return WorkspaceLabel.Equals(TEXT("AI"), ESearchCase::IgnoreCase)
        || WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase);
}

TSharedRef<SWidget> MakeStudioDivider(float Height = 1.0f, const FLinearColor& DividerColor = StudioDividerColor)
{
    return SNew(SBorder)
        .Padding(0.0f)
        .BorderBackgroundColor(DividerColor)
        [
            SNew(SBox)
            .HeightOverride(Height)
        ];
}

TSharedRef<SWidget> MakeStudioPill(
    const FText& Label,
    const FLinearColor& FillColor,
    const FLinearColor& TextColor = FLinearColor::White,
    int32 FontSize = 8,
    const FMargin& Padding = FMargin(10.0f, 4.0f))
{
    return SNew(SBorder)
        .Padding(Padding)
        .BorderBackgroundColor(FillColor)
        [
            SNew(STextBlock)
            .ColorAndOpacity(TextColor)
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", FontSize))
            .Text(Label)
        ];
}

struct FStudioSummaryRow
{
    FString Label;
    FString Value;
    bool bIsNote = false;
};

TArray<FStudioSummaryRow> ParseStudioSummaryRows(const FText& SummaryText, int32 MaxRows = MAX_int32)
{
    TArray<FStudioSummaryRow> ParsedRows;
    TArray<FString> RawLines;
    SummaryText.ToString().ParseIntoArrayLines(RawLines, true);

    for (const FString& RawLine : RawLines)
    {
        const FString TrimmedLine = RawLine.TrimStartAndEnd();

        if (TrimmedLine.IsEmpty())
        {
            continue;
        }

        FStudioSummaryRow& Row = ParsedRows.AddDefaulted_GetRef();
        const int32 ColonIndex = TrimmedLine.Find(TEXT(":"));

        if (ColonIndex > 0)
        {
            Row.Label = TrimmedLine.Left(ColonIndex).TrimStartAndEnd();
            Row.Value = TrimmedLine.Mid(ColonIndex + 1).TrimStartAndEnd();
        }
        else
        {
            Row.Value = TrimmedLine;
            Row.bIsNote = true;
        }

        if (ParsedRows.Num() >= MaxRows)
        {
            break;
        }
    }

    return ParsedRows;
}

TSharedRef<SWidget> BuildStudioSummaryRowsWidget(const FText& SummaryText, int32 MaxRows, float ValueWrapWidth = 220.0f)
{
    const TArray<FStudioSummaryRow> Rows = ParseStudioSummaryRows(SummaryText, MaxRows);
    TSharedRef<SVerticalBox> Layout = SNew(SVerticalBox);

    if (Rows.IsEmpty())
    {
        Layout->AddSlot()
        .AutoHeight()
        [
            SNew(STextBlock)
            .ColorAndOpacity(SecondaryTextColor)
            .Text(LOCTEXT("WanaWorksStudioSummaryEmpty", "No live details yet."))
        ];

        return Layout;
    }

    for (const FStudioSummaryRow& Row : Rows)
    {
        if (Row.bIsNote || Row.Label.IsEmpty())
        {
            Layout->AddSlot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 6.0f)
            [
                SNew(STextBlock)
                .AutoWrapText(true)
                .ColorAndOpacity(FLinearColor(0.88f, 0.91f, 0.95f, 1.0f))
                .Text(FText::FromString(Row.Value))
            ];

            continue;
        }

        Layout->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 8.0f)
        [
            SNew(SBorder)
            .Padding(FMargin(10.0f, 9.0f))
            .BorderBackgroundColor(FLinearColor(0.055f, 0.07f, 0.11f, 0.98f))
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(0.42f)
                .Padding(0.0f, 0.0f, 10.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
                    .ColorAndOpacity(TertiaryTextColor)
                    .Text(FText::FromString(Row.Label))
                ]
                + SHorizontalBox::Slot()
                .FillWidth(0.58f)
                [
                    SNew(SBox)
                    .MaxDesiredWidth(ValueWrapWidth)
                    [
                        SNew(STextBlock)
                        .AutoWrapText(true)
                        .ColorAndOpacity(FLinearColor(0.97f, 0.98f, 1.0f, 1.0f))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                        .Text(FText::FromString(Row.Value))
                    ]
                ]
            ]
        ];
    }

    return Layout;
}

class SWanaWorksStudioSummaryCard : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SWanaWorksStudioSummaryCard)
        : _AccentColor(StudioAccentColor)
        , _MaxRows(6)
        , _ValueWrapWidth(220.0f)
    {
    }
        SLATE_ARGUMENT(FText, Title)
        SLATE_ARGUMENT(FText, Eyebrow)
        SLATE_ARGUMENT(FLinearColor, AccentColor)
        SLATE_ARGUMENT(int32, MaxRows)
        SLATE_ARGUMENT(float, ValueWrapWidth)
        SLATE_ARGUMENT(TFunction<FText(void)>, GetSummaryText)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        Title = InArgs._Title;
        Eyebrow = InArgs._Eyebrow;
        AccentColor = InArgs._AccentColor;
        MaxRows = InArgs._MaxRows;
        ValueWrapWidth = InArgs._ValueWrapWidth;
        GetSummaryText = InArgs._GetSummaryText;

        ChildSlot
        [
            SNew(SBorder)
            .Padding(1.0f)
            .BorderBackgroundColor(StudioOutlineColor)
            [
                SNew(SBorder)
                .Padding(18.0f)
                .BorderBackgroundColor(StudioPanelRaisedColor)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        SNew(SBorder)
                        .Padding(0.0f)
                        .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.92f))
                        [
                            SNew(SBox)
                            .HeightOverride(3.0f)
                        ]
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 12.0f)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(SBox)
                                .Visibility_Lambda([this]()
                                {
                                    return Eyebrow.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible;
                                })
                                [
                                    SNew(STextBlock)
                                    .ColorAndOpacity(TertiaryTextColor)
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
                                    .Text(Eyebrow)
                                ]
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                            [
                                SNew(STextBlock)
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
                                .ColorAndOpacity(FLinearColor::White)
                                .Text(Title)
                            ]
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Top)
                        [
                            MakeStudioPill(
                                LOCTEXT("WanaWorksStudioLiveBadge", "LIVE DATA"),
                                AccentColor.CopyWithNewOpacity(0.22f),
                                AccentColor,
                                8,
                                FMargin(10.0f, 5.0f))
                        ]
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 12.0f)
                    [
                        MakeStudioDivider()
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SAssignNew(BodyBox, SBox)
                    ]
                ]
            ]
        ];

        RefreshBody();
    }

    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
    {
        SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

        const FString CurrentSummary = GetSummaryText ? GetSummaryText().ToString() : FString();

        if (CurrentSummary != CachedSummary)
        {
            RefreshBody();
        }
    }

private:
    void RefreshBody()
    {
        CachedSummary = GetSummaryText ? GetSummaryText().ToString() : FString();

        if (!BodyBox.IsValid())
        {
            return;
        }

        BodyBox->SetContent(BuildStudioSummaryRowsWidget(FText::FromString(CachedSummary), MaxRows, ValueWrapWidth));
    }

    FText Title;
    FText Eyebrow;
    FLinearColor AccentColor = StudioAccentColor;
    int32 MaxRows = 6;
    float ValueWrapWidth = 220.0f;
    TFunction<FText(void)> GetSummaryText;
    TSharedPtr<SBox> BodyBox;
    FString CachedSummary;
};

class SWanaWorksSandboxPreviewCard : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SWanaWorksSandboxPreviewCard)
    {
    }
        SLATE_ARGUMENT(TFunction<UObject*(void)>, GetPreviewObject)
        SLATE_ARGUMENT(TFunction<FText(void)>, GetPreviewSummaryText)
        SLATE_ARGUMENT(TFunction<FString(void)>, GetSelectedPreviewViewLabel)
        SLATE_ARGUMENT(TFunction<void(void)>, OnFocusPreviewSubject)
        SLATE_ARGUMENT(TFunction<void(const FString&)>, OnSelectPreviewView)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        GetPreviewObject = InArgs._GetPreviewObject;
        GetPreviewSummaryText = InArgs._GetPreviewSummaryText;
        GetSelectedPreviewViewLabel = InArgs._GetSelectedPreviewViewLabel;
        OnFocusPreviewSubject = InArgs._OnFocusPreviewSubject;
        OnSelectPreviewView = InArgs._OnSelectPreviewView;
        ThumbnailPool = MakeShared<FAssetThumbnailPool>(16, false);

        ChildSlot
        [
            SNew(SBorder)
            .Padding(1.0f)
            .BorderBackgroundColor(StudioOutlineColor)
            [
                SNew(SBorder)
                .Padding(18.0f)
                .BorderBackgroundColor(PreviewPanelColor)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 12.0f)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .VAlign(VAlign_Center)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(STextBlock)
                                .ColorAndOpacity(TertiaryTextColor)
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
                                .Text(LOCTEXT("WanaWorksSandboxPreviewCardEyebrow", "STUDIO STAGE"))
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                            [
                                SNew(STextBlock)
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
                                .ColorAndOpacity(FLinearColor::White)
                                .Text(LOCTEXT("WanaWorksSandboxPreviewCardTitle", "Workspace Preview"))
                            ]
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(12.0f, 0.0f, 12.0f, 0.0f)
                        [
                            SNew(SBorder)
                            .Padding(FMargin(10.0f, 5.0f))
                            .BorderBackgroundColor(StudioAccentColor.CopyWithNewOpacity(0.16f))
                            [
                                SNew(STextBlock)
                                .ColorAndOpacity(FLinearColor(0.88f, 0.84f, 0.98f, 1.0f))
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
                                .Text_Lambda([this]()
                                {
                                    return FText::FromString(GetSelectedPreviewViewLabel ? GetSelectedPreviewViewLabel() : FString(TEXT("Overview")));
                                })
                            ]
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        [
                            SNew(SButton)
                            .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                            .ContentPadding(FMargin(0.0f))
                            .OnClicked_Lambda([this]()
                            {
                                if (OnFocusPreviewSubject)
                                {
                                    OnFocusPreviewSubject();
                                }

                                return FReply::Handled();
                            })
                            [
                                MakeStudioPill(
                                    LOCTEXT("WanaWorksSandboxPreviewFocusButton", "Focus Subject"),
                                    StudioAccentBlueColor.CopyWithNewOpacity(0.18f),
                                    FLinearColor(0.80f, 0.89f, 1.0f, 1.0f),
                                    8,
                                    FMargin(12.0f, 6.0f))
                            ]
                        ]
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 12.0f)
                    [
                        MakeStudioDivider()
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        SNew(SWrapBox)
                        + SWrapBox::Slot()
                        .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
                        [
                            MakeStageViewButton(TEXT("Overview"))
                        ]
                        + SWrapBox::Slot()
                        .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
                        [
                            MakeStageViewButton(TEXT("Front"))
                        ]
                        + SWrapBox::Slot()
                        .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
                        [
                            MakeStageViewButton(TEXT("Back"))
                        ]
                        + SWrapBox::Slot()
                        .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
                        [
                            MakeStageViewButton(TEXT("Left"))
                        ]
                        + SWrapBox::Slot()
                        .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
                        [
                            MakeStageViewButton(TEXT("Right"))
                        ]
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SAssignNew(PreviewContentBox, SBox)
                        .MinDesiredHeight(440.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 14.0f, 0.0f, 0.0f)
                    [
                        SAssignNew(SummaryBox, SBox)
                    ]
                ]
            ]
        ];

        RefreshPreviewContent();
    }

    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
    {
        SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

        UObject* PreviewObject = GetPreviewObject ? GetPreviewObject() : nullptr;

        const FString CurrentSummary = GetPreviewSummaryText ? GetPreviewSummaryText().ToString() : FString();
        const FString CurrentViewLabel = GetSelectedPreviewViewLabel ? GetSelectedPreviewViewLabel() : FString();

        if (PreviewObject != CachedPreviewObject.Get()
            || CurrentSummary != CachedPreviewSummary
            || CurrentViewLabel != CachedPreviewViewLabel)
        {
            RefreshPreviewContent();
        }
    }

private:
    TSharedRef<SWidget> MakeStageViewButton(const FString& ViewLabel)
    {
        return SNew(SButton)
            .ButtonStyle(FCoreStyle::Get(), "NoBorder")
            .ContentPadding(FMargin(0.0f))
            .OnClicked_Lambda([this, ViewLabel]()
            {
                if (OnSelectPreviewView)
                {
                    OnSelectPreviewView(ViewLabel);
                }

                return FReply::Handled();
            })
            [
                SNew(SBorder)
                .Padding(FMargin(14.0f, 8.0f))
                .BorderBackgroundColor_Lambda([this, ViewLabel]()
                {
                    const bool bIsActive = GetSelectedPreviewViewLabel
                        && GetSelectedPreviewViewLabel().Equals(ViewLabel, ESearchCase::IgnoreCase);
                    return bIsActive
                        ? StudioAccentColor.CopyWithNewOpacity(0.30f)
                        : StudioMutedButtonColor;
                })
                [
                    SNew(STextBlock)
                    .ColorAndOpacity_Lambda([this, ViewLabel]()
                    {
                        const bool bIsActive = GetSelectedPreviewViewLabel
                            && GetSelectedPreviewViewLabel().Equals(ViewLabel, ESearchCase::IgnoreCase);
                        return bIsActive
                            ? FLinearColor(0.95f, 0.90f, 1.0f, 1.0f)
                            : SecondaryTextColor;
                    })
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                    .Text(FText::FromString(ViewLabel))
                ]
            ];
    }

    void RefreshPreviewContent()
    {
        CachedPreviewObject = GetPreviewObject ? GetPreviewObject() : nullptr;
        CachedPreviewSummary = GetPreviewSummaryText ? GetPreviewSummaryText().ToString() : FString();
        CachedPreviewViewLabel = GetSelectedPreviewViewLabel ? GetSelectedPreviewViewLabel() : FString();

        if (!PreviewContentBox.IsValid())
        {
            return;
        }

        if (UObject* PreviewObject = CachedPreviewObject.Get())
        {
            AssetThumbnail = MakeShared<FAssetThumbnail>(PreviewObject, 760, 420, ThumbnailPool);

            FAssetThumbnailConfig ThumbnailConfig;
            ThumbnailConfig.bAllowFadeIn = false;

            PreviewContentBox->SetContent(
                SNew(SBorder)
                .Padding(1.0f)
                .BorderBackgroundColor(StudioOutlineColor)
                [
                    SNew(SBorder)
                    .Padding(16.0f)
                    .BorderBackgroundColor(FLinearColor(0.022f, 0.032f, 0.065f, 1.0f))
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 0.0f, 0.0f, 12.0f)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .FillWidth(1.0f)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .Padding(0.0f, 0.0f, 8.0f, 0.0f)
                                [
                                    MakeStudioPill(
                                        LOCTEXT("WanaWorksStudioPreviewLiveBadge", "PREVIEW LIVE"),
                                        StudioAccentBlueColor.CopyWithNewOpacity(0.18f),
                                        FLinearColor(0.84f, 0.91f, 1.0f, 1.0f),
                                        8,
                                        FMargin(10.0f, 5.0f))
                                ]
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                [
                                    MakeStudioPill(
                                        FText::FromString(CachedPreviewViewLabel.IsEmpty() ? TEXT("Overview") : CachedPreviewViewLabel),
                                        StudioAccentColor.CopyWithNewOpacity(0.18f),
                                        FLinearColor(0.95f, 0.90f, 1.0f, 1.0f),
                                        8,
                                        FMargin(10.0f, 5.0f))
                                ]
                            ]
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            [
                                SNew(STextBlock)
                                .ColorAndOpacity(TertiaryTextColor)
                                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
                                .Text(LOCTEXT("WanaWorksStudioPreviewSyncLabel", "WORKSPACE STAGE"))
                            ]
                        ]
                        + SVerticalBox::Slot()
                        .FillHeight(1.0f)
                        [
                            SNew(SBorder)
                            .Padding(14.0f)
                            .BorderBackgroundColor(FLinearColor(0.038f, 0.055f, 0.10f, 1.0f))
                            [
                                AssetThumbnail->MakeThumbnailWidget(ThumbnailConfig)
                            ]
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 14.0f, 0.0f, 10.0f)
                        [
                            MakeStudioDivider(2.0f, StudioAccentColor.CopyWithNewOpacity(0.72f))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .FillWidth(1.0f)
                            [
                                SNew(SVerticalBox)
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                [
                                    SNew(STextBlock)
                                    .ColorAndOpacity(TertiaryTextColor)
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
                                    .Text(LOCTEXT("WanaWorksStudioPreviewAssetLabel", "STAGE SUBJECT"))
                                ]
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(0.0f, 5.0f, 0.0f, 0.0f)
                                [
                                    SNew(STextBlock)
                                    .ColorAndOpacity(FLinearColor::White)
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                                    .Text(FText::FromString(PreviewObject->GetName()))
                                ]
                            ]
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Bottom)
                            [
                                SNew(STextBlock)
                                .ColorAndOpacity(SecondaryTextColor)
                                .AutoWrapText(true)
                                .Text(LOCTEXT("WanaWorksStudioPreviewFooterNote", "Use Focus Subject for full world-space inspection."))
                            ]
                        ]
                    ]
                ]);
        }
        else
        {
            AssetThumbnail.Reset();
            PreviewContentBox->SetContent(
                SNew(SBorder)
                .Padding(1.0f)
                .BorderBackgroundColor(StudioOutlineColor)
                [
                    SNew(SBorder)
                    .Padding(24.0f)
                    .BorderBackgroundColor(FLinearColor(0.025f, 0.035f, 0.07f, 1.0f))
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .FillHeight(1.0f)
                        [
                            SNew(SBorder)
                            .Padding(20.0f)
                            .BorderBackgroundColor(FLinearColor(0.05f, 0.07f, 0.12f, 0.95f))
                            [
                                SNew(SVerticalBox)
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .HAlign(HAlign_Center)
                                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                                [
                                    MakeStudioPill(
                                        LOCTEXT("WanaWorksStudioPreviewStandbyBadge", "STAGE READY"),
                                        StudioAccentBlueColor.CopyWithNewOpacity(0.18f),
                                        FLinearColor(0.84f, 0.91f, 1.0f, 1.0f),
                                        8,
                                        FMargin(12.0f, 5.0f))
                                ]
                                + SVerticalBox::Slot()
                                .FillHeight(1.0f)
                                .VAlign(VAlign_Center)
                                [
                                    SNew(SVerticalBox)
                                    + SVerticalBox::Slot()
                                    .AutoHeight()
                                    .HAlign(HAlign_Center)
                                    .Padding(0.0f, 16.0f, 0.0f, 16.0f)
                                    [
                                        SNew(STextBlock)
                                        .Justification(ETextJustify::Center)
                                        .ColorAndOpacity(FLinearColor::White)
                                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
                                        .Text(LOCTEXT("WanaWorksSandboxPreviewStandbyTitle", "Workspace Stage Ready"))
                                    ]
                                    + SVerticalBox::Slot()
                                    .AutoHeight()
                                    .HAlign(HAlign_Center)
                                    [
                                        SNew(STextBlock)
                                        .Justification(ETextJustify::Center)
                                        .AutoWrapText(true)
                                        .ColorAndOpacity(SecondaryTextColor)
                                        .Text(LOCTEXT("WanaWorksSandboxPreviewPlaceholder", "Choose a subject or run enhancement to wake the stage with a live working preview."))
                                    ]
                                    + SVerticalBox::Slot()
                                    .AutoHeight()
                                    .Padding(0.0f, 18.0f, 0.0f, 0.0f)
                                    .HAlign(HAlign_Center)
                                    [
                                        SNew(SBox)
                                        .WidthOverride(200.0f)
                                        [
                                            MakeStudioDivider(2.0f, StudioAccentColor.CopyWithNewOpacity(0.72f))
                                        ]
                                    ]
                                ]
                            ]
                        ]
                    ]
                ]);
        }

        if (SummaryBox.IsValid())
        {
            SummaryBox->SetContent(
                SNew(SBorder)
                .Padding(1.0f)
                .BorderBackgroundColor(StudioOutlineColor)
                [
                    SNew(SBorder)
                    .Padding(12.0f)
                    .BorderBackgroundColor(InfoPanelColor)
                    [
                        BuildStudioSummaryRowsWidget(FText::FromString(CachedPreviewSummary), 4, 340.0f)
                    ]
                ]);
        }
    }

    TFunction<UObject*(void)> GetPreviewObject;
    TFunction<FText(void)> GetPreviewSummaryText;
    TFunction<FString(void)> GetSelectedPreviewViewLabel;
    TFunction<void(void)> OnFocusPreviewSubject;
    TFunction<void(const FString&)> OnSelectPreviewView;
    TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
    TSharedPtr<FAssetThumbnail> AssetThumbnail;
    TSharedPtr<SBox> PreviewContentBox;
    TSharedPtr<SBox> SummaryBox;
    TWeakObjectPtr<UObject> CachedPreviewObject;
    FString CachedPreviewSummary;
    FString CachedPreviewViewLabel;
};

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

TSharedRef<SWidget> MakeStringPickerControl(
    const FText& Label,
    const TArray<TSharedPtr<FString>>* Options,
    TFunction<TSharedPtr<FString>(void)> GetSelectedOption,
    TFunction<void(TSharedPtr<FString>)> OnSelectionChanged,
    const FText& DefaultText,
    float WidthOverride = 320.0f)
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 8.0f)
        [
            SNew(STextBlock)
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
            .ColorAndOpacity(TertiaryTextColor)
            .Text(Label)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SBox)
            .WidthOverride(WidthOverride)
            [
                SNew(SBorder)
                .Padding(1.0f)
                .BorderBackgroundColor(StudioOutlineColor)
                [
                    SNew(SBorder)
                    .Padding(FMargin(10.0f, 6.0f))
                    .BorderBackgroundColor(StudioMutedButtonColor)
                    [
                        SNew(SComboBox<TSharedPtr<FString>>)
                        .OptionsSource(Options)
                        .InitiallySelectedItem(GetSelectedOption ? GetSelectedOption() : nullptr)
                        .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                        {
                            return SNew(STextBlock)
                                .ColorAndOpacity(FLinearColor::White)
                                .Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty());
                        })
                        .OnSelectionChanged_Lambda([OnSelectionChanged](TSharedPtr<FString> SelectedItem, ESelectInfo::Type)
                        {
                            if (OnSelectionChanged)
                            {
                                OnSelectionChanged(SelectedItem);
                            }
                        })
                        [
                            SNew(STextBlock)
                            .ColorAndOpacity(FLinearColor::White)
                            .Text_Lambda([GetSelectedOption, DefaultText]()
                            {
                                const TSharedPtr<FString> SelectedOption = GetSelectedOption ? GetSelectedOption() : nullptr;
                                return SelectedOption.IsValid() ? FText::FromString(*SelectedOption) : DefaultText;
                            })
                        ]
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeSandboxPreviewSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksSandboxPreviewSection", "Workspace Preview"),
        SNew(SWanaWorksSandboxPreviewCard)
        .GetPreviewObject(Args.GetSandboxPreviewObject)
        .GetPreviewSummaryText(Args.GetSandboxPreviewSummaryText)
        .GetSelectedPreviewViewLabel(Args.GetSelectedPreviewViewLabel)
        .OnFocusPreviewSubject(Args.OnFocusSandboxPreviewSubject)
        .OnSelectPreviewView(Args.OnPreviewViewSelected),
        LOCTEXT("WanaWorksSandboxPreviewDescription", "A lightweight embedded preview card for the current working subject or finalized output. WanaWorks is the workspace, so this stays focused on the subject you are shaping inside the tool."),
        LOCTEXT("WanaWorksSandboxPreviewStatus", "LIVE"),
        true);
}

TSharedRef<SWidget> MakeCoreWorkflowSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksCoreWorkflowSection", "Core Workflow"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksCoreWorkflowGuidanceLabel", "How This Workspace Flows"),
                []()
                {
                    return LOCTEXT("WanaWorksCoreWorkflowGuidanceText", "Choose a subject, inspect it here, create a safe working copy, apply enhancement, test the result, then build a clean final asset. WanaWorks keeps the original source untouched.");
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
                    LOCTEXT("WanaWorksCreateWorkingCopyButton", "Create Copy"),
                    [OnCreateWorkingCopy = Args.OnCreateWorkingCopy]()
                    {
                        if (OnCreateWorkingCopy)
                        {
                            OnCreateWorkingCopy();
                        }
                    },
                    170.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksCoreApplyEnhancementButton", "Apply Enhancement"),
                    [OnApplyCharacterEnhancement = Args.OnApplyCharacterEnhancement]()
                    {
                        if (OnApplyCharacterEnhancement)
                        {
                            OnApplyCharacterEnhancement();
                        }
                    },
                    180.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksCoreTestSubjectButton", "Test Subject"),
                    [OnApplyStarterAndTestTarget = Args.OnApplyStarterAndTestTarget]()
                    {
                        if (OnApplyStarterAndTestTarget)
                        {
                            OnApplyStarterAndTestTarget();
                        }
                    },
                    160.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksCoreBuildFinalButton", "Build Final"),
                    [OnFinalizeSandboxBuild = Args.OnFinalizeSandboxBuild]()
                    {
                        if (OnFinalizeSandboxBuild)
                        {
                            OnFinalizeSandboxBuild();
                        }
                    },
                    160.0f)
            ]
        ],
        LOCTEXT("WanaWorksCoreWorkflowDescription", "Primary actions only. The rest of the setup, pair testing, presets, and internal tools stay available in the secondary details area."),
        LOCTEXT("WanaWorksCoreWorkflowStatus", "READY"),
        true);
}

TSharedRef<SWidget> MakeWorkspaceStatusSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksWorkspaceStatusSection", "Live Status"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksWorkspaceStatusSubjectLabel", "Detected Stack"),
                [GetSubjectStackSummaryText = Args.GetSubjectStackSummaryText]()
                {
                    return GetSubjectStackSummaryText ? GetSubjectStackSummaryText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksWorkspaceStatusAnimationLabel", "Animation Integration"),
                [GetAnimationIntegrationText = Args.GetAnimationIntegrationText]()
                {
                    return GetAnimationIntegrationText ? GetAnimationIntegrationText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksWorkspaceStatusBehaviorLabel", "Behavior Results"),
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
                LOCTEXT("WanaWorksWorkspaceStatusPhysicalLabel", "Physical State"),
                [GetPhysicalStateText = Args.GetPhysicalStateText]()
                {
                    return GetPhysicalStateText ? GetPhysicalStateText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksWorkspaceStatusEnhancementLabel", "Enhancement Results"),
                [GetEnhancementResultsText = Args.GetEnhancementResultsText]()
                {
                    return GetEnhancementResultsText ? GetEnhancementResultsText() : FText::GetEmpty();
                })
        ],
        LOCTEXT("WanaWorksWorkspaceStatusDescription", "Live status stays visible on the right so the main workspace feels like software, not a stack of utility panels."),
        LOCTEXT("WanaWorksWorkspaceStatusStatus", "LIVE"),
        true);
}

TSharedRef<SWidget> MakeCollapsibleWorkspaceDetailsSection(
    const FSlateFontInfo& SectionHeaderFont,
    const FText& Title,
    const FText& Description,
    const TSharedRef<SWidget>& Content)
{
    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(StudioOutlineColor)
        [
            SNew(SBorder)
            .Padding(14.0f)
            .BorderBackgroundColor(FLinearColor(0.045f, 0.06f, 0.10f, 0.92f))
            [
                SNew(SExpandableArea)
                .InitiallyCollapsed(true)
                .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                .BodyBorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
                .HeaderPadding(FMargin(0.0f))
                .HeaderContent()
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
                        .ColorAndOpacity(TertiaryTextColor)
                        .Text(Title)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 5.0f, 0.0f, 0.0f)
                    [
                        SNew(STextBlock)
                        .AutoWrapText(true)
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                        .ColorAndOpacity(SecondaryTextColor)
                        .Text(Description)
                    ]
                ]
                .BodyContent()
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 12.0f, 0.0f, 12.0f)
                    [
                        MakeStudioDivider()
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        Content
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeWorkflowWorkspaceSection(
    const FWanaWorksUITabBuilderArgs& Args,
    const FSlateFontInfo& SectionHeaderFont,
    const FText& Title,
    const FText& Description,
    const TSharedRef<SWidget>& ControlsContent)
{
    return MakeSection(
        SectionHeaderFont,
        Title,
        SNew(SSplitter)
        .PhysicalSplitterHandleSize(2.0f)
        + SSplitter::Slot()
        .Value(0.25f)
        [
            ControlsContent
        ]
        + SSplitter::Slot()
        .Value(0.40f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 10.0f)
            [
                MakeSandboxPreviewSection(Args, SectionHeaderFont)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                MakeCoreWorkflowSection(Args, SectionHeaderFont)
            ]
        ]
        + SSplitter::Slot()
        .Value(0.35f)
        [
            MakeWorkspaceStatusSection(Args, SectionHeaderFont)
        ],
        Description,
        LOCTEXT("WanaWorksSandboxWorkspaceStatus", "LIVE"),
        true);
}

TSharedRef<SWidget> MakeCharacterEnhancementSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksCharacterEnhancementSection", "Enhancement Setup"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksCharacterEnhancementSummaryLabel", "Selected Subject Summary"),
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
        LOCTEXT("WanaWorksCharacterEnhancementDescription", "Choose how you want to enhance the current subject. Create Working Copy is the safest testing path, and every option adds WanaAI on top of the existing setup instead of replacing it."),
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
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksSubjectSetupRestoreButton", "Restore Last Saved State"),
                    [OnRestoreSavedSubjectProgress = Args.OnRestoreSavedSubjectProgress]()
                    {
                        if (OnRestoreSavedSubjectProgress)
                        {
                            OnRestoreSavedSubjectProgress();
                        }
                    },
                    220.0f)
            ]
        ],
        LOCTEXT("WanaWorksSubjectSetupDescription", "Step 1. Start with the selected subject. WanaWorks enhances your existing Character BP, AI Controller, and Animation Blueprint stack without replacing it."),
        LOCTEXT("WanaWorksSubjectSetupStatus", "READY"),
        true);
}

TSharedRef<SWidget> MakeAnimationIntegrationSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksAnimationIntegrationSection", "Animation Integration"),
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksAnimationIntegrationSummaryLabel", "Current Animation Stack"),
            [GetAnimationIntegrationText = Args.GetAnimationIntegrationText]()
            {
                return GetAnimationIntegrationText ? GetAnimationIntegrationText() : FText::GetEmpty();
            }),
        LOCTEXT("WanaWorksAnimationIntegrationDescription", "WanaWorks detects the linked Animation Blueprint from the selected pawn and reports whether the working-copy or finalized path can auto-attach and auto-wire the safe animation bridge without touching the original asset."),
        LOCTEXT("WanaWorksAnimationIntegrationStatus", "SAFE"));
}

TSharedRef<SWidget> MakeAnimationHookUsageSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksAnimationHookUsageSection", "Animation Hook Usage"),
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksAnimationHookUsageSummaryLabel", "How To Use The Current Hook State"),
            [GetAnimationHookUsageText = Args.GetAnimationHookUsageText]()
            {
                return GetAnimationHookUsageText ? GetAnimationHookUsageText() : FText::GetEmpty();
            }),
        LOCTEXT("WanaWorksAnimationHookUsageDescription", "Read the current hook state from the owner's WAYPlayerProfileComponent inside your existing Anim BP. For supported working subjects, WanaWorks can also auto-fill matching Anim BP fields or component references without replacing the Animation Blueprint."),
        LOCTEXT("WanaWorksAnimationHookUsageStatus", "READY"));
}

TSharedRef<SWidget> MakePhysicalStateSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksPhysicalStateSection", "Physical State"),
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksPhysicalStateSummaryLabel", "Current Body-State Layer"),
            [GetPhysicalStateText = Args.GetPhysicalStateText]()
            {
                return GetPhysicalStateText ? GetPhysicalStateText() : FText::GetEmpty();
            }),
        LOCTEXT("WanaWorksPhysicalStateDescription", "This is a lightweight, readable body-state layer for characters. WanaWorks does not replace locomotion, the current Anim BP, or AI control here."),
        LOCTEXT("WanaWorksPhysicalStateStatus", "ACTIVE"));
}

TSharedRef<SWidget> MakePresetsSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    return MakeSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksPresetsSection", "Presets"),
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksPresetSummaryLabel", "Selected Preset Summary"),
                [GetWorkflowPresetSummaryText = Args.GetWorkflowPresetSummaryText]()
                {
                    return GetWorkflowPresetSummaryText ? GetWorkflowPresetSummaryText() : FText::GetEmpty();
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 6.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("WanaWorksPresetDropdownLabel", "Preset"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SBox)
            .WidthOverride(220.0f)
            [
                SNew(SComboBox<TSharedPtr<FString>>)
                .OptionsSource(Args.WorkflowPresetOptions)
                .InitiallySelectedItem(Args.GetSelectedWorkflowPresetOption ? Args.GetSelectedWorkflowPresetOption() : nullptr)
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                {
                    return SNew(STextBlock)
                        .Text(Item.IsValid() ? FText::FromString(*Item) : FText::GetEmpty());
                })
                .OnSelectionChanged_Lambda([OnWorkflowPresetOptionSelected = Args.OnWorkflowPresetOptionSelected](TSharedPtr<FString> SelectedItem, ESelectInfo::Type)
                {
                    if (OnWorkflowPresetOptionSelected)
                    {
                        OnWorkflowPresetOptionSelected(SelectedItem);
                    }
                })
                [
                    SNew(STextBlock)
                    .Text_Lambda([GetSelectedWorkflowPresetOption = Args.GetSelectedWorkflowPresetOption]()
                    {
                        const TSharedPtr<FString> SelectedOption = GetSelectedWorkflowPresetOption ? GetSelectedWorkflowPresetOption() : nullptr;
                        return SelectedOption.IsValid() ? FText::FromString(*SelectedOption) : LOCTEXT("WanaWorksPresetDefault", "Full WanaAI Starter");
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
                    LOCTEXT("WanaWorksApplyPresetButton", "Apply Preset"),
                    [OnApplyWorkflowPreset = Args.OnApplyWorkflowPreset]()
                    {
                        if (OnApplyWorkflowPreset)
                        {
                            OnApplyWorkflowPreset();
                        }
                    },
                    160.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksSavePresetButton", "Save Current as Preset"),
                    [OnSaveWorkflowPreset = Args.OnSaveWorkflowPreset]()
                    {
                        if (OnSaveWorkflowPreset)
                        {
                            OnSaveWorkflowPreset();
                        }
                    },
                    190.0f)
            ]
            + SWrapBox::Slot()
            .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
            [
                MakeFixedWidthButton(
                    LOCTEXT("WanaWorksShowPresetSummaryButton", "Show Preset Summary"),
                    [OnShowWorkflowPresetSummary = Args.OnShowWorkflowPresetSummary]()
                    {
                        if (OnShowWorkflowPresetSummary)
                        {
                            OnShowWorkflowPresetSummary();
                        }
                    },
                    180.0f)
            ]
        ],
        LOCTEXT("WanaWorksPresetsDescription", "Optional shortcut. Pick a reusable WanaWorks starter, apply it safely to the current subject, or save the current workflow as a lightweight reusable preset."),
        LOCTEXT("WanaWorksPresetsStatus", "READY"));
}

TSharedRef<SWidget> MakeValidationWorkflowSection(
    const FWanaWorksUITabBuilderArgs& Args,
    const FSlateFontInfo& SectionHeaderFont,
    const FText& Title,
    const FText& Description,
    const FText& StatusText,
    bool bIncludeEnvironmentReadiness)
{
    const FWanaCommandDefinition* ClassifyDefinition = WanaWorksCommandRegistry::FindCommandDefinitionById(FName(TEXT("classify_selected")));

    TSharedRef<SVerticalBox> Layout = SNew(SVerticalBox);

    Layout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksValidationGuidedLabel", "Guided Starter Test"),
            [GetGuidedWorkflowSummaryText = Args.GetGuidedWorkflowSummaryText]()
            {
                return GetGuidedWorkflowSummaryText ? GetGuidedWorkflowSummaryText() : FText::GetEmpty();
            })
    ];

    Layout->AddSlot()
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
    ];

    Layout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksValidationLiveTestLabel", "Live Test"),
            [GetLiveTestSummaryText = Args.GetLiveTestSummaryText]()
            {
                return GetLiveTestSummaryText ? GetLiveTestSummaryText() : FText::GetEmpty();
            })
    ];

    Layout->AddSlot()
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
    ];

    Layout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksValidationSandboxLabel", "Working Copy Pair"),
            [GetTestSandboxSummaryText = Args.GetTestSandboxSummaryText]()
            {
                return GetTestSandboxSummaryText ? GetTestSandboxSummaryText() : FText::GetEmpty();
            })
    ];

    Layout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksValidationSandboxBuildLabel", "Build Final Asset"),
            []()
            {
                return LOCTEXT("WanaWorksValidationSandboxBuildText", "When the working subject looks right, Build Final creates a separate finalized asset under /Game/WanaWorks/Builds. The original source and the working copy both stay preserved.");
            })
    ];

    Layout->AddSlot()
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
                    LOCTEXT("WanaWorksEvaluateSandboxPairButton_Validation", "Evaluate Working Pair"),
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
                    LOCTEXT("WanaWorksFinalizeSandboxBuildButton_Validation", "Build Final"),
                    [OnFinalizeSandboxBuild = Args.OnFinalizeSandboxBuild]()
                    {
                        if (OnFinalizeSandboxBuild)
                        {
                        OnFinalizeSandboxBuild();
                    }
                },
                190.0f)
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
    ];

    if (bIncludeEnvironmentReadiness)
    {
        Layout->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
        [
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksValidationWITLabel", "WIT Environment Readiness"),
                [GetWITEnvironmentReadinessText = Args.GetWITEnvironmentReadinessText]()
                {
                    return GetWITEnvironmentReadinessText ? GetWITEnvironmentReadinessText() : FText::GetEmpty();
                })
        ];

        Layout->AddSlot()
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
        ];
    }

    return MakeSection(SectionHeaderFont, Title, Layout, Description, StatusText);
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

TSharedRef<SWidget> MakeSubjectPathSection(
    const FSlateFontInfo& SectionHeaderFont,
    const FText& Title,
    const FText& Description,
    const TSharedRef<SWidget>& Content,
    bool bInitiallyCollapsed = true)
{
    return SNew(SBorder)
        .Padding(12.0f)
        .BorderBackgroundColor(FLinearColor(0.06f, 0.09f, 0.13f, 0.84f))
        [
            SNew(SExpandableArea)
            .InitiallyCollapsed(bInitiallyCollapsed)
            .BorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
            .BodyBorderImage(FCoreStyle::Get().GetBrush("NoBorder"))
            .HeaderPadding(FMargin(0.0f))
            .HeaderContent()
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
                    .Text(Title)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .AutoWrapText(true)
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                    .ColorAndOpacity(SecondaryTextColor)
                    .Text(Description)
                ]
            ]
            .BodyContent()
            [
                SNew(SBorder)
                .Padding(FMargin(0.0f, 12.0f, 0.0f, 0.0f))
                .BorderBackgroundColor(FLinearColor::Transparent)
                [
                    Content
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeAIPawnWorkflowSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    TSharedRef<SWrapBox> ActionRow = SNew(SWrapBox);

    ActionRow->AddSlot()
    .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
    [
        MakeFixedWidthButton(
            LOCTEXT("WanaWorksAIPawnCreateControllerButton", "Create AI-Ready Controller"),
            [OnCreateAIReadyController = Args.OnCreateAIReadyController]()
            {
                if (OnCreateAIReadyController)
                {
                    OnCreateAIReadyController();
                }
            },
            220.0f)
    ];

    ActionRow->AddSlot()
    .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
    [
        MakeFixedWidthButton(
            LOCTEXT("WanaWorksAIPawnSaveProgressButton", "Save Progress"),
            [OnSaveSubjectProgress = Args.OnSaveSubjectProgress]()
            {
                if (OnSaveSubjectProgress)
                {
                    OnSaveSubjectProgress();
                }
            },
            160.0f)
    ];

    ActionRow->AddSlot()
    .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
    [
        MakeFixedWidthButton(
            LOCTEXT("WanaWorksAIPawnRestoreProgressButton", "Restore Last Saved State"),
            [OnRestoreSavedSubjectProgress = Args.OnRestoreSavedSubjectProgress]()
            {
                if (OnRestoreSavedSubjectProgress)
                {
                    OnRestoreSavedSubjectProgress();
                }
            },
            220.0f)
    ];

    TSharedRef<SVerticalBox> WorkspaceControls = SNew(SVerticalBox);

    WorkspaceControls->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeStringPickerControl(
            LOCTEXT("WanaWorksAIPawnAssetPickerLabel", "AI Pawn Asset"),
            Args.AIPawnAssetOptions,
            Args.GetSelectedAIPawnAssetOption,
            Args.OnAIPawnAssetOptionSelected,
            LOCTEXT("WanaWorksAIPawnAssetPickerDefault", "(Choose AI Pawn)"))
    ];

    WorkspaceControls->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksAIPawnBindingLabel", "Selected AI Pawn Binding"),
            [GetSubjectSetupSummaryText = Args.GetSubjectSetupSummaryText]()
            {
                return GetSubjectSetupSummaryText ? GetSubjectSetupSummaryText() : FText::GetEmpty();
            })
    ];

    WorkspaceControls->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksAIPawnStackLabel", "AI Pawn Stack"),
            [GetSubjectStackSummaryText = Args.GetSubjectStackSummaryText]()
            {
                return GetSubjectStackSummaryText ? GetSubjectStackSummaryText() : FText::GetEmpty();
            })
    ];

    WorkspaceControls->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksAIPawnGuidanceLabel", "Path Guidance"),
            []()
            {
                return LOCTEXT("WanaWorksAIPawnGuidanceText", "Choose an AI Pawn directly in WanaWorks, inspect it in the workspace, and keep the primary flow simple: create a copy, enhance it, test it, and build the final asset. Editor selection remains only a fallback shortcut.");
            })
    ];

    TSharedRef<SVerticalBox> Layout = SNew(SVerticalBox);

    Layout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeWorkflowWorkspaceSection(
            Args,
            SectionHeaderFont,
            LOCTEXT("WanaWorksAIPawnWorkspaceTitle", "AI Workspace"),
            LOCTEXT("WanaWorksAIPawnWorkspaceDescription", "Work from the picked AI subject on the left, shape it in the center, and keep live status on the right. WanaWorks is the workspace, so the main flow stays inside this surface."),
            WorkspaceControls)
    ];

    TSharedRef<SVerticalBox> SecondaryLayout = SNew(SVerticalBox);

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        ActionRow
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakePresetsSection(Args, SectionHeaderFont)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeCharacterEnhancementSection(Args, SectionHeaderFont)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeValidationWorkflowSection(
            Args,
            SectionHeaderFont,
            LOCTEXT("WanaWorksAIPawnValidationSection", "Testing & Pair Tools"),
            LOCTEXT("WanaWorksAIPawnValidationDescription", "Use this secondary area for deeper pair setup, live evaluation, WIT readiness, and explicit testing tools after the main workspace flow is already in motion."),
            LOCTEXT("WanaWorksAIPawnValidationStatus", "LIVE"),
            true)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeAnimationHookUsageSection(Args, SectionHeaderFont)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeResultsWorkflowSection(Args, SectionHeaderFont)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeWrappedButtonSection(
            Args,
            SectionHeaderFont,
            LOCTEXT("WanaWorksAIPawnWAISection", "WAI"),
            {
                TEXT("add_memory_test"),
                TEXT("show_memory")
            },
            LOCTEXT("WanaWorksAIPawnWAIDescription", "Keeps memory and internal state tools close when you need deeper AI-side inspection."))
    ];

    Layout->AddSlot()
    .AutoHeight()
    [
        MakeCollapsibleWorkspaceDetailsSection(
            SectionHeaderFont,
            LOCTEXT("WanaWorksAIPawnDetailsTitle", "Details & Tools"),
            LOCTEXT("WanaWorksAIPawnDetailsDescription", "Open this when you want presets, saved progress, explicit pair setup, WIT scans, WAY controls, and the deeper testing tools without cluttering the main workspace surface."),
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SecondaryLayout
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                MakeWaySection(Args, SectionHeaderFont)
            ])
    ];

    return MakeSubjectPathSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksAIPawnPathTitle", "AI Pawn"),
        LOCTEXT("WanaWorksAIPawnPathDescription", "Primary path for AI-driven pawns. Pick the subject, preview it, run the core workflow, then open details only when you need deeper tools."),
        Layout);
}

TSharedRef<SWidget> MakeCharacterPawnWorkflowSection(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
    TSharedRef<SWrapBox> ActionRow = SNew(SWrapBox);

    ActionRow->AddSlot()
    .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
    [
        MakeFixedWidthButton(
            LOCTEXT("WanaWorksCharacterPawnConvertButton", "Convert to AI-Ready Subject"),
            [OnConvertToAIReadySubject = Args.OnConvertToAIReadySubject]()
            {
                if (OnConvertToAIReadySubject)
                {
                    OnConvertToAIReadySubject();
                }
            },
            220.0f)
    ];

    ActionRow->AddSlot()
    .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
    [
        MakeFixedWidthButton(
            LOCTEXT("WanaWorksCharacterPawnSaveProgressButton", "Save Progress"),
            [OnSaveSubjectProgress = Args.OnSaveSubjectProgress]()
            {
                if (OnSaveSubjectProgress)
                {
                    OnSaveSubjectProgress();
                }
            },
            160.0f)
    ];

    ActionRow->AddSlot()
    .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
    [
        MakeFixedWidthButton(
            LOCTEXT("WanaWorksCharacterPawnRestoreProgressButton", "Restore Last Saved State"),
            [OnRestoreSavedSubjectProgress = Args.OnRestoreSavedSubjectProgress]()
            {
                if (OnRestoreSavedSubjectProgress)
                {
                    OnRestoreSavedSubjectProgress();
                }
            },
            220.0f)
    ];

    TSharedRef<SVerticalBox> WorkspaceControls = SNew(SVerticalBox);

    WorkspaceControls->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeStringPickerControl(
            LOCTEXT("WanaWorksCharacterPawnAssetPickerLabel", "Character Pawn Asset"),
            Args.CharacterPawnAssetOptions,
            Args.GetSelectedCharacterPawnAssetOption,
            Args.OnCharacterPawnAssetOptionSelected,
            LOCTEXT("WanaWorksCharacterPawnAssetPickerDefault", "(Choose Character Pawn)"))
    ];

    WorkspaceControls->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksCharacterPawnBindingLabel", "Selected Character Pawn Binding"),
            [GetSubjectSetupSummaryText = Args.GetSubjectSetupSummaryText]()
            {
                return GetSubjectSetupSummaryText ? GetSubjectSetupSummaryText() : FText::GetEmpty();
            })
    ];

    WorkspaceControls->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksCharacterPawnStackLabel", "Character Pawn Stack"),
            [GetSubjectStackSummaryText = Args.GetSubjectStackSummaryText]()
            {
                return GetSubjectStackSummaryText ? GetSubjectStackSummaryText() : FText::GetEmpty();
            })
    ];

    WorkspaceControls->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksCharacterPawnGuidanceLabel", "Path Guidance"),
            []()
            {
                return LOCTEXT("WanaWorksCharacterPawnGuidanceText", "Choose a Character Pawn directly in WanaWorks, inspect it in the workspace, and keep the flow simple: create a copy, enhance it, test it, and build the final asset. Editor selection still works as a secondary fallback.");
            })
    ];

    TSharedRef<SVerticalBox> Layout = SNew(SVerticalBox);

    Layout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeWorkflowWorkspaceSection(
            Args,
            SectionHeaderFont,
            LOCTEXT("WanaWorksCharacterPawnWorkspaceTitle", "Character Workspace"),
            LOCTEXT("WanaWorksCharacterPawnWorkspaceDescription", "Keep subject setup on the left, the main working view in the center, and live results on the right so the workflow feels like one clean workspace."),
            WorkspaceControls)
    ];

    TSharedRef<SVerticalBox> SecondaryLayout = SNew(SVerticalBox);

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        ActionRow
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeIdentitySection(Args, SectionHeaderFont)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakePresetsSection(Args, SectionHeaderFont)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeCharacterEnhancementSection(Args, SectionHeaderFont)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeValidationWorkflowSection(
            Args,
            SectionHeaderFont,
            LOCTEXT("WanaWorksCharacterPawnValidationSection", "Testing & Pair Tools"),
            LOCTEXT("WanaWorksCharacterPawnValidationDescription", "Use this secondary area for live evaluation, pair assignment, and deeper workspace testing after the main character flow is already set up."),
            LOCTEXT("WanaWorksCharacterPawnValidationStatus", "LIVE"),
            false)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeAnimationHookUsageSection(Args, SectionHeaderFont)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksCharacterPawnWAYAccessLabel", "Relationship Access"),
            []()
            {
                return LOCTEXT("WanaWorksCharacterPawnWAYAccessText", "WAY is strongest in the AI Pawn path after this subject is converted or paired for AI-style testing. That keeps relationship tools available without overloading the main character workspace.");
            })
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
    [
        MakeResultsWorkflowSection(Args, SectionHeaderFont)
    ];

    SecondaryLayout->AddSlot()
    .AutoHeight()
    [
        MakeWrappedButtonSection(
            Args,
            SectionHeaderFont,
            LOCTEXT("WanaWorksCharacterPawnWAISection", "WAI"),
            {
                TEXT("add_memory_test"),
                TEXT("show_memory")
            },
            LOCTEXT("WanaWorksCharacterPawnWAIDescription", "Lets a character-focused subject use the same memory and internal-state tools without forcing an AI replacement workflow."))
    ];

    Layout->AddSlot()
    .AutoHeight()
    [
        MakeCollapsibleWorkspaceDetailsSection(
            SectionHeaderFont,
            LOCTEXT("WanaWorksCharacterPawnDetailsTitle", "Details & Tools"),
            LOCTEXT("WanaWorksCharacterPawnDetailsDescription", "Open this when you want Identity editing, saved progress, presets, deeper testing, or the lower-level helper tools without cluttering the main workspace surface."),
            SecondaryLayout)
    ];

    return MakeSubjectPathSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksCharacterPawnPathTitle", "Character Pawn"),
        LOCTEXT("WanaWorksCharacterPawnPathDescription", "Primary path for character-first setup. Pick the subject, preview it, run the core workflow, then open details only when you need the deeper tools."),
        Layout);
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
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksWanaAIPathChooserLabel", "Choose a Subject Path"),
                []()
                {
                    return LOCTEXT("WanaWorksWanaAIPathChooserText", "Pick the path that matches the subject you want to shape inside WanaWorks. The main flow is simple: choose a subject, preview it, create a working copy, apply enhancement, test it, then build a clean final asset.");
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeAIPawnWorkflowSection(Args, SectionHeaderFont)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            MakeCharacterPawnWorkflowSection(Args, SectionHeaderFont)
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
        LOCTEXT("WanaWorksOutputSection", "Logs & Output"),
        SNew(SBorder)
        .Padding(10.0f)
        .BorderBackgroundColor(FLinearColor(0.05f, 0.08f, 0.11f, 0.85f))
        [
            SNew(SBox)
            .MinDesiredHeight(180.0f)
            [
                SNew(SMultiLineEditableTextBox)
                .IsReadOnly(true)
                .Text_Lambda([GetLogText = Args.GetLogText]()
                {
                    return GetLogText ? GetLogText() : FText::GetEmpty();
                })
            ]
        ],
        LOCTEXT("WanaWorksOutputDescription", "Secondary details for action results, testing feedback, and structured observer-target summaries."));
}

struct FStudioWorkspaceEntry
{
    FString Label;
    FText Subtitle;
};

FLinearColor GetWorkspaceAccentColor(const FString& WorkspaceLabel)
{
    if (WorkspaceLabel.Equals(TEXT("AI"), ESearchCase::IgnoreCase))
    {
        return FLinearColor(0.42f, 0.20f, 0.82f, 1.0f);
    }

    if (WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase))
    {
        return FLinearColor(0.72f, 0.49f, 0.16f, 1.0f);
    }

    return FLinearColor(0.18f, 0.31f, 0.56f, 1.0f);
}

bool IsWorkspaceActive(const FWanaWorksUITabBuilderArgs& Args, const FString& WorkspaceLabel)
{
    return Args.GetSelectedWorkspaceLabel
        && Args.GetSelectedWorkspaceLabel().Equals(WorkspaceLabel, ESearchCase::IgnoreCase);
}

TSharedRef<SWidget> MakeStudioHeroStage(const FWanaWorksUITabBuilderArgs& Args)
{
    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(StudioOutlineColor)
        [
            SNew(SBorder)
            .Padding(20.0f)
            .BorderBackgroundColor(StudioPanelRaisedColor)
            [
                SNew(SWanaWorksSandboxPreviewCard)
                .GetPreviewObject(Args.GetSandboxPreviewObject)
                .GetPreviewSummaryText(Args.GetSandboxPreviewSummaryText)
                .GetSelectedPreviewViewLabel(Args.GetSelectedPreviewViewLabel)
                .OnFocusPreviewSubject(Args.OnFocusSandboxPreviewSubject)
                .OnSelectPreviewView(Args.OnPreviewViewSelected)
            ]
        ];
}

TSharedRef<SWidget> MakeStudioStatusCard(
    const FText& Title,
    const FText& Eyebrow,
    TFunction<FText(void)> GetSummaryText,
    const FLinearColor& AccentColor,
    int32 MaxRows = 6,
    float ValueWrapWidth = 220.0f)
{
    return SNew(SWanaWorksStudioSummaryCard)
        .Title(Title)
        .Eyebrow(Eyebrow)
        .AccentColor(AccentColor)
        .GetSummaryText(GetSummaryText)
        .MaxRows(MaxRows)
        .ValueWrapWidth(ValueWrapWidth);
}

TSharedRef<SWidget> MakeWorkspaceRailButton(
    const FWanaWorksUITabBuilderArgs& Args,
    const FString& WorkspaceLabel,
    const FText& Subtitle)
{
    const bool bLiveWorkspace = IsLiveWorkspaceLabel(WorkspaceLabel);

    return SNew(SButton)
        .ButtonStyle(FCoreStyle::Get(), "NoBorder")
        .ContentPadding(FMargin(0.0f))
        .OnClicked_Lambda([OnWorkspaceSelected = Args.OnWorkspaceSelected, WorkspaceLabel]()
        {
            if (OnWorkspaceSelected)
            {
                OnWorkspaceSelected(WorkspaceLabel);
            }

            return FReply::Handled();
        })
        [
            SNew(SBorder)
            .Padding(FMargin(14.0f, 13.0f))
            .BorderBackgroundColor_Lambda([Args, WorkspaceLabel]()
            {
                return IsWorkspaceActive(Args, WorkspaceLabel)
                    ? GetWorkspaceAccentColor(WorkspaceLabel).CopyWithNewOpacity(0.26f)
                    : StudioMutedButtonColor;
            })
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Fill)
                .Padding(0.0f, 0.0f, 12.0f, 0.0f)
                [
                    SNew(SBorder)
                    .Padding(0.0f)
                    .BorderBackgroundColor_Lambda([Args, WorkspaceLabel]()
                    {
                        return IsWorkspaceActive(Args, WorkspaceLabel)
                            ? GetWorkspaceAccentColor(WorkspaceLabel)
                            : FLinearColor(0.10f, 0.13f, 0.20f, 1.0f);
                    })
                    [
                        SNew(SBox)
                        .WidthOverride(3.0f)
                    ]
                ]
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
                        .ColorAndOpacity(FLinearColor::White)
                        .Text(FText::FromString(WorkspaceLabel))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                    [
                        SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                        .ColorAndOpacity(SecondaryTextColor)
                        .Text(Subtitle)
                    ]
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(10.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(SBorder)
                    .Padding(FMargin(9.0f, 4.0f))
                    .BorderBackgroundColor_Lambda([Args, WorkspaceLabel, bLiveWorkspace]()
                    {
                        if (IsWorkspaceActive(Args, WorkspaceLabel))
                        {
                            return GetWorkspaceAccentColor(WorkspaceLabel).CopyWithNewOpacity(0.20f);
                        }

                        return bLiveWorkspace
                            ? FLinearColor(0.13f, 0.32f, 0.52f, 0.18f)
                            : FLinearColor(0.20f, 0.22f, 0.30f, 0.35f);
                    })
                    [
                        SNew(STextBlock)
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 7))
                        .ColorAndOpacity_Lambda([Args, WorkspaceLabel, bLiveWorkspace]()
                        {
                            if (IsWorkspaceActive(Args, WorkspaceLabel))
                            {
                                return FLinearColor::White;
                            }

                            return bLiveWorkspace
                                ? FLinearColor(0.78f, 0.89f, 1.0f, 1.0f)
                                : TertiaryTextColor;
                        })
                        .Text_Lambda([Args, WorkspaceLabel, bLiveWorkspace]()
                        {
                            if (IsWorkspaceActive(Args, WorkspaceLabel))
                            {
                                return LOCTEXT("WanaWorksStudioNavStateActive", "ACTIVE");
                            }

                            return bLiveWorkspace
                                ? LOCTEXT("WanaWorksStudioNavStateAvailable", "AVAILABLE")
                                : LOCTEXT("WanaWorksStudioNavStateFuture", "COMING LATER");
                        })
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeStudioNavigationRail(const FWanaWorksUITabBuilderArgs& Args)
{
    const TArray<FStudioWorkspaceEntry> WorkspaceEntries =
    {
        { TEXT("AI"), LOCTEXT("WanaWorksStudioNavAISubtitle", "Flagship live workspace") },
        { TEXT("Character Building"), LOCTEXT("WanaWorksStudioNavCharacterSubtitle", "Live builder workspace") },
        { TEXT("Level Design"), LOCTEXT("WanaWorksStudioNavLevelSubtitle", "Platform lane queued next") },
        { TEXT("Logic & Blueprints"), LOCTEXT("WanaWorksStudioNavLogicSubtitle", "Platform lane queued next") },
        { TEXT("Physics"), LOCTEXT("WanaWorksStudioNavPhysicsSubtitle", "Platform lane queued next") },
        { TEXT("Audio"), LOCTEXT("WanaWorksStudioNavAudioSubtitle", "Platform lane queued next") },
        { TEXT("UI / UX"), LOCTEXT("WanaWorksStudioNavUISubtitle", "Platform lane queued next") },
        { TEXT("Optimize"), LOCTEXT("WanaWorksStudioNavOptimizeSubtitle", "Future studio lane") },
        { TEXT("Build & Deploy"), LOCTEXT("WanaWorksStudioNavBuildSubtitle", "Future studio lane") }
    };

    TSharedRef<SVerticalBox> WorkspaceButtons = SNew(SVerticalBox);

    for (const FStudioWorkspaceEntry& WorkspaceEntry : WorkspaceEntries)
    {
        WorkspaceButtons->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 8.0f)
        [
            MakeWorkspaceRailButton(Args, WorkspaceEntry.Label, WorkspaceEntry.Subtitle)
        ];
    }

    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(StudioOutlineColor)
        [
            SNew(SBorder)
            .Padding(18.0f)
            .BorderBackgroundColor(FLinearColor(0.03f, 0.04f, 0.08f, 0.99f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 18.0f)
                [
                    SNew(SBorder)
                    .Padding(16.0f)
                    .BorderBackgroundColor(StudioPanelColor)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
                            .ColorAndOpacity(FLinearColor::White)
                            .Text(LOCTEXT("WanaWorksStudioBrand", "WanaWorks"))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
                            .ColorAndOpacity(TertiaryTextColor)
                            .Text(LOCTEXT("WanaWorksStudioBrandEyebrow", "AI DEVELOPMENT STUDIO"))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 6.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                            .ColorAndOpacity(SecondaryTextColor)
                            .Text(LOCTEXT("WanaWorksStudioBrandSubtitle", "Studio shell for autonomous building, testing, and final output."))
                        ]
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 12.0f)
                [
                    SNew(STextBlock)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                    .ColorAndOpacity(TertiaryTextColor)
                    .Text(LOCTEXT("WanaWorksStudioWorkspacesLabel", "WORKSPACES"))
                ]
                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                [
                    WorkspaceButtons
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 16.0f, 0.0f, 0.0f)
                [
                    SNew(SBorder)
                    .Padding(14.0f)
                    .BorderBackgroundColor(StudioPanelColor)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
                            .ColorAndOpacity(TertiaryTextColor)
                            .Text(LOCTEXT("WanaWorksStudioProjectLabel", "PROJECT"))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 6.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .ColorAndOpacity(FLinearColor::White)
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                            .Text(LOCTEXT("WanaWorksStudioProjectValue", "WanaDemo"))
                        ]
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeStudioSubjectColumn(const FWanaWorksUITabBuilderArgs& Args, bool bAIWorkspace)
{
    const FText PickerLabel = bAIWorkspace
        ? LOCTEXT("WanaWorksStudioAIPickerLabel", "AI Pawn Asset")
        : LOCTEXT("WanaWorksStudioCharacterPickerLabel", "Character Pawn Asset");
    const FText PickerDefault = bAIWorkspace
        ? LOCTEXT("WanaWorksStudioAIPickerDefault", "(Choose AI Pawn)")
        : LOCTEXT("WanaWorksStudioCharacterPickerDefault", "(Choose Character Pawn)");

    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(StudioOutlineColor)
        [
            SNew(SBorder)
            .Padding(18.0f)
            .BorderBackgroundColor(StudioPanelColor)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
                    .ColorAndOpacity(TertiaryTextColor)
                    .Text(bAIWorkspace
                        ? LOCTEXT("WanaWorksStudioAISubjectPanelEyebrow", "SUBJECT STACK")
                        : LOCTEXT("WanaWorksStudioCharacterSubjectPanelEyebrow", "CHARACTER STACK"))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 5.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
                    .ColorAndOpacity(FLinearColor::White)
                    .Text(bAIWorkspace
                        ? LOCTEXT("WanaWorksStudioAISubjectPanelTitle", "AI Subject")
                        : LOCTEXT("WanaWorksStudioCharacterSubjectPanelTitle", "Character Subject"))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    SNew(STextBlock)
                    .AutoWrapText(true)
                    .ColorAndOpacity(SecondaryTextColor)
                    .Text(bAIWorkspace
                        ? LOCTEXT("WanaWorksStudioAISubjectPanelSubtitle", "Select the live AI-facing subject and let WanaWorks surface its detected stack automatically.")
                        : LOCTEXT("WanaWorksStudioCharacterSubjectPanelSubtitle", "Select the character-facing subject and let WanaWorks surface identity, stack fit, and build readiness."))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 16.0f)
                [
                    MakeStringPickerControl(
                        PickerLabel,
                        bAIWorkspace ? Args.AIPawnAssetOptions : Args.CharacterPawnAssetOptions,
                        bAIWorkspace ? Args.GetSelectedAIPawnAssetOption : Args.GetSelectedCharacterPawnAssetOption,
                        bAIWorkspace ? Args.OnAIPawnAssetOptionSelected : Args.OnCharacterPawnAssetOptionSelected,
                        PickerDefault,
                        260.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioSelectedSubjectCard", "Selected Subject"),
                        LOCTEXT("WanaWorksStudioSelectedSubjectEyebrow", "CURRENT"),
                        Args.GetSubjectSetupSummaryText,
                        GetWorkspaceAccentColor(bAIWorkspace ? TEXT("AI") : TEXT("Character Building")),
                        4,
                        190.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioDetectedStackCard", "Detected Stack"),
                        LOCTEXT("WanaWorksStudioDetectedStackEyebrow", "LIVE"),
                        Args.GetSubjectStackSummaryText,
                        FLinearColor(0.17f, 0.54f, 0.60f, 1.0f),
                        8,
                        190.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    bAIWorkspace
                        ? MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioPresetCard", "Preset Profile"),
                            LOCTEXT("WanaWorksStudioPresetEyebrow", "READY"),
                            Args.GetWorkflowPresetSummaryText,
                            FLinearColor(0.41f, 0.29f, 0.78f, 1.0f),
                            5,
                            190.0f)
                        : MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioIdentityCard", "Identity"),
                            LOCTEXT("WanaWorksStudioIdentityEyebrow", "CHARACTER"),
                            Args.GetIdentitySummaryText,
                            FLinearColor(0.62f, 0.46f, 0.20f, 1.0f),
                            5,
                            190.0f)
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeStudioStatusColumn(const FWanaWorksUITabBuilderArgs& Args, bool bAIWorkspace)
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 14.0f)
        [
            MakeStudioStatusCard(
                LOCTEXT("WanaWorksStudioAnimationCardTitle", "Animation Integration"),
                LOCTEXT("WanaWorksStudioAnimationCardEyebrow", "INTEGRATION"),
                Args.GetAnimationIntegrationText,
                FLinearColor(0.17f, 0.64f, 0.42f, 1.0f),
                7,
                210.0f)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 14.0f)
        [
            MakeStudioStatusCard(
                LOCTEXT("WanaWorksStudioPhysicalCardTitle", "Physical State"),
                LOCTEXT("WanaWorksStudioPhysicalCardEyebrow", "BODY"),
                Args.GetPhysicalStateText,
                FLinearColor(0.18f, 0.47f, 0.82f, 1.0f),
                7,
                210.0f)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            bAIWorkspace
                ? MakeStudioStatusCard(
                    LOCTEXT("WanaWorksStudioBehaviorCardTitle", "Behavior Results"),
                    LOCTEXT("WanaWorksStudioBehaviorCardEyebrow", "LIVE AI"),
                    Args.GetBehaviorResultsText,
                    FLinearColor(0.33f, 0.70f, 0.39f, 1.0f),
                    8,
                    210.0f)
                : MakeStudioStatusCard(
                    LOCTEXT("WanaWorksStudioEnhancementCardTitle", "Enhancement Results"),
                    LOCTEXT("WanaWorksStudioEnhancementCardEyebrow", "BUILD"),
                    Args.GetEnhancementResultsText,
                    FLinearColor(0.74f, 0.53f, 0.18f, 1.0f),
                    8,
                    210.0f)
        ];
}

TSharedRef<SWidget> MakeWorkflowTile(
    const FString& StepNumber,
    const FText& Title,
    const FText& Description,
    const FLinearColor& AccentColor,
    TFunction<void(void)> OnPressed)
{
    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(StudioOutlineColor)
        [
            SNew(SButton)
            .ButtonStyle(FCoreStyle::Get(), "NoBorder")
            .ContentPadding(FMargin(0.0f))
            .OnClicked_Lambda([OnPressed]()
            {
                if (OnPressed)
                {
                    OnPressed();
                }

                return FReply::Handled();
            })
            [
                SNew(SBorder)
                .Padding(18.0f)
                .BorderBackgroundColor(FLinearColor(0.055f, 0.07f, 0.11f, 0.99f))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        SNew(SBorder)
                        .Padding(0.0f)
                        .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.92f))
                        [
                            SNew(SBox)
                            .HeightOverride(3.0f)
                        ]
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        [
                            MakeStudioPill(
                                FText::FromString(StepNumber),
                                AccentColor.CopyWithNewOpacity(0.22f),
                                AccentColor,
                                9,
                                FMargin(12.0f, 6.0f))
                        ]
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        .Padding(12.0f, 0.0f, 0.0f, 0.0f)
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
                            .ColorAndOpacity(FLinearColor::White)
                            .Text(Title)
                        ]
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 12.0f, 0.0f, 16.0f)
                    [
                        SNew(STextBlock)
                        .AutoWrapText(true)
                        .ColorAndOpacity(SecondaryTextColor)
                        .Text(Description)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        MakeStudioPill(
                            LOCTEXT("WanaWorksStudioWorkflowActionChip", "Run Step"),
                            AccentColor.CopyWithNewOpacity(0.20f),
                            FLinearColor(0.95f, 0.97f, 1.0f, 1.0f),
                            8,
                            FMargin(12.0f, 7.0f))
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeStudioWorkflowStrip(const FWanaWorksUITabBuilderArgs& Args, bool bAIWorkspace)
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 12.0f)
        [
            SNew(STextBlock)
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
            .Text(LOCTEXT("WanaWorksStudioWorkflowStripTitle", "Workflow"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 12.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(0.0f, 0.0f, 12.0f, 0.0f)
            [
                MakeWorkflowTile(
                    TEXT("1"),
                    LOCTEXT("WanaWorksStudioWorkflowEnhanceTitle", "Enhance"),
                    LOCTEXT("WanaWorksStudioWorkflowEnhanceText", "Prepare a safe working subject and apply the current enhancement stack."),
                    FLinearColor(0.42f, 0.20f, 0.82f, 1.0f),
                    Args.OnApplyCharacterEnhancement)
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(0.0f, 0.0f, 12.0f, 0.0f)
            [
                MakeWorkflowTile(
                    TEXT("2"),
                    LOCTEXT("WanaWorksStudioWorkflowTestTitle", "Test"),
                    LOCTEXT("WanaWorksStudioWorkflowTestText", "Run the guided subject test and visible workspace behavior pass."),
                    FLinearColor(0.15f, 0.46f, 0.84f, 1.0f),
                    Args.OnApplyStarterAndTestTarget)
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(0.0f, 0.0f, 12.0f, 0.0f)
            [
                MakeWorkflowTile(
                    TEXT("3"),
                    LOCTEXT("WanaWorksStudioWorkflowAnalyzeTitle", "Analyze"),
                    bAIWorkspace
                        ? LOCTEXT("WanaWorksStudioWorkflowAnalyzeAIText", "Review movement confidence, environment fit, and WIT readiness.")
                        : LOCTEXT("WanaWorksStudioWorkflowAnalyzeCharacterText", "Review live target response, animation integration, and build readiness."),
                    FLinearColor(0.13f, 0.63f, 0.67f, 1.0f),
                    bAIWorkspace ? Args.OnScanEnvironmentReadiness : Args.OnEvaluateLiveTarget)
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                MakeWorkflowTile(
                    TEXT("4"),
                    LOCTEXT("WanaWorksStudioWorkflowBuildTitle", "Build"),
                    LOCTEXT("WanaWorksStudioWorkflowBuildText", "Save the final asset quietly to /Game/WanaWorks/Builds while the workspace stays in focus."),
                    FLinearColor(0.72f, 0.49f, 0.16f, 1.0f),
                    Args.OnFinalizeSandboxBuild)
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(0.0f, 0.0f, 12.0f, 0.0f)
            [
                MakeStudioStatusCard(
                    LOCTEXT("WanaWorksStudioSystemChainTitle", "System Status"),
                    LOCTEXT("WanaWorksStudioSystemChainEyebrow", "LIVE"),
                    Args.GetCharacterEnhancementChainText,
                    FLinearColor(0.20f, 0.42f, 0.72f, 1.0f),
                    4,
                    220.0f)
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .Padding(0.0f, 0.0f, 12.0f, 0.0f)
            [
                MakeStudioStatusCard(
                    LOCTEXT("WanaWorksStudioSavedProgressTitle", "Saved Progress"),
                    LOCTEXT("WanaWorksStudioSavedProgressEyebrow", "PERSISTENCE"),
                    Args.GetSavedSubjectProgressText,
                    FLinearColor(0.35f, 0.27f, 0.72f, 1.0f),
                    4,
                    220.0f)
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                bAIWorkspace
                    ? MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioEnvironmentTitle", "Environment"),
                        LOCTEXT("WanaWorksStudioEnvironmentEyebrow", "WIT"),
                        Args.GetWITEnvironmentReadinessText,
                        FLinearColor(0.16f, 0.61f, 0.58f, 1.0f),
                        4,
                        220.0f)
                    : MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioLiveTestTitle", "Live Test"),
                        LOCTEXT("WanaWorksStudioLiveTestEyebrow", "CURRENT"),
                        Args.GetLiveTestSummaryText,
                        FLinearColor(0.22f, 0.56f, 0.76f, 1.0f),
                        4,
                        220.0f)
            ]
        ];
}

TSharedRef<SWidget> MakeStudioDetailsArea(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont, bool bAIWorkspace)
{
    TSharedRef<SVerticalBox> WorkspaceDetails = SNew(SVerticalBox);

    if (bAIWorkspace)
    {
        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakePresetsSection(Args, SectionHeaderFont)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeCharacterEnhancementSection(Args, SectionHeaderFont)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeValidationWorkflowSection(
                Args,
                SectionHeaderFont,
                LOCTEXT("WanaWorksStudioAIDetailsValidationTitle", "AI Testing & Pair Tools"),
                LOCTEXT("WanaWorksStudioAIDetailsValidationDescription", "Use deeper validation, pair setup, and WIT tools here when you want more than the main studio workflow."),
                LOCTEXT("WanaWorksStudioAIDetailsValidationStatus", "LIVE"),
                true)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeAnimationHookUsageSection(Args, SectionHeaderFont)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeResultsWorkflowSection(Args, SectionHeaderFont)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeWrappedButtonSection(
                Args,
                SectionHeaderFont,
                LOCTEXT("WanaWorksStudioAIWAISection", "WAI"),
                {
                    TEXT("add_memory_test"),
                    TEXT("show_memory")
                },
                LOCTEXT("WanaWorksStudioAIWAIDescription", "Memory and internal-state tools for deeper AI-side inspection."))
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        [
            MakeWaySection(Args, SectionHeaderFont)
        ];
    }
    else
    {
        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeIdentitySection(Args, SectionHeaderFont)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakePresetsSection(Args, SectionHeaderFont)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeCharacterEnhancementSection(Args, SectionHeaderFont)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeValidationWorkflowSection(
                Args,
                SectionHeaderFont,
                LOCTEXT("WanaWorksStudioCharacterDetailsValidationTitle", "Character Testing & Pair Tools"),
                LOCTEXT("WanaWorksStudioCharacterDetailsValidationDescription", "Use deeper testing, pair setup, and conversion-side tools here when the main character workflow needs more control."),
                LOCTEXT("WanaWorksStudioCharacterDetailsValidationStatus", "LIVE"),
                false)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeAnimationHookUsageSection(Args, SectionHeaderFont)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeResultsWorkflowSection(Args, SectionHeaderFont)
        ];

        WorkspaceDetails->AddSlot()
        .AutoHeight()
        [
            MakeWrappedButtonSection(
                Args,
                SectionHeaderFont,
                LOCTEXT("WanaWorksStudioCharacterWAISection", "WAI"),
                {
                    TEXT("add_memory_test"),
                    TEXT("show_memory")
                },
                LOCTEXT("WanaWorksStudioCharacterWAIDescription", "Memory and internal-state tools for character-focused setup without cluttering the main shell."))
        ];
    }

    TSharedRef<SWidget> SceneTools = SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeWrappedButtonSection(
                Args,
                SectionHeaderFont,
                LOCTEXT("WanaWorksStudioWeatherSection", "Weather"),
                {
                    TEXT("weather_clear"),
                    TEXT("weather_overcast"),
                    TEXT("weather_storm")
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, WanaAISubsectionSpacing)
        [
            MakeWrappedButtonSection(
                Args,
                SectionHeaderFont,
                LOCTEXT("WanaWorksStudioSceneSection", "Scene"),
                {
                    TEXT("spawn_cube"),
                    TEXT("list_selection")
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            MakeAdvancedCommandSection(Args, SectionHeaderFont)
        ];

    TSharedRef<SWidget> OutputTools = SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            MakeOutputSection(Args, SectionHeaderFont)
        ];

    return MakeCollapsibleWorkspaceDetailsSection(
        SectionHeaderFont,
        LOCTEXT("WanaWorksStudioDetailsTitle", "Advanced Controls & Output"),
        LOCTEXT("WanaWorksStudioDetailsDescription", "Open this secondary drawer only when you need presets, deep testing, scene helpers, or logs beyond the main workflow surface."),
        SNew(SWrapBox)
        + SWrapBox::Slot()
        .Padding(FMargin(0.0f, 0.0f, 14.0f, 14.0f))
        [
            SNew(SBox)
            .WidthOverride(430.0f)
            [
                MakeCollapsibleWorkspaceDetailsSection(
                    SectionHeaderFont,
                    LOCTEXT("WanaWorksStudioWorkflowToolsTitle", "Workflow Tools"),
                    LOCTEXT("WanaWorksStudioWorkflowToolsDescription", "Presets, deeper testing, system guidance, and advanced subject controls stay here as a secondary lane."),
                    WorkspaceDetails)
            ]
        ]
        + SWrapBox::Slot()
        .Padding(FMargin(0.0f, 0.0f, 14.0f, 14.0f))
        [
            SNew(SBox)
            .WidthOverride(360.0f)
            [
                MakeCollapsibleWorkspaceDetailsSection(
                    SectionHeaderFont,
                    LOCTEXT("WanaWorksStudioSceneToolsTitle", "Scene & Utility Tools"),
                    LOCTEXT("WanaWorksStudioSceneToolsDescription", "Environment presets, quick scene actions, and command utilities remain available without crowding the workspace."),
                    SceneTools)
            ]
        ]
        + SWrapBox::Slot()
        .Padding(FMargin(0.0f, 0.0f, 0.0f, 14.0f))
        [
            SNew(SBox)
            .WidthOverride(420.0f)
            [
                MakeCollapsibleWorkspaceDetailsSection(
                    SectionHeaderFont,
                    LOCTEXT("WanaWorksStudioLogsToolsTitle", "Logs & Build Output"),
                    LOCTEXT("WanaWorksStudioLogsToolsDescription", "Quiet build references and detailed output stay here when you need the deeper audit trail."),
                    OutputTools)
            ]
        ]);
}

TSharedRef<SWidget> MakeActiveStudioWorkspace(
    const FWanaWorksUITabBuilderArgs& Args,
    const FSlateFontInfo& SectionHeaderFont,
    const FString& WorkspaceLabel)
{
    const bool bAIWorkspace = WorkspaceLabel.Equals(TEXT("AI"), ESearchCase::IgnoreCase);
    const FText WorkspaceTitle = bAIWorkspace
        ? LOCTEXT("WanaWorksStudioAITitle", "AI Workspace")
        : LOCTEXT("WanaWorksStudioCharacterTitle", "Character Building");
    const FText WorkspaceDescription = bAIWorkspace
        ? LOCTEXT("WanaWorksStudioAIDescription", "Enhance, test, analyze, and build intelligent subjects with a studio-style autonomous workflow.")
        : LOCTEXT("WanaWorksStudioCharacterDescription", "Shape the character stack, animation bridge, and safe build output inside a dedicated studio workspace.");

    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 16.0f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
                .ColorAndOpacity(FLinearColor::White)
                .Text(WorkspaceTitle)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 6.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .AutoWrapText(true)
                .ColorAndOpacity(SecondaryTextColor)
                .Text(WorkspaceDescription)
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 16.0f)
        [
            SNew(SSplitter)
            .PhysicalSplitterHandleSize(2.0f)
            + SSplitter::Slot()
            .Value(0.24f)
            [
                MakeStudioSubjectColumn(Args, bAIWorkspace)
            ]
            + SSplitter::Slot()
            .Value(0.46f)
            [
                MakeStudioHeroStage(Args)
            ]
            + SSplitter::Slot()
            .Value(0.30f)
            [
                MakeStudioStatusColumn(Args, bAIWorkspace)
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 16.0f)
        [
            MakeStudioWorkflowStrip(Args, bAIWorkspace)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            MakeStudioDetailsArea(Args, SectionHeaderFont, bAIWorkspace)
        ];
}

TSharedRef<SWidget> MakeFutureWorkspaceSurface(
    const FWanaWorksUITabBuilderArgs& Args,
    const FString& WorkspaceLabel)
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 16.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
                .ColorAndOpacity(FLinearColor::White)
                .Text(FText::FromString(WorkspaceLabel))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                MakeStudioPill(
                    LOCTEXT("WanaWorksStudioFutureWorkspaceBadge", "COMING LATER"),
                    GetWorkspaceAccentColor(WorkspaceLabel).CopyWithNewOpacity(0.18f),
                    GetWorkspaceAccentColor(WorkspaceLabel),
                    8,
                    FMargin(12.0f, 6.0f))
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 6.0f, 0.0f, 16.0f)
        [
            SNew(STextBlock)
            .AutoWrapText(true)
            .ColorAndOpacity(SecondaryTextColor)
            .Text(LOCTEXT("WanaWorksStudioFutureWorkspaceText", "This lane is part of the premium studio shell already, but its production workflow is intentionally staged for a later pass. AI and Character Building are the active workspaces right now."))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SSplitter)
            .PhysicalSplitterHandleSize(2.0f)
            + SSplitter::Slot()
            .Value(0.28f)
            [
                MakeStudioStatusCard(
                    LOCTEXT("WanaWorksStudioFutureIntentTitle", "Workspace Intent"),
                    LOCTEXT("WanaWorksStudioFutureIntentEyebrow", "PLATFORM"),
                    []()
                    {
                        return LOCTEXT("WanaWorksStudioFutureIntentText", "Status: Future workspace\nLayout: Premium studio shell\nPreview: Shared workspace stage\nReadiness: Shell present\nNext Step: Expand capability in a later phase");
                    },
                    GetWorkspaceAccentColor(WorkspaceLabel),
                    5,
                    220.0f)
            ]
            + SSplitter::Slot()
            .Value(0.42f)
            [
                MakeStudioHeroStage(Args)
            ]
            + SSplitter::Slot()
            .Value(0.30f)
            [
                MakeStudioStatusCard(
                    LOCTEXT("WanaWorksStudioFuturePlatformTitle", "Platform Status"),
                    LOCTEXT("WanaWorksStudioFuturePlatformEyebrow", "LIVE SHELL"),
                    Args.GetSavedSubjectProgressText,
                    FLinearColor(0.19f, 0.46f, 0.74f, 1.0f),
                    5,
                    220.0f)
            ]
        ];
}
}

namespace WanaWorksUITabBuilder
{
TSharedRef<SWidget> BuildTabContent(const FWanaWorksUITabBuilderArgs& Args)
{
    const FSlateFontInfo SectionHeaderFont = FCoreStyle::GetDefaultFontStyle("Bold", 11);
    const FString SelectedWorkspaceLabel = Args.GetSelectedWorkspaceLabel
        ? Args.GetSelectedWorkspaceLabel()
        : TEXT("AI");
    const bool bIsAIWorkspace = SelectedWorkspaceLabel.Equals(TEXT("AI"), ESearchCase::IgnoreCase);
    const bool bIsCharacterWorkspace = SelectedWorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase);
    const FText ActiveWorkspaceTitle = bIsAIWorkspace
        ? LOCTEXT("WanaWorksStudioShellAIWorkspaceTitle", "AI Workspace")
        : (bIsCharacterWorkspace
            ? LOCTEXT("WanaWorksStudioShellCharacterWorkspaceTitle", "Character Building")
            : FText::FromString(SelectedWorkspaceLabel));
    const FText ActiveWorkspaceSubtitle = bIsAIWorkspace
        ? LOCTEXT("WanaWorksStudioShellAISubtitle", "Enhance, test, and build intelligent characters with autonomous AI.")
        : (bIsCharacterWorkspace
            ? LOCTEXT("WanaWorksStudioShellCharacterSubtitle", "Shape subject identity, stack readiness, animation integration, and final build output.")
            : LOCTEXT("WanaWorksStudioShellFutureSubtitle", "Platform workspace shell present. Capability expansion can land here in a later phase."));

    TSharedRef<SWidget> WorkspaceSurface = (bIsAIWorkspace || bIsCharacterWorkspace)
        ? MakeActiveStudioWorkspace(Args, SectionHeaderFont, SelectedWorkspaceLabel)
        : MakeFutureWorkspaceSurface(Args, SelectedWorkspaceLabel);

    return SNew(SBorder)
        .Padding(18.0f)
        .BorderBackgroundColor(FLinearColor(0.015f, 0.02f, 0.05f, 1.0f))
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(0.0f, 0.0f, 18.0f, 0.0f)
            [
                SNew(SBox)
                .WidthOverride(250.0f)
                [
                    MakeStudioNavigationRail(Args)
                ]
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 16.0f)
                [
                    SNew(SBorder)
                    .Padding(1.0f)
                    .BorderBackgroundColor(StudioOutlineColor)
                    [
                        SNew(SBorder)
                        .Padding(FMargin(18.0f, 16.0f))
                        .BorderBackgroundColor(FLinearColor(0.025f, 0.04f, 0.08f, 0.99f))
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .FillWidth(0.42f)
                            .VAlign(VAlign_Center)
                            .Padding(0.0f, 0.0f, 18.0f, 0.0f)
                            [
                                SNew(SBorder)
                                .Padding(1.0f)
                                .BorderBackgroundColor(StudioOutlineColor)
                                [
                                    SNew(SBorder)
                                    .Padding(FMargin(15.0f, 11.0f))
                                    .BorderBackgroundColor(StudioPanelColor)
                                    [
                                        SNew(STextBlock)
                                        .ColorAndOpacity(SecondaryTextColor)
                                        .Text(LOCTEXT("WanaWorksStudioShellSearchPlaceholder", "Search WanaWorks..."))
                                    ]
                                ]
                            ]
                            + SHorizontalBox::Slot()
                            .FillWidth(0.34f)
                            .VAlign(VAlign_Center)
                            .Padding(0.0f, 0.0f, 18.0f, 0.0f)
                            [
                                SNew(SVerticalBox)
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                [
                                    SNew(STextBlock)
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
                                    .ColorAndOpacity(TertiaryTextColor)
                                    .Text(LOCTEXT("WanaWorksStudioShellWorkspaceLabel", "ACTIVE WORKSPACE"))
                                ]
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(0.0f, 5.0f, 0.0f, 0.0f)
                                [
                                    SNew(STextBlock)
                                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 17))
                                    .ColorAndOpacity(FLinearColor::White)
                                    .Text(ActiveWorkspaceTitle)
                                ]
                            ]
                            + SHorizontalBox::Slot()
                            .FillWidth(0.24f)
                            .VAlign(VAlign_Center)
                            [
                                SNew(SBorder)
                                .Padding(1.0f)
                                .BorderBackgroundColor(StudioOutlineColor)
                                [
                                    SNew(SBorder)
                                    .Padding(FMargin(12.0f, 10.0f))
                                    .BorderBackgroundColor(FLinearColor(0.06f, 0.08f, 0.13f, 1.0f))
                                    [
                                        SNew(STextBlock)
                                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
                                        .ColorAndOpacity(FLinearColor::White)
                                        .Text_Lambda([GetStatusText = Args.GetStatusText]()
                                        {
                                            return GetStatusText ? GetStatusText() : FText::GetEmpty();
                                        })
                                    ]
                                ]
                            ]
                        ]
                    ]
                ]
                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                [
                    SNew(SBorder)
                    .Padding(1.0f)
                    .BorderBackgroundColor(StudioOutlineColor)
                    [
                        SNew(SBorder)
                        .Padding(18.0f)
                        .BorderBackgroundColor(FLinearColor(0.02f, 0.04f, 0.08f, 0.99f))
                        [
                            SNew(SScrollBox)
                            + SScrollBox::Slot()
                            [
                                SNew(SVerticalBox)
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(0.0f, 0.0f, 0.0f, 16.0f)
                                [
                                    SNew(SVerticalBox)
                                    + SVerticalBox::Slot()
                                    .AutoHeight()
                                    [
                                        SNew(STextBlock)
                                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
                                        .ColorAndOpacity(FLinearColor::White)
                                        .Text(ActiveWorkspaceTitle)
                                    ]
                                    + SVerticalBox::Slot()
                                    .AutoHeight()
                                    .Padding(0.0f, 6.0f, 0.0f, 0.0f)
                                    [
                                        SNew(STextBlock)
                                        .AutoWrapText(true)
                                        .ColorAndOpacity(SecondaryTextColor)
                                        .Text(ActiveWorkspaceSubtitle)
                                    ]
                                    + SVerticalBox::Slot()
                                    .AutoHeight()
                                    .Padding(0.0f, 14.0f, 0.0f, 0.0f)
                                    [
                                        MakeStudioDivider()
                                    ]
                                ]
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(0.0f, 16.0f, 0.0f, 0.0f)
                                [
                                    WorkspaceSurface
                                ]
                            ]
                        ]
                    ]
                ]
            ]
        ];
}
}

#undef LOCTEXT_NAMESPACE
