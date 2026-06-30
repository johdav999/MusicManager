#include "FinanceManagerSubsystem.h"
#include "MusicSaveGame.h"
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

float UFinanceManagerSubsystem::GetLastMonthProfit(const FString& LabelId, const FDateTime& CurrentDate) const
{
    const FLabelAccount* Account = LabelAccounts.Find(LabelId);
    if (!Account)
    {
        UE_LOG(LogTemp, Warning, TEXT("GetLastMonthProfit: Label %s not found."), *LabelId);
        return 0.f;
    }

    // Start of current month
    const FDateTime StartOfCurrentMonth(CurrentDate.GetYear(), CurrentDate.GetMonth(), 1);

    // Compute previous month manually
    int32 PrevYear = StartOfCurrentMonth.GetYear();
    int32 PrevMonth = StartOfCurrentMonth.GetMonth() - 1;

    if (PrevMonth == 0)
    {
        PrevMonth = 12;
        PrevYear -= 1;
    }

    const FDateTime StartOfPreviousMonth(PrevYear, PrevMonth, 1);

    FMonthlyFinanceSummary CachedSummary;
    if (GetMonthlyFinanceSummary(LabelId, PrevYear, PrevMonth, CachedSummary))
    {
        return CachedSummary.NetTotal;
    }

    // Compatibility fallback for consumers that query before the first explicit month close.

    float Profit = 0.f;
    for (const FCashFlowEntry& Entry : Account->Ledger)
    {
        if (Entry.Timestamp >= StartOfPreviousMonth && Entry.Timestamp < StartOfCurrentMonth)
        {
            Profit += Entry.Amount;
        }
    }

    return Profit;
}

bool UFinanceManagerSubsystem::GetMonthlyFinanceSummary(const FString& LabelId, int32 Year, int32 Month, FMonthlyFinanceSummary& OutSummary) const
{
    const FString SummaryKey = BuildMonthlySummaryKey(LabelId, Year, Month);
    for (const FMonthlyFinanceSummary& Summary : MonthlySummaries)
    {
        if (BuildMonthlySummaryKey(Summary.LabelId, Summary.Year, Summary.Month) == SummaryKey)
        {
            OutSummary = Summary;
            return true;
        }
    }

    return false;
}

void UFinanceManagerSubsystem::GetMonthlyFinanceSummaries(const FString& LabelId, TArray<FMonthlyFinanceSummary>& OutSummaries) const
{
    OutSummaries.Reset();

    for (const FMonthlyFinanceSummary& Summary : MonthlySummaries)
    {
        if (Summary.LabelId == LabelId)
        {
            OutSummaries.Add(Summary);
        }
    }

    OutSummaries.Sort([](const FMonthlyFinanceSummary& A, const FMonthlyFinanceSummary& B)
    {
        if (A.Year != B.Year)
        {
            return A.Year < B.Year;
        }
        return A.Month < B.Month;
    });
}

float UFinanceManagerSubsystem::GetAccumulatedCash(const FString& LabelId) const
{
    if (const FLabelAccount* Account = LabelAccounts.Find(LabelId))
    {
        return Account->CurrentBalance;
    }

    return 0.f;
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

void UFinanceManagerSubsystem::HandleMonthClosed(int32 ClosedYear, int32 ClosedMonth, const FDateTime& PeriodStart, const FDateTime& PeriodEnd)
{
    for (const TPair<FString, FLabelAccount>& Pair : LabelAccounts)
    {
        const FString SummaryKey = BuildMonthlySummaryKey(Pair.Key, ClosedYear, ClosedMonth);
        if (ClosedMonthlySummaryKeys.Contains(SummaryKey))
        {
            UE_LOG(LogTemp, Verbose, TEXT("Finance monthly summary already closed: %s"), *SummaryKey);
            continue;
        }

        MonthlySummaries.Add(BuildMonthlySummaryForLabel(Pair.Key, ClosedYear, ClosedMonth, PeriodStart, PeriodEnd));
        ClosedMonthlySummaryKeys.Add(SummaryKey);

        UE_LOG(LogTemp, Log, TEXT("Finance monthly summary closed: %s"), *SummaryKey);
    }
}

void UFinanceManagerSubsystem::HandleMonthAdvanced(const FDateTime& NewDate)
{
    const FDateTime StartOfCurrentMonth(NewDate.GetYear(), NewDate.GetMonth(), 1);
    int32 PrevYear = StartOfCurrentMonth.GetYear();
    int32 PrevMonth = StartOfCurrentMonth.GetMonth() - 1;

    if (PrevMonth == 0)
    {
        PrevMonth = 12;
        --PrevYear;
    }

    HandleMonthClosed(PrevYear, PrevMonth, FDateTime(PrevYear, PrevMonth, 1), StartOfCurrentMonth);
}

FString UFinanceManagerSubsystem::BuildMonthlySummaryKey(const FString& LabelId, int32 Year, int32 Month) const
{
    return FString::Printf(TEXT("%s:%04d-%02d"), *LabelId, Year, Month);
}

FMonthlyFinanceSummary UFinanceManagerSubsystem::BuildMonthlySummaryForLabel(const FString& LabelId, int32 Year, int32 Month, const FDateTime& PeriodStart, const FDateTime& PeriodEnd) const
{
    FMonthlyFinanceSummary Summary;
    Summary.LabelId = LabelId;
    Summary.Year = Year;
    Summary.Month = Month;
    Summary.PeriodStart = PeriodStart;
    Summary.PeriodEnd = PeriodEnd;

    if (const FLabelAccount* Account = LabelAccounts.Find(LabelId))
    {
        for (const FCashFlowEntry& Entry : Account->Ledger)
        {
            if (Entry.Timestamp >= PeriodStart && Entry.Timestamp < PeriodEnd)
            {
                Summary.CategoryTotals.FindOrAdd(Entry.Type) += Entry.Amount;
                Summary.NetTotal += Entry.Amount;
                if (Entry.Amount >= 0.f)
                {
                    Summary.IncomeTotal += Entry.Amount;
                }
                else
                {
                    Summary.ExpenseTotal += Entry.Amount;
                }
            }
        }
    }

    return Summary;
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

void UFinanceManagerSubsystem::BuildSaveSnapshot(FFinanceSnapshot& OutSnapshot) const
{
    OutSnapshot.LabelAccounts = LabelAccounts;
    OutSnapshot.MonthlySummaries = MonthlySummaries;
    OutSnapshot.ClosedMonthlySummaryKeys = ClosedMonthlySummaryKeys;
}

void UFinanceManagerSubsystem::ValidateSaveSnapshot(const FFinanceSnapshot& Snapshot, const TSet<FString>& KnownLabelIds, FMusicSaveValidationResult& Result) const
{
    for (const TPair<FString, FLabelAccount>& Pair : Snapshot.LabelAccounts)
    {
        if (Pair.Key.IsEmpty() || Pair.Value.LabelId.IsEmpty())
        {
            Result.AddError(TEXT("Finance account has an empty label id."));
            continue;
        }

        if (Pair.Key != Pair.Value.LabelId)
        {
            Result.AddError(FString::Printf(TEXT("Finance account key %s does not match account label %s."), *Pair.Key, *Pair.Value.LabelId));
        }

        if (!KnownLabelIds.Contains(Pair.Key))
        {
            Result.AddError(FString::Printf(TEXT("Finance account references unknown label %s."), *Pair.Key));
        }

        if (!FMath::IsFinite(Pair.Value.CurrentBalance))
        {
            Result.AddError(FString::Printf(TEXT("Finance account %s has invalid balance."), *Pair.Key));
        }

        for (const FCashFlowEntry& Entry : Pair.Value.Ledger)
        {
            if (Entry.LabelId != Pair.Key)
            {
                Result.AddError(FString::Printf(TEXT("Finance ledger entry label %s is stored under account %s."), *Entry.LabelId, *Pair.Key));
            }
            if (!FMath::IsFinite(Entry.Amount))
            {
                Result.AddError(FString::Printf(TEXT("Finance ledger entry for label %s has invalid amount."), *Pair.Key));
            }
            if (Entry.Timestamp.GetTicks() <= 0)
            {
                Result.AddError(FString::Printf(TEXT("Finance ledger entry for label %s has unset timestamp."), *Pair.Key));
            }
            if (!StaticEnum<ETransactionType>()->IsValidEnumValue(static_cast<int64>(Entry.Type)))
            {
                Result.AddError(FString::Printf(TEXT("Finance ledger entry for label %s has invalid transaction type."), *Pair.Key));
            }
        }
    }

    for (const FMonthlyFinanceSummary& Summary : Snapshot.MonthlySummaries)
    {
        if (Summary.LabelId.IsEmpty() || !KnownLabelIds.Contains(Summary.LabelId))
        {
            Result.AddError(FString::Printf(TEXT("Monthly finance summary references unknown label %s."), *Summary.LabelId));
        }
        if (Summary.Month < 1 || Summary.Month > 12 || Summary.Year < 1955)
        {
            Result.AddError(FString::Printf(TEXT("Monthly finance summary has invalid period %04d-%02d."), Summary.Year, Summary.Month));
        }
        if (Summary.PeriodStart.GetTicks() <= 0 || Summary.PeriodEnd.GetTicks() <= 0 || Summary.PeriodEnd <= Summary.PeriodStart)
        {
            Result.AddError(FString::Printf(TEXT("Monthly finance summary for %s has invalid date range."), *Summary.LabelId));
        }
        if (!FMath::IsNearlyEqual(Summary.NetTotal, Summary.IncomeTotal + Summary.ExpenseTotal, 0.01f))
        {
            Result.AddError(FString::Printf(TEXT("Monthly finance summary for %s has inconsistent totals."), *Summary.LabelId));
        }
    }
}

void UFinanceManagerSubsystem::ApplySaveSnapshot(const FFinanceSnapshot& Snapshot)
{
    LabelAccounts = Snapshot.LabelAccounts;
    MonthlySummaries = Snapshot.MonthlySummaries;
    ClosedMonthlySummaryKeys = Snapshot.ClosedMonthlySummaryKeys;
}
