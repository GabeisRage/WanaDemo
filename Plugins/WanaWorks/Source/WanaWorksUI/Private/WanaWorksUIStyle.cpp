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
const FName AppBackgroundBrush(TEXT("Wana.Surface.AppBackground"));
const FName NavigationBrush(TEXT("Wana.Surface.Navigation"));
const FName TopBarBrush(TEXT("Wana.Surface.TopBar"));
const FName WorkspaceBrush(TEXT("Wana.Surface.Workspace"));
const FName PanelBrush(TEXT("Wana.Surface.Panel"));
const FName CardHoverBrush(TEXT("Wana.Surface.CardHover"));
const FName InputBrush(TEXT("Wana.Surface.Input"));
const FName DividerBrush(TEXT("Wana.Surface.Divider"));

const FName PrimaryButton(TEXT("Wana.Button.Primary"));
const FName SecondaryButton(TEXT("Wana.Button.Secondary"));
const FName GhostButton(TEXT("Wana.Button.Ghost"));
const FName DangerButton(TEXT("Wana.Button.Danger"));
const FName EnhanceButton(TEXT("Wana.Button.Enhance"));
const FName TestButton(TEXT("Wana.Button.Test"));
const FName AnalyzeButton(TEXT("Wana.Button.Analyze"));
const FName BuildButton(TEXT("Wana.Button.Build"));
const FName InputTextBox(TEXT("Wana.Input.Text"));
const FName InputMultilineTextBox(TEXT("Wana.Input.Multiline"));
const FName ComboBox(TEXT("Wana.Input.ComboBox"));
const FName ComboRow(TEXT("Wana.Input.ComboRow"));
const FName CheckBox(TEXT("Wana.Input.CheckBox"));
const FName ScrollBar(TEXT("Wana.Input.ScrollBar"));

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

FButtonStyle MakeButtonStyle(
    const FLinearColor& Normal,
    const FLinearColor& Hovered,
    const FLinearColor& Pressed,
    const FLinearColor& Disabled,
    const FLinearColor& Foreground)
{
    return FButtonStyle()
        .SetNormal(FSlateColorBrush(Normal))
        .SetHovered(FSlateColorBrush(Hovered))
        .SetPressed(FSlateColorBrush(Pressed))
        .SetDisabled(FSlateColorBrush(Disabled))
        .SetNormalForeground(Foreground)
        .SetHoveredForeground(Foreground)
        .SetPressedForeground(Foreground)
        .SetDisabledForeground(Tokens().TextDisabled)
        .SetNormalPadding(FMargin(0.0f))
        .SetPressedPadding(FMargin(0.0f));
}

FScrollBarStyle MakeScrollBarStyle()
{
    const FWanaDesignTokens& T = Tokens();
    return FScrollBarStyle()
        .SetHorizontalBackgroundImage(FSlateColorBrush(T.Input))
        .SetVerticalBackgroundImage(FSlateColorBrush(T.Input))
        .SetHorizontalTopSlotImage(FSlateColorBrush(T.Input))
        .SetVerticalTopSlotImage(FSlateColorBrush(T.Input))
        .SetHorizontalBottomSlotImage(FSlateColorBrush(T.Input))
        .SetVerticalBottomSlotImage(FSlateColorBrush(T.Input))
        .SetNormalThumbImage(FSlateColorBrush(T.BorderSubtle))
        .SetHoveredThumbImage(FSlateColorBrush(T.Cyan.CopyWithNewOpacity(0.90f)))
        .SetDraggedThumbImage(FSlateColorBrush(T.Blue))
        .SetThickness(7.0f);
}

FEditableTextBoxStyle MakeInputTextBoxStyle(bool bMultiline)
{
    const FWanaDesignTokens& T = Tokens();
    return FEditableTextBoxStyle()
        .SetBackgroundImageNormal(FSlateColorBrush(T.Input))
        .SetBackgroundImageHovered(FSlateColorBrush(T.CardHover.CopyWithNewOpacity(0.90f)))
        .SetBackgroundImageFocused(FSlateColorBrush(T.CardRaised))
        .SetBackgroundImageReadOnly(FSlateColorBrush(T.Panel))
        .SetForegroundColor(T.TextPrimary)
        .SetFocusedForegroundColor(T.TextPrimary)
        .SetReadOnlyForegroundColor(T.TextSecondary)
        .SetBackgroundColor(T.Input)
        .SetPadding(FMargin(11.0f, bMultiline ? 10.0f : 7.0f))
        .SetScrollBarStyle(MakeScrollBarStyle());
}

FComboBoxStyle MakeComboBoxStyle()
{
    const FWanaDesignTokens& T = Tokens();
    const FButtonStyle Button = MakeButtonStyle(
        T.Input,
        T.CardHover,
        T.CardRaised,
        T.Panel,
        T.TextPrimary);
    const FComboButtonStyle ComboButtonStyle = FComboButtonStyle()
        .SetButtonStyle(Button)
        .SetDownArrowImage(FSlateColorBrush(T.Cyan))
        .SetMenuBorderBrush(FSlateColorBrush(T.Panel))
        .SetMenuBorderPadding(FMargin(1.0f))
        .SetContentPadding(FMargin(11.0f, 7.0f))
        .SetDownArrowPadding(FMargin(9.0f, 0.0f, 4.0f, 0.0f))
        .SetDownArrowAlignment(VAlign_Center);

    return FComboBoxStyle()
        .SetComboButtonStyle(ComboButtonStyle)
        .SetContentPadding(FMargin(0.0f))
        .SetMenuRowPadding(FMargin(4.0f));
}

FTableRowStyle MakeComboRowStyle()
{
    const FWanaDesignTokens& T = Tokens();
    return FTableRowStyle()
        .SetSelectorFocusedBrush(FSlateColorBrush(T.Cyan.CopyWithNewOpacity(0.24f)))
        .SetActiveHoveredBrush(FSlateColorBrush(T.Cyan.CopyWithNewOpacity(0.28f)))
        .SetActiveBrush(FSlateColorBrush(T.Blue.CopyWithNewOpacity(0.22f)))
        .SetInactiveHoveredBrush(FSlateColorBrush(T.CardHover))
        .SetInactiveBrush(FSlateColorBrush(T.Panel))
        .SetEvenRowBackgroundHoveredBrush(FSlateColorBrush(T.CardHover))
        .SetEvenRowBackgroundBrush(FSlateColorBrush(T.Panel))
        .SetOddRowBackgroundHoveredBrush(FSlateColorBrush(T.CardHover))
        .SetOddRowBackgroundBrush(FSlateColorBrush(T.Input))
        .SetTextColor(T.TextPrimary)
        .SetSelectedTextColor(T.TextPrimary);
}

FCheckBoxStyle MakeCheckBoxStyle()
{
    const FWanaDesignTokens& T = Tokens();
    return FCheckBoxStyle()
        .SetCheckBoxType(ESlateCheckBoxType::CheckBox)
        .SetUncheckedImage(FSlateColorBrush(T.Input))
        .SetUncheckedHoveredImage(FSlateColorBrush(T.CardHover))
        .SetUncheckedPressedImage(FSlateColorBrush(T.CardRaised))
        .SetCheckedImage(FSlateColorBrush(T.Cyan.CopyWithNewOpacity(0.82f)))
        .SetCheckedHoveredImage(FSlateColorBrush(T.Cyan))
        .SetCheckedPressedImage(FSlateColorBrush(T.Blue))
        .SetPadding(FMargin(3.0f));
}

template <typename StyleType>
const StyleType& GetWidgetStyleOrFallback(const FName StyleName, const StyleType& Fallback)
{
    return WanaStyleSet.IsValid()
        ? WanaStyleSet->GetWidgetStyle<StyleType>(StyleName)
        : Fallback;
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
    SetImageBrush(StyleRef, GetWorkspaceIconName(TEXT("Project Blueprint")), TEXT("WorkspaceLogicBlueprints"), FVector2D(22.0f, 22.0f));
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
    StyleRef->Set(CardProminentBrush, MakeColorBrush(T.CardRaised));
    StyleRef->Set(RailActiveBrush, MakeColorBrush(T.Cyan.CopyWithNewOpacity(0.16f)));
    StyleRef->Set(RailHoverBrush, MakeColorBrush(T.CardHover.CopyWithNewOpacity(0.72f)));
    StyleRef->Set(ActionPrimaryBrush, MakeColorBrush(T.Blue.CopyWithNewOpacity(0.22f)));
    StyleRef->Set(ActionSecondaryBrush, MakeColorBrush(T.Card));
    StyleRef->Set(StatusPillSuccessBrush, MakeColorBrush(T.Emerald.CopyWithNewOpacity(0.20f)));
    StyleRef->Set(StatusPillWarningBrush, MakeColorBrush(T.Amber.CopyWithNewOpacity(0.20f)));
    StyleRef->Set(StatusPillInfoBrush, MakeColorBrush(T.Blue.CopyWithNewOpacity(0.20f)));
    StyleRef->Set(AppBackgroundBrush, MakeColorBrush(T.BackgroundDeep));
    StyleRef->Set(NavigationBrush, MakeColorBrush(T.Navigation));
    StyleRef->Set(TopBarBrush, MakeColorBrush(T.TopBar));
    StyleRef->Set(WorkspaceBrush, MakeColorBrush(T.Workspace));
    StyleRef->Set(PanelBrush, MakeColorBrush(T.Panel));
    StyleRef->Set(CardHoverBrush, MakeColorBrush(T.CardHover));
    StyleRef->Set(InputBrush, MakeColorBrush(T.Input));
    StyleRef->Set(DividerBrush, MakeColorBrush(T.Divider));

    StyleRef->Set(PrimaryButton, MakeButtonStyle(T.Card, T.CardHover, T.CardRaised, T.Panel, T.TextPrimary));
    StyleRef->Set(SecondaryButton, MakeButtonStyle(T.Panel, T.Card, T.CardRaised, T.BackgroundMain, T.TextPrimary));
    StyleRef->Set(GhostButton, MakeButtonStyle(FLinearColor::Transparent, T.CardHover.CopyWithNewOpacity(0.58f), T.CardRaised.CopyWithNewOpacity(0.72f), FLinearColor::Transparent, T.TextSecondary));
    StyleRef->Set(DangerButton, MakeButtonStyle(T.Red.CopyWithNewOpacity(0.14f), T.Red.CopyWithNewOpacity(0.24f), T.Red.CopyWithNewOpacity(0.34f), T.Panel, T.TextPrimary));
    StyleRef->Set(EnhanceButton, MakeButtonStyle(T.Blue.CopyWithNewOpacity(0.15f), T.Blue.CopyWithNewOpacity(0.25f), T.Blue.CopyWithNewOpacity(0.34f), T.Panel, T.TextPrimary));
    StyleRef->Set(TestButton, MakeButtonStyle(T.Cyan.CopyWithNewOpacity(0.13f), T.Cyan.CopyWithNewOpacity(0.22f), T.Cyan.CopyWithNewOpacity(0.31f), T.Panel, T.TextPrimary));
    StyleRef->Set(AnalyzeButton, MakeButtonStyle(T.Violet.CopyWithNewOpacity(0.15f), T.Violet.CopyWithNewOpacity(0.24f), T.Violet.CopyWithNewOpacity(0.34f), T.Panel, T.TextPrimary));
    StyleRef->Set(BuildButton, MakeButtonStyle(T.Blue.CopyWithNewOpacity(0.18f), T.Blue.CopyWithNewOpacity(0.28f), T.Blue.CopyWithNewOpacity(0.38f), T.Panel, T.TextPrimary));
    StyleRef->Set(InputTextBox, MakeInputTextBoxStyle(false));
    StyleRef->Set(InputMultilineTextBox, MakeInputTextBoxStyle(true));
    StyleRef->Set(ComboBox, MakeComboBoxStyle());
    StyleRef->Set(ComboRow, MakeComboRowStyle());
    StyleRef->Set(CheckBox, MakeCheckBoxStyle());
    StyleRef->Set(ScrollBar, MakeScrollBarStyle());

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
    if (WorkspaceLabel.Equals(TEXT("Project Blueprint"), ESearchCase::IgnoreCase))
    {
        return TEXT("WanaWorks.Icon.Workspace.ProjectBlueprint");
    }

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

FSlateFontInfo WorkspaceTitleFont()
{
    return WanaFont("Bold", 22);
}

FSlateFontInfo WorkspaceSubtitleFont()
{
    return WanaFont("Regular", 11);
}

FSlateFontInfo SectionHeadingFont()
{
    return WanaFont("Bold", 16);
}

FSlateFontInfo CardHeadingFont()
{
    return WanaFont("Bold", 13);
}

FSlateFontInfo MetricFont()
{
    return WanaFont("Bold", 14);
}

FSlateFontInfo BodyFont()
{
    return WanaFont("Regular", 10);
}

FSlateFontInfo MetadataFont()
{
    return WanaFont("Bold", 8);
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
FName AppBackgroundBrushName() { return AppBackgroundBrush; }
FName NavigationBrushName() { return NavigationBrush; }
FName TopBarBrushName() { return TopBarBrush; }
FName WorkspaceBrushName() { return WorkspaceBrush; }
FName PanelBrushName() { return PanelBrush; }
FName CardHoverBrushName() { return CardHoverBrush; }
FName InputBrushName() { return InputBrush; }
FName DividerBrushName() { return DividerBrush; }

const FButtonStyle& PrimaryButtonStyle() { return GetWidgetStyleOrFallback(PrimaryButton, FButtonStyle::GetDefault()); }
const FButtonStyle& SecondaryButtonStyle() { return GetWidgetStyleOrFallback(SecondaryButton, FButtonStyle::GetDefault()); }
const FButtonStyle& GhostButtonStyle() { return GetWidgetStyleOrFallback(GhostButton, FButtonStyle::GetDefault()); }
const FButtonStyle& DangerButtonStyle() { return GetWidgetStyleOrFallback(DangerButton, FButtonStyle::GetDefault()); }
const FButtonStyle& EnhanceButtonStyle() { return GetWidgetStyleOrFallback(EnhanceButton, FButtonStyle::GetDefault()); }
const FButtonStyle& TestButtonStyle() { return GetWidgetStyleOrFallback(TestButton, FButtonStyle::GetDefault()); }
const FButtonStyle& AnalyzeButtonStyle() { return GetWidgetStyleOrFallback(AnalyzeButton, FButtonStyle::GetDefault()); }
const FButtonStyle& BuildButtonStyle() { return GetWidgetStyleOrFallback(BuildButton, FButtonStyle::GetDefault()); }
const FEditableTextBoxStyle& InputTextBoxStyle() { return GetWidgetStyleOrFallback(InputTextBox, FEditableTextBoxStyle::GetDefault()); }
const FEditableTextBoxStyle& InputMultilineTextBoxStyle() { return GetWidgetStyleOrFallback(InputMultilineTextBox, FEditableTextBoxStyle::GetDefault()); }
const FComboBoxStyle& ComboBoxStyle() { return GetWidgetStyleOrFallback(ComboBox, FComboBoxStyle::GetDefault()); }
const FTableRowStyle& ComboRowStyle() { return GetWidgetStyleOrFallback(ComboRow, FTableRowStyle::GetDefault()); }
const FCheckBoxStyle& CheckBoxStyle() { return GetWidgetStyleOrFallback(CheckBox, FCheckBoxStyle::GetDefault()); }
const FScrollBarStyle& ScrollBarStyle() { return GetWidgetStyleOrFallback(ScrollBar, FScrollBarStyle::GetDefault()); }

const FButtonStyle& WorkflowButtonStyle(const FString& WorkflowLabel)
{
    if (WorkflowLabel.Equals(TEXT("Enhance"), ESearchCase::IgnoreCase))
    {
        return EnhanceButtonStyle();
    }

    if (WorkflowLabel.Equals(TEXT("Test"), ESearchCase::IgnoreCase))
    {
        return TestButtonStyle();
    }

    if (WorkflowLabel.Equals(TEXT("Analyze"), ESearchCase::IgnoreCase))
    {
        return AnalyzeButtonStyle();
    }

    if (WorkflowLabel.Equals(TEXT("Build"), ESearchCase::IgnoreCase))
    {
        return BuildButtonStyle();
    }

    return PrimaryButtonStyle();
}

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
            .ColorAndOpacity(bStrong ? T.TextPrimary : T.TextSecondary)
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
                .Font(CardHeadingFont())
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
    const float Padding = bProminent ? 24.0f : T.CardPadding;

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
                    .Padding(0.0f, bProminent ? 18.0f : 14.0f, 0.0f, 0.0f)
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
        .Clipping(EWidgetClipping::ClipToBounds)
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
                            .WrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping)
                            .Font(bProminent ? MetricFont() : BodyFont())
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
    const FWanaDesignTokens& T = Tokens();

    return SNew(SBorder)
        .Padding(0.0f)
        .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.30f))
        [
            SNew(SBorder)
            .Padding(13.0f)
            .BorderBackgroundColor(T.BackgroundDeep)
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
                    .BorderBackgroundColor(T.BackgroundMain)
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
                    .BorderBackgroundColor(T.Blue.CopyWithNewOpacity(0.045f))
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
                    .BorderBackgroundColor(T.Cyan.CopyWithNewOpacity(0.034f))
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
        .ButtonStyle(&WorkflowButtonStyle(Title.ToString()))
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

TSharedRef<SWidget> WanaWorkflowCommand(
    const FString& StepNumber,
    const FText& Title,
    const FText& Description,
    FName IconBrushName,
    const FLinearColor& AccentColor,
    TFunction<void(void)> OnPressed)
{
    const FWanaDesignTokens& T = Tokens();

    return SNew(SButton)
        .ButtonStyle(&WorkflowButtonStyle(Title.ToString()))
        .ContentPadding(FMargin(0.0f))
        .ToolTipText(Description)
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
            .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.36f))
            [
                SNew(SBorder)
                .Padding(FMargin(12.0f, 9.0f))
                .BorderBackgroundColor(T.SurfaceRaised.CopyWithNewOpacity(0.98f))
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(0.0f, 0.0f, 10.0f, 0.0f)
                    [
                        SNew(SBorder)
                        .Padding(6.0f)
                        .BorderBackgroundColor(AccentColor.CopyWithNewOpacity(0.22f))
                        [
                            MakeIconWidget(IconBrushName, AccentColor, 17.0f)
                        ]
                    ]
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .VAlign(VAlign_Center)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SNew(STextBlock)
                            .Text(Title)
                            .Font(WanaFont("Bold", 11))
                            .ColorAndOpacity(T.TextPrimary)
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0.0f, 2.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Text(Description)
                            .Font(WanaFont("Regular", 7))
                            .ColorAndOpacity(T.TextMuted)
                        ]
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(8.0f, 0.0f, 0.0f, 0.0f)
                    [
                        WanaStatusPill(FText::FromString(StepNumber), AccentColor, true, 7, FMargin(6.0f, 3.0f))
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
        .ButtonStyle(&GhostButtonStyle())
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
            .Padding(FMargin(11.0f, 9.0f))
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
                        .Padding(0.0f, 1.0f, 10.0f, 0.0f)
                    [
                        SNew(SBorder)
                        .Padding(6.0f)
                        .BorderBackgroundColor_Lambda([IsActive, bAvailable, AccentColor]()
                        {
                            const bool bActive = IsActive ? IsActive() : false;
                            return bActive
                                ? AccentColor.CopyWithNewOpacity(0.36f)
                                : (bAvailable ? AccentColor.CopyWithNewOpacity(0.16f) : FLinearColor(0.20f, 0.23f, 0.32f, 0.46f));
                        })
                        [
                            MakeIconWidget(IconBrushName, bAvailable ? T.TextPrimary : T.TextMuted, 17.0f)
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
                            .Font(WanaFont("Bold", 10))
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
                        .Padding(0.0f, 3.0f, 0.0f, 0.0f)
                        [
                            SNew(STextBlock)
                            .Text(Subtitle)
                            .AutoWrapText(true)
                            .WrapTextAt(176.0f)
                            .Font(WanaFont("Regular", 7))
                            .ColorAndOpacity(bAvailable ? T.TextSecondary.CopyWithNewOpacity(0.88f) : T.TextMuted.CopyWithNewOpacity(0.74f))
                        ]
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(39.0f, 6.0f, 0.0f, 0.0f)
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
