#include "UI/RegionMapButton.h"

void URegionMapButton::NativeConstruct()
{
    Super::NativeConstruct();

    if (RegionButton)
    {
        RegionButton->OnClicked.Clear();
        RegionButton->OnClicked.AddDynamic(this, &URegionMapButton::HandleClicked);
    }

    if (!RegionId.IsEmpty() && RegionText)
    {
        RegionText->SetText(FText::FromString(RegionId));
    }

    ApplyStyle();
}

void URegionMapButton::InitializeRegion(const FString& InRegionId)
{
    RegionId = InRegionId;

    if (RegionText)
    {
        RegionText->SetText(FText::FromString(RegionId));
    }

    ApplyStyle();
}

void URegionMapButton::HandleClicked()
{
    OnRegionClicked.Broadcast(RegionId);
}

void URegionMapButton::ApplyStyle()
{
    if (RegionButton)
    {
        FButtonStyle NewStyle = RegionButton->WidgetStyle;
        NewStyle.Normal.TintColor = FSlateColor(ButtonColor);
        NewStyle.Hovered.TintColor = FSlateColor(HoverColor);
        RegionButton->SetStyle(NewStyle);
    }

    if (RegionText)
    {
        RegionText->SetColorAndOpacity(TextColor);

        FSlateFontInfo FontInfo = RegionText->GetFont();
        FontInfo.Size = FontSize;
        RegionText->SetFont(FontInfo);
    }

    if (RootSizeBox)
    {
        if (ButtonSize.X > 0.f)
        {
            RootSizeBox->SetWidthOverride(ButtonSize.X);
        }
        else
        {
            RootSizeBox->ClearWidthOverride();
        }

        if (ButtonSize.Y > 0.f)
        {
            RootSizeBox->SetHeightOverride(ButtonSize.Y);
        }
        else
        {
            RootSizeBox->ClearHeightOverride();
        }
    }
}

void URegionMapButton::SetButtonColor(const FLinearColor& InColor)
{
    ButtonColor = InColor;
    ApplyStyle();
}

void URegionMapButton::SetHoverColor(const FLinearColor& InColor)
{
    HoverColor = InColor;
    ApplyStyle();
}

void URegionMapButton::SetTextColor(const FLinearColor& InColor)
{
    TextColor = InColor;
    ApplyStyle();
}

void URegionMapButton::SetFontSize(int32 InSize)
{
    FontSize = InSize;
    ApplyStyle();
}

void URegionMapButton::SetButtonSize(const FVector2D& InSize)
{
    ButtonSize = InSize;
    ApplyStyle();
}
