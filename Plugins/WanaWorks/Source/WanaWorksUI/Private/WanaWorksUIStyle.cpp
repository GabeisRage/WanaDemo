#include "WanaWorksUIStyle.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateImageBrush.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace WanaWorksUIStyle
{
namespace
{
TSharedPtr<FSlateStyleSet> WanaStyleSet;

const FName WanaStyleSetName(TEXT("WanaWorksUIStyle"));
const FName LauncherIconName(TEXT("WanaWorks.Icon.Launcher"));

const FName CardBrush(TEXT("Wana.Card"));
const FName CardProminentBrush(TEXT("Wana.CardProminent"));
const FName RailActiveBrush(TEXT("Wana.Rail.Active"));
const FName RailHoverBrush(TEXT("Wana.Rail.Hover"));
const FName ActionPrimaryBrush(TEXT("Wana.ActionPrimary"));
const FName ActionSecondaryBrush(TEXT("Wana.ActionSecondary"));
const FName StatusPillSuccessBrush(TEXT("Wana.StatusPill.Success"));
const FName StatusPillWarningBrush(TEXT("Wana.StatusPill.Warning"));
const FName StatusPillInfoBrush(TEXT("Wana.StatusPill.Info"));

void SetImageBrush(const TSharedRef<FSlateStyleSet>& StyleSet, const FName BrushName, const TCHAR* ResourceName, const FVector2D& Size)
{
    StyleSet->Set(BrushName, new FSlateImageBrush(StyleSet->RootToContentDir(ResourceName, TEXT(".png")), Size));
}

TSharedRef<SWidget> MakeIconWidget(FName IconBrushName, const FLinearColor& IconTint, float Size)
{
    if (const FSlateBrush* IconBrush = GetBrush(IconBrushName))
    {
        return SNew(SBox)
            .WidthOverride(Size)
            .HeightOverride(Size)
            [
                SNew(SImage)
                .Image(IconBrush)
                .ColorAndOpacity(IconTint)
            ];
    }

    return SNew(SBox)
        .WidthOverride(Size)
        .HeightOverride(Size);
}
}

void Register()
{
    if (WanaStyleSet.IsValid())
    {
        return;
    }

    WanaStyleSet = MakeShared<FSlateStyleSet>(WanaStyleSetName);

    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("WanaWorks"));
    const FString ResourcesRoot = Plugin.IsValid()
        ? FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"))
        : FPaths::ProjectPluginsDir() / TEXT("WanaWorks/Resources");

    WanaStyleSet->SetContentRoot(ResourcesRoot);

    const TSharedRef<FSlateStyleSet> StyleRef = WanaStyleSet.ToSharedRef();
    SetImageBrush(StyleRef, LauncherIconName, TEXT("WanaWorksLauncher"), FVector2D(40.0f, 40.0f));
    SetImageBrush(StyleRef, GetWorkspaceIconName(TEXT("AI")), TEXT("WorkspaceCharacterIntelligence"), FVector2D(22.0f, 22.0f));
    SetImageBrush(StyleRef, GetWorkspaceIconName(TEXT("Character Building")), TEXT("WorkspaceCharacterBuilding"), FVector2D(22.0f, 22.0f));
    SetImageBrush(StyleRef, GetWorkspaceIconName(TEXT("Level Design")), TEXT("WorkspaceLevelDesign"), FVector2D(22.0f, 22.0f));
    SetImageBrush(StyleRef, GetWorkspaceIconName(TEXT("Logic & Blueprints")), TEXT("WorkspaceLogicBlueprints"), FVector2D(22.0f, 22.0f));
    SetImageBrush(StyleRef, GetWorkspaceIconName(TEXT("Physics")), TEXT("WorkspacePhysics"), FVector2D(22.0f, 22.0f));
    SetImageBrush(StyleRef, GetWorkspaceIconName(TEXT("Audio")), TEXT("WorkspaceAudio"), FVector2D(22.0f, 22.0f));
    SetImageBrush(StyleRef, GetWorkspaceIconName(TEXT("UI / UX")), TEXT("WorkspaceUIUX"), FVector2D(22.0f, 22.0f));
    SetImageBrush(StyleRef, GetWorkspaceIconName(TEXT("Optimize")), TEXT("WorkspaceOptimize"), FVector2D(22.0f, 22.0f));
    SetImageBrush(StyleRef, GetWorkspaceIconName(TEXT("Build & Deploy")), TEXT("WorkspaceBuildDeploy"), FVector2D(22.0f, 22.0f));
    SetImageBrush(StyleRef, GetWorkflowIconName(TEXT("Enhance")), TEXT("WorkflowEnhance"), FVector2D(24.0f, 24.0f));
    SetImageBrush(StyleRef, GetWorkflowIconName(TEXT("Test")), TEXT("WorkflowTest"), FVector2D(24.0f, 24.0f));
    SetImageBrush(StyleRef, GetWorkflowIconName(TEXT("Analyze")), TEXT("WorkflowAnalyze"), FVector2D(24.0f, 24.0f));
    SetImageBrush(StyleRef, GetWorkflowIconName(TEXT("Build")), TEXT("WorkflowBuild"), FVector2D(24.0f, 24.0f));

    const FWanaDesignTokens& T = Tokens();

    auto MakeColorBrush = [](const FLinearColor& Color) -> FSlateColorBrush*
    {
        return new FSlateColorBrush(Color);
    };

    StyleRef->Set(CardBrush, MakeColorBrush(T.SurfaceRaised));
    StyleRef->Set(CardProminentBrush, MakeColorBrush(T.Info.CopyWithNewOpacity(0.15f)));
    StyleRef->Set(RailActiveBrush, MakeColorBrush(T.Info.CopyWithNewOpacity(0.18f)));
    StyleRef->Set(RailHoverBrush, MakeColorBrush(T.SurfaceGlass));
    StyleRef->Set(ActionPrimaryBrush, MakeColorBrush(T.Info));
    StyleRef->Set(ActionSecondaryBrush, MakeColorBrush(T.SurfaceRaised));
    StyleRef->Set(StatusPillSuccessBrush, MakeColorBrush(T.Success.CopyWithNewOpacity(0.20f)));
    StyleRef->Set(StatusPillWarningBrush, MakeColorBrush(T.Warning.CopyWithNewOpacity(0.20f)));
    StyleRef->Set(StatusPillInfoBrush, MakeColorBrush(T.Info.CopyWithNewOpacity(0.20f)));

    FSlateStyleRegistry::RegisterSlateStyle(*WanaStyleSet);
}

void Unregister()
{
    if (!WanaStyleSet.IsValid())
    {
        return;
    }

    FSlateStyleRegistry::UnRegisterSlateStyle(*WanaStyleSet);
    ensure(WanaStyleSet.IsUnique());
    WanaStyleSet.Reset();
}

FName GetStyleSetName()
{
    return WanaStyleSetName;
}

const FSlateBrush* GetBrush(FName BrushName)
{
    return WanaStyleSet.IsValid() ? WanaStyleSet->GetBrush(BrushName) : nullptr;
}

FName GetLauncherIconName()
{
    return LauncherIconName;
}

FName GetWorkspaceIconName(const FString& WorkspaceLabel)
{
    if (WorkspaceLabel.Equals(TEXT("AI"), ESearchCase::IgnoreCase))
    {
        return TEXT("WanaWorks.Icon.Workspace.CharacterIntelligence");
    }

    if (WorkspaceLabel.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase))
    {
        return TEXT("WanaWorks.Icon.Workspace.CharacterBuilding");
    }

    if (WorkspaceLabel.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
    {
        return TEXT("WanaWorks.Icon.Workspace.LevelDesign");
    }

    if (WorkspaceLabel.Equals(TEXT("Logic & Blueprints"), ESearchCase::IgnoreCase))
    {
        return TEXT("WanaWorks.Icon.Workspace.LogicBlueprints");
    }

    if (WorkspaceLabel.Equals(TEXT("Physics"), ESearchCase::IgnoreCase))
    {
        return TEXT("WanaWorks.Icon.Workspace.Physics");
    }

    if (WorkspaceLabel.Equals(TEXT("Audio"), ESearchCase::IgnoreCase))
    {
        return TEXT("WanaWorks.Icon.Workspace.Audio");
    }

    if (WorkspaceLabel.Equals(TEXT("UI / UX"), ESearchCase::IgnoreCase))
    {
        return TEXT("WanaWorks.Icon.Workspace.UIUX");
    }

    if (WorkspaceLabel.Equals(TEXT("Optimize"), ESearchCase::IgnoreCase))
    {
        return TEXT("WanaWorks.Icon.Workspace.Optimize");
    }

    if (WorkspaceLabel.Equals(TEXT("Build & Deploy"), ESearchCase::IgnoreCase))
    {
        return TEXT("WanaWorks.Icon.Workspace.BuildDeploy");
    }

    return TEXT("WanaWorks.Icon.Workspace.Future");
}

FName GetWorkflowIconName(const FString& WorkflowLabel)
{
    if (WorkflowLabel.Equals(TEXT("Enhance"), ESearchCase::IgnoreCase))
    {
        return TEXT("WanaWorks.Icon.Workflow.Enhance");
    }

    if (WorkflowLabel.Equals(TEXT("Test"), ESearchCase::IgnoreCase))
    {
        return TEXT("WanaWorks.Icon.Workflow.Test");
    }

    if (WorkflowLabel.Equals(TEXT("Analyze"), ESearchCase::IgnoreCase))
    {
        return TEXT("WanaWorks.Icon.Workflow.Analyze");
    }

    if (WorkflowLabel.Equals(TEXT("Build"), ESearchCase::IgnoreCase))
    {
        return TEXT("WanaWorks.Icon.Workflow.Build");
    }

    return TEXT("WanaWorks.Icon.Workflow.Enhance");
}

const FWanaDesignTokens& Tokens()
{
    static const FWanaDesignTokens DesignTokens;
    return DesignTokens;
}

FSlateFontInfo WanaFont(const ANSICHAR* Typeface, int32 Size)
{
    return FCoreStyle::GetDefaultFontStyle(Typeface, Size);
}

FSlateFontInfo HeadingFont()
{
    return WanaFont("Bold", 18);
}

FSlateFontInfo SubheadingFont()
{
    return WanaFont("Bold", 12);
}

FSlateFontInfo LabelFont()
{
    return WanaFont("Regular", 10);
}

FSlateFontInfo CaptionFont()
{
    return WanaFont("Regular", 8);
}

FSlateFontInfo MonoFont()
{
    return WanaFont("Mono", 9);
}

FName CardBrushName() { return CardBrush; }
FName CardProminentBrushName() { return CardProminentBrush; }
FName RailActiveBrushName() { return RailActiveBrush; }
FName RailHoverBrushName() { return RailHoverBrush; }
FName ActionPrimaryBrushName() { return ActionPrimaryBrush; }
FName ActionSecondaryBrushName() { return ActionSecondaryBrush; }
FName StatusPillSuccessBrushName() { return StatusPillSuccessBrush; }
FName StatusPillWarningBrushName() { return StatusPillWarningBrush; }
FName StatusPillInfoBrushName() { return StatusPillInfoBrush; }

TSharedRef<SWidget> WanaStatusPill(
    const FText& Label,
    const FLinearColor& AccentColor,
    bool bStrong,
    int32 FontSize,
    const FMargin& Padding)
{
    const FWanaDesignTokens& T = Tokens();

    return SNew(SBorder)
        .Padding(Padding)
        .BorderBackgroundColor(bStrong ? AccentColor.CopyWithNewOpacity(0.34f) : AccentColor.CopyWithNewOpacity(0.16f))
        [
            SNew(STextBlock)
            .Text(Label)
            .Font(WanaFont("Bold", FontSize))
            .ColorAndOpacity(bStrong ? T.TextPrimary : FLinearColor(0.88f, 0.93f, 1.0f, 1.0f))
            .ShadowColorAndOpacity(T.Shadow)
            .ShadowOffset(FVector2D(0.0f, 1.0f))
        ];
}

TSharedRef<SWidget> WanaSectionHeader(
    const FText& Eyebrow,
    const FText& Title,
    const FText& Description,
    const FLinearColor& AccentColor,
    const TSharedPtr<SWidget>& TrailingContent)
{
    const FWanaDesignTokens& T = Tokens();

    return SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                WanaStatusPill(Eyebrow, AccentColor, false, 8, FMargin(12.0f, 6.0f))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 14.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .Text(Title)
                .Font(WanaFont("Bold", 20))
                .ColorAndOpacity(T.TextPrimary)
                .ShadowColorAndOpacity(T.Shadow)
                .ShadowOffset(FVector2D(0.0f, 1.0f))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 9.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .Visibility(Description.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
                .Text(Description)
                .AutoWrapText(true)
                .Font(WanaFont("Regular", 10))
                .ColorAndOpacity(T.TextSecondary)
            ]
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Top)
        .Padding(16.0f, 0.0f, 0.0f, 0.0f)
        [
            TrailingContent.IsValid() ? TrailingContent.ToSharedRef() : SNullWidget::NullWidget
        ];
}

TSharedRef<SWidget> WanaCard(
    const FText& Eyebrow,
    const FText& Title,
    const FLinearColor& AccentColor,
    const TSharedRef<SWidget>& Content,
    const TSharedPtr<SWidget>& TrailingContent,
    bool bProminent)
{
    const FWanaDesignTokens& T = Tokens();
    const float Padding = bProminent ? 28.0f : T.CardPadding;

    return SNew(SBorder)
        .Padding(0.0f)
        .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(bProminent ? 0.20f : 0.12f))
        [
            SNew(SOverlay)
            + SOverlay::Slot()
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Fill)
            .Padding(FMargin(4.0f, 7.0f, 0.0f, 0.0f))
            [
                SNew(SBorder)
                .Padding(0.0f)
                .BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, bProminent ? 0.34f : 0.24f))
            ]
            + SOverlay::Slot()
            .HAlign(HAlign_Fill)
            .VAlign(VAlign_Top)
            [
                SNew(SBorder)
                .Padding(0.0f)
                .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(bProminent ? 0.28f : 0.16f))
                [
                    SNew(SBox)
                    .HeightOverride(bProminent ? 4.0f : 2.0f)
                ]
            ]
            + SOverlay::Slot()
            .HAlign(HAlign_Right)
            .VAlign(VAlign_Top)
            [
                SNew(SBorder)
                .Padding(0.0f)
                .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(bProminent ? 0.075f : 0.050f))
                [
                    SNew(SBox)
                    .WidthOverride(bProminent ? 220.0f : 136.0f)
                    .HeightOverride(bProminent ? 120.0f : 76.0f)
                ]
            ]
            + SOverlay::Slot()
            [
                SNew(SBorder)
                .Padding(Padding)
                .BorderBackgroundColor((bProminent ? T.SurfaceRaised : T.Surface).CopyWithNewOpacity(bProminent ? 0.985f : 0.945f))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        WanaSectionHeader(Eyebrow, Title, FText::GetEmpty(), AccentColor, TrailingContent)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, bProminent ? 22.0f : 18.0f, 0.0f, 0.0f)
                    [
                        Content
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> WanaMetricBlock(
    const FText& Label,
    const FText& Value,
    const FLinearColor& AccentColor,
    bool bProminent,
    float ValueWrapWidth)
{
    const FWanaDesignTokens& T = Tokens();
    const bool bHasLabel = !Label.IsEmpty();

    return SNew(SBorder)
        .Padding(0.0f)
        .BorderBackgroundColor(bProminent ? AccentColor.CopyWithNewOpacity(0.17f) : T.SurfaceGlass.CopyWithNewOpacity(0.52f))
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SBorder)
                .Padding(0.0f)
                .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(bProminent ? 0.72f : 0.36f))
                [
                    SNew(SBox)
                    .WidthOverride(bProminent ? 4.0f : 2.0f)
                ]
            ]
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SNew(SBorder)
                .Padding(FMargin(15.0f, bProminent ? 13.0f : 11.0f))
                .BorderBackgroundColor(bProminent ? AccentColor.CopyWithNewOpacity(0.08f) : T.Surface.CopyWithNewOpacity(0.26f))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Visibility(bHasLabel ? EVisibility::Visible : EVisibility::Collapsed)
                        .Text(Label)
                        .Font(WanaFont("Bold", 8))
                        .ColorAndOpacity(T.TextMuted.CopyWithNewOpacity(0.96f))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(0.0f, bHasLabel ? 7.0f : 0.0f, 0.0f, 0.0f)
                    [
                        SNew(SBox)
                        .MaxDesiredWidth(ValueWrapWidth)
                        [
                            SNew(STextBlock)
                            .Text(Value)
                            .AutoWrapText(true)
                            .Font(WanaFont(bProminent ? "Bold" : "Regular", bProminent ? 12 : 10))
                            .ColorAndOpacity(T.TextPrimary)
                            .ShadowColorAndOpacity(T.Shadow)
                            .ShadowOffset(FVector2D(0.0f, 1.0f))
                        ]
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> WanaHeroStage(
    const FLinearColor& AccentColor,
    const TSharedRef<SWidget>& StageContent,
    const TSharedPtr<SWidget>& TopOverlay,
    const TSharedPtr<SWidget>& BottomOverlay)
{
    return SNew(SBorder)
        .Padding(0.0f)
        .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.30f))
        [
            SNew(SBorder)
            .Padding(13.0f)
            .BorderBackgroundColor(FLinearColor(0.000f, 0.002f, 0.012f, 1.0f))
            [
                SNew(SOverlay)
                + SOverlay::Slot()
                .HAlign(HAlign_Fill)
                .VAlign(VAlign_Fill)
                .Padding(FMargin(8.0f, 12.0f, 0.0f, 0.0f))
                [
                    SNew(SBorder)
                    .Visibility(EVisibility::HitTestInvisible)
                    .Padding(0.0f)
                    .BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.42f))
                ]
                + SOverlay::Slot()
                [
                    SNew(SBorder)
                    .Padding(0.0f)
                    .BorderBackgroundColor(FLinearColor(0.001f, 0.006f, 0.024f, 1.0f))
                    [
                        StageContent
                    ]
                ]
                + SOverlay::Slot()
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Fill)
                [
                    SNew(SBorder)
                    .Visibility(EVisibility::HitTestInvisible)
                    .Padding(0.0f)
                    .BorderBackgroundColor(FLinearColor(0.16f, 0.34f, 0.76f, 0.045f))
                    [
                        SNew(SBox)
                        .WidthOverride(420.0f)
                    ]
                ]
                + SOverlay::Slot()
                .HAlign(HAlign_Fill)
                .VAlign(VAlign_Top)
                [
                    SNew(SBorder)
                    .Visibility(EVisibility::HitTestInvisible)
                    .Padding(0.0f)
                    .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.14f))
                    [
                        SNew(SBox)
                        .HeightOverride(190.0f)
                    ]
                ]
                + SOverlay::Slot()
                .HAlign(HAlign_Right)
                .VAlign(VAlign_Fill)
                [
                    SNew(SBorder)
                    .Visibility(EVisibility::HitTestInvisible)
                    .Padding(0.0f)
                    .BorderBackgroundColor(FLinearColor(0.24f, 0.52f, 1.0f, 0.034f))
                    [
                        SNew(SBox)
                        .WidthOverride(130.0f)
                    ]
                ]
                + SOverlay::Slot()
                .HAlign(HAlign_Left)
                .VAlign(VAlign_Fill)
                [
                    SNew(SBorder)
                    .Visibility(EVisibility::HitTestInvisible)
                    .Padding(0.0f)
                    .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.042f))
                    [
                        SNew(SBox)
                        .WidthOverride(112.0f)
                    ]
                ]
                + SOverlay::Slot()
                .HAlign(HAlign_Fill)
                .VAlign(VAlign_Bottom)
                [
                    SNew(SBorder)
                    .Visibility(EVisibility::HitTestInvisible)
                    .Padding(0.0f)
                    .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.115f))
                    [
                        SNew(SBox)
                        .HeightOverride(152.0f)
                    ]
                ]
                + SOverlay::Slot()
                .HAlign(HAlign_Fill)
                .VAlign(VAlign_Bottom)
                [
                    SNew(SBorder)
                    .Visibility(EVisibility::HitTestInvisible)
                    .Padding(0.0f)
                    .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.54f))
                    [
                        SNew(SBox)
                        .HeightOverride(4.0f)
                    ]
                ]
                + SOverlay::Slot()
                .HAlign(HAlign_Fill)
                .VAlign(VAlign_Top)
                .Padding(18.0f)
                [
                    TopOverlay.IsValid() ? TopOverlay.ToSharedRef() : SNullWidget::NullWidget
                ]
                + SOverlay::Slot()
                .HAlign(HAlign_Fill)
                .VAlign(VAlign_Bottom)
                .Padding(18.0f)
                [
                    BottomOverlay.IsValid() ? BottomOverlay.ToSharedRef() : SNullWidget::NullWidget
                ]
            ]
        ];
}

TSharedRef<SWidget> WanaActionTile(
    const FString& StepNumber,
    const FText& Title,
    const FText& Description,
    FName IconBrushName,
    const FLinearColor& AccentColor,
    TFunction<void(void)> OnPressed)
{
    const FWanaDesignTokens& T = Tokens();

    return SNew(SButton)
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
            .Padding(0.0f)
            .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.19f))
            [
                SNew(SOverlay)
                + SOverlay::Slot()
                .HAlign(HAlign_Fill)
                .VAlign(VAlign_Fill)
                .Padding(FMargin(3.0f, 5.0f, 0.0f, 0.0f))
                [
                    SNew(SBorder)
                    .Padding(0.0f)
                    .BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.30f))
                ]
                + SOverlay::Slot()
                .HAlign(HAlign_Right)
                .VAlign(VAlign_Top)
                [
                    SNew(SBorder)
                    .Padding(0.0f)
                    .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.095f))
                    [
                        SNew(SBox)
                        .WidthOverride(86.0f)
                        .HeightOverride(62.0f)
                    ]
                ]
                + SOverlay::Slot()
                [
                    SNew(SBorder)
                    .Padding(18.0f)
                    .BorderBackgroundColor(T.SurfaceRaised.CopyWithNewOpacity(0.965f))
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 0.0f, 0.0f, 14.0f)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            .Padding(0.0f, 0.0f, 14.0f, 0.0f)
                            [
                                SNew(SBorder)
                                .Padding(9.0f)
                                .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.24f))
                                [
                                    MakeIconWidget(IconBrushName, T.TextPrimary, 22.0f)
                                ]
                            ]
                            + SHorizontalBox::Slot()
                            .FillWidth(1.0f)
                            .VAlign(VAlign_Center)
                            [
                                SNew(STextBlock)
                                .Text(Title)
                                .Font(WanaFont("Bold", 15))
                                .ColorAndOpacity(T.TextPrimary)
                                .ShadowColorAndOpacity(T.Shadow)
                                .ShadowOffset(FVector2D(0.0f, 1.0f))
                            ]
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .VAlign(VAlign_Center)
                            .Padding(12.0f, 0.0f, 0.0f, 0.0f)
                            [
                                WanaStatusPill(FText::FromString(StepNumber), AccentColor, true, 9, FMargin(11.0f, 6.0f))
                            ]
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 0.0f, 0.0f, 18.0f)
                        [
                            SNew(STextBlock)
                            .Text(Description)
                            .AutoWrapText(true)
                            .Font(WanaFont("Regular", 9))
                            .ColorAndOpacity(T.TextSecondary.CopyWithNewOpacity(0.92f))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            WanaStatusPill(FText::FromString(FString::Printf(TEXT("Run %s"), *Title.ToString())), AccentColor, false, 8, FMargin(12.0f, 7.0f))
                        ]
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> WanaWorkspaceRailItem(
    const FText& Label,
    const FText& Subtitle,
    TFunction<FText(void)> GetStateLabel,
    FName IconBrushName,
    const FLinearColor& AccentColor,
    bool bAvailable,
    TFunction<bool(void)> IsActive,
    TFunction<void(void)> OnClicked)
{
    const FWanaDesignTokens& T = Tokens();

    return SNew(SButton)
        .ButtonStyle(FCoreStyle::Get(), "NoBorder")
        .ContentPadding(FMargin(0.0f))
        .OnClicked_Lambda([OnClicked]()
        {
            if (OnClicked)
            {
                OnClicked();
            }

            return FReply::Handled();
        })
        [
            SNew(SBorder)
            .Padding(FMargin(13.0f, 12.0f))
            .BorderBackgroundColor_Lambda([IsActive, bAvailable, AccentColor, T]()
            {
                const bool bActive = IsActive ? IsActive() : false;
                if (bActive)
                {
                    return AccentColor.CopyWithNewOpacity(0.34f);
                }

                return bAvailable
                    ? T.SurfaceGlass.CopyWithNewOpacity(0.46f)
                    : FLinearColor(0.018f, 0.026f, 0.058f, 0.70f);
            })
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Top)
                    .Padding(0.0f, 1.0f, 12.0f, 0.0f)
                    [
                        SNew(SBorder)
                        .Padding(7.0f)
                        .BorderBackgroundColor_Lambda([IsActive, bAvailable, AccentColor]()
                        {
                            const bool bActive = IsActive ? IsActive() : false;
                            return bActive
                                ? AccentColor.CopyWithNewOpacity(0.36f)
                                : (bAvailable ? AccentColor.CopyWithNewOpacity(0.16f) : FLinearColor(0.20f, 0.23f, 0.32f, 0.46f));
                        })
                        [
                            MakeIconWidget(IconBrushName, bAvailable ? T.TextPrimary : T.TextMuted, 19.0f)
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
                            .Text(Label)
                            .AutoWrapText(true)
                            .WrapTextAt(174.0f)
                            .Font(WanaFont("Bold", 11))
                            .ColorAndOpacity_Lambda([IsActive, bAvailable, T]()
                            {
                                const bool bActive = IsActive ? IsActive() : false;
                                return bActive || bAvailable ? T.TextPrimary : T.TextMuted;
                            })
                            .ShadowColorAndOpacity(T.Shadow)
                            .ShadowOffset(FVector2D(0.0f, 1.0f))
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 5.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Text(Subtitle)
                            .AutoWrapText(true)
                            .WrapTextAt(176.0f)
                            .Font(WanaFont("Regular", 8))
                            .ColorAndOpacity(bAvailable ? T.TextSecondary.CopyWithNewOpacity(0.88f) : T.TextMuted.CopyWithNewOpacity(0.74f))
                        ]
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(45.0f, 8.0f, 0.0f, 0.0f)
                .HAlign(HAlign_Left)
                [
                    SNew(SBorder)
                    .Padding(FMargin(7.0f, 3.0f))
                    .BorderBackgroundColor_Lambda([IsActive, bAvailable, AccentColor]()
                    {
                        const bool bActive = IsActive ? IsActive() : false;
                        if (bActive)
                        {
                            return AccentColor.CopyWithNewOpacity(0.28f);
                        }

                        return bAvailable
                            ? AccentColor.CopyWithNewOpacity(0.13f)
                            : FLinearColor(0.20f, 0.23f, 0.32f, 0.28f);
                    })
                    [
                        SNew(STextBlock)
                        .Text_Lambda([GetStateLabel]()
                        {
                            return GetStateLabel ? GetStateLabel() : FText::GetEmpty();
                        })
                        .Font(WanaFont("Bold", 7))
                        .ColorAndOpacity(bAvailable ? FLinearColor(0.84f, 0.90f, 1.0f, 1.0f) : T.TextMuted.CopyWithNewOpacity(0.82f))
                    ]
                ]
            ]
        ];
}

TSharedRef<SWidget> WanaToast(
    const FText& Label,
    const FText& Message,
    const FLinearColor& AccentColor)
{
    const FWanaDesignTokens& T = Tokens();

    return SNew(SBorder)
        .Padding(FMargin(16.0f, 12.0f))
        .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.11f))
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(Label)
                .Font(WanaFont("Bold", 8))
                .ColorAndOpacity(AccentColor)
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 5.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .Text(Message)
                .AutoWrapText(true)
                .Font(WanaFont("Regular", 9))
                .ColorAndOpacity(T.TextSecondary)
            ]
        ];
}
}
