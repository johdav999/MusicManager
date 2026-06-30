#include "UI/ActiveContractItemWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UObject/SoftObjectPath.h"

namespace
{
    FString DisplayArtistName(const FArtistContract& Contract)
    {
        if (!Contract.ArtistData.ArtistName.IsEmpty())
        {
            return Contract.ArtistData.ArtistName;
        }

        return Contract.ArtistId.IsEmpty() ? TEXT("Unknown Artist") : Contract.ArtistId;
    }

    FString FormatCurrencyCompact(float Value)
    {
        const int32 RoundedValue = FMath::RoundToInt(Value);
        return FString::Printf(TEXT("$%d"), RoundedValue);
    }

    float GetTermProgress(const FArtistContract& Contract)
    {
        const int32 TotalMonths = FMath::Max(Contract.Terms.ContractYears * 12, 1);
        return FMath::Clamp(static_cast<float>(Contract.MonthsActive) / static_cast<float>(TotalMonths), 0.f, 1.f);
    }
}

void UActiveContractItemWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (ContractButton)
    {
        ContractButton->OnClicked.RemoveDynamic(this, &UActiveContractItemWidget::HandleClicked);
        ContractButton->OnClicked.AddDynamic(this, &UActiveContractItemWidget::HandleClicked);
    }

    ApplyVisualState();
}

void UActiveContractItemWidget::NativeDestruct()
{
    if (ContractButton)
    {
        ContractButton->OnClicked.RemoveDynamic(this, &UActiveContractItemWidget::HandleClicked);
    }

    Super::NativeDestruct();
}

void UActiveContractItemWidget::SetupContractItem(const FArtistContract& InContract)
{
    ContractData = InContract;

    SetTextSafe(ArtistNameText, DisplayArtistName(ContractData));
    SetTextSafe(GenreText, ContractData.ArtistData.Genre.IsEmpty() ? TEXT("Genre unknown") : ContractData.ArtistData.Genre);
    SetTextSafe(TermText, FString::Printf(TEXT("%d / %d months"), ContractData.MonthsActive, FMath::Max(ContractData.Terms.ContractYears * 12, 0)));
    SetTextSafe(RecordsText, FString::Printf(TEXT("%d / %d records"), ContractData.RecordsDelivered, ContractData.Terms.NumRecords));
    SetTextSafe(StatusText, ContractData.bContractActive ? TEXT("ACTIVE") : TEXT("INACTIVE"));
    SetTextSafe(EconomicsText, FString::Printf(TEXT("Rev %s  Cost %s"), *FormatCurrencyCompact(ContractData.LifetimeRevenue), *FormatCurrencyCompact(ContractData.LifetimeCost)));

    if (TermProgressBar)
    {
        TermProgressBar->SetPercent(GetTermProgress(ContractData));
    }

    ApplyPortrait();
    ApplyVisualState();
}

void UActiveContractItemWidget::SetSelected(bool bInSelected)
{
    bSelected = bInSelected;
    ApplyVisualState();
}

void UActiveContractItemWidget::HandleClicked()
{
    OnContractSelected.Broadcast(ContractData.ArtistId);
}

void UActiveContractItemWidget::ApplyVisualState()
{
    if (ContractButton)
    {
        ContractButton->SetBackgroundColor(bSelected
            ? FLinearColor(0.24f, 0.17f, 0.06f, 0.96f)
            : FLinearColor(0.04f, 0.04f, 0.035f, 0.96f));
    }

    if (StatusText)
    {
        StatusText->SetColorAndOpacity(FSlateColor(bSelected
            ? FLinearColor(1.f, 0.84f, 0.36f, 1.f)
            : FLinearColor(0.78f, 0.72f, 0.62f, 1.f)));
    }
}

void UActiveContractItemWidget::SetTextSafe(UTextBlock* TextBlock, const FString& Value) const
{
    if (TextBlock)
    {
        TextBlock->SetText(FText::FromString(Value));
    }
}

void UActiveContractItemWidget::ApplyPortrait()
{
    if (!ArtistPortraitImage || ContractData.ArtistData.ImageAssetRef.IsEmpty())
    {
        return;
    }

    FSoftObjectPath TexturePath(ContractData.ArtistData.ImageAssetRef);
    if (UTexture2D* Texture = Cast<UTexture2D>(TexturePath.TryLoad()))
    {
        FSlateBrush Brush;
        Brush.SetResourceObject(Texture);
        Brush.ImageSize = FVector2D(64.f, 64.f);
        ArtistPortraitImage->SetBrush(Brush);
    }
}
