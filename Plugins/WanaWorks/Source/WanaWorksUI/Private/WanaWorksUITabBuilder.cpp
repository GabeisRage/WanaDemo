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
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SOverlay.h"
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
const FLinearColor SecondaryTextColor(0.76f, 0.80f, 0.88f, 1.0f);
const FLinearColor TertiaryTextColor(0.46f, 0.53f, 0.66f, 1.0f);
const FLinearColor InfoPanelColor(0.045f, 0.055f, 0.085f, 0.92f);
const FLinearColor PreviewPanelColor(0.018f, 0.023f, 0.046f, 0.985f);
const FLinearColor StudioPanelColor(0.030f, 0.035f, 0.060f, 0.965f);
const FLinearColor StudioPanelRaisedColor(0.040f, 0.047f, 0.075f, 0.985f);
const FLinearColor StudioPanelElevatedColor(0.060f, 0.070f, 0.105f, 1.0f);
const FLinearColor StudioOutlineColor(0.145f, 0.165f, 0.255f, 0.78f);
const FLinearColor StudioDividerColor(0.085f, 0.100f, 0.155f, 0.92f);
const FLinearColor StudioAccentColor(0.46f, 0.37f, 0.92f, 1.0f);
const FLinearColor StudioAccentBlueColor(0.30f, 0.54f, 0.94f, 1.0f);
const FLinearColor StudioAccentGoldColor(0.86f, 0.69f, 0.30f, 1.0f);
const FLinearColor StudioSuccessColor(0.34f, 0.76f, 0.58f, 1.0f);
const FLinearColor StudioShellColor(0.008f, 0.012f, 0.028f, 1.0f);
const FLinearColor StudioShellRaisedColor(0.018f, 0.022f, 0.042f, 0.99f);
const FLinearColor StudioShellTrackColor(0.022f, 0.027f, 0.050f, 0.98f);
const FLinearColor StudioStageBackdropColor(0.012f, 0.018f, 0.040f, 1.0f);
const FLinearColor StudioMutedButtonColor(0.045f, 0.050f, 0.082f, 1.0f);
const FLinearColor StudioShadowColor(0.0f, 0.0f, 0.0f, 0.55f);

FSlateFontInfo MakeStudioFont(const ANSICHAR* Typeface, int32 Size)
{
    return FCoreStyle::GetDefaultFontStyle(Typeface, Size);
}

bool ShouldRenderCompactValueChip(const FString& Value)
{
    FString NormalizedValue = Value.TrimStartAndEnd();
    TArray<FString> Tokens;
    NormalizedValue.ParseIntoArrayWS(Tokens);

    return !NormalizedValue.IsEmpty()
        && NormalizedValue.Len() <= 20
        && Tokens.Num() <= 3
        && !NormalizedValue.Contains(TEXT("/"))
        && !NormalizedValue.Contains(TEXT("\\"))
        && !NormalizedValue.Contains(TEXT("."));
}

bool IsLiveWorkspaceLabel(const FString& WorkspaceLabel)
{
    return WorkspaceLabel.Equals(TEXT("AI"), ESearchCase::IgnoreCase)
        || WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase)
        || WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase);
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
        .Padding(1.0f)
        .BorderBackgroundColor(FillColor.CopyWithNewOpacity(FMath::Clamp(FillColor.A + 0.08f, 0.0f, 1.0f)))
        [
            SNew(SBorder)
            .Padding(Padding)
            .BorderBackgroundColor(FillColor)
            [
                SNew(STextBlock)
                .ColorAndOpacity(TextColor)
                .Font(MakeStudioFont("Bold", FontSize))
                .ShadowColorAndOpacity(StudioShadowColor)
                .ShadowOffset(FVector2D(0.0f, 1.0f))
                .Text(Label)
            ]
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

TSharedRef<SWidget> BuildStudioSummaryRowsWidget(
    const FText& SummaryText,
    int32 MaxRows,
    float ValueWrapWidth = 220.0f,
    const FLinearColor& AccentColor = StudioAccentColor)
{
    const TArray<FStudioSummaryRow> Rows = ParseStudioSummaryRows(SummaryText, MaxRows);
    TSharedRef<SVerticalBox> Layout = SNew(SVerticalBox);

    if (Rows.IsEmpty())
    {
        Layout->AddSlot()
        .AutoHeight()
        [
            SNew(STextBlock)
            .Font(MakeStudioFont("Regular", 9))
            .ColorAndOpacity(SecondaryTextColor)
            .ShadowColorAndOpacity(StudioShadowColor)
            .ShadowOffset(FVector2D(0.0f, 1.0f))
            .Text(LOCTEXT("WanaWorksStudioSummaryEmpty", "No live details yet."))
        ];

        return Layout;
    }

    for (int32 RowIndex = 0; RowIndex < Rows.Num(); ++RowIndex)
    {
        const FStudioSummaryRow& Row = Rows[RowIndex];
        const bool bRenderCompactChip = ShouldRenderCompactValueChip(Row.Value);

        if (Row.bIsNote || Row.Label.IsEmpty())
        {
            Layout->AddSlot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, RowIndex + 1 < Rows.Num() ? 12.0f : 0.0f)
            [
                SNew(STextBlock)
                .AutoWrapText(true)
                .Font(MakeStudioFont("Regular", 9))
                .ColorAndOpacity(SecondaryTextColor)
                .ShadowColorAndOpacity(StudioShadowColor)
                .ShadowOffset(FVector2D(0.0f, 1.0f))
                .Text(FText::FromString(Row.Value))
            ];

            continue;
        }

        const TSharedRef<SWidget> ValueWidget = bRenderCompactChip
            ? StaticCastSharedRef<SWidget>(
                MakeStudioPill(
                    FText::FromString(Row.Value),
                    AccentColor.CopyWithNewOpacity(0.16f),
                    FLinearColor(0.96f, 0.97f, 1.0f, 1.0f),
                    9,
                    FMargin(12.0f, 6.0f)))
            : StaticCastSharedRef<SWidget>(
                SNew(SBox)
                .MaxDesiredWidth(ValueWrapWidth)
                [
                    SNew(STextBlock)
                    .AutoWrapText(true)
                    .ColorAndOpacity(FLinearColor(0.97f, 0.98f, 1.0f, 1.0f))
                    .Font(MakeStudioFont("Bold", 11))
                    .ShadowColorAndOpacity(StudioShadowColor)
                    .ShadowOffset(FVector2D(0.0f, 1.0f))
                    .Text(FText::FromString(Row.Value))
                ]);

        const TSharedRef<SWidget> DividerWidget = RowIndex + 1 < Rows.Num()
            ? StaticCastSharedRef<SWidget>(MakeStudioDivider(1.0f, StudioDividerColor.CopyWithNewOpacity(0.78f)))
            : StaticCastSharedRef<SWidget>(SNew(SSpacer).Size(FVector2D::ZeroVector));

        Layout->AddSlot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, RowIndex + 1 < Rows.Num() ? 12.0f : 0.0f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Font(MakeStudioFont("Bold", 8))
                .ColorAndOpacity(TertiaryTextColor)
                .ShadowColorAndOpacity(StudioShadowColor)
                .ShadowOffset(FVector2D(0.0f, 1.0f))
                .Text(FText::FromString(Row.Label))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 5.0f, 0.0f, 0.0f)
            [
                ValueWidget
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 12.0f, 0.0f, 0.0f)
            [
                DividerWidget
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
            .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.18f))
            [
                SNew(SBorder)
                .Padding(22.0f)
                .BorderBackgroundColor(StudioPanelRaisedColor)
                [
                    SNew(SVerticalBox)
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
                                .Visibility(Eyebrow.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
                                .ColorAndOpacity(TertiaryTextColor)
                                .Font(MakeStudioFont("Bold", 8))
                                .ShadowColorAndOpacity(StudioShadowColor)
                                .ShadowOffset(FVector2D(0.0f, 1.0f))
                                .Text(Eyebrow)
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0.0f, 6.0f, 0.0f, 0.0f)
                            [
                                SNew(STextBlock)
                                .Font(MakeStudioFont("Bold", 16))
                                .ColorAndOpacity(FLinearColor::White)
                                .ShadowColorAndOpacity(StudioShadowColor)
                                .ShadowOffset(FVector2D(0.0f, 1.0f))
                                .Text(Title)
                            ]
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Top)
                        [
                            MakeStudioPill(
                                LOCTEXT("WanaWorksStudioModuleBadge", "LIVE MODULE"),
                                AccentColor.CopyWithNewOpacity(0.14f),
                                AccentColor,
                                8,
                                FMargin(12.0f, 6.0f))
                        ]
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 14.0f, 0.0f, 14.0f)
                    [
                        MakeStudioDivider(1.0f, AccentColor.CopyWithNewOpacity(0.42f))
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

        BodyBox->SetContent(BuildStudioSummaryRowsWidget(FText::FromString(CachedSummary), MaxRows, ValueWrapWidth, AccentColor));
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
            .BorderBackgroundColor(StudioAccentBlueColor.CopyWithNewOpacity(0.18f))
            [
                SNew(SBorder)
                .Padding(20.0f)
                .BorderBackgroundColor(PreviewPanelColor)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 16.0f)
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
                                .Font(MakeStudioFont("Bold", 8))
                                .ShadowColorAndOpacity(StudioShadowColor)
                                .ShadowOffset(FVector2D(0.0f, 1.0f))
                                .Text(LOCTEXT("WanaWorksSandboxPreviewCardEyebrow", "STUDIO STAGE"))
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0.0f, 6.0f, 0.0f, 0.0f)
                            [
                                SNew(STextBlock)
                                .Font(MakeStudioFont("Bold", 18))
                                .ColorAndOpacity(FLinearColor::White)
                                .ShadowColorAndOpacity(StudioShadowColor)
                                .ShadowOffset(FVector2D(0.0f, 1.0f))
                                .Text(LOCTEXT("WanaWorksSandboxPreviewCardTitle", "Workspace Preview"))
                            ]
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        .Padding(12.0f, 0.0f, 12.0f, 0.0f)
                        [
                            SNew(SBorder)
                            .Padding(FMargin(12.0f, 6.0f))
                            .BorderBackgroundColor(StudioAccentColor.CopyWithNewOpacity(0.16f))
                            [
                                SNew(STextBlock)
                                .ColorAndOpacity(FLinearColor(0.95f, 0.92f, 1.0f, 1.0f))
                                .Font(MakeStudioFont("Bold", 8))
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
                .Padding(1.0f)
                .BorderBackgroundColor_Lambda([this, ViewLabel]()
                {
                    const bool bIsActive = GetSelectedPreviewViewLabel
                        && GetSelectedPreviewViewLabel().Equals(ViewLabel, ESearchCase::IgnoreCase);
                    return bIsActive
                        ? StudioAccentColor.CopyWithNewOpacity(0.42f)
                        : StudioOutlineColor.CopyWithNewOpacity(0.42f);
                })
                [
                    SNew(SBorder)
                    .Padding(FMargin(16.0f, 9.0f))
                    .BorderBackgroundColor_Lambda([this, ViewLabel]()
                    {
                        const bool bIsActive = GetSelectedPreviewViewLabel
                            && GetSelectedPreviewViewLabel().Equals(ViewLabel, ESearchCase::IgnoreCase);
                        return bIsActive
                            ? StudioAccentColor.CopyWithNewOpacity(0.22f)
                            : StudioMutedButtonColor;
                    })
                    [
                        SNew(STextBlock)
                        .ColorAndOpacity_Lambda([this, ViewLabel]()
                        {
                            const bool bIsActive = GetSelectedPreviewViewLabel
                                && GetSelectedPreviewViewLabel().Equals(ViewLabel, ESearchCase::IgnoreCase);
                            return bIsActive
                                ? FLinearColor(0.98f, 0.95f, 1.0f, 1.0f)
                                : SecondaryTextColor;
                        })
                        .Font(MakeStudioFont("Bold", 9))
                        .ShadowColorAndOpacity(StudioShadowColor)
                        .ShadowOffset(FVector2D(0.0f, 1.0f))
                        .Text(FText::FromString(ViewLabel))
                    ]
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
            AssetThumbnail = MakeShared<FAssetThumbnail>(PreviewObject, 900, 520, ThumbnailPool);

            FAssetThumbnailConfig ThumbnailConfig;
            ThumbnailConfig.bAllowFadeIn = false;

            PreviewContentBox->SetContent(
                SNew(SBorder)
                .Padding(1.0f)
                .BorderBackgroundColor(StudioAccentBlueColor.CopyWithNewOpacity(0.22f))
                [
                    SNew(SBorder)
                    .Padding(18.0f)
                    .BorderBackgroundColor(StudioStageBackdropColor)
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()
                        [
                            SNew(SBorder)
                            .Padding(16.0f)
                            .BorderBackgroundColor(FLinearColor(0.030f, 0.040f, 0.082f, 1.0f))
                            [
                                AssetThumbnail->MakeThumbnailWidget(ThumbnailConfig)
                            ]
                        ]
                        + SOverlay::Slot()
                        .HAlign(HAlign_Fill)
                        .VAlign(VAlign_Top)
                        .Padding(18.0f)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .FillWidth(1.0f)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .Padding(0.0f, 0.0f, 10.0f, 0.0f)
                                [
                                    MakeStudioPill(
                                        LOCTEXT("WanaWorksStudioPreviewLiveBadge", "STAGE LIVE"),
                                        StudioAccentBlueColor.CopyWithNewOpacity(0.16f),
                                        FLinearColor(0.88f, 0.94f, 1.0f, 1.0f),
                                        8,
                                        FMargin(12.0f, 6.0f))
                                ]
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                [
                                    MakeStudioPill(
                                        FText::FromString(CachedPreviewViewLabel.IsEmpty() ? TEXT("Overview") : CachedPreviewViewLabel),
                                        StudioAccentColor.CopyWithNewOpacity(0.18f),
                                        FLinearColor(0.98f, 0.95f, 1.0f, 1.0f),
                                        8,
                                        FMargin(12.0f, 6.0f))
                                ]
                            ]
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            [
                                SNew(STextBlock)
                                .ColorAndOpacity(TertiaryTextColor)
                                .Font(MakeStudioFont("Bold", 8))
                                .ShadowColorAndOpacity(StudioShadowColor)
                                .ShadowOffset(FVector2D(0.0f, 1.0f))
                                .Text(LOCTEXT("WanaWorksStudioPreviewSyncLabel", "WANAWORKS STAGE"))
                            ]
                        ]
                        + SOverlay::Slot()
                        .HAlign(HAlign_Fill)
                        .VAlign(VAlign_Bottom)
                        .Padding(18.0f)
                        [
                            SNew(SBorder)
                            .Padding(FMargin(16.0f, 14.0f))
                            .BorderBackgroundColor(FLinearColor(0.018f, 0.026f, 0.050f, 0.92f))
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
                                        .Font(MakeStudioFont("Bold", 8))
                                        .ShadowColorAndOpacity(StudioShadowColor)
                                        .ShadowOffset(FVector2D(0.0f, 1.0f))
                                        .Text(LOCTEXT("WanaWorksStudioPreviewAssetLabel", "ACTIVE SUBJECT"))
                                    ]
                                    + SVerticalBox::Slot()
                                    .AutoHeight()
                                    .Padding(0.0f, 6.0f, 0.0f, 0.0f)
                                    [
                                        SNew(STextBlock)
                                        .ColorAndOpacity(FLinearColor::White)
                                        .Font(MakeStudioFont("Bold", 13))
                                        .ShadowColorAndOpacity(StudioShadowColor)
                                        .ShadowOffset(FVector2D(0.0f, 1.0f))
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
                                    .Font(MakeStudioFont("Regular", 9))
                                    .Text(LOCTEXT("WanaWorksStudioPreviewFooterNote", "Use Focus Subject for full world-space inspection."))
                                ]
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
                .BorderBackgroundColor(StudioAccentColor.CopyWithNewOpacity(0.22f))
                [
                    SNew(SBorder)
                    .Padding(24.0f)
                    .BorderBackgroundColor(StudioStageBackdropColor)
                    [
                        SNew(SOverlay)
                        + SOverlay::Slot()
                        [
                            SNew(SBorder)
                            .Padding(20.0f)
                            .BorderBackgroundColor(FLinearColor(0.030f, 0.040f, 0.082f, 1.0f))
                            [
                                SNew(SBorder)
                                .Padding(36.0f)
                                .BorderBackgroundColor(FLinearColor(0.042f, 0.052f, 0.095f, 0.96f))
                            ]
                        ]
                        + SOverlay::Slot()
                        .HAlign(HAlign_Fill)
                        .VAlign(VAlign_Top)
                        .Padding(18.0f)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .FillWidth(1.0f)
                            [
                                MakeStudioPill(
                                    LOCTEXT("WanaWorksStudioPreviewStandbyBadge", "STAGE READY"),
                                    StudioAccentBlueColor.CopyWithNewOpacity(0.16f),
                                    FLinearColor(0.88f, 0.94f, 1.0f, 1.0f),
                                    8,
                                    FMargin(12.0f, 6.0f))
                            ]
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            [
                                MakeStudioPill(
                                    FText::FromString(CachedPreviewViewLabel.IsEmpty() ? TEXT("Overview") : CachedPreviewViewLabel),
                                    StudioAccentColor.CopyWithNewOpacity(0.18f),
                                    FLinearColor(0.98f, 0.95f, 1.0f, 1.0f),
                                    8,
                                    FMargin(12.0f, 6.0f))
                            ]
                        ]
                        + SOverlay::Slot()
                        .HAlign(HAlign_Center)
                        .VAlign(VAlign_Center)
                        .Padding(32.0f)
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .HAlign(HAlign_Center)
                            .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                            [
                                SNew(STextBlock)
                                .Justification(ETextJustify::Center)
                                .ColorAndOpacity(FLinearColor::White)
                                .Font(MakeStudioFont("Bold", 22))
                                .ShadowColorAndOpacity(StudioShadowColor)
                                .ShadowOffset(FVector2D(0.0f, 1.0f))
                                .Text(LOCTEXT("WanaWorksSandboxPreviewStandbyTitle", "Workspace Stage Ready"))
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .HAlign(HAlign_Center)
                            [
                                SNew(SBox)
                                .MaxDesiredWidth(420.0f)
                                [
                                    SNew(STextBlock)
                                    .Justification(ETextJustify::Center)
                                    .AutoWrapText(true)
                                    .Font(MakeStudioFont("Regular", 10))
                                    .ColorAndOpacity(SecondaryTextColor)
                                    .Text(LOCTEXT("WanaWorksSandboxPreviewPlaceholder", "Choose a subject or run enhancement to wake the stage with a live working preview. WanaWorks will keep this space focused on the subject you are shaping."))
                                ]
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(0.0f, 20.0f, 0.0f, 0.0f)
                            .HAlign(HAlign_Center)
                            [
                                SNew(SBox)
                                .WidthOverride(220.0f)
                                [
                                    MakeStudioDivider(2.0f, StudioAccentColor.CopyWithNewOpacity(0.70f))
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
                .BorderBackgroundColor(StudioOutlineColor.CopyWithNewOpacity(0.46f))
                [
                    SNew(SBorder)
                    .Padding(16.0f)
                    .BorderBackgroundColor(InfoPanelColor)
                    [
                        BuildStudioSummaryRowsWidget(FText::FromString(CachedPreviewSummary), 4, 340.0f, StudioAccentBlueColor)
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
        .Padding(1.0f)
        .BorderBackgroundColor(GetStatusBadgeColor(NormalizedStatus).CopyWithNewOpacity(0.32f))
        [
            SNew(SBorder)
            .Padding(FMargin(10.0f, 4.0f))
            .BorderBackgroundColor(GetStatusBadgeColor(NormalizedStatus).CopyWithNewOpacity(0.18f))
            [
                SNew(STextBlock)
                .Font(MakeStudioFont("Bold", 8))
                .ColorAndOpacity(FLinearColor::White)
                .ShadowColorAndOpacity(StudioShadowColor)
                .ShadowOffset(FVector2D(0.0f, 1.0f))
                .Text(FText::FromString(NormalizedStatus))
            ]
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
            .Font(MakeStudioFont("Bold", 8))
            .ColorAndOpacity(TertiaryTextColor)
            .ShadowColorAndOpacity(StudioShadowColor)
            .ShadowOffset(FVector2D(0.0f, 1.0f))
            .Text(Title)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SBorder)
            .Padding(1.0f)
            .BorderBackgroundColor(StudioOutlineColor.CopyWithNewOpacity(0.36f))
            [
                SNew(SBorder)
                .Padding(12.0f)
                .BorderBackgroundColor(InfoPanelColor)
                [
                    SNew(STextBlock)
                    .AutoWrapText(true)
                    .Font(MakeStudioFont("Regular", 9))
                    .ColorAndOpacity(SecondaryTextColor)
                    .ShadowColorAndOpacity(StudioShadowColor)
                    .ShadowOffset(FVector2D(0.0f, 1.0f))
                    .Text_Lambda([GetBodyText]()
                    {
                        return GetBodyText ? GetBodyText() : FText::GetEmpty();
                    })
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeFixedWidthButton(const FText& ButtonText, TFunction<void(void)> OnPressed, float WidthOverride = 156.0f)
{
    return SNew(SBox)
        .WidthOverride(WidthOverride)
        .HeightOverride(38.0f)
        [
            SNew(SBorder)
            .Padding(1.0f)
            .BorderBackgroundColor(StudioOutlineColor.CopyWithNewOpacity(0.42f))
            [
                SNew(SButton)
                .ButtonStyle(FCoreStyle::Get(), "NoBorder")
                .ContentPadding(FMargin(0.0f))
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
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
                    .Padding(FMargin(14.0f, 10.0f))
                    .BorderBackgroundColor(StudioMutedButtonColor)
                    [
                        SNew(STextBlock)
                        .Font(MakeStudioFont("Bold", 9))
                        .ColorAndOpacity(FLinearColor(0.95f, 0.97f, 1.0f, 1.0f))
                        .ShadowColorAndOpacity(StudioShadowColor)
                        .ShadowOffset(FVector2D(0.0f, 1.0f))
                        .Text(ButtonText)
                    ]
                ]
            ]
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
            .Font(MakeStudioFont("Bold", 8))
            .ColorAndOpacity(TertiaryTextColor)
            .ShadowColorAndOpacity(StudioShadowColor)
            .ShadowOffset(FVector2D(0.0f, 1.0f))
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
                .BorderBackgroundColor(StudioOutlineColor.CopyWithNewOpacity(0.44f))
                [
                    SNew(SBorder)
                    .Padding(FMargin(14.0f, 8.0f))
                    .BorderBackgroundColor(StudioMutedButtonColor)
                    [
                        SNew(SComboBox<TSharedPtr<FString>>)
                        .OptionsSource(Options)
                        .InitiallySelectedItem(GetSelectedOption ? GetSelectedOption() : nullptr)
                        .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
                        {
                            return SNew(STextBlock)
                                .Font(MakeStudioFont("Bold", 9))
                                .ColorAndOpacity(FLinearColor::White)
                                .ShadowColorAndOpacity(StudioShadowColor)
                                .ShadowOffset(FVector2D(0.0f, 1.0f))
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
                            .Font(MakeStudioFont("Bold", 10))
                            .ColorAndOpacity(FLinearColor::White)
                            .ShadowColorAndOpacity(StudioShadowColor)
                            .ShadowOffset(FVector2D(0.0f, 1.0f))
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
            MakeReadOnlyInfoPanel(
                LOCTEXT("WanaWorksPresetAutonomyLabel", "Core Workflow"),
                []()
                {
                    return LOCTEXT("WanaWorksPresetAutonomyText", "Enhance already uses the selected preset profile automatically. Open the preset controls below only when you want to force-apply the profile now or save a reusable variation.");
                })
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 10.0f, 0.0f, 0.0f)
        [
            MakeCollapsibleWorkspaceDetailsSection(
                SectionHeaderFont,
                LOCTEXT("WanaWorksPresetActionsTitle", "Preset Actions"),
                LOCTEXT("WanaWorksPresetActionsDescription", "These are secondary controls for saving the current setup or manually forcing the preset now. The main workflow already stays preset-aware."),
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
                ])
        ],
        LOCTEXT("WanaWorksPresetsDescription", "Pick a reusable WanaWorks starter profile here. The main workflow uses the selected profile automatically while the manual preset controls stay tucked into this secondary area."),
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
            LOCTEXT("WanaWorksValidationGuidedLabel", "Guided Test"),
            [GetGuidedWorkflowSummaryText = Args.GetGuidedWorkflowSummaryText]()
            {
                return GetGuidedWorkflowSummaryText ? GetGuidedWorkflowSummaryText() : FText::GetEmpty();
            })
    ];

    Layout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 10.0f)
    [
        MakeReadOnlyInfoPanel(
            LOCTEXT("WanaWorksValidationMainFlowLabel", "Main Workflow"),
            []()
            {
                return LOCTEXT("WanaWorksValidationMainFlowText", "Use Test on the main workflow strip for the guided evaluation pass. The helper controls below stay folded away for manual observer-target setup or deeper diagnostics only.");
            })
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
            LOCTEXT("WanaWorksValidationSandboxBuildLabel", "Build Final"),
            []()
            {
                return LOCTEXT("WanaWorksValidationSandboxBuildText", "When the working subject looks right, Build Final creates a separate finalized asset under /Game/WanaWorks/Builds. The original source and the working copy both stay preserved.");
            })
    ];

    Layout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 0.0f, 0.0f, 12.0f)
    [
        MakeCollapsibleWorkspaceDetailsSection(
            SectionHeaderFont,
            LOCTEXT("WanaWorksValidationManualPairTitle", "Manual Pair Controls"),
            LOCTEXT("WanaWorksValidationManualPairDescription", "Use these only when you want explicit observer-target assignments or manual focus control beyond the main workflow."),
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
            ])
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
            MakeCollapsibleWorkspaceDetailsSection(
                SectionHeaderFont,
                LOCTEXT("WanaWorksValidationDiagnosticsTitle", "Manual Diagnostics"),
                LOCTEXT("WanaWorksValidationDiagnosticsDescription", "Use these deeper checks only when you want to classify the current subject or inspect environment fit beyond the Analyze step."),
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
                ])
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
    const FSlateFontInfo DescriptionFont = MakeStudioFont("Regular", 9);
    const FSlateFontInfo EffectiveHeaderFont = bProminent ? MakeStudioFont("Bold", 13) : SectionHeaderFont;
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
            .ColorAndOpacity(FLinearColor::White)
            .ShadowColorAndOpacity(StudioShadowColor)
            .ShadowOffset(FVector2D(0.0f, 1.0f))
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
            .ShadowColorAndOpacity(StudioShadowColor)
            .ShadowOffset(FVector2D(0.0f, 1.0f))
            .Text(Description.GetValue())
        ];
    }

    SectionLayout->AddSlot()
    .AutoHeight()
    .Padding(0.0f, 12.0f, 0.0f, 14.0f)
    [
        MakeStudioDivider(1.0f, StudioDividerColor.CopyWithNewOpacity(0.80f))
    ];

    SectionLayout->AddSlot()
    .AutoHeight()
    [
        Content
    ];

    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(StudioOutlineColor.CopyWithNewOpacity(bProminent ? 0.58f : 0.40f))
        [
            SNew(SBorder)
            .Padding(bProminent ? FMargin(20.0f, 18.0f) : FMargin(16.0f, 14.0f))
            .BorderBackgroundColor(bProminent ? StudioPanelRaisedColor : StudioPanelColor)
            [
                SectionLayout
            ]
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
        return StudioAccentColor;
    }

    if (WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase))
    {
        return StudioAccentGoldColor;
    }

    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        return FLinearColor(0.25f, 0.74f, 0.74f, 1.0f);
    }

    return FLinearColor(0.42f, 0.54f, 0.76f, 1.0f);
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
        .BorderBackgroundColor(StudioAccentBlueColor.CopyWithNewOpacity(0.18f))
        [
            SNew(SBorder)
            .Padding(22.0f)
            .BorderBackgroundColor(PreviewPanelColor)
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
    const FLinearColor AccentColor = GetWorkspaceAccentColor(WorkspaceLabel);

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
            .Padding(1.0f)
            .BorderBackgroundColor_Lambda([Args, WorkspaceLabel, AccentColor]()
            {
                return IsWorkspaceActive(Args, WorkspaceLabel)
                    ? AccentColor.CopyWithNewOpacity(0.34f)
                    : StudioOutlineColor.CopyWithNewOpacity(0.24f);
            })
            [
                SNew(SBorder)
                .Padding(FMargin(16.0f, 14.0f))
                .BorderBackgroundColor_Lambda([Args, WorkspaceLabel, bLiveWorkspace, AccentColor]()
                {
                    if (IsWorkspaceActive(Args, WorkspaceLabel))
                    {
                        return AccentColor.CopyWithNewOpacity(0.18f);
                    }

                    return bLiveWorkspace
                        ? StudioShellTrackColor
                        : FLinearColor(0.028f, 0.032f, 0.052f, 0.94f);
                })
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(0.0f, 0.0f, 14.0f, 0.0f)
                    [
                        SNew(SBorder)
                        .Padding(0.0f)
                        .BorderBackgroundColor_Lambda([Args, WorkspaceLabel, bLiveWorkspace, AccentColor]()
                        {
                            if (IsWorkspaceActive(Args, WorkspaceLabel))
                            {
                                return AccentColor;
                            }

                            return bLiveWorkspace
                                ? FLinearColor(0.26f, 0.34f, 0.50f, 1.0f)
                                : FLinearColor(0.16f, 0.19f, 0.28f, 1.0f);
                        })
                        [
                            SNew(SBox)
                            .WidthOverride(6.0f)
                            .HeightOverride(6.0f)
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
                            .Font(MakeStudioFont("Bold", 12))
                            .ColorAndOpacity(FLinearColor::White)
                            .ShadowColorAndOpacity(StudioShadowColor)
                            .ShadowOffset(FVector2D(0.0f, 1.0f))
                            .Text(FText::FromString(WorkspaceLabel))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Font(MakeStudioFont("Regular", 8))
                            .ColorAndOpacity(SecondaryTextColor.CopyWithNewOpacity(0.90f))
                            .Text(Subtitle)
                        ]
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(12.0f, 0.0f, 0.0f, 0.0f)
                    [
                        SNew(SBorder)
                        .Padding(FMargin(10.0f, 4.0f))
                        .BorderBackgroundColor_Lambda([Args, WorkspaceLabel, bLiveWorkspace, AccentColor]()
                        {
                            if (IsWorkspaceActive(Args, WorkspaceLabel))
                            {
                                return AccentColor.CopyWithNewOpacity(0.18f);
                            }

                            return bLiveWorkspace
                                ? StudioAccentBlueColor.CopyWithNewOpacity(0.12f)
                                : FLinearColor(0.18f, 0.20f, 0.28f, 0.28f);
                        })
                        [
                            SNew(STextBlock)
                            .Font(MakeStudioFont("Bold", 7))
                            .ColorAndOpacity_Lambda([Args, WorkspaceLabel, bLiveWorkspace]()
                            {
                                if (IsWorkspaceActive(Args, WorkspaceLabel))
                                {
                                    return FLinearColor::White;
                                }

                                return bLiveWorkspace
                                    ? FLinearColor(0.84f, 0.91f, 1.0f, 1.0f)
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
            ]
        ];
}

TSharedRef<SWidget> MakeStudioNavigationRail(const FWanaWorksUITabBuilderArgs& Args)
{
    const TArray<FStudioWorkspaceEntry> WorkspaceEntries =
    {
        { TEXT("AI"), LOCTEXT("WanaWorksStudioNavAISubtitle", "Flagship live workspace") },
        { TEXT("Character Building"), LOCTEXT("WanaWorksStudioNavCharacterSubtitle", "Live builder workspace") },
        { TEXT("Level Design"), LOCTEXT("WanaWorksStudioNavLevelSubtitle", "Live scene workspace") },
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
        .BorderBackgroundColor(StudioOutlineColor.CopyWithNewOpacity(0.32f))
        [
            SNew(SBorder)
            .Padding(22.0f)
            .BorderBackgroundColor(StudioShellRaisedColor)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 22.0f)
                [
                    SNew(SBorder)
                    .Padding(20.0f)
                    .BorderBackgroundColor(FLinearColor(0.028f, 0.033f, 0.058f, 0.98f))
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Font(MakeStudioFont("Bold", 20))
                            .ColorAndOpacity(FLinearColor::White)
                            .ShadowColorAndOpacity(StudioShadowColor)
                            .ShadowOffset(FVector2D(0.0f, 1.0f))
                            .Text(LOCTEXT("WanaWorksStudioBrand", "WanaWorks"))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 5.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Font(MakeStudioFont("Bold", 8))
                            .ColorAndOpacity(TertiaryTextColor)
                            .Text(LOCTEXT("WanaWorksStudioBrandEyebrow", "AI DEVELOPMENT STUDIO"))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 10.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Font(MakeStudioFont("Regular", 9))
                            .ColorAndOpacity(SecondaryTextColor)
                            .AutoWrapText(true)
                            .Text(LOCTEXT("WanaWorksStudioBrandSubtitle", "Studio shell for autonomous building, testing, and final output."))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 14.0f, 0.0f, 0.0f)
                        [
                            MakeStudioDivider(1.0f, StudioAccentColor.CopyWithNewOpacity(0.45f))
                        ]
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    SNew(STextBlock)
                    .Font(MakeStudioFont("Bold", 8))
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
                .Padding(0.0f, 18.0f, 0.0f, 0.0f)
                [
                    SNew(SBorder)
                    .Padding(16.0f)
                    .BorderBackgroundColor(FLinearColor(0.026f, 0.031f, 0.054f, 0.98f))
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Font(MakeStudioFont("Bold", 8))
                            .ColorAndOpacity(TertiaryTextColor)
                            .Text(LOCTEXT("WanaWorksStudioProjectLabel", "PROJECT"))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 8.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .ColorAndOpacity(FLinearColor::White)
                            .Font(MakeStudioFont("Bold", 11))
                            .ShadowColorAndOpacity(StudioShadowColor)
                            .ShadowOffset(FVector2D(0.0f, 1.0f))
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
        .BorderBackgroundColor(StudioOutlineColor.CopyWithNewOpacity(0.42f))
        [
            SNew(SBorder)
            .Padding(22.0f)
            .BorderBackgroundColor(StudioPanelColor)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Font(MakeStudioFont("Bold", 8))
                    .ColorAndOpacity(TertiaryTextColor)
                    .ShadowColorAndOpacity(StudioShadowColor)
                    .ShadowOffset(FVector2D(0.0f, 1.0f))
                    .Text(bAIWorkspace
                        ? LOCTEXT("WanaWorksStudioAISubjectPanelEyebrow", "SUBJECT STACK")
                        : LOCTEXT("WanaWorksStudioCharacterSubjectPanelEyebrow", "CHARACTER BUILD"))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 6.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Font(MakeStudioFont("Bold", 17))
                    .ColorAndOpacity(FLinearColor::White)
                    .ShadowColorAndOpacity(StudioShadowColor)
                    .ShadowOffset(FVector2D(0.0f, 1.0f))
                    .Text(bAIWorkspace
                        ? LOCTEXT("WanaWorksStudioAISubjectPanelTitle", "AI Subject")
                        : LOCTEXT("WanaWorksStudioCharacterSubjectPanelTitle", "Character Build Subject"))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 8.0f, 0.0f, 18.0f)
                [
                    SNew(STextBlock)
                    .AutoWrapText(true)
                    .Font(MakeStudioFont("Regular", 9))
                    .ColorAndOpacity(SecondaryTextColor)
                    .Text(bAIWorkspace
                        ? LOCTEXT("WanaWorksStudioAISubjectPanelSubtitle", "Select the AI-facing subject and let WanaWorks surface stack readiness automatically.")
                        : LOCTEXT("WanaWorksStudioCharacterSubjectPanelSubtitle", "Select the character-facing subject and let WanaWorks surface identity fit, animation readiness, and final build context."))
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
            bAIWorkspace
                ? MakeStudioStatusCard(
                    LOCTEXT("WanaWorksStudioAnimationCardTitle", "Animation Integration"),
                    LOCTEXT("WanaWorksStudioAnimationCardEyebrow", "INTEGRATION"),
                    Args.GetAnimationIntegrationText,
                    StudioSuccessColor,
                    7,
                    210.0f)
                : MakeStudioStatusCard(
                    LOCTEXT("WanaWorksStudioCharacterIdentityCardTitle", "Identity Profile"),
                    LOCTEXT("WanaWorksStudioCharacterIdentityCardEyebrow", "CHARACTER"),
                    Args.GetIdentitySummaryText,
                    StudioAccentGoldColor,
                    6,
                    210.0f)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 14.0f)
        [
            MakeStudioStatusCard(
                bAIWorkspace
                    ? LOCTEXT("WanaWorksStudioPhysicalCardTitle", "Physical State")
                    : LOCTEXT("WanaWorksStudioCharacterAnimationCardTitle", "Animation Integration"),
                bAIWorkspace
                    ? LOCTEXT("WanaWorksStudioPhysicalCardEyebrow", "BODY")
                    : LOCTEXT("WanaWorksStudioCharacterAnimationCardEyebrow", "BRIDGE"),
                bAIWorkspace ? Args.GetPhysicalStateText : Args.GetAnimationIntegrationText,
                bAIWorkspace
                    ? StudioAccentBlueColor
                    : StudioSuccessColor,
                7,
                210.0f)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 14.0f)
        [
            MakeStudioStatusCard(
                bAIWorkspace
                    ? LOCTEXT("WanaWorksStudioBehaviorCardTitle", "Behavior Results")
                    : LOCTEXT("WanaWorksStudioCharacterPhysicalCardTitle", "Physical State"),
                bAIWorkspace
                    ? LOCTEXT("WanaWorksStudioBehaviorCardEyebrow", "LIVE AI")
                    : LOCTEXT("WanaWorksStudioCharacterPhysicalCardEyebrow", "BODY"),
                bAIWorkspace ? Args.GetBehaviorResultsText : Args.GetPhysicalStateText,
                bAIWorkspace
                    ? StudioSuccessColor
                    : StudioAccentBlueColor,
                8,
                210.0f)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            MakeStudioStatusCard(
                bAIWorkspace
                    ? LOCTEXT("WanaWorksStudioEnvironmentCardTitle", "Environment Fit")
                    : LOCTEXT("WanaWorksStudioCharacterBuildCardTitle", "Build Readiness"),
                bAIWorkspace
                    ? LOCTEXT("WanaWorksStudioEnvironmentCardEyebrow", "ANALYZE")
                    : LOCTEXT("WanaWorksStudioCharacterBuildCardEyebrow", "OUTPUT"),
                bAIWorkspace ? Args.GetWITEnvironmentReadinessText : Args.GetEnhancementResultsText,
                bAIWorkspace
                    ? FLinearColor(0.25f, 0.74f, 0.74f, 1.0f)
                    : StudioAccentGoldColor,
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
        .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.24f))
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
                .Padding(20.0f)
                .BorderBackgroundColor(FLinearColor(0.034f, 0.040f, 0.070f, 0.99f))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 18.0f)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .FillWidth(1.0f)
                        [
                            MakeStudioDivider(2.0f, AccentColor.CopyWithNewOpacity(0.90f))
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(12.0f, 0.0f, 0.0f, 0.0f)
                        [
                            MakeStudioPill(
                                FText::FromString(StepNumber),
                                AccentColor.CopyWithNewOpacity(0.18f),
                                AccentColor,
                                9,
                                FMargin(12.0f, 6.0f))
                        ]
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Font(MakeStudioFont("Bold", 16))
                        .ColorAndOpacity(FLinearColor::White)
                        .ShadowColorAndOpacity(StudioShadowColor)
                        .ShadowOffset(FVector2D(0.0f, 1.0f))
                        .Text(Title)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 12.0f, 0.0f, 18.0f)
                    [
                        SNew(STextBlock)
                        .AutoWrapText(true)
                        .Font(MakeStudioFont("Regular", 9))
                        .ColorAndOpacity(SecondaryTextColor)
                        .Text(Description)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        MakeStudioPill(
                            LOCTEXT("WanaWorksStudioWorkflowActionChip", "Run Step"),
                            AccentColor.CopyWithNewOpacity(0.16f),
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
    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(StudioOutlineColor.CopyWithNewOpacity(0.42f))
        [
            SNew(SBorder)
            .Padding(20.0f)
            .BorderBackgroundColor(StudioPanelColor)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
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
                            .Font(MakeStudioFont("Bold", 8))
                            .ColorAndOpacity(TertiaryTextColor)
                            .Text(LOCTEXT("WanaWorksStudioWorkflowStripEyebrow", "CORE FLOW"))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 5.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Font(MakeStudioFont("Bold", 16))
                            .ColorAndOpacity(FLinearColor::White)
                            .ShadowColorAndOpacity(StudioShadowColor)
                            .ShadowOffset(FVector2D(0.0f, 1.0f))
                            .Text(LOCTEXT("WanaWorksStudioWorkflowStripTitle", "Enhance. Test. Analyze. Build."))
                        ]
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    [
                        MakeStudioPill(
                            bAIWorkspace
                                ? LOCTEXT("WanaWorksStudioWorkflowStripAIBadge", "AI WORKFLOW")
                                : LOCTEXT("WanaWorksStudioWorkflowStripCharacterBadge", "CHARACTER WORKFLOW"),
                            GetWorkspaceAccentColor(bAIWorkspace ? TEXT("AI") : TEXT("Character Building")).CopyWithNewOpacity(0.16f),
                            FLinearColor(0.95f, 0.97f, 1.0f, 1.0f),
                            8,
                            FMargin(12.0f, 6.0f))
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 18.0f)
                [
                    MakeStudioDivider(1.0f, StudioDividerColor.CopyWithNewOpacity(0.82f))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .Padding(0.0f, 0.0f, 12.0f, 0.0f)
                    [
                        MakeWorkflowTile(
                            TEXT("1"),
                            LOCTEXT("WanaWorksStudioWorkflowEnhanceTitle", "Enhance"),
                            bAIWorkspace
                                ? LOCTEXT("WanaWorksStudioWorkflowEnhanceAIText", "Prepare the AI subject automatically and apply the active enhancement stack.")
                                : LOCTEXT("WanaWorksStudioWorkflowEnhanceCharacterText", "Prepare the character build subject automatically and apply the active enhancement stack."),
                            GetWorkspaceAccentColor(bAIWorkspace ? TEXT("AI") : TEXT("Character Building")),
                            Args.OnApplyCharacterEnhancement)
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .Padding(0.0f, 0.0f, 12.0f, 0.0f)
                    [
                        MakeWorkflowTile(
                            TEXT("2"),
                            LOCTEXT("WanaWorksStudioWorkflowTestTitle", "Test"),
                            bAIWorkspace
                                ? LOCTEXT("WanaWorksStudioWorkflowTestAIText", "Run the guided AI reaction pass and visible workspace behavior test.")
                                : LOCTEXT("WanaWorksStudioWorkflowTestCharacterText", "Run the character response pass and validate visible build behavior safely."),
                            StudioAccentBlueColor,
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
                                : LOCTEXT("WanaWorksStudioWorkflowAnalyzeCharacterText", "Review identity fit, animation integration, and build readiness."),
                            FLinearColor(0.22f, 0.73f, 0.78f, 1.0f),
                            bAIWorkspace ? Args.OnScanEnvironmentReadiness : Args.OnEvaluateLiveTarget)
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    [
                        MakeWorkflowTile(
                            TEXT("4"),
                            LOCTEXT("WanaWorksStudioWorkflowBuildTitle", "Build"),
                            bAIWorkspace
                                ? LOCTEXT("WanaWorksStudioWorkflowBuildAIText", "Save the final AI-ready asset quietly to /Game/WanaWorks/Builds while the workspace stays in focus.")
                                : LOCTEXT("WanaWorksStudioWorkflowBuildCharacterText", "Save the final character build asset quietly to /Game/WanaWorks/Builds while the workspace stays in focus."),
                            StudioAccentGoldColor,
                            Args.OnFinalizeSandboxBuild)
                    ]
                ]
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
        ? LOCTEXT("WanaWorksStudioAIDescription", "Run the premium AI workflow with only the live subject, hero preview, status cards, and the four core actions in view.")
        : LOCTEXT("WanaWorksStudioCharacterDescription", "Focus on character identity, animation bridge quality, physical response, and clean build readiness inside a dedicated studio workspace.");

    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 16.0f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                MakeStudioPill(
                    bAIWorkspace
                        ? LOCTEXT("WanaWorksStudioAIWorkspaceChip", "AUTONOMOUS AI")
                        : LOCTEXT("WanaWorksStudioCharacterWorkspaceChip", "CHARACTER STUDIO"),
                    GetWorkspaceAccentColor(WorkspaceLabel).CopyWithNewOpacity(0.16f),
                    FLinearColor(0.95f, 0.97f, 1.0f, 1.0f),
                    8,
                    FMargin(12.0f, 6.0f))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 10.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .Font(MakeStudioFont("Bold", 22))
                .ColorAndOpacity(FLinearColor::White)
                .ShadowColorAndOpacity(StudioShadowColor)
                .ShadowOffset(FVector2D(0.0f, 1.0f))
                .Text(WorkspaceTitle)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 8.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .AutoWrapText(true)
                .Font(MakeStudioFont("Regular", 10))
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
            .Value(bAIWorkspace ? 0.24f : 0.25f)
            [
                MakeStudioSubjectColumn(Args, bAIWorkspace)
            ]
            + SSplitter::Slot()
            .Value(bAIWorkspace ? 0.46f : 0.43f)
            [
                MakeStudioHeroStage(Args)
            ]
            + SSplitter::Slot()
            .Value(bAIWorkspace ? 0.30f : 0.32f)
            [
                MakeStudioStatusColumn(Args, bAIWorkspace)
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 0.0f)
        [
            MakeStudioWorkflowStrip(Args, bAIWorkspace)
        ];
}

TSharedRef<SWidget> MakeLevelDesignWorkspaceSurface(const FWanaWorksUITabBuilderArgs& Args, const FSlateFontInfo& SectionHeaderFont)
{
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
                .Text(LOCTEXT("WanaWorksStudioLevelDesignTitle", "Level Design"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 6.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .AutoWrapText(true)
                .ColorAndOpacity(SecondaryTextColor)
                .Text(LOCTEXT("WanaWorksStudioLevelDesignDescription", "Scene-side utilities live here so the AI workspace can stay clean, minimal, and fully focused on the four-step subject workflow."))
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SSplitter)
            .PhysicalSplitterHandleSize(2.0f)
            + SSplitter::Slot()
            .Value(0.26f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeSection(
                        SectionHeaderFont,
                        LOCTEXT("WanaWorksStudioLevelWorkspaceToolsTitle", "Scene Workspace"),
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
                        [
                            MakeReadOnlyInfoPanel(
                                LOCTEXT("WanaWorksStudioLevelWorkspaceToolsLabel", "Intent"),
                                []()
                                {
                                    return LOCTEXT("WanaWorksStudioLevelWorkspaceToolsText", "This lane now owns the quick scene and utility actions that were removed from the AI workspace surface.");
                                })
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
                        [
                            MakeWrappedButtonSection(
                                Args,
                                SectionHeaderFont,
                                LOCTEXT("WanaWorksStudioLevelWeatherSection", "Weather"),
                                {
                                    TEXT("weather_clear"),
                                    TEXT("weather_overcast"),
                                    TEXT("weather_storm")
                                })
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            MakeWrappedButtonSection(
                                Args,
                                SectionHeaderFont,
                                LOCTEXT("WanaWorksStudioLevelSceneSection", "Scene"),
                                {
                                    TEXT("spawn_cube"),
                                    TEXT("list_selection")
                                })
                        ],
                        LOCTEXT("WanaWorksStudioLevelWorkspaceSectionDescription", "Environment and scene actions stay available here without polluting the AI or Character workspaces."),
                        LOCTEXT("WanaWorksStudioLevelWorkspaceSectionStatus", "AVAILABLE"))
                ]
            ]
            + SSplitter::Slot()
            .Value(0.46f)
            [
                MakeStudioHeroStage(Args)
            ]
            + SSplitter::Slot()
            .Value(0.28f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioLevelStatusTitle", "Workspace Status"),
                        LOCTEXT("WanaWorksStudioLevelStatusEyebrow", "LIVE"),
                        Args.GetStatusText,
                        FLinearColor(0.20f, 0.46f, 0.76f, 1.0f),
                        4,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioLevelPreviewTitle", "Preview Subject"),
                        LOCTEXT("WanaWorksStudioLevelPreviewEyebrow", "STAGE"),
                        Args.GetSandboxPreviewSummaryText,
                        FLinearColor(0.42f, 0.28f, 0.82f, 1.0f),
                        4,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioLevelSavedTitle", "Saved Progress"),
                        LOCTEXT("WanaWorksStudioLevelSavedEyebrow", "PROJECT"),
                        Args.GetSavedSubjectProgressText,
                        FLinearColor(0.18f, 0.62f, 0.58f, 1.0f),
                        4,
                        210.0f)
                ]
            ]
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
                .Font(MakeStudioFont("Bold", 22))
                .ColorAndOpacity(FLinearColor::White)
                .ShadowColorAndOpacity(StudioShadowColor)
                .ShadowOffset(FVector2D(0.0f, 1.0f))
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
            .Font(MakeStudioFont("Regular", 10))
            .ColorAndOpacity(SecondaryTextColor)
            .Text(LOCTEXT("WanaWorksStudioFutureWorkspaceText", "This lane is part of the premium studio shell already, but its production workflow is intentionally staged for a later pass. AI, Character Building, and Level Design are the active workspaces right now."))
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

TSharedRef<SWidget> MakeWorkspaceContextPanel(
    const FText& Eyebrow,
    const FText& Title,
    const FText& Description,
    const FLinearColor& AccentColor,
    const TSharedRef<SWidget>& Content)
{
    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.30f))
        [
            SNew(SBorder)
            .Padding(24.0f)
            .BorderBackgroundColor(StudioPanelColor)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    MakeStudioPill(
                        Eyebrow,
                        AccentColor.CopyWithNewOpacity(0.16f),
                        FLinearColor(0.97f, 0.98f, 1.0f, 1.0f),
                        8,
                        FMargin(12.0f, 6.0f))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 12.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Font(MakeStudioFont("Bold", 19))
                    .ColorAndOpacity(FLinearColor::White)
                    .ShadowColorAndOpacity(StudioShadowColor)
                    .ShadowOffset(FVector2D(0.0f, 1.0f))
                    .Text(Title)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 8.0f, 0.0f, 16.0f)
                [
                    SNew(STextBlock)
                    .AutoWrapText(true)
                    .Font(MakeStudioFont("Regular", 10))
                    .ColorAndOpacity(SecondaryTextColor)
                    .Text(Description)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 16.0f)
                [
                    MakeStudioDivider(1.0f, AccentColor.CopyWithNewOpacity(0.42f))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    Content
                ]
            ]
        ];
}

void AddWrappedCommandButton(
    TSharedRef<SWrapBox> ButtonRow,
    const FWanaWorksUITabBuilderArgs& Args,
    const TCHAR* CommandId)
{
    if (const FWanaCommandDefinition* Definition = WanaWorksCommandRegistry::FindCommandDefinitionById(FName(CommandId)))
    {
        ButtonRow->AddSlot()
        .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
        [
            MakeCommandButton(Args, *Definition)
        ];
    }
}

TSharedRef<SWidget> MakeWorkspaceStageShell(
    const FWanaWorksUITabBuilderArgs& Args,
    const FString& WorkspaceLabel,
    const FText& Eyebrow,
    const FText& Title,
    const FText& Description,
    const TArray<FText>& ContextChips)
{
    const FLinearColor AccentColor = GetWorkspaceAccentColor(WorkspaceLabel);
    TSharedRef<SWrapBox> ChipRow = SNew(SWrapBox);

    for (const FText& ChipLabel : ContextChips)
    {
        ChipRow->AddSlot()
        .Padding(FMargin(0.0f, 0.0f, 10.0f, 10.0f))
        [
            MakeStudioPill(
                ChipLabel,
                AccentColor.CopyWithNewOpacity(0.15f),
                FLinearColor(0.95f, 0.97f, 1.0f, 1.0f),
                8,
                FMargin(12.0f, 6.0f))
        ];
    }

    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.34f))
        [
            SNew(SBorder)
            .Padding(22.0f)
            .BorderBackgroundColor(StudioPanelRaisedColor)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    MakeStudioPill(
                        Eyebrow,
                        AccentColor.CopyWithNewOpacity(0.16f),
                        FLinearColor(0.97f, 0.98f, 1.0f, 1.0f),
                        8,
                        FMargin(12.0f, 6.0f))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 12.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Font(MakeStudioFont("Bold", 24))
                    .ColorAndOpacity(FLinearColor::White)
                    .ShadowColorAndOpacity(StudioShadowColor)
                    .ShadowOffset(FVector2D(0.0f, 1.0f))
                    .Text(Title)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 8.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .AutoWrapText(true)
                    .Font(MakeStudioFont("Regular", 10))
                    .ColorAndOpacity(SecondaryTextColor)
                    .Text(Description)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 16.0f, 0.0f, 12.0f)
                [
                    ChipRow
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 16.0f)
                [
                    MakeStudioDivider(1.0f, AccentColor.CopyWithNewOpacity(0.44f))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    MakeStudioHeroStage(Args)
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeAIWorkspaceSurface(const FWanaWorksUITabBuilderArgs& Args)
{
    const FString WorkspaceLabel(TEXT("AI"));

    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 18.0f)
        [
            SNew(SSplitter)
            .PhysicalSplitterHandleSize(2.0f)
            + SSplitter::Slot()
            .Value(0.24f)
            [
                MakeWorkspaceContextPanel(
                    LOCTEXT("WanaWorksStudioAILeftEyebrow", "AI SUBJECT"),
                    LOCTEXT("WanaWorksStudioAILeftTitle", "AI Runtime Context"),
                    LOCTEXT("WanaWorksStudioAILeftDescription", "Pick the AI-facing subject, review its live stack, and let WanaWorks surface WAI identity and enhancement context without exposing tool clutter."),
                    StudioAccentColor,
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 16.0f)
                    [
                        MakeStringPickerControl(
                            LOCTEXT("WanaWorksStudioAIOnlyPickerLabel", "AI Pawn Asset"),
                            Args.AIPawnAssetOptions,
                            Args.GetSelectedAIPawnAssetOption,
                            Args.OnAIPawnAssetOptionSelected,
                            LOCTEXT("WanaWorksStudioAIOnlyPickerDefault", "(Choose AI Pawn)"),
                            260.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioAISelectedSubjectCardTitle", "Selected Subject"),
                            LOCTEXT("WanaWorksStudioAISelectedSubjectCardEyebrow", "LIVE BINDING"),
                            Args.GetSubjectSetupSummaryText,
                            StudioAccentColor,
                            4,
                            188.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioAIStackCardTitle", "Detected AI Stack"),
                            LOCTEXT("WanaWorksStudioAIStackCardEyebrow", "STACK"),
                            Args.GetSubjectStackSummaryText,
                            FLinearColor(0.18f, 0.60f, 0.67f, 1.0f),
                            7,
                            188.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioAIIdentityCardTitle", "WAI Identity"),
                            LOCTEXT("WanaWorksStudioAIIdentityCardEyebrow", "WHO AM I"),
                            Args.GetIdentitySummaryText,
                            StudioAccentBlueColor,
                            5,
                            188.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioAIProfileCardTitle", "Enhancement Profile"),
                            LOCTEXT("WanaWorksStudioAIProfileCardEyebrow", "ACTIVE PRESET"),
                            Args.GetWorkflowPresetSummaryText,
                            StudioSuccessColor,
                            5,
                            188.0f)
                    ])
            ]
            + SSplitter::Slot()
            .Value(0.46f)
            [
                MakeWorkspaceStageShell(
                    Args,
                    WorkspaceLabel,
                    LOCTEXT("WanaWorksStudioAIStageEyebrow", "AI HERO STAGE"),
                    LOCTEXT("WanaWorksStudioAIStageTitle", "Autonomous Runtime Stage"),
                    LOCTEXT("WanaWorksStudioAIStageDescription", "This stage keeps the AI subject visually central while WanaWorks routes identity, world understanding, physical disruption, and behavior response into one premium workspace."),
                    {
                        LOCTEXT("WanaWorksStudioAIStageChipOne", "AI RUNTIME"),
                        LOCTEXT("WanaWorksStudioAIStageChipTwo", "WAI IDENTITY"),
                        LOCTEXT("WanaWorksStudioAIStageChipThree", "WIT READY")
                    })
            ]
            + SSplitter::Slot()
            .Value(0.30f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioAIAnimationStatusTitle", "Animation Integration"),
                        LOCTEXT("WanaWorksStudioAIAnimationStatusEyebrow", "INTEGRATION"),
                        Args.GetAnimationIntegrationText,
                        StudioSuccessColor,
                        7,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioAIPhysicalStatusTitle", "Physical State"),
                        LOCTEXT("WanaWorksStudioAIPhysicalStatusEyebrow", "BODY"),
                        Args.GetPhysicalStateText,
                        StudioAccentBlueColor,
                        7,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioAIBehaviorStatusTitle", "Behavior Results"),
                        LOCTEXT("WanaWorksStudioAIBehaviorStatusEyebrow", "REACTION"),
                        Args.GetBehaviorResultsText,
                        StudioSuccessColor,
                        8,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioAIEnvironmentStatusTitle", "WIT Environment"),
                        LOCTEXT("WanaWorksStudioAIEnvironmentStatusEyebrow", "WORLD UNDERSTANDING"),
                        Args.GetWITEnvironmentReadinessText,
                        FLinearColor(0.24f, 0.74f, 0.78f, 1.0f),
                        8,
                        210.0f)
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            MakeStudioWorkflowStrip(Args, true)
        ];
}

TSharedRef<SWidget> MakeCharacterBuildingWorkspaceSurface(const FWanaWorksUITabBuilderArgs& Args)
{
    const FString WorkspaceLabel(TEXT("Character Building"));

    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 18.0f)
        [
            SNew(SSplitter)
            .PhysicalSplitterHandleSize(2.0f)
            + SSplitter::Slot()
            .Value(0.25f)
            [
                MakeWorkspaceContextPanel(
                    LOCTEXT("WanaWorksStudioCharacterLeftEyebrow", "CHARACTER BUILD"),
                    LOCTEXT("WanaWorksStudioCharacterLeftTitle", "Character Identity Context"),
                    LOCTEXT("WanaWorksStudioCharacterLeftDescription", "Shape the character-facing subject with clear visibility into mesh, animation, identity, and relationship context without duplicating the AI workspace."),
                    StudioAccentGoldColor,
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 16.0f)
                    [
                        MakeStringPickerControl(
                            LOCTEXT("WanaWorksStudioCharacterOnlyPickerLabel", "Character Pawn Asset"),
                            Args.CharacterPawnAssetOptions,
                            Args.GetSelectedCharacterPawnAssetOption,
                            Args.OnCharacterPawnAssetOptionSelected,
                            LOCTEXT("WanaWorksStudioCharacterOnlyPickerDefault", "(Choose Character Pawn)"),
                            260.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioCharacterSelectedSubjectTitle", "Character Subject"),
                            LOCTEXT("WanaWorksStudioCharacterSelectedSubjectEyebrow", "CURRENT BUILD"),
                            Args.GetSubjectSetupSummaryText,
                            StudioAccentGoldColor,
                            4,
                            188.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioCharacterStackTitle", "Rig + Animation Stack"),
                            LOCTEXT("WanaWorksStudioCharacterStackEyebrow", "SKELETAL STACK"),
                            Args.GetSubjectStackSummaryText,
                            FLinearColor(0.18f, 0.56f, 0.72f, 1.0f),
                            7,
                            188.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioCharacterIdentityFocusTitle", "Identity Profile"),
                            LOCTEXT("WanaWorksStudioCharacterIdentityFocusEyebrow", "WHO AM I"),
                            Args.GetIdentitySummaryText,
                            StudioAccentGoldColor,
                            5,
                            188.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioCharacterRelationshipTitle", "Relationship Lens"),
                            LOCTEXT("WanaWorksStudioCharacterRelationshipEyebrow", "WAY"),
                            Args.GetRelationshipSummaryText,
                            StudioAccentBlueColor,
                            6,
                            188.0f)
                    ])
            ]
            + SSplitter::Slot()
            .Value(0.43f)
            [
                MakeWorkspaceStageShell(
                    Args,
                    WorkspaceLabel,
                    LOCTEXT("WanaWorksStudioCharacterStageEyebrow", "CHARACTER HERO STAGE"),
                    LOCTEXT("WanaWorksStudioCharacterStageTitle", "Character Build Stage"),
                    LOCTEXT("WanaWorksStudioCharacterStageDescription", "This stage emphasizes the character as a buildable asset: mesh-aware, animation-aware, relationship-aware, and ready for a clean final output path."),
                    {
                        LOCTEXT("WanaWorksStudioCharacterStageChipOne", "CHARACTER BUILD"),
                        LOCTEXT("WanaWorksStudioCharacterStageChipTwo", "WAY RELATION"),
                        LOCTEXT("WanaWorksStudioCharacterStageChipThree", "RIG AWARE")
                    })
            ]
            + SSplitter::Slot()
            .Value(0.32f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioCharacterAnimationRightTitle", "Animation Bridge"),
                        LOCTEXT("WanaWorksStudioCharacterAnimationRightEyebrow", "ANIM BP"),
                        Args.GetAnimationIntegrationText,
                        StudioSuccessColor,
                        7,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioCharacterPhysicalRightTitle", "Physical Readiness"),
                        LOCTEXT("WanaWorksStudioCharacterPhysicalRightEyebrow", "BODY"),
                        Args.GetPhysicalStateText,
                        StudioAccentBlueColor,
                        7,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioCharacterIdentityRightTitle", "Build Identity"),
                        LOCTEXT("WanaWorksStudioCharacterIdentityRightEyebrow", "PROFILE"),
                        Args.GetIdentitySummaryText,
                        StudioAccentGoldColor,
                        6,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioCharacterOutputRightTitle", "Build Readiness"),
                        LOCTEXT("WanaWorksStudioCharacterOutputRightEyebrow", "OUTPUT"),
                        Args.GetEnhancementResultsText,
                        StudioAccentGoldColor,
                        7,
                        210.0f)
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            MakeStudioWorkflowStrip(Args, false)
        ];
}

TSharedRef<SWidget> MakeLevelWorkspaceWorkflowStrip(const FWanaWorksUITabBuilderArgs& Args)
{
    const FLinearColor LevelAccentColor = GetWorkspaceAccentColor(TEXT("Level Design"));

    return SNew(SBorder)
        .Padding(1.0f)
        .BorderBackgroundColor(LevelAccentColor.CopyWithNewOpacity(0.28f))
        [
            SNew(SBorder)
            .Padding(20.0f)
            .BorderBackgroundColor(StudioPanelColor)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
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
                            .Font(MakeStudioFont("Bold", 8))
                            .ColorAndOpacity(TertiaryTextColor)
                            .Text(LOCTEXT("WanaWorksStudioLevelWorkflowEyebrow", "LEVEL FLOW"))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 5.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Font(MakeStudioFont("Bold", 16))
                            .ColorAndOpacity(FLinearColor::White)
                            .ShadowColorAndOpacity(StudioShadowColor)
                            .ShadowOffset(FVector2D(0.0f, 1.0f))
                            .Text(LOCTEXT("WanaWorksStudioLevelWorkflowTitle", "Enhance. Test. Analyze. Build."))
                        ]
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    [
                        MakeStudioPill(
                            LOCTEXT("WanaWorksStudioLevelWorkflowBadge", "LEVEL DESIGN"),
                            LevelAccentColor.CopyWithNewOpacity(0.16f),
                            FLinearColor(0.95f, 0.97f, 1.0f, 1.0f),
                            8,
                            FMargin(12.0f, 6.0f))
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 18.0f)
                [
                    MakeStudioDivider(1.0f, StudioDividerColor.CopyWithNewOpacity(0.82f))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .Padding(0.0f, 0.0f, 12.0f, 0.0f)
                    [
                        MakeWorkflowTile(
                            TEXT("1"),
                            LOCTEXT("WanaWorksStudioLevelWorkflowEnhanceTile", "Enhance"),
                            LOCTEXT("WanaWorksStudioLevelWorkflowEnhanceText", "Prepare the scene-side workspace context and seed the active subject path without polluting the AI shell."),
                            LevelAccentColor,
                            Args.OnApplyCharacterEnhancement)
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .Padding(0.0f, 0.0f, 12.0f, 0.0f)
                    [
                        MakeWorkflowTile(
                            TEXT("2"),
                            LOCTEXT("WanaWorksStudioLevelWorkflowTestTile", "Test"),
                            LOCTEXT("WanaWorksStudioLevelWorkflowTestText", "Run a level-facing stage pass so the current subject and scene context stay readable inside the workspace."),
                            StudioAccentBlueColor,
                            Args.OnEvaluateLiveTarget)
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .Padding(0.0f, 0.0f, 12.0f, 0.0f)
                    [
                        MakeWorkflowTile(
                            TEXT("3"),
                            LOCTEXT("WanaWorksStudioLevelWorkflowAnalyzeTile", "Analyze"),
                            LOCTEXT("WanaWorksStudioLevelWorkflowAnalyzeText", "Use WIT-driven environment understanding to inspect space limits, obstacle pressure, and world readiness."),
                            LevelAccentColor,
                            Args.OnScanEnvironmentReadiness)
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    [
                        MakeWorkflowTile(
                            TEXT("4"),
                            LOCTEXT("WanaWorksStudioLevelWorkflowBuildTile", "Build"),
                            LOCTEXT("WanaWorksStudioLevelWorkflowBuildText", "Save a clean output quietly while the world-stage workspace remains in focus."),
                            StudioAccentGoldColor,
                            Args.OnFinalizeSandboxBuild)
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> MakeLevelDesignWorkspaceSurfaceV2(const FWanaWorksUITabBuilderArgs& Args)
{
    TSharedRef<SWrapBox> SceneCommandRow = SNew(SWrapBox);
    AddWrappedCommandButton(SceneCommandRow, Args, TEXT("weather_clear"));
    AddWrappedCommandButton(SceneCommandRow, Args, TEXT("weather_overcast"));
    AddWrappedCommandButton(SceneCommandRow, Args, TEXT("weather_storm"));
    AddWrappedCommandButton(SceneCommandRow, Args, TEXT("spawn_cube"));
    AddWrappedCommandButton(SceneCommandRow, Args, TEXT("list_selection"));

    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 18.0f)
        [
            SNew(SSplitter)
            .PhysicalSplitterHandleSize(2.0f)
            + SSplitter::Slot()
            .Value(0.26f)
            [
                MakeWorkspaceContextPanel(
                    LOCTEXT("WanaWorksStudioLevelLeftEyebrow", "LEVEL CONTEXT"),
                    LOCTEXT("WanaWorksStudioLevelLeftTitle", "Scene + World Workspace"),
                    LOCTEXT("WanaWorksStudioLevelLeftDescription", "Level Design now owns the scene-side tools and environment intelligence concepts that do not belong on the AI workspace face. WIT lives here as world understanding, not a button pile."),
                    GetWorkspaceAccentColor(TEXT("Level Design")),
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioLevelWorldCardTitle", "World Understanding"),
                            LOCTEXT("WanaWorksStudioLevelWorldCardEyebrow", "WIT"),
                            Args.GetWITEnvironmentReadinessText,
                            GetWorkspaceAccentColor(TEXT("Level Design")),
                            6,
                            188.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                    [
                        MakeStudioStatusCard(
                            LOCTEXT("WanaWorksStudioLevelContextCardTitle", "Workspace Status"),
                            LOCTEXT("WanaWorksStudioLevelContextCardEyebrow", "SCENE"),
                            Args.GetStatusText,
                            StudioAccentBlueColor,
                            4,
                            188.0f)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 0.0f, 0.0f, 10.0f)
                        [
                            SNew(STextBlock)
                            .Font(MakeStudioFont("Bold", 8))
                            .ColorAndOpacity(TertiaryTextColor)
                            .Text(LOCTEXT("WanaWorksStudioLevelSceneToolsEyebrow", "SCENE ACTIONS"))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SceneCommandRow
                        ]
                    ])
            ]
            + SSplitter::Slot()
            .Value(0.44f)
            [
                MakeWorkspaceStageShell(
                    Args,
                    TEXT("Level Design"),
                    LOCTEXT("WanaWorksStudioLevelStageEyebrow", "WORLD STAGE"),
                    LOCTEXT("WanaWorksStudioLevelStageTitle", "Level Intelligence Stage"),
                    LOCTEXT("WanaWorksStudioLevelStageDescription", "This stage presents the subject inside scene context so world understanding, boundary pressure, preview focus, and level-side output all feel like one product surface."),
                    {
                        LOCTEXT("WanaWorksStudioLevelStageChipOne", "WORLD READ"),
                        LOCTEXT("WanaWorksStudioLevelStageChipTwo", "WIT SCAN"),
                        LOCTEXT("WanaWorksStudioLevelStageChipThree", "SCENE CONTEXT")
                    })
            ]
            + SSplitter::Slot()
            .Value(0.30f)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioLevelRightWITTitle", "Environment Intelligence"),
                        LOCTEXT("WanaWorksStudioLevelRightWITEyebrow", "LIVE WIT"),
                        Args.GetWITEnvironmentReadinessText,
                        GetWorkspaceAccentColor(TEXT("Level Design")),
                        7,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioLevelRightPreviewTitle", "Active Stage Subject"),
                        LOCTEXT("WanaWorksStudioLevelRightPreviewEyebrow", "PREVIEW"),
                        Args.GetSandboxPreviewSummaryText,
                        StudioAccentBlueColor,
                        4,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioLevelRightStatusTitle", "Live Workspace Status"),
                        LOCTEXT("WanaWorksStudioLevelRightStatusEyebrow", "STATUS"),
                        Args.GetStatusText,
                        StudioSuccessColor,
                        4,
                        210.0f)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    MakeStudioStatusCard(
                        LOCTEXT("WanaWorksStudioLevelRightSavedTitle", "Saved Progress"),
                        LOCTEXT("WanaWorksStudioLevelRightSavedEyebrow", "PROJECT"),
                        Args.GetSavedSubjectProgressText,
                        StudioAccentGoldColor,
                        4,
                        210.0f)
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            MakeLevelWorkspaceWorkflowStrip(Args)
        ];
}
}

namespace WanaWorksUITabBuilder
{
TSharedRef<SWidget> BuildTabContent(const FWanaWorksUITabBuilderArgs& Args)
{
    const FString SelectedWorkspaceLabel = Args.GetSelectedWorkspaceLabel
        ? Args.GetSelectedWorkspaceLabel()
        : TEXT("AI");
    const bool bIsAIWorkspace = SelectedWorkspaceLabel.Equals(TEXT("AI"), ESearchCase::IgnoreCase);
    const bool bIsCharacterWorkspace = SelectedWorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase);
    const bool bIsLevelDesignWorkspace = SelectedWorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase);
    const FText ActiveWorkspaceTitle = bIsAIWorkspace
        ? LOCTEXT("WanaWorksStudioShellAIWorkspaceTitle", "AI Workspace")
        : (bIsCharacterWorkspace
            ? LOCTEXT("WanaWorksStudioShellCharacterWorkspaceTitle", "Character Building")
            : (bIsLevelDesignWorkspace
                ? LOCTEXT("WanaWorksStudioShellLevelWorkspaceTitle", "Level Design")
                : FText::FromString(SelectedWorkspaceLabel)));
    const FText ActiveWorkspaceSubtitle = bIsAIWorkspace
        ? LOCTEXT("WanaWorksStudioShellAISubtitle", "Enhance, test, and build intelligent characters with autonomous AI.")
        : (bIsCharacterWorkspace
            ? LOCTEXT("WanaWorksStudioShellCharacterSubtitle", "Shape subject identity, stack readiness, animation integration, and final build output.")
            : (bIsLevelDesignWorkspace
                ? LOCTEXT("WanaWorksStudioShellLevelSubtitle", "Stage scene-side workspace utilities here so the AI workspace can remain clean and premium.")
                : LOCTEXT("WanaWorksStudioShellFutureSubtitle", "Platform workspace shell present. Capability expansion can land here in a later phase.")));

    TSharedRef<SWidget> WorkspaceSurface = bIsAIWorkspace
        ? MakeAIWorkspaceSurface(Args)
        : (bIsCharacterWorkspace
            ? MakeCharacterBuildingWorkspaceSurface(Args)
            : (bIsLevelDesignWorkspace
                ? MakeLevelDesignWorkspaceSurfaceV2(Args)
                : MakeFutureWorkspaceSurface(Args, SelectedWorkspaceLabel)));

    return SNew(SBorder)
        .Padding(20.0f)
        .BorderBackgroundColor(StudioShellColor)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(0.0f, 0.0f, 20.0f, 0.0f)
            [
                SNew(SBox)
                .WidthOverride(268.0f)
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
                    .BorderBackgroundColor(StudioOutlineColor.CopyWithNewOpacity(0.38f))
                    [
                        SNew(SBorder)
                        .Padding(FMargin(20.0f, 18.0f))
                        .BorderBackgroundColor(StudioShellRaisedColor)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .FillWidth(0.40f)
                            .VAlign(VAlign_Center)
                            .Padding(0.0f, 0.0f, 18.0f, 0.0f)
                            [
                                SNew(SBorder)
                                .Padding(1.0f)
                                .BorderBackgroundColor(StudioOutlineColor.CopyWithNewOpacity(0.36f))
                                [
                                    SNew(SBorder)
                                    .Padding(FMargin(16.0f, 12.0f))
                                    .BorderBackgroundColor(StudioPanelColor)
                                    [
                                        SNew(SVerticalBox)
                                        + SVerticalBox::Slot()
                                        .AutoHeight()
                                        [
                                            SNew(STextBlock)
                                            .Font(MakeStudioFont("Bold", 8))
                                            .ColorAndOpacity(TertiaryTextColor)
                                            .Text(LOCTEXT("WanaWorksStudioShellSearchLabel", "STUDIO SEARCH"))
                                        ]
                                        + SVerticalBox::Slot()
                                        .AutoHeight()
                                        .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                                        [
                                            SNew(STextBlock)
                                            .Font(MakeStudioFont("Regular", 10))
                                            .ColorAndOpacity(SecondaryTextColor)
                                            .Text(LOCTEXT("WanaWorksStudioShellSearchPlaceholder", "Search WanaWorks..."))
                                        ]
                                    ]
                                ]
                            ]
                            + SHorizontalBox::Slot()
                            .FillWidth(0.33f)
                            .VAlign(VAlign_Center)
                            .Padding(0.0f, 0.0f, 18.0f, 0.0f)
                            [
                                SNew(SVerticalBox)
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                [
                                    SNew(STextBlock)
                                    .Font(MakeStudioFont("Bold", 8))
                                    .ColorAndOpacity(TertiaryTextColor)
                                    .Text(LOCTEXT("WanaWorksStudioShellWorkspaceLabel", "ACTIVE WORKSPACE"))
                                ]
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(0.0f, 6.0f, 0.0f, 0.0f)
                                [
                                    SNew(STextBlock)
                                    .Font(MakeStudioFont("Bold", 18))
                                    .ColorAndOpacity(FLinearColor::White)
                                    .ShadowColorAndOpacity(StudioShadowColor)
                                    .ShadowOffset(FVector2D(0.0f, 1.0f))
                                    .Text(ActiveWorkspaceTitle)
                                ]
                            ]
                            + SHorizontalBox::Slot()
                            .FillWidth(0.27f)
                            .VAlign(VAlign_Center)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot()
                                .FillWidth(1.0f)
                                .VAlign(VAlign_Center)
                                [
                                    MakeStudioPill(
                                        LOCTEXT("WanaWorksStudioShellCommandCenter", "COMMAND CENTER"),
                                        GetWorkspaceAccentColor(SelectedWorkspaceLabel).CopyWithNewOpacity(0.14f),
                                        FLinearColor(0.95f, 0.97f, 1.0f, 1.0f),
                                        8,
                                        FMargin(14.0f, 8.0f))
                                ]
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(12.0f, 0.0f, 0.0f, 0.0f)
                                [
                                    SNew(SBorder)
                                    .Padding(FMargin(12.0f, 8.0f))
                                    .BorderBackgroundColor(FLinearColor(0.032f, 0.040f, 0.068f, 0.98f))
                                    [
                                        SNew(STextBlock)
                                        .Font(MakeStudioFont("Bold", 9))
                                        .ColorAndOpacity(FLinearColor::White)
                                        .ShadowColorAndOpacity(StudioShadowColor)
                                        .ShadowOffset(FVector2D(0.0f, 1.0f))
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
                    .BorderBackgroundColor(StudioOutlineColor.CopyWithNewOpacity(0.38f))
                    [
                        SNew(SBorder)
                        .Padding(20.0f)
                        .BorderBackgroundColor(StudioShellRaisedColor)
                        [
                            SNew(SScrollBox)
                            + SScrollBox::Slot()
                            [
                                SNew(SVerticalBox)
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(0.0f, 0.0f, 0.0f, 18.0f)
                                [
                                    SNew(SVerticalBox)
                                    + SVerticalBox::Slot()
                                    .AutoHeight()
                                    [
                                        MakeStudioPill(
                                            bIsAIWorkspace
                                                ? LOCTEXT("WanaWorksStudioShellAIBadge", "AI STUDIO")
                                                : (bIsCharacterWorkspace
                                                    ? LOCTEXT("WanaWorksStudioShellCharacterBadge", "CHARACTER BUILDING")
                                                    : (bIsLevelDesignWorkspace
                                                        ? LOCTEXT("WanaWorksStudioShellLevelBadge", "LEVEL DESIGN")
                                                        : LOCTEXT("WanaWorksStudioShellFutureBadge", "PLATFORM WORKSPACE"))),
                                            GetWorkspaceAccentColor(SelectedWorkspaceLabel).CopyWithNewOpacity(0.16f),
                                            FLinearColor(0.95f, 0.97f, 1.0f, 1.0f),
                                            8,
                                            FMargin(12.0f, 6.0f))
                                    ]
                                    + SVerticalBox::Slot()
                                    .AutoHeight()
                                    .Padding(0.0f, 12.0f, 0.0f, 0.0f)
                                    [
                                        SNew(STextBlock)
                                        .Font(MakeStudioFont("Bold", 26))
                                        .ColorAndOpacity(FLinearColor::White)
                                        .ShadowColorAndOpacity(StudioShadowColor)
                                        .ShadowOffset(FVector2D(0.0f, 1.0f))
                                        .Text(ActiveWorkspaceTitle)
                                    ]
                                    + SVerticalBox::Slot()
                                    .AutoHeight()
                                    .Padding(0.0f, 8.0f, 0.0f, 0.0f)
                                    [
                                        SNew(STextBlock)
                                        .AutoWrapText(true)
                                        .Font(MakeStudioFont("Regular", 10))
                                        .ColorAndOpacity(SecondaryTextColor)
                                        .Text(ActiveWorkspaceSubtitle)
                                    ]
                                    + SVerticalBox::Slot()
                                    .AutoHeight()
                                    .Padding(0.0f, 18.0f, 0.0f, 0.0f)
                                    [
                                        MakeStudioDivider(1.0f, GetWorkspaceAccentColor(SelectedWorkspaceLabel).CopyWithNewOpacity(0.34f))
                                    ]
                                ]
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(0.0f, 18.0f, 0.0f, 0.0f)
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
