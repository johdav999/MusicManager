#include "UI/SignedArtistItemWidget.h"

#include "ArtistManagerSubsystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UIManagerSubsystem.h"

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

    if (IsValid(FrameImage))
    {
        if (UMaterialInterface* BaseMaterial = Cast<UMaterialInterface>(FrameImage->GetBrush().GetResourceObject()))
        {
            FrameMID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
            FrameImage->SetBrushFromMaterial(FrameMID);
        }
    }

    UpdateVisualState();
}

void USignedArtistItemWidget::NativeDestruct()
{
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

    UpdateVisualState();
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
    if (!FrameMID)
    {
        return;
    }

    const EArtistVisualState VisualState = DetermineVisualState();

    FLinearColor StateColor = IdleStateColor;
    switch (VisualState)
    {
    case EArtistVisualState::Rising:
        StateColor = RisingStateColor;
        break;
    case EArtistVisualState::Stable:
        StateColor = StableStateColor;
        break;
    case EArtistVisualState::Declining:
        StateColor = DecliningStateColor;
        break;
    case EArtistVisualState::Idle:
    default:
        StateColor = IdleStateColor;
        break;
    }

    float RimIntensity = BaseRimIntensity;
    if (bIsHovered)
    {
        RimIntensity += HoverRimBoost;
    }
    if (bIsSelected)
    {
        RimIntensity += SelectedRimBoost;
    }

    FrameMID->SetVectorParameterValue(TEXT("StateColor"), StateColor);
    FrameMID->SetScalarParameterValue(TEXT("RimIntensity"), RimIntensity);
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

    if (UUIManagerSubsystem* UIManager = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUIManagerSubsystem>() : nullptr)
    {
        // Layer-1 items never spawn widgets directly; route to the UI manager (Layer-2).
        const FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(this);
        UIManager->ShowArtistHover(LocalArtistData, MousePosition);
    }
}

void USignedArtistItemWidget::HandleUnhovered()
{
    if (!IsInGameThread())
    {
        return;
    }

    SetHovered(false);

    if (UUIManagerSubsystem* UIManager = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUIManagerSubsystem>() : nullptr)
    {
        // Layer-2 hover details are dismissed centrally through the UI manager.
        UIManager->HideArtistHover();
    }
}

EArtistVisualState USignedArtistItemWidget::DetermineVisualState() const
{
    const float CombinedScore = (LocalArtistData.PerformanceScore
        + LocalArtistData.StagePresence
        + LocalArtistData.AudienceEngagement
        + LocalArtistData.VocalQuality
        + LocalArtistData.SongwritingQuality) / 5.0f;

    if (CombinedScore >= 80.0f)
    {
        return EArtistVisualState::Rising;
    }

    if (CombinedScore >= 55.0f)
    {
        return EArtistVisualState::Stable;
    }

    if (CombinedScore >= 30.0f)
    {
        return EArtistVisualState::Declining;
    }

    return EArtistVisualState::Idle;
}
