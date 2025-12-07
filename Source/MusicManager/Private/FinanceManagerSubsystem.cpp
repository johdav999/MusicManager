#include "FinanceManagerSubsystem.h"

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
