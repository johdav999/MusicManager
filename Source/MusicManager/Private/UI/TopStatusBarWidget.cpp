#include "UI/TopStatusBarWidget.h"

#include "CommandDispatcherSubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "FinanceManagerSubsystem.h"
#include "GameTimeSubsystem.h"
#include "MusicCommandResult.h"
#include "PlayerLabelSubsystem.h"
#include "UIManagerSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogTopStatusBarWidget, Log, All);

namespace
{
    const FLinearColor PillBackgroundColor(0.025f, 0.026f, 0.022f, 0.96f);
}

UTopStatusBarWidget::UTopStatusBarWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UTopStatusBarWidget::NativeConstruct()
{
    Super::NativeConstruct();

    CachedTimeSubsystem = GetTimeSubsystem();
    CachedFinanceSubsystem = GetFinanceSubsystem();
    CachedPlayerLabelSubsystem = GetPlayerLabelSubsystem();
    CachedUIManagerSubsystem = GetUIManagerSubsystem();
    CachedCommandDispatcherSubsystem = GetCommandDispatcherSubsystem();

    BindButtonEvents();

    if (UGameTimeSubsystem* TimeSubsystem = CachedTimeSubsystem.Get())
    {
        TimeSubsystem->OnWeekAdvanced.RemoveDynamic(this, &UTopStatusBarWidget::HandleWeekAdvanced);
        TimeSubsystem->OnMonthAdvanced.RemoveDynamic(this, &UTopStatusBarWidget::HandleMonthAdvanced);
        TimeSubsystem->OnTimeBatchAdvanced.RemoveDynamic(this, &UTopStatusBarWidget::HandleTimeBatchAdvanced);
        TimeSubsystem->OnWeekAdvanced.AddDynamic(this, &UTopStatusBarWidget::HandleWeekAdvanced);
        TimeSubsystem->OnMonthAdvanced.AddDynamic(this, &UTopStatusBarWidget::HandleMonthAdvanced);
        TimeSubsystem->OnTimeBatchAdvanced.AddDynamic(this, &UTopStatusBarWidget::HandleTimeBatchAdvanced);
    }
    else
    {
        UE_LOG(LogTopStatusBarWidget, Warning, TEXT("Top status bar constructed without GameTimeSubsystem."));
    }

    if (UUIManagerSubsystem* UIManager = CachedUIManagerSubsystem.Get())
    {
        UIManager->OnCurrentLabelChanged.RemoveDynamic(this, &UTopStatusBarWidget::HandleCurrentLabelChanged);
        UIManager->OnCurrentLabelChanged.AddDynamic(this, &UTopStatusBarWidget::HandleCurrentLabelChanged);
    }

    if (UCommandDispatcherSubsystem* Commands = CachedCommandDispatcherSubsystem.Get())
    {
        Commands->OnCommandExecuted.RemoveDynamic(this, &UTopStatusBarWidget::HandleCommandExecuted);
        Commands->OnCommandExecuted.AddDynamic(this, &UTopStatusBarWidget::HandleCommandExecuted);
    }

    RefreshFromSubsystems();
}

void UTopStatusBarWidget::NativeDestruct()
{
    UnbindButtonEvents();

    if (UGameTimeSubsystem* TimeSubsystem = CachedTimeSubsystem.Get())
    {
        TimeSubsystem->OnWeekAdvanced.RemoveDynamic(this, &UTopStatusBarWidget::HandleWeekAdvanced);
        TimeSubsystem->OnMonthAdvanced.RemoveDynamic(this, &UTopStatusBarWidget::HandleMonthAdvanced);
        TimeSubsystem->OnTimeBatchAdvanced.RemoveDynamic(this, &UTopStatusBarWidget::HandleTimeBatchAdvanced);
    }

    if (UUIManagerSubsystem* UIManager = CachedUIManagerSubsystem.Get())
    {
        UIManager->OnCurrentLabelChanged.RemoveDynamic(this, &UTopStatusBarWidget::HandleCurrentLabelChanged);
    }

    if (UCommandDispatcherSubsystem* Commands = CachedCommandDispatcherSubsystem.Get())
    {
        Commands->OnCommandExecuted.RemoveDynamic(this, &UTopStatusBarWidget::HandleCommandExecuted);
    }

    Super::NativeDestruct();
}

void UTopStatusBarWidget::BindButtonEvents()
{
    if (PauseButton)
    {
        PauseButton->OnClicked.RemoveDynamic(this, &UTopStatusBarWidget::HandlePauseClicked);
        PauseButton->OnClicked.AddDynamic(this, &UTopStatusBarWidget::HandlePauseClicked);
    }

    if (PlayButton)
    {
        PlayButton->OnClicked.RemoveDynamic(this, &UTopStatusBarWidget::HandlePlayClicked);
        PlayButton->OnClicked.AddDynamic(this, &UTopStatusBarWidget::HandlePlayClicked);
    }

    if (FastForwardButton)
    {
        FastForwardButton->OnClicked.RemoveDynamic(this, &UTopStatusBarWidget::HandleFastForwardClicked);
        FastForwardButton->OnClicked.AddDynamic(this, &UTopStatusBarWidget::HandleFastForwardClicked);
    }

    if (MenuButton)
    {
        MenuButton->OnClicked.RemoveDynamic(this, &UTopStatusBarWidget::HandleMenuClicked);
        MenuButton->OnClicked.AddDynamic(this, &UTopStatusBarWidget::HandleMenuClicked);
    }
}

void UTopStatusBarWidget::UnbindButtonEvents()
{
    if (PauseButton)
    {
        PauseButton->OnClicked.RemoveDynamic(this, &UTopStatusBarWidget::HandlePauseClicked);
    }

    if (PlayButton)
    {
        PlayButton->OnClicked.RemoveDynamic(this, &UTopStatusBarWidget::HandlePlayClicked);
    }

    if (FastForwardButton)
    {
        FastForwardButton->OnClicked.RemoveDynamic(this, &UTopStatusBarWidget::HandleFastForwardClicked);
    }

    if (MenuButton)
    {
        MenuButton->OnClicked.RemoveDynamic(this, &UTopStatusBarWidget::HandleMenuClicked);
    }
}

void UTopStatusBarWidget::RefreshFromSubsystems()
{
    ApplyViewModel(BuildViewModel());
}

void UTopStatusBarWidget::ApplyViewModel(const FTopStatusBarViewModel& ViewModel)
{
    if (DateText)
    {
        DateText->SetText(ViewModel.DateText);
    }

    if (LabelNameText)
    {
        LabelNameText->SetText(ViewModel.LabelNameText);
    }

    if (CashText)
    {
        CashText->SetText(ViewModel.CashText);
    }

    if (ReputationText)
    {
        ReputationText->SetText(ViewModel.ReputationText);
    }

    const FLinearColor PauseColor = ViewModel.bTimeRunning ? PillBackgroundColor : FLinearColor(0.145f, 0.1f, 0.035f, 1.f);
    const FLinearColor PlayColor = ViewModel.bTimeRunning ? FLinearColor(0.145f, 0.1f, 0.035f, 1.f) : PillBackgroundColor;
    if (PauseButton)
    {
        PauseButton->SetBackgroundColor(PauseColor);
    }
    if (PlayButton)
    {
        PlayButton->SetBackgroundColor(PlayColor);
    }
}

FTopStatusBarViewModel UTopStatusBarWidget::BuildViewModel() const
{
    FTopStatusBarViewModel ViewModel;

    const UGameTimeSubsystem* TimeSubsystem = GetTimeSubsystem();
    const UPlayerLabelSubsystem* PlayerLabelSubsystem = GetPlayerLabelSubsystem();
    const UFinanceManagerSubsystem* FinanceSubsystem = GetFinanceSubsystem();

    const FDateTime CurrentDate = TimeSubsystem ? TimeSubsystem->GetCurrentGameDate() : FDateTime(1955, 1, 1);
    ViewModel.DateText = FormatMonthYear(CurrentDate);
    ViewModel.bTimeRunning = TimeSubsystem ? TimeSubsystem->IsTimeRunning() : false;

    const FString LabelName = PlayerLabelSubsystem ? PlayerLabelSubsystem->GetPlayerLabelName() : FString();
    ViewModel.LabelNameText = FText::FromString(LabelName.IsEmpty() ? TEXT("Unknown Label") : LabelName);

    const FString LabelId = ResolveLabelId();
    const float Balance = (FinanceSubsystem && !LabelId.IsEmpty()) ? FinanceSubsystem->GetLabelBalance(LabelId) : 0.f;
    ViewModel.CashText = FormatCurrency(Balance);

    const float Reputation = PlayerLabelSubsystem ? PlayerLabelSubsystem->GetLabelState().Reputation : 0.f;
    ViewModel.ReputationText = FormatReputation(Reputation);

    return ViewModel;
}

void UTopStatusBarWidget::HandlePauseClicked()
{
    if (UGameTimeSubsystem* TimeSubsystem = GetTimeSubsystem())
    {
        TimeSubsystem->PauseTime(true);
        RefreshFromSubsystems();
    }
}

void UTopStatusBarWidget::HandlePlayClicked()
{
    if (UGameTimeSubsystem* TimeSubsystem = GetTimeSubsystem())
    {
        TimeSubsystem->PauseTime(false);
        RefreshFromSubsystems();
    }
}

void UTopStatusBarWidget::HandleFastForwardClicked()
{
    if (UCommandDispatcherSubsystem* Commands = GetCommandDispatcherSubsystem())
    {
        FAdvanceTimeCommand Command;
        Command.WeeksToAdvance = 4;
        Commands->ExecuteAdvanceTime(Command);
    }
}

void UTopStatusBarWidget::HandleMenuClicked()
{
    UE_LOG(LogTopStatusBarWidget, Log, TEXT("Top status menu requested."));
    OnMenuRequested.Broadcast();
}

void UTopStatusBarWidget::HandleWeekAdvanced(const FDateTime& PreviousDate, const FDateTime& NewDate)
{
    UE_LOG(LogTopStatusBarWidget, Verbose, TEXT("Top status received week advanced: %s -> %s."), *PreviousDate.ToString(), *NewDate.ToString());
    if (UGameTimeSubsystem* TimeSubsystem = GetTimeSubsystem(); TimeSubsystem && TimeSubsystem->IsBatchAdvancing())
    {
        return;
    }
    RefreshFromSubsystems();
}

void UTopStatusBarWidget::HandleMonthAdvanced(const FDateTime& NewDate)
{
    UE_LOG(LogTopStatusBarWidget, Verbose, TEXT("Top status received month advanced: %s."), *NewDate.ToString());
    if (UGameTimeSubsystem* TimeSubsystem = GetTimeSubsystem(); TimeSubsystem && TimeSubsystem->IsBatchAdvancing())
    {
        return;
    }
    RefreshFromSubsystems();
}

void UTopStatusBarWidget::HandleTimeBatchAdvanced(int32 WeeksAdvanced, const FDateTime& NewDate)
{
    UE_LOG(LogTopStatusBarWidget, Verbose, TEXT("Top status received batch time advanced: Weeks=%d Date=%s."), WeeksAdvanced, *NewDate.ToString());
    RefreshFromSubsystems();
}

void UTopStatusBarWidget::HandleCurrentLabelChanged(const FString& NewLabelId)
{
    UE_LOG(LogTopStatusBarWidget, Verbose, TEXT("Top status current label changed: %s."), *NewLabelId);
    RefreshFromSubsystems();
}

void UTopStatusBarWidget::HandleCommandExecuted(const FMusicCommandResult& Result)
{
    UE_LOG(LogTopStatusBarWidget, Verbose, TEXT("Top status observed command result: Success=%s Message='%s'."),
        Result.bSuccess ? TEXT("true") : TEXT("false"),
        *Result.Message.ToString());
    RefreshFromSubsystems();
}

FString UTopStatusBarWidget::ResolveLabelId() const
{
    if (const UUIManagerSubsystem* UIManager = GetUIManagerSubsystem())
    {
        const FString LabelId = UIManager->GetCurrentLabelId();
        if (!LabelId.IsEmpty())
        {
            return LabelId;
        }
    }

    if (const UPlayerLabelSubsystem* PlayerLabelSubsystem = GetPlayerLabelSubsystem())
    {
        return PlayerLabelSubsystem->GetPlayerLabelId();
    }

    return FString();
}

UGameTimeSubsystem* UTopStatusBarWidget::GetTimeSubsystem() const
{
    if (UGameTimeSubsystem* Cached = CachedTimeSubsystem.Get())
    {
        return Cached;
    }

    if (const UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<UGameTimeSubsystem>();
        }
    }

    return nullptr;
}

UFinanceManagerSubsystem* UTopStatusBarWidget::GetFinanceSubsystem() const
{
    if (UFinanceManagerSubsystem* Cached = CachedFinanceSubsystem.Get())
    {
        return Cached;
    }

    if (const UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<UFinanceManagerSubsystem>();
        }
    }

    return nullptr;
}

UPlayerLabelSubsystem* UTopStatusBarWidget::GetPlayerLabelSubsystem() const
{
    if (UPlayerLabelSubsystem* Cached = CachedPlayerLabelSubsystem.Get())
    {
        return Cached;
    }

    if (const UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<UPlayerLabelSubsystem>();
        }
    }

    return nullptr;
}

UUIManagerSubsystem* UTopStatusBarWidget::GetUIManagerSubsystem() const
{
    if (UUIManagerSubsystem* Cached = CachedUIManagerSubsystem.Get())
    {
        return Cached;
    }

    if (const UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<UUIManagerSubsystem>();
        }
    }

    return nullptr;
}

UCommandDispatcherSubsystem* UTopStatusBarWidget::GetCommandDispatcherSubsystem() const
{
    if (UCommandDispatcherSubsystem* Cached = CachedCommandDispatcherSubsystem.Get())
    {
        return Cached;
    }

    if (const UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<UCommandDispatcherSubsystem>();
        }
    }

    return nullptr;
}

FText UTopStatusBarWidget::FormatMonthYear(const FDateTime& Date)
{
    static const FString MonthNames[12] = {
        TEXT("January"), TEXT("February"), TEXT("March"),
        TEXT("April"), TEXT("May"), TEXT("June"),
        TEXT("July"), TEXT("August"), TEXT("September"),
        TEXT("October"), TEXT("November"), TEXT("December")
    };

    const int32 Month = Date.GetMonth();
    const FString MonthString = (Month >= 1 && Month <= 12) ? MonthNames[Month - 1] : TEXT("Unknown");
    return FText::FromString(FString::Printf(TEXT("%s %d"), *MonthString, Date.GetYear()));
}

FText UTopStatusBarWidget::FormatCurrency(float Value)
{
    FNumberFormattingOptions Options;
    Options.MinimumIntegralDigits = 1;
    Options.MaximumFractionalDigits = 0;
    Options.MinimumFractionalDigits = 0;

    const FString Sign = Value < 0.f ? TEXT("-") : TEXT("");
    const FString NumberString = FText::AsNumber(FMath::Abs(Value), &Options).ToString();
    return FText::FromString(FString::Printf(TEXT("Cash %s$%s"), *Sign, *NumberString));
}

FText UTopStatusBarWidget::FormatReputation(float Reputation)
{
    FNumberFormattingOptions Options;
    Options.MinimumIntegralDigits = 1;
    Options.MaximumFractionalDigits = 1;
    Options.MinimumFractionalDigits = 0;
    return FText::FromString(FString::Printf(TEXT("Reputation %s"), *FText::AsNumber(Reputation, &Options).ToString()));
}
