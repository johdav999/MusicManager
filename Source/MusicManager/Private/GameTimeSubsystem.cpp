#include "GameTimeSubsystem.h"

#include "ArtistManagerSubsystem.h"
#include "EventSubsystem.h"
#include "FinanceManagerSubsystem.h"
#include "MarketManagerSubsystem.h"
#include "MusicSaveGame.h"
#include "RecordManagerSubsystem.h"
#include "UIManagerSubsystem.h"

DEFINE_LOG_CATEGORY(LogMusicSimTime);

namespace
{
    const TCHAR* ToPhaseName(EMusicWeeklySimulationPhase Phase)
    {
        switch (Phase)
        {
        case EMusicWeeklySimulationPhase::TrendDrift:
            return TEXT("TrendDrift");
        case EMusicWeeklySimulationPhase::ArtistStateUpdate:
            return TEXT("ArtistStateUpdate");
        case EMusicWeeklySimulationPhase::ProductionProgress:
            return TEXT("ProductionProgress");
        case EMusicWeeklySimulationPhase::ReleaseLaunchProcessing:
            return TEXT("ReleaseLaunchProcessing");
        case EMusicWeeklySimulationPhase::MarketExposureUpdate:
            return TEXT("MarketExposureUpdate");
        case EMusicWeeklySimulationPhase::ChartCalculation:
            return TEXT("ChartCalculation");
        case EMusicWeeklySimulationPhase::TourResolution:
            return TEXT("TourResolution");
        case EMusicWeeklySimulationPhase::FinanceSettlement:
            return TEXT("FinanceSettlement");
        case EMusicWeeklySimulationPhase::CriticNewsGeneration:
            return TEXT("CriticNewsGeneration");
        case EMusicWeeklySimulationPhase::Notifications:
            return TEXT("Notifications");
        default:
            return TEXT("Unknown");
        }
    }
}

UGameTimeSubsystem::UGameTimeSubsystem()
    : CurrentGameDate(1955, 1, 1)
    , bIsTimeRunning(false)
    , bIsBatchAdvancing(false)
    , bHasReachedSimulationEnd(false)
{
    // This order is intentional: do not derive simulation sequencing from subsystem discovery,
    // delegate binding, or unordered containers. Future weekly systems should be inserted here.
    WeeklyPhaseOrder =
    {
        EMusicWeeklySimulationPhase::TrendDrift,
        EMusicWeeklySimulationPhase::ArtistStateUpdate,
        EMusicWeeklySimulationPhase::ProductionProgress,
        EMusicWeeklySimulationPhase::ReleaseLaunchProcessing,
        EMusicWeeklySimulationPhase::MarketExposureUpdate,
        EMusicWeeklySimulationPhase::ChartCalculation,
        EMusicWeeklySimulationPhase::TourResolution,
        EMusicWeeklySimulationPhase::FinanceSettlement,
        EMusicWeeklySimulationPhase::CriticNewsGeneration,
        EMusicWeeklySimulationPhase::Notifications
    };
}

void UGameTimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    CurrentGameDate = FDateTime(1955, 1, 1);
    bHasReachedSimulationEnd = false;
    bIsTimeRunning = false;
    bIsBatchAdvancing = false;
    LastExecutedWeeklyPhases.Reset();

    UE_LOG(LogMusicSimTime, Log, TEXT("Game time initialized at %s. Automatic timer advancement is disabled; use explicit weekly advancement."), *CurrentGameDate.ToString());
}

void UGameTimeSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UGameTimeSubsystem::AdvanceOneWeek()
{
    AdvanceWeeks(1);
}

int32 UGameTimeSubsystem::AdvanceWeeks(int32 NumWeeks)
{
    check(IsInGameThread());

    if (NumWeeks <= 0)
    {
        return 0;
    }

    if (HasSimulationEnded())
    {
        return 0;
    }

    const bool bWasBatchAdvancing = bIsBatchAdvancing;
    bIsBatchAdvancing = NumWeeks > 1;
    const FDateTime BatchStartDate = CurrentGameDate;
    int32 WeeksProcessed = 0;
    int32 MonthsClosed = 0;
    bool bCrossedYear = false;
    UUIManagerSubsystem* BatchUIManager = nullptr;

    if (bIsBatchAdvancing)
    {
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            BatchUIManager = GameInstance->GetSubsystem<UUIManagerSubsystem>();
            if (BatchUIManager)
            {
                BatchUIManager->BeginSimulationBatchUpdate(NumWeeks, BatchStartDate);
            }
        }
    }

    for (int32 WeekIndex = 0; WeekIndex < NumWeeks; ++WeekIndex)
    {
        if (HasSimulationEnded())
        {
            break;
        }

        const FDateTime PreviousDate = CurrentGameDate;
        const FDateTime NewDate = CurrentGameDate + FTimespan::FromDays(7);

        if (WouldEndSimulation(NewDate))
        {
            bHasReachedSimulationEnd = true;
            break;
        }

        const bool bClosedMonth = DidCrossMonthBoundary(PreviousDate, NewDate);
        const bool bAdvancedYear = DidCrossYearBoundary(PreviousDate, NewDate);

        CurrentGameDate = NewDate;

        UE_LOG(LogMusicSimTime, Log, TEXT("Week advanced: %s -> %s"),
            *PreviousDate.ToString(),
            *CurrentGameDate.ToString());

        RunWeeklySimulation(PreviousDate, CurrentGameDate, bClosedMonth);

        // Delegates are notifications only; core simulation mutation has already completed.
        OnWeekAdvanced.Broadcast(PreviousDate, CurrentGameDate);

        if (bClosedMonth)
        {
            ProcessClosedMonths(PreviousDate, CurrentGameDate);
            ++MonthsClosed;
        }

        if (bAdvancedYear)
        {
            bCrossedYear = true;

            UE_LOG(LogMusicSimTime, Log, TEXT("Year advanced: %d -> %d"),
                PreviousDate.GetYear(),
                CurrentGameDate.GetYear());

            OnYearAdvanced.Broadcast(CurrentGameDate.GetYear(), PreviousDate, CurrentGameDate);
        }

        ++WeeksProcessed;
    }

    bIsBatchAdvancing = bWasBatchAdvancing;

    if (BatchUIManager)
    {
        BatchUIManager->EndSimulationBatchUpdate(WeeksProcessed, CurrentGameDate);
    }

    if (WeeksProcessed > 1)
    {
        UE_LOG(LogMusicSimTime, Log, TEXT("Batch weekly advancement finished: RequestedWeeks=%d ProcessedWeeks=%d StartDate=%s EndDate=%s MonthsClosed=%d CrossedYear=%s"),
            NumWeeks,
            WeeksProcessed,
            *BatchStartDate.ToString(),
            *CurrentGameDate.ToString(),
            MonthsClosed,
            bCrossedYear ? TEXT("true") : TEXT("false"));

        OnTimeBatchAdvanced.Broadcast(WeeksProcessed, CurrentGameDate);
    }

    return WeeksProcessed;
}

void UGameTimeSubsystem::AdvanceMonth()
{
    check(IsInGameThread());

    if (HasSimulationEnded())
    {
        return;
    }

    const int32 StartingMonth = CurrentGameDate.GetMonth();
    const int32 StartingYear = CurrentGameDate.GetYear();

    UE_LOG(LogMusicSimTime, Log, TEXT("AdvanceMonth compatibility path entered at %s."),
        *CurrentGameDate.ToString());

    // Six weekly steps cross any calendar month from any in-month start date.
    for (int32 Guard = 0; Guard < 6; ++Guard)
    {
        AdvanceOneWeek();

        if (HasSimulationEnded() || CurrentGameDate.GetMonth() != StartingMonth || CurrentGameDate.GetYear() != StartingYear)
        {
            return;
        }
    }

    UE_LOG(LogMusicSimTime, Warning, TEXT("AdvanceMonth compatibility path did not cross a month from %04d-%02d within guard limit."),
        StartingYear,
        StartingMonth);
}

void UGameTimeSubsystem::RunWeeklySimulation(const FDateTime& PreviousDate, const FDateTime& NewDate, bool bClosedMonth)
{
    LastExecutedWeeklyPhases.Reset();

    UE_LOG(LogMusicSimTime, Verbose, TEXT("Running weekly simulation phases for %s. ClosedMonth=%s"),
        *NewDate.ToString(),
        bClosedMonth ? TEXT("true") : TEXT("false"));

    for (EMusicWeeklySimulationPhase Phase : WeeklyPhaseOrder)
    {
        RunWeeklySimulationPhase(Phase, PreviousDate, NewDate, bClosedMonth);
    }
}

void UGameTimeSubsystem::RunWeeklySimulationPhase(EMusicWeeklySimulationPhase Phase, const FDateTime& PreviousDate, const FDateTime& NewDate, bool bClosedMonth)
{
    LastExecutedWeeklyPhases.Add(Phase);

    UE_LOG(LogMusicSimTime, Verbose, TEXT("WeeklySimulation Phase=%s PreviousDate=%s NewDate=%s ClosedMonth=%s"),
        ToPhaseName(Phase),
        *PreviousDate.ToString(),
        *NewDate.ToString(),
        bClosedMonth ? TEXT("true") : TEXT("false"));

    switch (Phase)
    {
    case EMusicWeeklySimulationPhase::TrendDrift:
        break;

    case EMusicWeeklySimulationPhase::ArtistStateUpdate:
        break;

    case EMusicWeeklySimulationPhase::ProductionProgress:
        break;

    case EMusicWeeklySimulationPhase::ReleaseLaunchProcessing:
        break;

    case EMusicWeeklySimulationPhase::MarketExposureUpdate:
        break;

    case EMusicWeeklySimulationPhase::ChartCalculation:
        break;

    case EMusicWeeklySimulationPhase::TourResolution:
        break;

    case EMusicWeeklySimulationPhase::FinanceSettlement:
        break;

    case EMusicWeeklySimulationPhase::CriticNewsGeneration:
        break;

    case EMusicWeeklySimulationPhase::Notifications:
        break;
    }
}

void UGameTimeSubsystem::ProcessClosedMonths(const FDateTime& PreviousDate, const FDateTime& NewDate)
{
    FDateTime MonthCursor(PreviousDate.GetYear(), PreviousDate.GetMonth(), 1);
    const FDateTime NewMonthStart(NewDate.GetYear(), NewDate.GetMonth(), 1);

    // A single week usually closes one month, but fast-forward/test hooks may skip farther.
    while (MonthCursor < NewMonthStart)
    {
        const FMonthlyCloseSummary Summary = BuildMonthlyCloseSummary(
            MonthCursor.GetYear(),
            MonthCursor.GetMonth(),
            PreviousDate,
            NewDate);

        CloseMonth(Summary);

        int32 NextMonth = MonthCursor.GetMonth() + 1;
        int32 NextYear = MonthCursor.GetYear();
        if (NextMonth > 12)
        {
            NextMonth = 1;
            ++NextYear;
        }

        MonthCursor = FDateTime(NextYear, NextMonth, 1);
    }
}

FMonthlyCloseSummary UGameTimeSubsystem::BuildMonthlyCloseSummary(int32 ClosedYear, int32 ClosedMonth, const FDateTime& PreviousDate, const FDateTime& NewDate) const
{
    int32 NextMonth = ClosedMonth + 1;
    int32 NextYear = ClosedYear;
    if (NextMonth > 12)
    {
        NextMonth = 1;
        ++NextYear;
    }

    FMonthlyCloseSummary Summary;
    Summary.ClosedYear = ClosedYear;
    Summary.ClosedMonth = ClosedMonth;
    Summary.PeriodStart = FDateTime(ClosedYear, ClosedMonth, 1);
    Summary.PeriodEnd = FDateTime(NextYear, NextMonth, 1);
    Summary.PreviousDate = PreviousDate;
    Summary.NewDate = NewDate;
    Summary.MonthKey = FString::Printf(TEXT("%04d-%02d"), ClosedYear, ClosedMonth);
    return Summary;
}

void UGameTimeSubsystem::CloseMonth(const FMonthlyCloseSummary& Summary)
{
    UE_LOG(LogMusicSimTime, Log, TEXT("Month closed: %s at weekly boundary %s -> %s"),
        *Summary.MonthKey,
        *Summary.PreviousDate.ToString(),
        *Summary.NewDate.ToString());

    RunMonthlyCompatibilityPass(Summary);

    // Summary delegates fire after compatibility finalizers so finance/news snapshots are queryable.
    OnMonthlySummaryClosed.Broadcast(Summary);
    OnMonthClosed.Broadcast(Summary.ClosedYear, Summary.ClosedMonth, Summary.PreviousDate, Summary.NewDate);
    OnMonthAdvanced.Broadcast(Summary.NewDate);
}

void UGameTimeSubsystem::RunMonthlyCompatibilityPass(const FMonthlyCloseSummary& Summary)
{
    UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        return;
    }

    // Previous monthly order was Market -> Artist -> Record -> Finance in AdvanceMonth().
    // Keep it explicit while monthly-only systems migrate into the weekly phase switch above.
    if (UMarketManagerSubsystem* MarketSubsystem = GameInstance->GetSubsystem<UMarketManagerSubsystem>())
    {
        UE_LOG(LogMusicSimTime, Verbose, TEXT("MonthlyCompatibility Subsystem=MarketManagerSubsystem ClosedMonth=%s Date=%s"), *Summary.MonthKey, *Summary.NewDate.ToString());
        MarketSubsystem->HandleMonthAdvanced(Summary.NewDate);
    }

    if (UArtistManagerSubsystem* ArtistSubsystem = GameInstance->GetSubsystem<UArtistManagerSubsystem>())
    {
        UE_LOG(LogMusicSimTime, Verbose, TEXT("MonthlyCompatibility Subsystem=ArtistManagerSubsystem ClosedMonth=%s Date=%s"), *Summary.MonthKey, *Summary.NewDate.ToString());
        ArtistSubsystem->HandleMonthAdvanced(Summary.NewDate);
    }

    if (URecordManagerSubsystem* RecordSubsystem = GameInstance->GetSubsystem<URecordManagerSubsystem>())
    {
        UE_LOG(LogMusicSimTime, Verbose, TEXT("MonthlyCompatibility Subsystem=RecordManagerSubsystem ClosedMonth=%s Date=%s"), *Summary.MonthKey, *Summary.NewDate.ToString());
        RecordSubsystem->HandleMonthAdvanced(Summary.NewDate);
    }

    if (UFinanceManagerSubsystem* FinanceSubsystem = GameInstance->GetSubsystem<UFinanceManagerSubsystem>())
    {
        UE_LOG(LogMusicSimTime, Verbose, TEXT("MonthlyCompatibility Subsystem=FinanceManagerSubsystem ClosedMonth=%s Date=%s"), *Summary.MonthKey, *Summary.NewDate.ToString());
        FinanceSubsystem->HandleMonthClosed(Summary.ClosedYear, Summary.ClosedMonth, Summary.PeriodStart, Summary.PeriodEnd);
    }

    if (UEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<UEventSubsystem>())
    {
        UE_LOG(LogMusicSimTime, Verbose, TEXT("MonthlyCompatibility Subsystem=EventSubsystem ClosedMonth=%s Date=%s"), *Summary.MonthKey, *Summary.NewDate.ToString());
        EventSubsystem->HandleMonthClosed(Summary.ClosedYear, Summary.ClosedMonth, Summary.PeriodStart, Summary.PeriodEnd, Summary.NewDate);
    }
}

bool UGameTimeSubsystem::DidCrossMonthBoundary(const FDateTime& PreviousDate, const FDateTime& NewDate) const
{
    return PreviousDate.GetMonth() != NewDate.GetMonth() || PreviousDate.GetYear() != NewDate.GetYear();
}

bool UGameTimeSubsystem::DidCrossYearBoundary(const FDateTime& PreviousDate, const FDateTime& NewDate) const
{
    return PreviousDate.GetYear() != NewDate.GetYear();
}

#if WITH_AUTOMATION_TESTS
void UGameTimeSubsystem::AdvanceToDateForTesting(const FDateTime& NewDate)
{
    check(IsInGameThread());

    if (NewDate.GetYear() > 2026)
    {
        bHasReachedSimulationEnd = true;
        return;
    }

    CurrentGameDate = NewDate;
    bHasReachedSimulationEnd = false;
}
#endif

void UGameTimeSubsystem::PauseTime(bool bPause)
{
    check(IsInGameThread());

    // TASK-7.1.7: business simulation must not self-advance from a repeating timer.
    // Keep this Blueprint-callable method as a no-op compatibility hook.
    bIsTimeRunning = false;
    UE_LOG(LogMusicSimTime, Verbose, TEXT("PauseTime(%s) ignored; simulation advances only through explicit AdvanceOneWeek/AdvanceWeeks calls."),
        bPause ? TEXT("true") : TEXT("false"));
}

bool UGameTimeSubsystem::HasSimulationEnded() const
{
    if (bHasReachedSimulationEnd)
    {
        return true;
    }

    return CurrentGameDate.GetYear() > 2026;
}

bool UGameTimeSubsystem::WouldEndSimulation(const FDateTime& CandidateDate) const
{
    return CandidateDate.GetYear() > 2026;
}

void UGameTimeSubsystem::SaveState(UMusicSaveGame* SaveObject)
{
    if (!ensure(IsInGameThread()))
    {
        return;
    }

    if (SaveObject)
    {
        SaveObject->SavedGameDate = CurrentGameDate;
    }
}

void UGameTimeSubsystem::LoadState(const UMusicSaveGame* SaveObject)
{
    if (!ensure(IsInGameThread()))
    {
        return;
    }

    if (SaveObject)
    {
        CurrentGameDate = SaveObject->SavedGameDate;
    }
}
