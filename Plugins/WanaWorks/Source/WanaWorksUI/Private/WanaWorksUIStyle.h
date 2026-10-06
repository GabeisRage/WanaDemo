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

// sRGB hex bytes -> linear FLinearColor. Slate treats FLinearColor components as linear,
// so authoring 0-1 "hex fractions" directly washes the Midnight Core navy out to steel blue.
inline FLinearColor WanaSRGB(uint8 R, uint8 G, uint8 B, uint8 A = 255)
{
    return FLinearColor::FromSRGBColor(FColor(R, G, B, A));
}

struct FWanaDesignTokens
{
    // Midnight Core anchors, stored linear: #070B18 / #0B1020 / #151D38.
    FLinearColor BackgroundDeep = WanaSRGB(0x07, 0x0B, 0x18);
    FLinearColor BackgroundMain = WanaSRGB(0x0B, 0x10, 0x20);
    FLinearColor Navigation = WanaSRGB(0x0B, 0x10, 0x20);
    FLinearColor TopBar = WanaSRGB(0x0B, 0x10, 0x20);
    FLinearColor Workspace = WanaSRGB(0x0A, 0x0E, 0x1C);
    FLinearColor Panel = WanaSRGB(0x10, 0x16, 0x2C);
    FLinearColor Card = WanaSRGB(0x15, 0x1D, 0x38);
    FLinearColor CardRaised = WanaSRGB(0x1B, 0x24, 0x44);
    FLinearColor CardHover = WanaSRGB(0x24, 0x2E, 0x54);
    FLinearColor Input = WanaSRGB(0x07, 0x0B, 0x18);
    FLinearColor Divider = WanaSRGB(0x2A, 0x36, 0x5C);
    FLinearColor BorderSubtle = WanaSRGB(0x32, 0x3E, 0x68);
    FLinearColor BorderStrong = WanaSRGB(0x4A, 0x5A, 0x88);
    FLinearColor AppBackground = WanaSRGB(0x07, 0x0B, 0x18);
    FLinearColor Surface = WanaSRGB(0x15, 0x1D, 0x38, 250);
    FLinearColor SurfaceRaised = WanaSRGB(0x1B, 0x24, 0x44, 253);
    FLinearColor SurfaceGlass = WanaSRGB(0x15, 0x1D, 0x38, 128);
    FLinearColor TextPrimary = WanaSRGB(0xF3, 0xF1, 0xEC);
    FLinearColor TextSecondary = WanaSRGB(0xC7, 0xCB, 0xD4);
    FLinearColor TextMuted = WanaSRGB(0x9A, 0xA0, 0xAC);
    FLinearColor TextDisabled = WanaSRGB(0x6E, 0x74, 0x82);
    FLinearColor Shadow = FLinearColor(0.0f, 0.0f, 0.0f, 0.72f);
    // Blue is the supporting bridge between cyan and violet so Enhance/Build stay distinct.
    FLinearColor Blue = WanaSRGB(0x7A, 0xA2, 0xFF);
    FLinearColor Cyan = WanaSRGB(0x6A, 0xD7, 0xFF);
    FLinearColor Violet = WanaSRGB(0x8C, 0x7C, 0xFF);
    FLinearColor Emerald = WanaSRGB(0x3D, 0xDC, 0x97);
    FLinearColor Amber = WanaSRGB(0xF1, 0xC9, 0x6B);
    FLinearColor Red = WanaSRGB(0xFF, 0x6B, 0x7A);
    FLinearColor Success = WanaSRGB(0x3D, 0xDC, 0x97);
    FLinearColor Warning = WanaSRGB(0xF1, 0xC9, 0x6B);
    FLinearColor Info = WanaSRGB(0x6A, 0xD7, 0xFF);
    FLinearColor Critical = WanaSRGB(0xFF, 0x6B, 0x7A);
    FLinearColor ElectricBlue = WanaSRGB(0x6A, 0xD7, 0xFF);
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
const FExpandableAreaStyle& ExpandableAreaStyle();
const FSplitterStyle& SplitterStyle();
const FScrollBoxStyle& ScrollBoxStyle();

const FSlateBrush* RoundedTintBrush();
const FSlateBrush* RoundedFillBrush();
const FSlateBrush* RoundedChipBrush();
const FSlateBrush* FlatTintBrush();

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
