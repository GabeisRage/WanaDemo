#include "WanaMemoryEditorStyle.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FSlateStyleSet> FWanaMemoryEditorStyle::StyleSet;

FLinearColor FWanaMemoryEditorStyle::FromHex(const TCHAR* Hex)
{
    return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
}

FLinearColor FWanaMemoryEditorStyle::Background() { return FromHex(TEXT("070B18")); }
FLinearColor FWanaMemoryEditorStyle::Panel() { return FromHex(TEXT("0B1020")); }
FLinearColor FWanaMemoryEditorStyle::Inset() { return FromHex(TEXT("151D38")); }
FLinearColor FWanaMemoryEditorStyle::TextPrimary() { return FromHex(TEXT("F3F1EC")); }
FLinearColor FWanaMemoryEditorStyle::TextSecondary() { return FromHex(TEXT("C7CBD4")); }
FLinearColor FWanaMemoryEditorStyle::TextMuted() { return FromHex(TEXT("9AA0AC")); }
FLinearColor FWanaMemoryEditorStyle::Cyan() { return FromHex(TEXT("6AD7FF")); }
FLinearColor FWanaMemoryEditorStyle::Violet() { return FromHex(TEXT("8C7CFF")); }
FLinearColor FWanaMemoryEditorStyle::Gold() { return FromHex(TEXT("F1C96B")); }

FName FWanaMemoryEditorStyle::GetStyleSetName()
{
    return FName(TEXT("WanaMemoryEditorStyle"));
}

void FWanaMemoryEditorStyle::Initialize()
{
    if (StyleSet.IsValid())
    {
        return;
    }

    StyleSet = MakeShared<FSlateStyleSet>(GetStyleSetName());
    const FLinearColor PanelColor = Panel();
    const FLinearColor InsetColor = Inset();
    const FLinearColor CyanColor = Cyan();
    const FLinearColor GoldColor = Gold();

    StyleSet->Set("WanaMemory.Background", new FSlateRoundedBoxBrush(Background(), 0.f));
    StyleSet->Set("WanaMemory.Panel", new FSlateRoundedBoxBrush(PanelColor, 12.f, InsetColor, 1.f));
    StyleSet->Set("WanaMemory.Inset", new FSlateRoundedBoxBrush(InsetColor, 8.f));
    StyleSet->Set("WanaMemory.Chip", new FSlateRoundedBoxBrush(InsetColor, 6.f));

    FButtonStyle Button;
    const FSlateRoundedBoxBrush Normal(InsetColor, 8.f);
    const FSlateRoundedBoxBrush Hovered(FromHex(TEXT("1C2748")), 8.f, CyanColor, 1.f);
    const FSlateRoundedBoxBrush Pressed(FromHex(TEXT("243156")), 8.f, GoldColor, 1.f);
    Button.SetNormal(Normal);
    Button.SetHovered(Hovered);
    Button.SetPressed(Pressed);
    Button.SetDisabled(Normal);
    Button.SetNormalForeground(TextPrimary());
    Button.SetHoveredForeground(CyanColor);
    Button.SetPressedForeground(GoldColor);
    Button.SetDisabledForeground(TextMuted());
    Button.SetNormalPadding(FMargin(12.f, 6.f));
    Button.SetPressedPadding(FMargin(12.f, 7.f, 12.f, 5.f));
    StyleSet->Set("WanaMemory.Button", Button);

    FEditableTextBoxStyle TextBox = FAppStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
    const FSlateRoundedBoxBrush Field(InsetColor, 8.f);
    TextBox.SetBackgroundImageNormal(Field);
    TextBox.SetBackgroundImageHovered(Field);
    TextBox.SetBackgroundImageFocused(FSlateRoundedBoxBrush(InsetColor, 8.f, CyanColor, 1.f));
    TextBox.SetBackgroundImageReadOnly(Field);
    TextBox.SetForegroundColor(TextPrimary());
    StyleSet->Set("WanaMemory.TextBox", TextBox);

    FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
}

void FWanaMemoryEditorStyle::Shutdown()
{
    if (StyleSet.IsValid())
    {
        FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
        StyleSet.Reset();
    }
}

const ISlateStyle& FWanaMemoryEditorStyle::Get()
{
    return *StyleSet;
}

const FSlateBrush* FWanaMemoryEditorStyle::BackgroundBrush()
{
    return StyleSet->GetBrush("WanaMemory.Background");
}

const FSlateBrush* FWanaMemoryEditorStyle::PanelBrush()
{
    return StyleSet->GetBrush("WanaMemory.Panel");
}

const FSlateBrush* FWanaMemoryEditorStyle::InsetBrush()
{
    return StyleSet->GetBrush("WanaMemory.Inset");
}

const FButtonStyle& FWanaMemoryEditorStyle::ButtonStyle()
{
    return StyleSet->GetWidgetStyle<FButtonStyle>("WanaMemory.Button");
}

const FEditableTextBoxStyle& FWanaMemoryEditorStyle::TextBoxStyle()
{
    return StyleSet->GetWidgetStyle<FEditableTextBoxStyle>("WanaMemory.TextBox");
}

FSlateFontInfo FWanaMemoryEditorStyle::TitleFont()
{
    return FCoreStyle::GetDefaultFontStyle("Bold", 18);
}

FSlateFontInfo FWanaMemoryEditorStyle::BodyFont()
{
    return FCoreStyle::GetDefaultFontStyle("Regular", 12);
}

FSlateFontInfo FWanaMemoryEditorStyle::SmallFont()
{
    return FCoreStyle::GetDefaultFontStyle("Regular", 10);
}
