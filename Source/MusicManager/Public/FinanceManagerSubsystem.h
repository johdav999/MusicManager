#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FinanceManagerSubsystem.generated.h"

struct FRecordSalesEntry;
struct FRecordFormatRule;
struct FRecordData;
struct FFinanceSnapshot;
struct FMusicSaveValidationResult;
enum class ERecordFormat : uint8;

UENUM(BlueprintType)
enum class ETransactionType : uint8
{
    RecordSales,
    RecordingCost,
    MarketingCost,
    TourRevenue,
    TourCost,
    RoyaltyPayment,
    GenericExpense,
    GenericIncome
};

USTRUCT(BlueprintType)
struct FCashFlowEntry
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString LabelId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETransactionType Type;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Amount = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime Timestamp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RefId;
};

USTRUCT(BlueprintType)
struct FLabelAccount
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString LabelId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CurrentBalance = 0.f;

    UPROPERTY()
    TArray<FCashFlowEntry> Ledger;
};

USTRUCT(BlueprintType)
struct FMonthlyFinanceSummary
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString LabelId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Year = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Month = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime PeriodStart;

    /** Exclusive first day of the next month. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime PeriodEnd;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float IncomeTotal = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ExpenseTotal = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float NetTotal = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<ETransactionType, float> CategoryTotals;
};

UCLASS()
class MUSICMANAGER_API UFinanceManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION()
    void RegisterTransaction(const FCashFlowEntry& Entry);

    UFUNCTION(BlueprintCallable)
    float GetLabelBalance(const FString& LabelId) const;

    UFUNCTION(BlueprintCallable)
    void GetLabelLedger(const FString& LabelId, TArray<FCashFlowEntry>& OutEntries) const;

    // Returns the net profit for the previous calendar month based on transaction timestamps.
    UFUNCTION(BlueprintCallable)
    float GetLastMonthProfit(const FString& LabelId, const FDateTime& CurrentDate) const;

    UFUNCTION(BlueprintCallable)
    bool GetMonthlyFinanceSummary(const FString& LabelId, int32 Year, int32 Month, FMonthlyFinanceSummary& OutSummary) const;

    UFUNCTION(BlueprintCallable)
    void GetMonthlyFinanceSummaries(const FString& LabelId, TArray<FMonthlyFinanceSummary>& OutSummaries) const;

    // Returns the accumulated cash balance for the specified label.
    UFUNCTION(BlueprintCallable)
    float GetAccumulatedCash(const FString& LabelId) const;

    /** Closes a month as a reporting snapshot without driving monthly-only simulation. */
    void HandleMonthClosed(int32 ClosedYear, int32 ClosedMonth, const FDateTime& PeriodStart, const FDateTime& PeriodEnd);

    UFUNCTION()
    void RegisterRecordSalesRevenue(const FString& LabelId, const FString& RecordId, float Amount, const FDateTime& Timestamp);

    /** Placeholder for month-end finance hooks (interest, accruals, etc.). */
    void HandleMonthAdvanced(const FDateTime& NewDate);

    /** Book ledger entries for monthly sales without altering unit demand. */
    void ProcessRecordSalesEntries(const TArray<FRecordSalesEntry>& Entries, const TMap<ERecordFormat, FRecordFormatRule>& FormatRules, const TMap<FString, FRecordData>& RecordDataById);

    void BuildSaveSnapshot(FFinanceSnapshot& OutSnapshot) const;
    void ValidateSaveSnapshot(const FFinanceSnapshot& Snapshot, const TSet<FString>& KnownLabelIds, FMusicSaveValidationResult& Result) const;
    void ApplySaveSnapshot(const FFinanceSnapshot& Snapshot);

private:
    UPROPERTY()
    TMap<FString, FLabelAccount> LabelAccounts;

    UPROPERTY()
    TArray<FMonthlyFinanceSummary> MonthlySummaries;

    UPROPERTY()
    TSet<FString> ClosedMonthlySummaryKeys;

    FString BuildMonthlySummaryKey(const FString& LabelId, int32 Year, int32 Month) const;
    FMonthlyFinanceSummary BuildMonthlySummaryForLabel(const FString& LabelId, int32 Year, int32 Month, const FDateTime& PeriodStart, const FDateTime& PeriodEnd) const;
};
