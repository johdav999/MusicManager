#include "UI/ActiveContractsWidget.h"

#include "ArtistManagerSubsystem.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "UI/ActiveContractItemWidget.h"

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

    FString FormatCurrency(float Value)
    {
        const int32 RoundedValue = FMath::RoundToInt(Value);
        FString Raw = FString::FromInt(FMath::Abs(RoundedValue));
        FString Grouped;
        int32 Digits = 0;
        for (int32 Index = Raw.Len() - 1; Index >= 0; --Index)
        {
            if (Digits > 0 && Digits % 3 == 0)
            {
                Grouped.InsertAt(0, TEXT(","));
            }
            Grouped.InsertAt(0, Raw[Index]);
            ++Digits;
        }

        return FString::Printf(TEXT("%s$%s"), RoundedValue < 0 ? TEXT("-") : TEXT(""), *Grouped);
    }

    FString FormatDate(const FDateTime& Date)
    {
        if (Date.GetTicks() <= 0)
        {
            return TEXT("--");
        }

        return Date.ToString(TEXT("%b %d, %Y"));
    }

    FString FormatPercent(float Fraction)
    {
        return FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Fraction * 100.f));
    }

    float TermProgress(const FArtistContract& Contract)
    {
        const int32 TotalMonths = FMath::Max(Contract.Terms.ContractYears * 12, 1);
        return FMath::Clamp(static_cast<float>(Contract.MonthsActive) / static_cast<float>(TotalMonths), 0.f, 1.f);
    }

    float ProductionProgress(const FArtistContract& Contract)
    {
        if (Contract.Terms.NumRecords <= 0)
        {
            return 0.f;
        }

        const float Delivered = static_cast<float>(Contract.RecordsDelivered);
        const float Partial = FMath::Clamp(Contract.ProductionProgress, 0.f, 1.f);
        return FMath::Clamp((Delivered + Partial) / static_cast<float>(Contract.Terms.NumRecords), 0.f, 1.f);
    }
}

void UActiveContractsWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (CloseButton)
    {
        CloseButton->OnClicked.RemoveDynamic(this, &UActiveContractsWidget::HandleCloseClicked);
        CloseButton->OnClicked.AddDynamic(this, &UActiveContractsWidget::HandleCloseClicked);
    }

    if (ViewArtistButton)
    {
        ViewArtistButton->OnClicked.RemoveDynamic(this, &UActiveContractsWidget::HandleViewArtistClicked);
        ViewArtistButton->OnClicked.AddDynamic(this, &UActiveContractsWidget::HandleViewArtistClicked);
    }

    if (CloseActionButton)
    {
        CloseActionButton->OnClicked.RemoveDynamic(this, &UActiveContractsWidget::HandleCloseClicked);
        CloseActionButton->OnClicked.AddDynamic(this, &UActiveContractsWidget::HandleCloseClicked);
    }

    BindSubsystemEvents();
    RefreshContracts();
}

void UActiveContractsWidget::NativeDestruct()
{
    UnbindSubsystemEvents();

    if (CloseButton)
    {
        CloseButton->OnClicked.RemoveDynamic(this, &UActiveContractsWidget::HandleCloseClicked);
    }

    if (ViewArtistButton)
    {
        ViewArtistButton->OnClicked.RemoveDynamic(this, &UActiveContractsWidget::HandleViewArtistClicked);
    }

    if (CloseActionButton)
    {
        CloseActionButton->OnClicked.RemoveDynamic(this, &UActiveContractsWidget::HandleCloseClicked);
    }

    Super::NativeDestruct();
}

void UActiveContractsWidget::RefreshContracts()
{
    if (UArtistManagerSubsystem* ArtistSubsystem = GetArtistSubsystem())
    {
        CachedContracts = ArtistSubsystem->ActiveContracts;
    }
    else
    {
        CachedContracts.Reset();
    }

    RebuildList(CachedContracts);

    if (SelectedArtistId.IsEmpty() || !FindContractByArtistId(SelectedArtistId))
    {
        SelectedArtistId = CachedContracts.Num() > 0 ? CachedContracts[0].ArtistId : FString();
    }

    UpdateDetailPanel(FindContractByArtistId(SelectedArtistId));
    UpdateSelectionVisuals();
}

void UActiveContractsWidget::ShowForArtist(const FString& ArtistId)
{
    RefreshContracts();

    if (!ArtistId.IsEmpty())
    {
        SelectArtist(ArtistId);
    }
}

void UActiveContractsWidget::SelectArtist(const FString& ArtistId)
{
    SelectedArtistId = ArtistId;

    if (UArtistManagerSubsystem* ArtistSubsystem = GetArtistSubsystem())
    {
        if (!SelectedArtistId.IsEmpty())
        {
            ArtistSubsystem->SetSelectedArtist(SelectedArtistId);
        }
    }

    UpdateDetailPanel(FindContractByArtistId(SelectedArtistId));
    UpdateSelectionVisuals();
}

void UActiveContractsWidget::HandleCloseClicked()
{
    OnCloseRequested.Broadcast();
}

void UActiveContractsWidget::HandleViewArtistClicked()
{
    if (UArtistManagerSubsystem* ArtistSubsystem = GetArtistSubsystem())
    {
        if (!SelectedArtistId.IsEmpty())
        {
            ArtistSubsystem->SetSelectedArtist(SelectedArtistId);
        }
    }
}

void UActiveContractsWidget::HandleContractSelected(FString ArtistId)
{
    SelectArtist(ArtistId);
}

void UActiveContractsWidget::HandleArtistSigned(const FArtistContract& SignedContract)
{
    SelectedArtistId = SignedContract.ArtistId;
    RefreshContracts();
}

void UActiveContractsWidget::HandleContractExpired(const FArtistContract& ExpiredContract)
{
    if (SelectedArtistId == ExpiredContract.ArtistId)
    {
        SelectedArtistId.Reset();
    }
    RefreshContracts();
}

void UActiveContractsWidget::HandleContractsUpdated(const TArray<FArtistContract>& UpdatedContracts)
{
    CachedContracts = UpdatedContracts;
    RebuildList(CachedContracts);
    UpdateDetailPanel(FindContractByArtistId(SelectedArtistId));
    UpdateSelectionVisuals();
}

void UActiveContractsWidget::RebuildList(const TArray<FArtistContract>& Contracts)
{
    if (!ContractScrollBox)
    {
        return;
    }

    ContractScrollBox->ClearChildren();
    SpawnedItems.Reset();

    if (!ItemClass)
    {
        ItemClass = UActiveContractItemWidget::StaticClass();
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (const FArtistContract& Contract : Contracts)
    {
        UActiveContractItemWidget* Item = CreateWidget<UActiveContractItemWidget>(World, ItemClass);
        if (!Item)
        {
            continue;
        }

        Item->SetupContractItem(Contract);
        Item->OnContractSelected.RemoveDynamic(this, &UActiveContractsWidget::HandleContractSelected);
        Item->OnContractSelected.AddDynamic(this, &UActiveContractsWidget::HandleContractSelected);
        ContractScrollBox->AddChild(Item);
        SpawnedItems.Add(Item);
    }

    if (EmptyStateText)
    {
        EmptyStateText->SetVisibility(Contracts.Num() == 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    SetTextSafe(HeaderCountText, FString::Printf(TEXT("%d active"), Contracts.Num()));
}

void UActiveContractsWidget::UpdateDetailPanel(const FArtistContract* Contract)
{
    if (!Contract)
    {
        SetTextSafe(DetailArtistNameText, TEXT("No active contracts"));
        SetTextSafe(DetailGenreText, TEXT("Sign an artist to create the first contract."));
        SetTextSafe(DetailStatusText, TEXT("NO CONTRACT"));
        SetTextSafe(DetailPeriodText, TEXT("--"));
        SetTextSafe(DetailTermsText, TEXT("--"));
        SetTextSafe(DetailRoyaltyText, TEXT("--"));
        SetTextSafe(DetailBonusText, TEXT("--"));
        SetTextSafe(DetailUpkeepText, TEXT("--"));
        SetTextSafe(DetailRevenueText, TEXT("--"));
        SetTextSafe(DetailCostText, TEXT("--"));
        SetTextSafe(DetailLastRoyaltyText, TEXT("--"));
        SetTextSafe(DetailRecordsText, TEXT("--"));
        SetTextSafe(DetailMomentumText, TEXT("--"));
        SetTextSafe(DetailProgressText, TEXT("--"));

        if (DetailTermProgressBar) DetailTermProgressBar->SetPercent(0.f);
        if (DetailProductionProgressBar) DetailProductionProgressBar->SetPercent(0.f);
        return;
    }

    SetTextSafe(DetailArtistNameText, DisplayArtistName(*Contract));
    SetTextSafe(DetailGenreText, Contract->ArtistData.Genre.IsEmpty() ? TEXT("Genre unknown") : Contract->ArtistData.Genre);
    SetTextSafe(DetailStatusText, Contract->bContractActive ? TEXT("ACTIVE CONTRACT") : TEXT("INACTIVE CONTRACT"));
    SetTextSafe(DetailPeriodText, FString::Printf(TEXT("%s - %s"), *FormatDate(Contract->StartDate), *FormatDate(Contract->EndDate)));
    SetTextSafe(DetailTermsText, FString::Printf(TEXT("%d years  |  %d records"), Contract->Terms.ContractYears, Contract->Terms.NumRecords));
    SetTextSafe(DetailRoyaltyText, FormatPercent(Contract->Terms.RoyaltyRate));
    SetTextSafe(DetailBonusText, FormatCurrency(Contract->Terms.SignUpBonus));
    SetTextSafe(DetailUpkeepText, FormatCurrency(Contract->MonthlyUpkeepCost));
    SetTextSafe(DetailRevenueText, FormatCurrency(Contract->LifetimeRevenue));
    SetTextSafe(DetailCostText, FormatCurrency(Contract->LifetimeCost));
    SetTextSafe(DetailLastRoyaltyText, FormatCurrency(Contract->LastRoyaltyPayment));
    SetTextSafe(DetailRecordsText, FString::Printf(TEXT("%d delivered / %d committed"), Contract->RecordsDelivered, Contract->Terms.NumRecords));
    SetTextSafe(DetailMomentumText, FString::Printf(TEXT("%d momentum"), FMath::RoundToInt(Contract->PerformanceMomentum)));
    SetTextSafe(DetailProgressText, FString::Printf(TEXT("%d%% production"), FMath::RoundToInt(ProductionProgress(*Contract) * 100.f)));

    if (DetailTermProgressBar)
    {
        DetailTermProgressBar->SetPercent(TermProgress(*Contract));
    }

    if (DetailProductionProgressBar)
    {
        DetailProductionProgressBar->SetPercent(ProductionProgress(*Contract));
    }
}

void UActiveContractsWidget::UpdateSelectionVisuals()
{
    for (UActiveContractItemWidget* Item : SpawnedItems)
    {
        if (Item)
        {
            Item->SetSelected(Item->GetArtistId() == SelectedArtistId);
        }
    }
}

void UActiveContractsWidget::SetTextSafe(UTextBlock* TextBlock, const FString& Value) const
{
    if (TextBlock)
    {
        TextBlock->SetText(FText::FromString(Value));
    }
}

void UActiveContractsWidget::BindSubsystemEvents()
{
    if (UArtistManagerSubsystem* ArtistSubsystem = GetArtistSubsystem())
    {
        ArtistSubsystem->OnArtistSigned.RemoveDynamic(this, &UActiveContractsWidget::HandleArtistSigned);
        ArtistSubsystem->OnArtistSigned.AddDynamic(this, &UActiveContractsWidget::HandleArtistSigned);

        ArtistSubsystem->OnContractExpired.RemoveDynamic(this, &UActiveContractsWidget::HandleContractExpired);
        ArtistSubsystem->OnContractExpired.AddDynamic(this, &UActiveContractsWidget::HandleContractExpired);

        ArtistSubsystem->OnMonthlyFinancialUpdate.RemoveDynamic(this, &UActiveContractsWidget::HandleContractsUpdated);
        ArtistSubsystem->OnMonthlyFinancialUpdate.AddDynamic(this, &UActiveContractsWidget::HandleContractsUpdated);
    }
}

void UActiveContractsWidget::UnbindSubsystemEvents()
{
    if (UArtistManagerSubsystem* ArtistSubsystem = GetArtistSubsystem())
    {
        ArtistSubsystem->OnArtistSigned.RemoveDynamic(this, &UActiveContractsWidget::HandleArtistSigned);
        ArtistSubsystem->OnContractExpired.RemoveDynamic(this, &UActiveContractsWidget::HandleContractExpired);
        ArtistSubsystem->OnMonthlyFinancialUpdate.RemoveDynamic(this, &UActiveContractsWidget::HandleContractsUpdated);
    }
}

const FArtistContract* UActiveContractsWidget::FindContractByArtistId(const FString& ArtistId) const
{
    if (ArtistId.IsEmpty())
    {
        return nullptr;
    }

    return CachedContracts.FindByPredicate([&ArtistId](const FArtistContract& Contract)
    {
        return Contract.ArtistId == ArtistId || Contract.ArtistData.ArtistName == ArtistId;
    });
}

UArtistManagerSubsystem* UActiveContractsWidget::GetArtistSubsystem() const
{
    UGameInstance* GameInstance = GetGameInstance();
    return GameInstance ? GameInstance->GetSubsystem<UArtistManagerSubsystem>() : nullptr;
}
