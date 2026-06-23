#include "UI/StatusWidget.h"

#include "Async/Async.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "FinanceManagerSubsystem.h"
#include "UIManagerSubsystem.h"

void UStatusWidget::NativeConstruct()
{
    ensure(IsInGameThread());

    UE_LOG(LogTemp, Verbose, TEXT("StatusWidget constructed"));

    Super::NativeConstruct();

    CachedTimeSubsystem = GetTimeSubsystem();
    CachedFinanceSubsystem = GetFinanceSubsystem();
    CachedUIManagerSubsystem = GetUIManagerSubsystem();

    if (UUIManagerSubsystem* UIManager = CachedUIManagerSubsystem.Get())
    {
        UIManager->RegisterStatusWidget(this);

        if (UIManager->OnCurrentLabelChanged.IsAlreadyBound(this, &UStatusWidget::HandleCurrentLabelChanged))
        {
            UIManager->OnCurrentLabelChanged.RemoveDynamic(this, &UStatusWidget::HandleCurrentLabelChanged);
        }

        UIManager->OnCurrentLabelChanged.AddDynamic(this, &UStatusWidget::HandleCurrentLabelChanged);
    }

    if (UGameTimeSubsystem* TimeSys = CachedTimeSubsystem.Get())
    {
        if (TimeSys->OnWeekAdvanced.IsAlreadyBound(this, &UStatusWidget::HandleWeekAdvanced))
        {
            TimeSys->OnWeekAdvanced.RemoveDynamic(this, &UStatusWidget::HandleWeekAdvanced);
        }

        if (TimeSys->OnMonthAdvanced.IsAlreadyBound(this, &UStatusWidget::HandleMonthAdvanced))
        {
            TimeSys->OnMonthAdvanced.RemoveDynamic(this, &UStatusWidget::HandleMonthAdvanced);
        }

        if (TimeSys->OnTimeBatchAdvanced.IsAlreadyBound(this, &UStatusWidget::HandleTimeBatchAdvanced))
        {
            TimeSys->OnTimeBatchAdvanced.RemoveDynamic(this, &UStatusWidget::HandleTimeBatchAdvanced);
        }

        TimeSys->OnWeekAdvanced.AddDynamic(this, &UStatusWidget::HandleWeekAdvanced);
        TimeSys->OnMonthAdvanced.AddDynamic(this, &UStatusWidget::HandleMonthAdvanced);
        TimeSys->OnTimeBatchAdvanced.AddDynamic(this, &UStatusWidget::HandleTimeBatchAdvanced);
        HandleMonthAdvanced(TimeSys->GetCurrentGameDate());
    }
}

void UStatusWidget::NativeDestruct()
{
    ensure(IsInGameThread());

    if (UGameTimeSubsystem* TimeSys = CachedTimeSubsystem.Get())
    {
        if (TimeSys->OnWeekAdvanced.IsAlreadyBound(this, &UStatusWidget::HandleWeekAdvanced))
        {
            TimeSys->OnWeekAdvanced.RemoveDynamic(this, &UStatusWidget::HandleWeekAdvanced);
        }

        if (TimeSys->OnMonthAdvanced.IsAlreadyBound(this, &UStatusWidget::HandleMonthAdvanced))
        {
            TimeSys->OnMonthAdvanced.RemoveDynamic(this, &UStatusWidget::HandleMonthAdvanced);
        }

        if (TimeSys->OnTimeBatchAdvanced.IsAlreadyBound(this, &UStatusWidget::HandleTimeBatchAdvanced))
        {
            TimeSys->OnTimeBatchAdvanced.RemoveDynamic(this, &UStatusWidget::HandleTimeBatchAdvanced);
        }
    }

    if (UUIManagerSubsystem* UIManager = CachedUIManagerSubsystem.Get())
    {
        UIManager->UnregisterStatusWidget(this);

        if (UIManager->OnCurrentLabelChanged.IsAlreadyBound(this, &UStatusWidget::HandleCurrentLabelChanged))
        {
            UIManager->OnCurrentLabelChanged.RemoveDynamic(this, &UStatusWidget::HandleCurrentLabelChanged);
        }
    }

    Super::NativeDestruct();
}

void UStatusWidget::HandleWeekAdvanced(const FDateTime& PreviousDate, const FDateTime& NewDate)
{
    UE_LOG(LogTemp, Verbose, TEXT("StatusWidget received week advanced: %s -> %s"),
        *PreviousDate.ToString(),
        *NewDate.ToString());

    if (!IsInGameThread())
    {
        const TWeakObjectPtr<UStatusWidget> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, PreviousDate, NewDate]()
        {
            if (UStatusWidget* StrongThis = WeakThis.Get())
            {
                StrongThis->HandleWeekAdvanced(PreviousDate, NewDate);
            }
        });
        return;
    }

    if (UGameTimeSubsystem* TimeSubsystem = GetTimeSubsystem(); TimeSubsystem && TimeSubsystem->IsBatchAdvancing())
    {
        return;
    }

    RefreshStatus(NewDate);
}

void UStatusWidget::HandleMonthAdvanced(const FDateTime& NewDate)
{
    UE_LOG(LogTemp, Verbose, TEXT("StatusWidget received month advanced: %s"), *NewDate.ToString());

    if (!IsInGameThread())
    {
        const TWeakObjectPtr<UStatusWidget> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, NewDate]()
        {
            if (UStatusWidget* StrongThis = WeakThis.Get())
            {
                StrongThis->HandleMonthAdvanced(NewDate);
            }
        });
        return;
    }

    if (UGameTimeSubsystem* TimeSubsystem = GetTimeSubsystem(); TimeSubsystem && TimeSubsystem->IsBatchAdvancing())
    {
        return;
    }

    RefreshStatus(NewDate);
}

void UStatusWidget::HandleTimeBatchAdvanced(int32 WeeksAdvanced, const FDateTime& NewDate)
{
    UE_LOG(LogTemp, Verbose, TEXT("StatusWidget received batch time advancement: Weeks=%d Date=%s"),
        WeeksAdvanced,
        *NewDate.ToString());

    if (!IsInGameThread())
    {
        const TWeakObjectPtr<UStatusWidget> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, WeeksAdvanced, NewDate]()
        {
            if (UStatusWidget* StrongThis = WeakThis.Get())
            {
                StrongThis->HandleTimeBatchAdvanced(WeeksAdvanced, NewDate);
            }
        });
        return;
    }

    RefreshStatus(NewDate);
}

void UStatusWidget::RefreshStatus(const FDateTime& CurrentDate)
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<UStatusWidget> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, CurrentDate]()
        {
            if (UStatusWidget* StrongThis = WeakThis.Get())
            {
                StrongThis->RefreshStatus(CurrentDate);
            }
        });
        return;
    }

    ensure(DateText);
    ensure(MonthlyProfitText);
    ensure(CashBalanceText);

    if (DateText)
    {
        DateText->SetText(FormatMonthYear(CurrentDate));
    }

    const FString LabelId = ResolveLabelId();
    if (LabelId.IsEmpty())
    {
        UE_LOG(LogTemp, Verbose, TEXT("StatusWidget skipping finance refresh: label is empty for date %s"), *CurrentDate.ToString());
        if (MonthlyProfitText)
        {
            MonthlyProfitText->SetText(FText::FromString(TEXT("--")));
        }

        if (CashBalanceText)
        {
            CashBalanceText->SetText(FText::FromString(TEXT("--")));
        }

        return;
    }

    float LastMonthProfit = 0.f;
    float Balance = 0.f;

    if (UFinanceManagerSubsystem* FinanceSubsystem = GetFinanceSubsystem())
    {
        LastMonthProfit = FinanceSubsystem->GetLastMonthProfit(LabelId, CurrentDate);
        Balance = FinanceSubsystem->GetLabelBalance(LabelId);
    }
    else
    {
        ensureMsgf(false, TEXT("FinanceManagerSubsystem missing"));
    }

    if (MonthlyProfitText)
    {
        MonthlyProfitText->SetText(FormatCurrency(LastMonthProfit, true, TEXT(""), false));
    }

    if (CashBalanceText)
    {
        CashBalanceText->SetText(FormatCurrency(Balance, false, TEXT("€"), true));
    }

    UE_LOG(LogTemp, Verbose, TEXT("StatusWidget finance updated: Profit=%s, Cash=%s"),
        *FormatCurrency(LastMonthProfit, true, TEXT(""), false).ToString(),
        *FormatCurrency(Balance, false, TEXT("€"), true).ToString());
}

FString UStatusWidget::ResolveLabelId()
{
    if (UUIManagerSubsystem* UIManager = CachedUIManagerSubsystem.Get())
    {
        const FString LabelId = UIManager->GetCurrentLabelId();
        if (!LabelId.IsEmpty())
        {
            UE_LOG(LogTemp, Verbose, TEXT("StatusWidget ResolveLabelId using cached manager: %s"), *LabelId);
            return LabelId;
        }
    }

    if (UUIManagerSubsystem* UIManager = GetUIManagerSubsystem())
    {
        const FString LabelId = UIManager->GetCurrentLabelId();
        if (!LabelId.IsEmpty())
        {
            CachedUIManagerSubsystem = UIManager;
            UE_LOG(LogTemp, Verbose, TEXT("StatusWidget ResolveLabelId refreshed cache: %s"), *LabelId);
            return LabelId;
        }
    }

    UE_LOG(LogTemp, Verbose, TEXT("StatusWidget ResolveLabelId could not resolve a label id."));
    return FString();
}

void UStatusWidget::HandleCurrentLabelChanged(const FString& NewLabelId)
{
    UE_LOG(LogTemp, Verbose, TEXT("StatusWidget observed label change: %s"), *NewLabelId);

    if (UGameTimeSubsystem* TimeSubsystem = GetTimeSubsystem())
    {
        RefreshStatus(TimeSubsystem->GetCurrentGameDate());
    }
}

UGameTimeSubsystem* UStatusWidget::GetTimeSubsystem()
{
    if (UGameTimeSubsystem* Cached = CachedTimeSubsystem.Get())
    {
        return Cached;
    }

    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            CachedTimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>();
        }
    }

    return CachedTimeSubsystem.Get();
}

UFinanceManagerSubsystem* UStatusWidget::GetFinanceSubsystem()
{
    if (UFinanceManagerSubsystem* Cached = CachedFinanceSubsystem.Get())
    {
        return Cached;
    }

    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            CachedFinanceSubsystem = GameInstance->GetSubsystem<UFinanceManagerSubsystem>();
        }
    }

    return CachedFinanceSubsystem.Get();
}

UUIManagerSubsystem* UStatusWidget::GetUIManagerSubsystem()
{
    if (UUIManagerSubsystem* Cached = CachedUIManagerSubsystem.Get())
    {
        return Cached;
    }

    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            CachedUIManagerSubsystem = GameInstance->GetSubsystem<UUIManagerSubsystem>();
        }
    }

    return CachedUIManagerSubsystem.Get();
}

FText UStatusWidget::FormatMonthYear(const FDateTime& Date)
{
    static const FString MonthNames[12] = {
        TEXT("January"), TEXT("February"), TEXT("March"),
        TEXT("April"), TEXT("May"), TEXT("June"),
        TEXT("July"), TEXT("August"), TEXT("September"),
        TEXT("October"), TEXT("November"), TEXT("December")
    };

    const int32 Month = Date.GetMonth();
    const int32 Year = Date.GetYear();

    const FString MonthString = (Month >= 1 && Month <= 12)
        ? MonthNames[Month - 1]
        : TEXT("Unknown");

    return FText::FromString(FString::Printf(TEXT("%s %d"), *MonthString, Year));
}

FText UStatusWidget::FormatCurrency(float Value, bool bForceSign, const FString& CurrencySymbol, bool bAlwaysPositive)
{
    FNumberFormattingOptions FormatOptions;
    FormatOptions.MinimumIntegralDigits = 1;
    FormatOptions.MaximumFractionalDigits = 2;
    FormatOptions.MinimumFractionalDigits = 2;

    const float DisplayValue = bAlwaysPositive ? FMath::Abs(Value) : Value;
    const FString NumberString = FText::AsNumber(FMath::Abs(DisplayValue), &FormatOptions).ToString();

    FString SignString;
    if (bForceSign)
    {
        SignString = (DisplayValue >= 0.f) ? TEXT("+") : TEXT("-");
    }
    else if (DisplayValue < 0.f)
    {
        SignString = TEXT("-");
    }

    return FText::FromString(FString::Printf(TEXT("%s%s%s"), *SignString, *CurrencySymbol, *NumberString));
}
