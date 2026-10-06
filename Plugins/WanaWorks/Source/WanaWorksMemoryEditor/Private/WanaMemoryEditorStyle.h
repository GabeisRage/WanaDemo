#pragma once

#include "Styling/SlateStyle.h"

/* Midnight palette, local to this editor module so it does not touch
   WanaWorksUIStyle (review/midnight-core-ui owns that theme).
   Hex values are sRGB. Callers convert with FLinearColor::FromSRGBColor. */

class FWanaMemoryEditorStyle
{
public:
    static void Initialize();
    static void Shutdown();
    static const ISlateStyle& Get();
    static FName GetStyleSetName();

    static FLinearColor Background();
    static FLinearColor Panel();
    static FLinearColor Inset();
    static FLinearColor TextPrimary();
    static FLinearColor TextSecondary();
    static FLinearColor TextMuted();
    static FLinearColor Cyan();
    static FLinearColor Violet();
    static FLinearColor Gold();

    static const FSlateBrush* BackgroundBrush();
    static const FSlateBrush* PanelBrush();
    static const FSlateBrush* InsetBrush();
    static const FButtonStyle& ButtonStyle();
    static const FEditableTextBoxStyle& TextBoxStyle();
    static FSlateFontInfo TitleFont();
    static FSlateFontInfo BodyFont();
    static FSlateFontInfo SmallFont();

private:
    static TSharedPtr<FSlateStyleSet> StyleSet;
    static FLinearColor FromHex(const TCHAR* Hex);
};
