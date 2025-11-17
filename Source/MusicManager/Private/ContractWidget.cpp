#include "ContractWidget.h"

#include "Components/TextBlock.h"

namespace
{
    /** Utility to safely write text to bound widgets without risking invalid pointer access. */
    void SetTextSafe(UTextBlock* TextBlock, const FString& Value)
    {
        if (IsValid(TextBlock))
        {
            TextBlock->SetText(FText::FromString(Value));
        }
    }
}

void UContractWidget::SetContractData(const FArtistContract& Contract)
{
    ContractData = Contract;

    SetTextSafe(TextArtistName, ContractData.ArtistData.ArtistName);
    SetTextSafe(TextContractYears, LexToString(ContractData.Terms.ContractYears));
    SetTextSafe(TextNumRecords, LexToString(ContractData.Terms.NumRecords));
    SetTextSafe(TextRoyaltyRate, FString::SanitizeFloat(ContractData.Terms.RoyaltyRate));
    SetTextSafe(TextSignUpBonus, FString::SanitizeFloat(ContractData.Terms.SignUpBonus));
    SetTextSafe(TextStartDate, ContractData.StartDate.ToString());
    SetTextSafe(TextEndDate, ContractData.EndDate.ToString());
    SetTextSafe(TextRecordsDelivered, LexToString(ContractData.RecordsDelivered));
    //SetTextSafe(TextContractActive, ContractData.bContractActive ? TEXT("True") : TEXT("False"));
    //SetTextSafe(TextLifetimeRevenue, FString::SanitizeFloat(ContractData.LifetimeRevenue));
    //SetTextSafe(TextLifetimeCost, FString::SanitizeFloat(ContractData.LifetimeCost));
    //SetTextSafe(TextLastRoyaltyPayment, FString::SanitizeFloat(ContractData.LastRoyaltyPayment));
    //SetTextSafe(TextMonthlyUpkeepCost, FString::SanitizeFloat(ContractData.MonthlyUpkeepCost));
    //SetTextSafe(TextCumulativeRoyaltyPaid, FString::SanitizeFloat(ContractData.CumulativeRoyaltyPaid));
    //SetTextSafe(TextMonthsActive, LexToString(ContractData.MonthsActive));
    //SetTextSafe(TextPerformanceMomentum, FString::SanitizeFloat(ContractData.PerformanceMomentum));
    //SetTextSafe(TextProductionProgress, FString::SanitizeFloat(ContractData.ProductionProgress));
}
