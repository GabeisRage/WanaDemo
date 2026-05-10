#pragma once

#include "CoreMinimal.h"
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
    FLinearColor AppBackground = FLinearColor(0.0008f, 0.0020f, 0.0100f, 1.0f);
    FLinearColor Surface = FLinearColor(0.011f, 0.017f, 0.043f, 0.98f);
    FLinearColor SurfaceRaised = FLinearColor(0.025f, 0.035f, 0.078f, 0.992f);
    FLinearColor SurfaceGlass = FLinearColor(0.070f, 0.092f, 0.180f, 0.42f);
    FLinearColor TextPrimary = FLinearColor(0.975f, 0.986f, 1.000f, 1.0f);
    FLinearColor TextSecondary = FLinearColor(0.735f, 0.805f, 0.930f, 1.0f);
    FLinearColor TextMuted = FLinearColor(0.455f, 0.535f, 0.700f, 1.0f);
    FLinearColor Shadow = FLinearColor(0.0f, 0.0f, 0.0f, 0.72f);
    FLinearColor Success = FLinearColor(0.34f, 0.76f, 0.58f, 1.0f);
    FLinearColor Warning = FLinearColor(0.90f, 0.66f, 0.28f, 1.0f);
    FLinearColor Info = FLinearColor(0.30f, 0.54f, 0.94f, 1.0f);
    float CardPadding = 24.0f;
    float DensePadding = 14.0f;
};

const FWanaDesignTokens& Tokens();

FSlateFontInfo WanaFont(const ANSICHAR* Typeface, int32 Size);

FSlateFontInfo HeadingFont();
FSlateFontInfo SubheadingFont();
FSlateFontInfo LabelFont();
FSlateFontInfo CaptionFont();
FSlateFontInfo MonoFont();

FName CardBrushName();
FName CardProminentBrushName();
FName RailActiveBrushName();
FName RailHoverBrushName();
FName ActionPrimaryBrushName();
FName ActionSecondaryBrushName();
FName StatusPillSuccessBrushName();
FName StatusPillWarningBrushName();
FName StatusPillInfoBrushName();

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
