// File: Private/EventSubsystem.cpp
#include "EventSubsystem.h"
#include "Async/Async.h"
#include "ArtistManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "AuditionTypes.h"
#include "Layout.h"  
#include "GameTimeSubsystem.h"
#include "UIManagerSubsystem.h"

DEFINE_LOG_CATEGORY(LogEventSubsystem);

void UEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);
    FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UEventSubsystem::HandleWorldInitialized);
}

void UEventSubsystem::Deinitialize()
{
    FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);

    if (UGameTimeSubsystem* GameTime = GameTimeSubsystem.Get())
    {
        GameTime->OnTimeBatchAdvanced.RemoveAll(this);
    }
    GameTimeSubsystem.Reset();
    PendingBatchNewsEvents.Reset();

    Super::Deinitialize();
}

void UEventSubsystem::HandleWorldInitialized(UWorld* World, const UWorld::InitializationValues IVS)
{
    if (!IsInGameThread() || !IsValid(World))
    {
        return;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!IsValid(GameInstance))
    {
        UE_LOG(LogEventSubsystem, Warning, TEXT("HandleWorldInitialized: GameInstance is invalid."));
        return;
    }

    UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>();
    UE_LOG(LogEventSubsystem, Display, TEXT("HandleWorldInitialized: UGameTimeSubsystem = %s (%p)"),
        TimeSubsystem ? TEXT("VALID") : TEXT("NULL"),
        TimeSubsystem);

    if (!IsValid(TimeSubsystem))
    {
        return;
    }

    if (UGameTimeSubsystem* Existing = GameTimeSubsystem.Get())
    {
        if (Existing == TimeSubsystem)
        {
            return;
        }

    }

    GameTimeSubsystem = TimeSubsystem;

    if (!TimeSubsystem->OnTimeBatchAdvanced.IsAlreadyBound(this, &UEventSubsystem::HandleTimeBatchAdvanced))
    {
        TimeSubsystem->OnTimeBatchAdvanced.AddDynamic(this, &UEventSubsystem::HandleTimeBatchAdvanced);
    }

    // Initial hookup observes the current date only. News generation is driven by explicit month closes.
}

void UEventSubsystem::HandleMonthAdvanced(const FDateTime& NewDate)
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<UEventSubsystem> WeakThis = this;
        AsyncTask(ENamedThreads::GameThread, [WeakThis, NewDate]()
        {
            if (UEventSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->ProcessMonthAdvanced(NewDate);
            }
        });
        return;
    }

    const FDateTime StartOfCurrentMonth(NewDate.GetYear(), NewDate.GetMonth(), 1);
    int32 PrevYear = NewDate.GetYear();
    int32 PrevMonth = NewDate.GetMonth() - 1;
    if (PrevMonth == 0)
    {
        PrevMonth = 12;
        --PrevYear;
    }

    HandleMonthClosed(PrevYear, PrevMonth, FDateTime(PrevYear, PrevMonth, 1), StartOfCurrentMonth, NewDate);
}

void UEventSubsystem::HandleMonthClosed(int32 ClosedYear, int32 ClosedMonth, const FDateTime& PeriodStart, const FDateTime& PeriodEnd, const FDateTime& NewDate)
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<UEventSubsystem> WeakThis = this;
        AsyncTask(ENamedThreads::GameThread, [WeakThis, ClosedYear, ClosedMonth, PeriodStart, PeriodEnd, NewDate]()
        {
            if (UEventSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->ProcessMonthClosed(ClosedYear, ClosedMonth, PeriodStart, PeriodEnd, NewDate);
            }
        });
        return;
    }

    ProcessMonthClosed(ClosedYear, ClosedMonth, PeriodStart, PeriodEnd, NewDate);
}

void UEventSubsystem::GetMonthlyNewsSummaries(TArray<FMonthlyNewsSummary>& OutSummaries) const
{
    OutSummaries = MonthlyNewsSummaries;
}

void UEventSubsystem::HandleTimeBatchAdvanced(int32 WeeksAdvanced, const FDateTime& NewDate)
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<UEventSubsystem> WeakThis = this;
        AsyncTask(ENamedThreads::GameThread, [WeakThis, WeeksAdvanced, NewDate]()
        {
            if (UEventSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->HandleTimeBatchAdvanced(WeeksAdvanced, NewDate);
            }
        });
        return;
    }

    UE_LOG(LogEventSubsystem, Verbose, TEXT("Flushing %d deferred news events after %d batched weeks ending at %s."),
        PendingBatchNewsEvents.Num(),
        WeeksAdvanced,
        *NewDate.ToString());

    if (PendingBatchNewsEvents.Num() == 0)
    {
        return;
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UUIManagerSubsystem* UI = GameInstance->GetSubsystem<UUIManagerSubsystem>())
        {
            for (const FMusicNewsEvent& Event : PendingBatchNewsEvents)
            {
                UI->HandleNewsEvent(Event);
                OnNewsEventGenerated.Broadcast(Event);
            }
        }
    }

    PendingBatchNewsEvents.Reset();
}

void UEventSubsystem::RegisterLayout(ULayout* InLayout)
{
    if (!ensure(IsInGameThread()))
    {
        return;
    }

    LayoutWeak = InLayout;
}

void UEventSubsystem::UnregisterLayout(ULayout* InLayout)
{
    if (!ensure(IsInGameThread()))
    {
        return;
    }

    if (LayoutWeak.Get() == InLayout)
    {
        LayoutWeak.Reset();
    }
}

bool UEventSubsystem::HasNewsKeyBeenProcessed(const FString& Key) const
{
    return ProcessedNewsKeys.Contains(Key);
}

void UEventSubsystem::MarkNewsKeyProcessed(const FString& Key)
{
    ProcessedNewsKeys.Add(Key);
}

FString UEventSubsystem::BuildNewsKey(const FMusicNewsEvent& Event) const
{
    // Unique key for each distinct event type.
    // For NewUpcomingArtistPerforming: "NewUpcomingArtistPerforming:ArtistName"
    return FString::Printf(TEXT("%d:%s"),
        static_cast<int32>(Event.NewsType),
        *Event.SourceName);
}

void UEventSubsystem::ProcessMonthAdvanced(const FDateTime& NewDate)
{
    if (!ensure(IsInGameThread()))
    {
        return;
    }

    UE_LOG(LogEventSubsystem, Verbose, TEXT("Processing simulated date change to %s."), *NewDate.ToString());

    const FMusicNewsEvent NewEvent = BuildMonthlyNews(NewDate);
    if (NewEvent.Headline != "")
    {
        // Build stable event key
        const FString EventKey = BuildNewsKey(NewEvent);

        // Prevent duplicates
        if (HasNewsKeyBeenProcessed(EventKey))
        {
            UE_LOG(LogEventSubsystem, Verbose, TEXT("Skipping duplicate event key: %s"), *EventKey);
            return;
        }

        MarkNewsKeyProcessed(EventKey);

        if (UGameTimeSubsystem* TimeSubsystem = GameTimeSubsystem.Get())
        {
            if (TimeSubsystem->IsBatchAdvancing())
            {
                PendingBatchNewsEvents.Add(NewEvent);
                return;
            }
        }

        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UUIManagerSubsystem* UI = GameInstance->GetSubsystem<UUIManagerSubsystem>())
            {
                UI->HandleNewsEvent(NewEvent);
            }
        }

        OnNewsEventGenerated.Broadcast(NewEvent);
    }
}

void UEventSubsystem::ProcessMonthClosed(int32 ClosedYear, int32 ClosedMonth, const FDateTime& PeriodStart, const FDateTime& PeriodEnd, const FDateTime& NewDate)
{
    if (!ensure(IsInGameThread()))
    {
        return;
    }

    const FString SummaryKey = FString::Printf(TEXT("%04d-%02d"), ClosedYear, ClosedMonth);
    if (ClosedMonthlyNewsKeys.Contains(SummaryKey))
    {
        UE_LOG(LogEventSubsystem, Verbose, TEXT("Skipping duplicate monthly news summary: %s"), *SummaryKey);
        return;
    }

    FMonthlyNewsSummary Summary;
    Summary.Year = ClosedYear;
    Summary.Month = ClosedMonth;
    Summary.PeriodStart = PeriodStart;
    Summary.PeriodEnd = PeriodEnd;
    Summary.SummaryKey = SummaryKey;

    const FMusicNewsEvent NewEvent = BuildMonthlyNews(ClosedYear, ClosedMonth, NewDate);
    if (NewEvent.Headline != "")
    {
        const FString EventKey = FString::Printf(TEXT("MonthlySummary:%s"), *SummaryKey);
        if (!HasNewsKeyBeenProcessed(EventKey))
        {
            MarkNewsKeyProcessed(EventKey);
            Summary.GeneratedNewsIds.Add(NewEvent.NewsId);

            if (UGameTimeSubsystem* TimeSubsystem = GameTimeSubsystem.Get())
            {
                if (TimeSubsystem->IsBatchAdvancing())
                {
                    PendingBatchNewsEvents.Add(NewEvent);
                }
                else if (UGameInstance* GameInstance = GetGameInstance())
                {
                    if (UUIManagerSubsystem* UI = GameInstance->GetSubsystem<UUIManagerSubsystem>())
                    {
                        UI->HandleNewsEvent(NewEvent);
                    }
                    OnNewsEventGenerated.Broadcast(NewEvent);
                }
            }
        }
    }

    ClosedMonthlyNewsKeys.Add(SummaryKey);
    MonthlyNewsSummaries.Add(Summary);
}

UGameTimeSubsystem* UEventSubsystem::GetOrCreateGameTimeSubsystem()
{
    if (UGameTimeSubsystem* Existing = GameTimeSubsystem.Get())
    {
        return Existing;
    }

    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>();
            if (IsValid(TimeSubsystem))
            {
                GameTimeSubsystem = TimeSubsystem;
                return TimeSubsystem;
            }
        }
  }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("GameInstance=null"));
    }

    return nullptr;
}

FMusicNewsEvent UEventSubsystem::BuildMonthlyNews(const FDateTime& NewDate) const
{
    FMusicNewsEvent NewEvent;
    NewEvent.NewsId = FGuid::NewGuid();
    NewEvent.Timestamp = NewDate;
    return BuildMonthlyNews(NewDate.GetYear(), NewDate.GetMonth(), NewDate);
}

FMusicNewsEvent UEventSubsystem::BuildMonthlyNews(int32 ClosedYear, int32 ClosedMonth, const FDateTime& NewDate) const
{
    FMusicNewsEvent NewEvent;
    NewEvent.NewsId = FGuid::NewGuid();
    NewEvent.Timestamp = NewDate;
    NewEvent.Metadata.Add(TEXT("ClosedYear"), FString::FromInt(ClosedYear));
    NewEvent.Metadata.Add(TEXT("ClosedMonth"), FString::FromInt(ClosedMonth));

    if (UWorld* World = GetWorld())
    {
        if (IsValid(World))
        {
            if (UGameInstance* GameInstance = World->GetGameInstance())
            {
                if (IsValid(GameInstance))
                {
                    if (UArtistManagerSubsystem* ArtistSub = GameInstance->GetSubsystem<UArtistManagerSubsystem>())
                    {
                        if (IsValid(ArtistSub))
                        {
                            FArtistData Artist;
                            if (ArtistSub->GetNextUnsignedArtist(Artist))
                            {
                                NewEvent.NewsType = EMusicNewsType::NewUpcomingArtistPerforming;
                                NewEvent.SourceName = Artist.ArtistName;
                                NewEvent.SubjectName = Artist.Genre;

                                NewEvent.Headline = FString::Printf(
                                    TEXT("%s local gig"),
                                    *Artist.ArtistName);

                                NewEvent.BodyText = FString::Printf(
                                    TEXT("Rumors are circulating that %s is preparing an exciting upcoming performance in the %s scene."),
                                    *Artist.ArtistName,
                                    *Artist.Genre);

                                NewEvent.Tags = { TEXT("Artist"), TEXT("Unsigned"), Artist.Genre };
                                return NewEvent;
                            }
                        }
                    }
                }
            }
        }
    }

    //NewEvent.NewsType = EMusicNewsType::IndustryTrend;

    //const FString MonthYearString = NewDate.ToString(TEXT("%B %Y"));
    //NewEvent.SourceName = TEXT("Global Market Desk");
    //NewEvent.SubjectName = MonthYearString;
    //NewEvent.Headline = FString::Printf(TEXT("%s Market Recap Released"), *MonthYearString);
    //NewEvent.BodyText = FString::Printf(
    //    TEXT("Simulated time advanced to %s, generating scheduled market coverage."),
    //    *MonthYearString
    //);
    //NewEvent.Tags = { TEXT("Auto"), TEXT("TimeSubsystem") };

    return NewEvent;
}
