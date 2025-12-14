#include "FinanceManagerSubsystem.h"
#include "RecordManagerSubsystem.h"

void UFinanceManagerSubsystem::RegisterTransaction(const FCashFlowEntry& Entry)
{
    FLabelAccount& Account = LabelAccounts.FindOrAdd(Entry.LabelId);
    Account.LabelId = Entry.LabelId;

    Account.CurrentBalance += Entry.Amount;
    Account.Ledger.Add(Entry);
}

float UFinanceManagerSubsystem::GetLabelBalance(const FString& LabelId) const
{
    if (const FLabelAccount* Account = LabelAccounts.Find(LabelId))
    {
        return Account->CurrentBalance;
    }
    return 0.f;
}

void UFinanceManagerSubsystem::GetLabelLedger(const FString& LabelId, TArray<FCashFlowEntry>& OutEntries) const
{
    if (const FLabelAccount* Account = LabelAccounts.Find(LabelId))
    {
        OutEntries = Account->Ledger;
    }
}

void UFinanceManagerSubsystem::RegisterRecordSalesRevenue(const FString& LabelId, const FString& RecordId, float Amount, const FDateTime& Timestamp)
{
    FCashFlowEntry Entry;
    Entry.LabelId = LabelId;
    Entry.Type = ETransactionType::RecordSales;
    Entry.Amount = Amount;
    Entry.Timestamp = Timestamp;
    Entry.RefId = RecordId;

    RegisterTransaction(Entry);
}

void UFinanceManagerSubsystem::HandleMonthAdvanced(const FDateTime& NewDate)
{
    // Reserved for future monthly finance processes; intentionally empty.
    (void)NewDate;
}

void UFinanceManagerSubsystem::ProcessRecordSalesEntries(const TArray<FRecordSalesEntry>& Entries, const TMap<ERecordFormat, FRecordFormatRule>& FormatRules, const TMap<FString, FRecordData>& RecordDataById)
{
    for (const FRecordSalesEntry& Entry : Entries)
    {
        const FRecordData* Record = RecordDataById.Find(Entry.RecordId);
        if (!Record)
        {
            continue;
        }

        const FRecordFormatRule* Rule = FormatRules.Find(Entry.Format);
        if (!Rule)
        {
            continue;
        }

        const float GrossRevenue = Entry.UnitsSold * Rule->BasePrice;
        const float DistributionCost = GrossRevenue * Rule->CostRate;
        const float RoyaltyPayment = GrossRevenue * 0.12f; // Simple default royalty until label terms are modeled.

        // Gross revenue booking.
        RegisterRecordSalesRevenue(Record->LabelId, Entry.RecordId, GrossRevenue, Entry.Month);

        // Distribution / manufacturing expense.
        FCashFlowEntry CostEntry;
        CostEntry.LabelId = Record->LabelId;
        CostEntry.Type = ETransactionType::GenericExpense;
        CostEntry.Amount = -DistributionCost;
        CostEntry.Timestamp = Entry.Month;
        CostEntry.RefId = FString::Printf(TEXT("%s-%s-%d"), *Entry.RecordId, *Entry.MarketId, static_cast<int32>(Entry.Format));
        RegisterTransaction(CostEntry);

        // Royalty accrual.
        FCashFlowEntry RoyaltyEntry;
        RoyaltyEntry.LabelId = Record->LabelId;
        RoyaltyEntry.Type = ETransactionType::RoyaltyPayment;
        RoyaltyEntry.Amount = -RoyaltyPayment;
        RoyaltyEntry.Timestamp = Entry.Month;
        RoyaltyEntry.RefId = Entry.RecordId;
        RegisterTransaction(RoyaltyEntry);
    }
}
