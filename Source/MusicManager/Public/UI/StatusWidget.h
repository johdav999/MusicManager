#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "GameTimeSubsystem.h"
#include "StatusWidget.generated.h"

class UFinanceManagerSubsystem;
class UUIManagerSubsystem;

/**
 * Lightweight status HUD widget that displays current date and financial snapshots.
 */
UCLASS()
class MUSICMANAGER_API UStatusWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /** Refreshes the displayed status values using the supplied date. Safe to call repeatedly. */
    UFUNCTION()
    void RefreshStatus(const FDateTime& CurrentDate);

protected:
    /** Text block bound in the designer for displaying the formatted date. */
    UPROPERTY(meta = (BindWidget))
    UTextBlock* DateText;

    /** Text block bound in the designer for showing last month's profit (signed currency). */
    UPROPERTY(meta = (BindWidget))
    UTextBlock* MonthlyProfitText;

    /** Text block bound in the designer for showing current cash balance (currency). */
    UPROPERTY(meta = (BindWidget))
    UTextBlock* CashBalanceText;

    /** Optional background image configured in the designer. */
    UPROPERTY(meta = (BindWidgetOptional))
    UImage* BackgroundImage;

private:
    UFUNCTION()
    void HandleWeekAdvanced(const FDateTime& PreviousDate, const FDateTime& NewDate);
    UFUNCTION()
    void HandleMonthAdvanced(const FDateTime& NewDate);
    UFUNCTION()
    void HandleTimeBatchAdvanced(int32 WeeksAdvanced, const FDateTime& NewDate);
    UFUNCTION()
    void HandleCurrentLabelChanged(const FString& NewLabelId);

    FString ResolveLabelId();
    UGameTimeSubsystem* GetTimeSubsystem();
    UFinanceManagerSubsystem* GetFinanceSubsystem();
    UUIManagerSubsystem* GetUIManagerSubsystem();

    static FText FormatMonthYear(const FDateTime& Date);
    static FText FormatCurrency(float Value, bool bForceSign, const FString& CurrencySymbol, bool bAlwaysPositive);

    TWeakObjectPtr<UGameTimeSubsystem> CachedTimeSubsystem;
    TWeakObjectPtr<UFinanceManagerSubsystem> CachedFinanceSubsystem;
    TWeakObjectPtr<UUIManagerSubsystem> CachedUIManagerSubsystem;
};
