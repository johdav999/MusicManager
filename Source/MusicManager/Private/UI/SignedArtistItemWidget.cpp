#include "UI/SignedArtistItemWidget.h"

#include "ArtistManagerSubsystem.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UI/ArtistActionIconSet.h"
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

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UArtistManagerSubsystem* ArtistSubsystem = GameInstance->GetSubsystem<UArtistManagerSubsystem>())
        {
            ActionAvailabilityHandle = ArtistSubsystem->OnArtistActionAvailabilityChanged.AddUObject(
                this,
                &USignedArtistItemWidget::HandleActionAvailabilityChanged
            );
        }
    }

    if (IsValid(FrameImage))
    {
        if (UMaterialInterface* BaseMaterial = Cast<UMaterialInterface>(FrameImage->GetBrush().GetResourceObject()))
        {
            FrameMID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
            FrameImage->SetBrushFromMaterial(FrameMID);
        }
    }

    if (FrameMID)
    {
        FrameMID->SetScalarParameterValue(TEXT("AttentionBoost"), 1.0f);
    }

    //if (IsValid(ActionIconImage))
    //{
    //    ActionIconImage->SetVisibility(ESlateVisibility::Collapsed);
    //}

    bIsInitialized = true;

    // Apply any deferred setup data after construction is complete.
    if (bHasPendingSetupData)
    {
        ApplySetupData();
    }
}

void USignedArtistItemWidget::NativeDestruct()
{
    if (IsValid(ItemButton))
    {
        ItemButton->OnClicked.RemoveDynamic(this, &USignedArtistItemWidget::HandleClicked);
        ItemButton->OnHovered.RemoveDynamic(this, &USignedArtistItemWidget::HandleHovered);
        ItemButton->OnUnhovered.RemoveDynamic(this, &USignedArtistItemWidget::HandleUnhovered);
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UArtistManagerSubsystem* ArtistSubsystem = GameInstance->GetSubsystem<UArtistManagerSubsystem>())
        {
            if (ActionAvailabilityHandle.IsValid())
            {
                ArtistSubsystem->OnArtistActionAvailabilityChanged.Remove(ActionAvailabilityHandle);
            }
        }
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AttentionBoostTimerHandle);
    }
    Super::NativeDestruct();
}

void USignedArtistItemWidget::SetupItem(const FArtistData& InData, UTexture2D* PortraitTexture)
{
    PendingArtistData = InData;
    PendingPortraitTexture = PortraitTexture;
    bHasPendingSetupData = true;

    if (bIsInitialized)
    {
        ApplySetupData();
    }
}

void USignedArtistItemWidget::ApplySetupData()
{
    LocalArtistData = PendingArtistData;

    if (IsValid(PortraitImage))
    {
        PortraitImage->SetBrushFromTexture(PendingPortraitTexture, true);
    }

    UpdateVisualState();
    RefreshActionAvailabilityFromSubsystem(false);
    bHasPendingSetupData = false;
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

    if (StateColor != CachedStateColor)
    {
        FrameMID->SetVectorParameterValue(TEXT("StateColor"), StateColor);
        CachedStateColor = StateColor;
    }

    if (!FMath::IsNearlyEqual(RimIntensity, CachedRimIntensity))
    {
        FrameMID->SetScalarParameterValue(TEXT("RimIntensity"), RimIntensity);
        CachedRimIntensity = RimIntensity;
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

void USignedArtistItemWidget::HandleHovered()
{
    if (!IsInGameThread())
    {
        return;
    }

    SetHovered(true);

    if (UUIManagerSubsystem* UIManager = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUIManagerSubsystem>() : nullptr)
    {
        // Layer-1 items express intent only; layout decisions live in the UI manager (Layer-2).
        UIManager->ShowArtistHover(LocalArtistData);
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

void USignedArtistItemWidget::HandleActionAvailabilityChanged(const FString& ArtistId, EArtistActionAvailability NewAvailability)
{
    if (LocalArtistData.ArtistName.IsEmpty() || LocalArtistData.ArtistName != ArtistId)
    {
        return;
    }

    ApplyActionAvailability(NewAvailability, true);
}

void USignedArtistItemWidget::ApplyActionAvailability(EArtistActionAvailability NewAvailability, bool bTriggerAttention)
{
    if (CachedAvailability == NewAvailability)
    {
        return;
    }

    CachedAvailability = NewAvailability;

    if (NewAvailability == EArtistActionAvailability::None)
    {
        if (IsValid(ActionIconImage))
        {
            ActionIconImage->SetVisibility(ESlateVisibility::Collapsed);
        }
        ResetAttentionBoost();
        return;
    }

    if (IsValid(ActionIconImage))
    {
        const UTexture2D* IconTexture = ResolveActionIcon(NewAvailability);
        if (IconTexture)
        {
            ActionIconImage->SetBrushFromTexture(const_cast<UTexture2D*>(IconTexture), true);
            ActionIconImage->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            ActionIconImage->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    //if (bTriggerAttention)
    {
        TriggerAttentionBoost();
    }
}

void USignedArtistItemWidget::RefreshActionAvailabilityFromSubsystem(bool bTriggerAttention)
{
    if (LocalArtistData.ArtistName.IsEmpty())
    {
        return;
    }

    // Layer-1 widgets only reflect subsystem state; business rules live in subsystems.
    if (const UGameInstance* GameInstance = GetGameInstance())
    {
        if (const UArtistManagerSubsystem* ArtistSubsystem = GameInstance->GetSubsystem<UArtistManagerSubsystem>())
        {
            const EArtistActionAvailability Availability = ArtistSubsystem->GetArtistActionAvailability(LocalArtistData.ArtistName);
            ApplyActionAvailability(Availability, bTriggerAttention);
        }
    }
}

const UTexture2D* USignedArtistItemWidget::ResolveActionIcon(EArtistActionAvailability Availability) const
{
    if (!ActionIconSet)
    {
        return nullptr;
    }

    for (const FArtistActionIconData& Entry : ActionIconSet->Icons)
    {
        if (Entry.Availability == Availability)
        {
            return Entry.IconTexture;
        }
    }

    return nullptr;
}

void USignedArtistItemWidget::TriggerAttentionBoost()
{
    if (!FrameMID)
    {
        return;
    }

    FrameMID->SetScalarParameterValue(TEXT("AttentionBoost"), AttentionBoostValue);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AttentionBoostTimerHandle);
        World->GetTimerManager().SetTimer(
            AttentionBoostTimerHandle,
            this,
            &USignedArtistItemWidget::ResetAttentionBoost,
            AttentionBoostResetDelay,
            false
        );
    }
}

void USignedArtistItemWidget::ResetAttentionBoost()
{
    if (FrameMID)
    {
        FrameMID->SetScalarParameterValue(TEXT("AttentionBoost"), 1.0f);
    }
}

EArtistVisualState USignedArtistItemWidget::DetermineVisualState() const
{
    if (const UGameInstance* GI = GetGameInstance())
    {
        if (const UArtistManagerSubsystem* Sub = GI->GetSubsystem<UArtistManagerSubsystem>())
        {
            return Sub->GetArtistVisualState(LocalArtistData.ArtistName);
        }
    }
    return EArtistVisualState::Idle;
}
