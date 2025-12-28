#include "UI/SignedArtistItemWidget.h"

#include "ArtistManagerSubsystem.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "RecordManagerSubsystem.h"

void USignedArtistItemWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (IsValid(ItemButton))
    {
        ItemButton->OnClicked.RemoveDynamic(this, &USignedArtistItemWidget::HandleClicked);
        ItemButton->OnClicked.AddDynamic(this, &USignedArtistItemWidget::HandleClicked);

        ItemButton->OnHovered.RemoveDynamic(this, &USignedArtistItemWidget::HandleHovered);
        ItemButton->OnHovered.AddDynamic(this, &USignedArtistItemWidget::HandleHovered);

        ItemButton->OnUnhovered.RemoveDynamic(this, &USignedArtistItemWidget::HandleUnhovered);
        ItemButton->OnUnhovered.AddDynamic(this, &USignedArtistItemWidget::HandleUnhovered);
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (URecordManagerSubsystem* Subsystem = GameInstance->GetSubsystem<URecordManagerSubsystem>())
        {
            RecordSubsystem = Subsystem;
            TWeakObjectPtr<USignedArtistItemWidget> WeakThis(this);
            RecordCreatedHandle = Subsystem->OnArtistRecordCreated.AddLambda([WeakThis](const FString& ArtistId)
            {
                if (!IsInGameThread())
                {
                    UE_LOG(LogTemp, Warning, TEXT("SignedArtistItemWidget: Record created callback invoked off the game thread."));
                    return;
                }

                if (USignedArtistItemWidget* StrongThis = WeakThis.Get())
                {
                    if (ArtistId == StrongThis->GetArtistId())
                    {
                        StrongThis->RefreshRecordCount();
                    }
                }
            });
        }
    }

    if (!LocalArtistData.ArtistName.IsEmpty())
    {
        RefreshRecordCount();
    }

    UpdateVisualState();
}

void USignedArtistItemWidget::NativeDestruct()
{
    if (RecordCreatedHandle.IsValid())
    {
        if (URecordManagerSubsystem* Subsystem = RecordSubsystem.Get())
        {
            Subsystem->OnArtistRecordCreated.Remove(RecordCreatedHandle);
        }
        RecordCreatedHandle.Reset();
    }

    if (IsValid(ItemButton))
    {
        ItemButton->OnClicked.RemoveDynamic(this, &USignedArtistItemWidget::HandleClicked);
        ItemButton->OnHovered.RemoveDynamic(this, &USignedArtistItemWidget::HandleHovered);
        ItemButton->OnUnhovered.RemoveDynamic(this, &USignedArtistItemWidget::HandleUnhovered);
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

    RefreshRecordCount();
}

void USignedArtistItemWidget::SetHovered(bool bHovered)
{
    bIsHovered = bHovered;
    UpdateVisualState();
}

void USignedArtistItemWidget::SetSelected(bool bSelected)
{
    bIsSelected = bSelected;
    UpdateVisualState();
}

void USignedArtistItemWidget::UpdateVisualState()
{
    FLinearColor DesiredColor = NormalColor;

    if (bIsSelected)
    {
        DesiredColor = SelectedColor;
    }
    else if (bIsHovered)
    {
        DesiredColor = HoveredColor;
    }

    if (IsValid(BackgroundBorder))
    {
        BackgroundBorder->SetBrushColor(DesiredColor);
    }
    else if (IsValid(ItemButton))
    {
        ItemButton->SetBackgroundColor(DesiredColor);
    }
}

void USignedArtistItemWidget::RefreshRecordCount()
{
    if (!IsInGameThread())
    {
        UE_LOG(LogTemp, Warning, TEXT("SignedArtistItemWidget: RefreshRecordCount called off the game thread."));
        return;
    }

    if (!IsValid(RecordsNumText))
    {
        return;
    }

    if (LocalArtistData.ArtistName.IsEmpty())
    {
        RecordsNumText->SetText(FText::AsNumber(0));
        return;
    }

    URecordManagerSubsystem* Subsystem = RecordSubsystem.Get();
    if (!Subsystem)
    {
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            Subsystem = GameInstance->GetSubsystem<URecordManagerSubsystem>();
            RecordSubsystem = Subsystem;
        }
    }

    if (!Subsystem)
    {
        RecordsNumText->SetText(FText::AsNumber(0));
        return;
    }

    const int32 RecordCount = Subsystem->GetRecordCountForArtist(LocalArtistData.ArtistName);
    RecordsNumText->SetText(FText::AsNumber(RecordCount));
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

void USignedArtistItemWidget::HandleHovered()
{
    if (!IsInGameThread())
    {
        return;
    }

    SetHovered(true);
}

void USignedArtistItemWidget::HandleUnhovered()
{
    if (!IsInGameThread())
    {
        return;
    }

    SetHovered(false);
}
