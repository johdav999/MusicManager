#include "UI/SignedArtistItemWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void USignedArtistItemWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (IsValid(ItemButton))
    {
        ItemButton->OnClicked.RemoveDynamic(this, &USignedArtistItemWidget::HandleClicked);
        ItemButton->OnClicked.AddDynamic(this, &USignedArtistItemWidget::HandleClicked);
    }
}

void USignedArtistItemWidget::NativeDestruct()
{
    if (IsValid(ItemButton))
    {
        ItemButton->OnClicked.RemoveDynamic(this, &USignedArtistItemWidget::HandleClicked);
    }
    Super::NativeDestruct();
}

void USignedArtistItemWidget::SetupItem(const FArtistData& InData, UTexture2D* PortraitTexture)
{
    LocalArtistData = InData;

    if (IsValid(ArtistNameText))
    {
        ArtistNameText->SetText(FText::FromString(InData.ArtistName));
    }

    if (IsValid(ArtistGenreText))
    {
        ArtistGenreText->SetText(FText::FromString(InData.Genre));
    }

    if (IsValid(PortraitImage))
    {
        FSlateBrush Brush;
        Brush.SetResourceObject(PortraitTexture);
        Brush.ImageSize = FVector2D(96.f, 96.f);
        PortraitImage->SetBrush(Brush);
    }
}

void USignedArtistItemWidget::HandleClicked()
{
    if (!IsInGameThread())
    {
        return;
    }

    OnArtistClicked.Broadcast(LocalArtistData.ArtistName);
}
