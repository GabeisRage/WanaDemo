#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SWidget.h"

namespace WanaWorksUIStyle
{
void Register();
void Unregister();
FName GetStyleSetName();
const FSlateBrush* GetBrush(FName BrushName);
FName GetLauncherIconName();
FName GetWorkspaceIconName(const FString& WorkspaceLabel);
FName GetWorkflowIconName(const FString& WorkflowLabel);

struct FWanaDesignTokens
{
    FLinearColor BackgroundDeep = FLinearColor(0.012f, 0.039f, 0.071f, 1.0f);
    FLinearColor BackgroundMain = FLinearColor(0.020f, 0.059f, 0.102f, 1.0f);
    FLinearColor Navigation = FLinearColor(0.016f, 0.071f, 0.122f, 1.0f);
    FLinearColor TopBar = FLinearColor(0.024f, 0.078f, 0.133f, 1.0f);
    FLinearColor Workspace = FLinearColor(0.027f, 0.090f, 0.153f, 1.0f);
    FLinearColor Panel = FLinearColor(0.035f, 0.110f, 0.184f, 1.0f);
    FLinearColor Card = FLinearColor(0.043f, 0.133f, 0.216f, 1.0f);
    FLinearColor CardRaised = FLinearColor(0.055f, 0.161f, 0.259f, 1.0f);
    FLinearColor CardHover = FLinearColor(0.071f, 0.196f, 0.306f, 1.0f);
    FLinearColor Input = FLinearColor(0.016f, 0.067f, 0.114f, 1.0f);
    FLinearColor Divider = FLinearColor(0.075f, 0.200f, 0.286f, 1.0f);
    FLinearColor BorderSubtle = FLinearColor(0.102f, 0.247f, 0.349f, 1.0f);
    FLinearColor BorderStrong = FLinearColor(0.153f, 0.396f, 0.537f, 1.0f);
    FLinearColor AppBackground = FLinearColor(0.012f, 0.039f, 0.071f, 1.0f);
    FLinearColor Surface = FLinearColor(0.043f, 0.133f, 0.216f, 0.98f);
    FLinearColor SurfaceRaised = FLinearColor(0.055f, 0.161f, 0.259f, 0.992f);
    FLinearColor SurfaceGlass = FLinearColor(0.071f, 0.196f, 0.306f, 0.50f);
    FLinearColor TextPrimary = FLinearColor(0.937f, 0.969f, 0.988f, 1.0f);
    FLinearColor TextSecondary = FLinearColor(0.675f, 0.765f, 0.827f, 1.0f);
    FLinearColor TextMuted = FLinearColor(0.412f, 0.529f, 0.612f, 1.0f);
    FLinearColor TextDisabled = FLinearColor(0.271f, 0.357f, 0.424f, 1.0f);
    FLinearColor Shadow = FLinearColor(0.0f, 0.0f, 0.0f, 0.72f);
    FLinearColor Blue = FLinearColor(0.094f, 0.537f, 1.000f, 1.0f);
    FLinearColor Cyan = FLinearColor(0.000f, 0.863f, 0.886f, 1.0f);
    FLinearColor Violet = FLinearColor(0.616f, 0.345f, 1.000f, 1.0f);
    FLinearColor Emerald = FLinearColor(0.114f, 0.839f, 0.537f, 1.0f);
    FLinearColor Amber = FLinearColor(0.957f, 0.682f, 0.216f, 1.0f);
    FLinearColor Red = FLinearColor(0.933f, 0.275f, 0.306f, 1.0f);
    FLinearColor Success = FLinearColor(0.114f, 0.839f, 0.537f, 1.0f);
    FLinearColor Warning = FLinearColor(0.957f, 0.682f, 0.216f, 1.0f);
    FLinearColor Info = FLinearColor(0.094f, 0.537f, 1.000f, 1.0f);
    FLinearColor Critical = FLinearColor(0.933f, 0.275f, 0.306f, 1.0f);
    FLinearColor ElectricBlue = FLinearColor(0.094f, 0.537f, 1.000f, 1.0f);
    float CardPadding = 18.0f;
    float DensePadding = 12.0f;
};

const FWanaDesignTokens& Tokens();

FSlateFontInfo WanaFont(const ANSICHAR* Typeface, int32 Size);

FSlateFontInfo HeadingFont();
FSlateFontInfo SubheadingFont();
FSlateFontInfo LabelFont();
FSlateFontInfo CaptionFont();
FSlateFontInfo MonoFont();
FSlateFontInfo WorkspaceTitleFont();
FSlateFontInfo WorkspaceSubtitleFont();
FSlateFontInfo SectionHeadingFont();
FSlateFontInfo CardHeadingFont();
FSlateFontInfo MetricFont();
FSlateFontInfo BodyFont();
FSlateFontInfo MetadataFont();

FName CardBrushName();
FName CardProminentBrushName();
FName RailActiveBrushName();
FName RailHoverBrushName();
FName ActionPrimaryBrushName();
FName ActionSecondaryBrushName();
FName StatusPillSuccessBrushName();
FName StatusPillWarningBrushName();
FName StatusPillInfoBrushName();
FName AppBackgroundBrushName();
FName NavigationBrushName();
FName TopBarBrushName();
FName WorkspaceBrushName();
FName PanelBrushName();
FName CardHoverBrushName();
FName InputBrushName();
FName DividerBrushName();

const FButtonStyle& PrimaryButtonStyle();
const FButtonStyle& SecondaryButtonStyle();
const FButtonStyle& GhostButtonStyle();
const FButtonStyle& DangerButtonStyle();
const FButtonStyle& EnhanceButtonStyle();
const FButtonStyle& TestButtonStyle();
const FButtonStyle& AnalyzeButtonStyle();
const FButtonStyle& BuildButtonStyle();
const FButtonStyle& WorkflowButtonStyle(const FString& WorkflowLabel);
const FEditableTextBoxStyle& InputTextBoxStyle();
const FEditableTextBoxStyle& InputMultilineTextBoxStyle();
const FComboBoxStyle& ComboBoxStyle();
const FTableRowStyle& ComboRowStyle();
const FCheckBoxStyle& CheckBoxStyle();
const FScrollBarStyle& ScrollBarStyle();

TSharedRef<SWidget> WanaStatusPill(
    const FText& Label,
    const FLinearColor& AccentColor,
    bool bStrong = false,
    int32 FontSize = 8,
    const FMargin& Padding = FMargin(12.0f, 6.0f));

TSharedRef<SWidget> WanaSectionHeader(
    const FText& Eyebrow,
    const FText& Title,
    const FText& Description,
    const FLinearColor& AccentColor,
    const TSharedPtr<SWidget>& TrailingContent = TSharedPtr<SWidget>());

TSharedRef<SWidget> WanaCard(
    const FText& Eyebrow,
    const FText& Title,
    const FLinearColor& AccentColor,
    const TSharedRef<SWidget>& Content,
    const TSharedPtr<SWidget>& TrailingContent = TSharedPtr<SWidget>(),
    bool bProminent = false);

TSharedRef<SWidget> WanaMetricBlock(
    const FText& Label,
    const FText& Value,
    const FLinearColor& AccentColor,
    bool bProminent = false,
    float ValueWrapWidth = 220.0f);

TSharedRef<SWidget> WanaHeroStage(
    const FLinearColor& AccentColor,
    const TSharedRef<SWidget>& StageContent,
    const TSharedPtr<SWidget>& TopOverlay = TSharedPtr<SWidget>(),
    const TSharedPtr<SWidget>& BottomOverlay = TSharedPtr<SWidget>());

TSharedRef<SWidget> WanaActionTile(
    const FString& StepNumber,
    const FText& Title,
    const FText& Description,
    FName IconBrushName,
    const FLinearColor& AccentColor,
    TFunction<void(void)> OnPressed);

TSharedRef<SWidget> WanaWorkflowCommand(
    const FString& StepNumber,
    const FText& Title,
    const FText& Description,
    FName IconBrushName,
    const FLinearColor& AccentColor,
    TFunction<void(void)> OnPressed);

TSharedRef<SWidget> WanaWorkspaceRailItem(
    const FText& Label,
    const FText& Subtitle,
    TFunction<FText(void)> GetStateLabel,
    FName IconBrushName,
    const FLinearColor& AccentColor,
    bool bAvailable,
    TFunction<bool(void)> IsActive,
    TFunction<void(void)> OnClicked);

TSharedRef<SWidget> WanaToast(
    const FText& Label,
    const FText& Message,
    const FLinearColor& AccentColor);
}
