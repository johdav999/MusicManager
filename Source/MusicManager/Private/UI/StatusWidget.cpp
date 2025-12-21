#include "UI/StatusWidget.h"

#include "Async/Async.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "FinanceManagerSubsystem.h"
#include "UIManagerSubsystem.h"

void UStatusWidget::NativeConstruct()
{
    ensure(IsInGameThread());

    Super::NativeConstruct();

    CachedTimeSubsystem = GetTimeSubsystem();
    CachedFinanceSubsystem = GetFinanceSubsystem();
    CachedUIManagerSubsystem = GetUIManagerSubsystem();

    if (UUIManagerSubsystem* UIManager = CachedUIManagerSubsystem.Get())
    {
        UIManager->RegisterStatusWidget(this);
    }

    if (UGameTimeSubsystem* TimeSys = CachedTimeSubsystem.Get())
    {
        TimeSys->OnMonthAdvanced.AddDynamic(this, &UStatusWidget::HandleMonthAdvanced);
        HandleMonthAdvanced(TimeSys->GetCurrentGameDate());
    }
}

void UStatusWidget::NativeDestruct()
{
    ensure(IsInGameThread());

    if (UGameTimeSubsystem* TimeSys = CachedTimeSubsystem.Get())
    {
        TimeSys->OnMonthAdvanced.RemoveDynamic(this, &UStatusWidget::HandleMonthAdvanced);
    }

    if (UUIManagerSubsystem* UIManager = CachedUIManagerSubsystem.Get())
    {
        UIManager->UnregisterStatusWidget(this);
    }

    Super::NativeDestruct();
}

void UStatusWidget::HandleMonthAdvanced(const FDateTime& NewDate)
{
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

    if (DateText)
    {
        DateText->SetText(FormatMonthYear(CurrentDate));
    }

    const FString LabelId = ResolveLabelId();
    if (LabelId.IsEmpty())
    {
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

    if (MonthlyProfitText)
    {
        MonthlyProfitText->SetText(FormatCurrency(LastMonthProfit, true));
    }

    if (CashBalanceText)
    {
        CashBalanceText->SetText(FormatCurrency(Balance, false));
    }
}

FString UStatusWidget::ResolveLabelId()
{
    if (UUIManagerSubsystem* UIManager = CachedUIManagerSubsystem.Get())
    {
        const FString LabelId = UIManager->GetCurrentLabelId();
        if (!LabelId.IsEmpty())
        {
            return LabelId;
        }
    }

    if (UUIManagerSubsystem* UIManager = GetUIManagerSubsystem())
    {
        const FString LabelId = UIManager->GetCurrentLabelId();
        if (!LabelId.IsEmpty())
        {
            CachedUIManagerSubsystem = UIManager;
            return LabelId;
        }
    }

    return FString();
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

FText UStatusWidget::FormatCurrency(float Value, bool bForceSign)
{
    FNumberFormattingOptions FormatOptions;
    FormatOptions.MinimumIntegralDigits = 1;
    FormatOptions.MaximumFractionalDigits = 2;
    FormatOptions.MinimumFractionalDigits = 2;

    const FString NumberString = FText::AsNumber(FMath::Abs(Value), &FormatOptions).ToString();

    FString SignString;
    if (bForceSign)
    {
        SignString = (Value >= 0.f) ? TEXT("+") : TEXT("-");
    }
    else if (Value < 0.f)
    {
        SignString = TEXT("-");
    }

    return FText::FromString(FString::Printf(TEXT("%s$%s"), *SignString, *NumberString));
}
