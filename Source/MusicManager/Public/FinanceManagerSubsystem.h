#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FinanceManagerSubsystem.generated.h"

struct FRecordSalesEntry;
struct FRecordFormatRule;
struct FRecordData;
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

    // Returns the accumulated cash balance for the specified label.
    UFUNCTION(BlueprintCallable)
    float GetAccumulatedCash(const FString& LabelId) const;

    UFUNCTION()
    void RegisterRecordSalesRevenue(const FString& LabelId, const FString& RecordId, float Amount, const FDateTime& Timestamp);

    /** Placeholder for month-end finance hooks (interest, accruals, etc.). */
    void HandleMonthAdvanced(const FDateTime& NewDate);

    /** Book ledger entries for monthly sales without altering unit demand. */
    void ProcessRecordSalesEntries(const TArray<FRecordSalesEntry>& Entries, const TMap<ERecordFormat, FRecordFormatRule>& FormatRules, const TMap<FString, FRecordData>& RecordDataById);

private:
    UPROPERTY()
    TMap<FString, FLabelAccount> LabelAccounts;
};
