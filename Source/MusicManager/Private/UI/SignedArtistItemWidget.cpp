#include "UI/SignedArtistItemWidget.h"

#include "ArtistManagerSubsystem.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

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
        if (PortraitTexture)
        {
            PortraitImage->SetBrushFromTexture(PortraitTexture, true);
        }
        else
        {
            PortraitImage->SetBrushFromTexture(nullptr, true);
        }
    }
}

void USignedArtistItemWidget::HandleClicked()
{
    if (!IsInGameThread())
    {
        return;
    }

    OnArtistClicked.Broadcast(LocalArtistData.ArtistName);

    // Update global selected artist
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UArtistManagerSubsystem* ArtistSub = GI->GetSubsystem<UArtistManagerSubsystem>())
        {
            ArtistSub->SetSelectedArtist(LocalArtistData.ArtistName);
        }
    }
}
