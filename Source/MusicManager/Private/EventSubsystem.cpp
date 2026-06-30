// File: Private/EventSubsystem.cpp
#include "EventSubsystem.h"
#include "Async/Async.h"
#include "ArtistManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "AuditionTypes.h"
#include "Layout.h"  
#include "GameTimeSubsystem.h"
#include "MusicSaveGame.h"
#include "UIManagerSubsystem.h"

DEFINE_LOG_CATEGORY(LogEventSubsystem);

void UEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);
    FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UEventSubsystem::HandleWorldInitialized);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>())
        {
            GameTimeSubsystem = TimeSubsystem;

            if (!TimeSubsystem->OnTimeBatchAdvanced.IsAlreadyBound(this, &UEventSubsystem::HandleTimeBatchAdvanced))
            {
                TimeSubsystem->OnTimeBatchAdvanced.AddDynamic(this, &UEventSubsystem::HandleTimeBatchAdvanced);
        UE_LOG(LogEventSubsystem, Warning, TEXT("Initialize: bound to OnTimeBatchAdvanced on GameTimeSubsystem %p."), TimeSubsystem);
            }

            if (!TimeSubsystem->OnMonthlySummaryClosed.IsAlreadyBound(this, &UEventSubsystem::HandleMonthlySummaryClosed))
            {
                TimeSubsystem->OnMonthlySummaryClosed.AddDynamic(this, &UEventSubsystem::HandleMonthlySummaryClosed);
                UE_LOG(LogEventSubsystem, Warning, TEXT("Initialize: bound to OnMonthlySummaryClosed on GameTimeSubsystem %p."), TimeSubsystem);
            }
        }
        else
        {
            UE_LOG(LogEventSubsystem, Warning, TEXT("Initialize: GameTimeSubsystem is unavailable; waiting for world initialization."));
        }
    }
    else
    {
        UE_LOG(LogEventSubsystem, Warning, TEXT("Initialize: GameInstance is unavailable; waiting for world initialization."));
    }
}

void UEventSubsystem::Deinitialize()
{
    FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);

    if (UGameTimeSubsystem* GameTime = GameTimeSubsystem.Get())
    {
        GameTime->OnTimeBatchAdvanced.RemoveAll(this);
        GameTime->OnMonthlySummaryClosed.RemoveAll(this);
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
            UE_LOG(LogEventSubsystem, Verbose, TEXT("HandleWorldInitialized: already bound to GameTimeSubsystem %p."), TimeSubsystem);
            return;
        }

        Existing->OnTimeBatchAdvanced.RemoveAll(this);
        Existing->OnMonthlySummaryClosed.RemoveAll(this);
    }

    GameTimeSubsystem = TimeSubsystem;

    if (!TimeSubsystem->OnTimeBatchAdvanced.IsAlreadyBound(this, &UEventSubsystem::HandleTimeBatchAdvanced))
    {
        TimeSubsystem->OnTimeBatchAdvanced.AddDynamic(this, &UEventSubsystem::HandleTimeBatchAdvanced);
        UE_LOG(LogEventSubsystem, Warning, TEXT("Bound to OnTimeBatchAdvanced on GameTimeSubsystem %p."), TimeSubsystem);
    }

    if (!TimeSubsystem->OnMonthlySummaryClosed.IsAlreadyBound(this, &UEventSubsystem::HandleMonthlySummaryClosed))
    {
        TimeSubsystem->OnMonthlySummaryClosed.AddDynamic(this, &UEventSubsystem::HandleMonthlySummaryClosed);
        UE_LOG(LogEventSubsystem, Warning, TEXT("Bound to OnMonthlySummaryClosed on GameTimeSubsystem %p."), TimeSubsystem);
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

void UEventSubsystem::HandleMonthlySummaryClosed(const FMonthlyCloseSummary& Summary)
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<UEventSubsystem> WeakThis = this;
        AsyncTask(ENamedThreads::GameThread, [WeakThis, Summary]()
        {
            if (UEventSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->HandleMonthlySummaryClosed(Summary);
            }
        });
        return;
    }

    UE_LOG(LogEventSubsystem, Warning, TEXT("Monthly summary closed received: MonthKey=%s Period=%s -> %s CurrentDate=%s."),
        *Summary.MonthKey,
        *Summary.PeriodStart.ToString(),
        *Summary.PeriodEnd.ToString(),
        *Summary.NewDate.ToString());

    ProcessMonthClosed(Summary.ClosedYear, Summary.ClosedMonth, Summary.PeriodStart, Summary.PeriodEnd, Summary.NewDate);
}

void UEventSubsystem::GetMonthlyNewsSummaries(TArray<FMonthlyNewsSummary>& OutSummaries) const
{
    OutSummaries = MonthlyNewsSummaries;
}

bool UEventSubsystem::PublishNewsEvent(const FMusicNewsEvent& Event, const FString& DeduplicationKey)
{
    if (!ensure(IsInGameThread()))
    {
        return false;
    }

    if (DeduplicationKey.IsEmpty() || HasNewsKeyBeenProcessed(DeduplicationKey))
    {
        UE_LOG(LogEventSubsystem, Verbose, TEXT("PublishNewsEvent skipped: DeduplicationKey='%s' AlreadyProcessed=%s Headline='%s'."),
            *DeduplicationKey,
            HasNewsKeyBeenProcessed(DeduplicationKey) ? TEXT("true") : TEXT("false"),
            *Event.Headline);
        return false;
    }

    MarkNewsKeyProcessed(DeduplicationKey);
    UE_LOG(LogEventSubsystem, Warning, TEXT("PublishNewsEvent accepted: Key='%s' Type=%d Headline='%s'."),
        *DeduplicationKey,
        static_cast<int32>(Event.NewsType),
        *Event.Headline);

    if (UGameTimeSubsystem* TimeSubsystem = GameTimeSubsystem.Get())
    {
        if (TimeSubsystem->IsBatchAdvancing())
        {
            PendingBatchNewsEvents.Add(Event);
            UE_LOG(LogEventSubsystem, Warning, TEXT("PublishNewsEvent deferred during batch. PendingBatchNewsEvents=%d Headline='%s'."),
                PendingBatchNewsEvents.Num(),
                *Event.Headline);
            return true;
        }
    }

    EmitNewsEvent(Event, TEXT("PublishNewsEvent"));
    return true;
}

void UEventSubsystem::BuildSaveSnapshot(FNewsSnapshot& OutSnapshot) const
{
    OutSnapshot.MonthlyNewsSummaries = MonthlyNewsSummaries;
    OutSnapshot.ProcessedNewsKeys = ProcessedNewsKeys;
    OutSnapshot.ClosedMonthlyNewsKeys = ClosedMonthlyNewsKeys;
}

void UEventSubsystem::ValidateSaveSnapshot(const FNewsSnapshot& Snapshot, FMusicSaveValidationResult& Result) const
{
    TSet<FString> SeenSummaryKeys;
    for (const FMonthlyNewsSummary& Summary : Snapshot.MonthlyNewsSummaries)
    {
        if (Summary.SummaryKey.IsEmpty())
        {
            Result.AddError(TEXT("Monthly news summary has an empty summary key."));
        }
        else if (SeenSummaryKeys.Contains(Summary.SummaryKey))
        {
            Result.AddError(FString::Printf(TEXT("Duplicate monthly news summary key %s."), *Summary.SummaryKey));
        }
        SeenSummaryKeys.Add(Summary.SummaryKey);

        if (Summary.Month < 1 || Summary.Month > 12 || Summary.Year < 1955)
        {
            Result.AddError(FString::Printf(TEXT("Monthly news summary has invalid period %04d-%02d."), Summary.Year, Summary.Month));
        }
        if (Summary.PeriodStart.GetTicks() <= 0 || Summary.PeriodEnd.GetTicks() <= 0 || Summary.PeriodEnd <= Summary.PeriodStart)
        {
            Result.AddError(FString::Printf(TEXT("Monthly news summary %s has invalid date range."), *Summary.SummaryKey));
        }
        for (const FGuid& NewsId : Summary.GeneratedNewsIds)
        {
            if (!NewsId.IsValid())
            {
                Result.AddError(FString::Printf(TEXT("Monthly news summary %s contains an invalid generated news id."), *Summary.SummaryKey));
            }
        }
    }

    for (const FString& Key : Snapshot.ProcessedNewsKeys)
    {
        if (Key.IsEmpty())
        {
            Result.AddError(TEXT("Processed news key set contains an empty key."));
        }
    }

    for (const FString& Key : Snapshot.ClosedMonthlyNewsKeys)
    {
        if (Key.IsEmpty())
        {
            Result.AddError(TEXT("Closed monthly news key set contains an empty key."));
        }
    }
}

void UEventSubsystem::ApplySaveSnapshot(const FNewsSnapshot& Snapshot)
{
    MonthlyNewsSummaries = Snapshot.MonthlyNewsSummaries;
    ProcessedNewsKeys = Snapshot.ProcessedNewsKeys;
    ClosedMonthlyNewsKeys = Snapshot.ClosedMonthlyNewsKeys;
    PendingBatchNewsEvents.Reset();
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

    for (const FMusicNewsEvent& Event : PendingBatchNewsEvents)
    {
        EmitNewsEvent(Event, TEXT("BatchFlush"));
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
            UE_LOG(LogEventSubsystem, Verbose, TEXT("ProcessMonthAdvanced skipped duplicate event key: %s"), *EventKey);
            return;
        }

        MarkNewsKeyProcessed(EventKey);
        UE_LOG(LogEventSubsystem, Warning, TEXT("ProcessMonthAdvanced generated news: Key='%s' Headline='%s' Date=%s."),
            *EventKey,
            *NewEvent.Headline,
            *NewDate.ToString());

        if (UGameTimeSubsystem* TimeSubsystem = GameTimeSubsystem.Get())
        {
            if (TimeSubsystem->IsBatchAdvancing())
            {
                PendingBatchNewsEvents.Add(NewEvent);
                UE_LOG(LogEventSubsystem, Warning, TEXT("ProcessMonthAdvanced deferred news during batch. PendingBatchNewsEvents=%d."),
                    PendingBatchNewsEvents.Num());
                return;
            }
        }

        EmitNewsEvent(NewEvent, TEXT("ProcessMonthAdvanced"));
    }
}

void UEventSubsystem::ProcessMonthClosed(int32 ClosedYear, int32 ClosedMonth, const FDateTime& PeriodStart, const FDateTime& PeriodEnd, const FDateTime& NewDate)
{
    if (!ensure(IsInGameThread()))
    {
        return;
    }

    const FString SummaryKey = FString::Printf(TEXT("%04d-%02d"), ClosedYear, ClosedMonth);
    UE_LOG(LogEventSubsystem, Warning, TEXT("ProcessMonthClosed entered: SummaryKey=%s Period=%s -> %s CurrentDate=%s."),
        *SummaryKey,
        *PeriodStart.ToString(),
        *PeriodEnd.ToString(),
        *NewDate.ToString());

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
            UE_LOG(LogEventSubsystem, Warning, TEXT("Monthly news generated: Key='%s' Headline='%s' Type=%d Source='%s'."),
                *EventKey,
                *NewEvent.Headline,
                static_cast<int32>(NewEvent.NewsType),
                *NewEvent.SourceName);

            if (UGameTimeSubsystem* TimeSubsystem = GameTimeSubsystem.Get())
            {
                if (TimeSubsystem->IsBatchAdvancing())
                {
                    PendingBatchNewsEvents.Add(NewEvent);
                    UE_LOG(LogEventSubsystem, Warning, TEXT("Monthly news deferred during batch. PendingBatchNewsEvents=%d."),
                        PendingBatchNewsEvents.Num());
                }
                else
                {
                    EmitNewsEvent(NewEvent, TEXT("ProcessMonthClosed"));
                }
            }
            else
            {
                UE_LOG(LogEventSubsystem, Warning, TEXT("Monthly news generated without cached GameTimeSubsystem; emitting immediately."));
                EmitNewsEvent(NewEvent, TEXT("ProcessMonthClosedNoTimeSubsystem"));
            }
        }
        else
        {
            UE_LOG(LogEventSubsystem, Verbose, TEXT("Monthly news skipped because event key was already processed: %s."), *EventKey);
        }
    }
    else
    {
        UE_LOG(LogEventSubsystem, Warning, TEXT("Monthly news builder returned no headline for SummaryKey=%s."), *SummaryKey);
    }

    ClosedMonthlyNewsKeys.Add(SummaryKey);
    MonthlyNewsSummaries.Add(Summary);
    UE_LOG(LogEventSubsystem, Warning, TEXT("Monthly news summary stored: SummaryKey=%s GeneratedNewsIds=%d TotalSummaries=%d."),
        *SummaryKey,
        Summary.GeneratedNewsIds.Num(),
        MonthlyNewsSummaries.Num());
}

void UEventSubsystem::EmitNewsEvent(const FMusicNewsEvent& Event, const FString& SourceContext)
{
    UE_LOG(LogEventSubsystem, Warning, TEXT("Emitting news event: Context=%s Type=%d Headline='%s' Source='%s' Subject='%s' Listeners=%d."),
        *SourceContext,
        static_cast<int32>(Event.NewsType),
        *Event.Headline,
        *Event.SourceName,
        *Event.SubjectName,
        OnNewsEventGenerated.IsBound() ? 1 : 0);

    bool bRoutedToUI = false;
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UUIManagerSubsystem* UI = GameInstance->GetSubsystem<UUIManagerSubsystem>())
        {
            UI->HandleNewsEvent(Event);
            bRoutedToUI = true;
        }
    }

    UE_LOG(LogEventSubsystem, Warning, TEXT("News event UI route complete: Context=%s RoutedToUI=%s Headline='%s'."),
        *SourceContext,
        bRoutedToUI ? TEXT("true") : TEXT("false"),
        *Event.Headline);

    OnNewsEventGenerated.Broadcast(Event);
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
                                NewEvent.Metadata.Add(TEXT("ArtistId"), Artist.ArtistId.IsEmpty() ? Artist.ArtistName : Artist.ArtistId);
                                NewEvent.Metadata.Add(TEXT("ArtistName"), Artist.ArtistName);
                                UE_LOG(LogEventSubsystem, Verbose, TEXT("BuildMonthlyNews selected unsigned artist '%s' for %04d-%02d."),
                                    *Artist.ArtistName,
                                    ClosedYear,
                                    ClosedMonth);
                                return NewEvent;
                            }
                            UE_LOG(LogEventSubsystem, Warning, TEXT("BuildMonthlyNews found no unsigned artist for %04d-%02d."), ClosedYear, ClosedMonth);
                        }
                        else
                        {
                            UE_LOG(LogEventSubsystem, Warning, TEXT("BuildMonthlyNews: ArtistManagerSubsystem is invalid."));
                        }
                    }
                    else
                    {
                        UE_LOG(LogEventSubsystem, Warning, TEXT("BuildMonthlyNews: ArtistManagerSubsystem is unavailable."));
                    }
                }
                else
                {
                    UE_LOG(LogEventSubsystem, Warning, TEXT("BuildMonthlyNews: GameInstance is invalid."));
                }
            }
        }
        else
        {
            UE_LOG(LogEventSubsystem, Warning, TEXT("BuildMonthlyNews: World is invalid."));
        }
    }

    NewEvent.NewsType = EMusicNewsType::IndustryTrend;

    const FString MonthYearString = FDateTime(ClosedYear, ClosedMonth, 1).ToString(TEXT("%B %Y"));
    NewEvent.SourceName = TEXT("Global Market Desk");
    NewEvent.SubjectName = MonthYearString;
    NewEvent.Headline = FString::Printf(TEXT("%s market recap"), *MonthYearString);
    NewEvent.BodyText = FString::Printf(
        TEXT("The %s market cycle closed with labels watching regional demand, release timing, and artist momentum."),
        *MonthYearString
    );
    NewEvent.Tags = { TEXT("Market"), TEXT("Monthly"), TEXT("Industry") };

    UE_LOG(LogEventSubsystem, Warning, TEXT("BuildMonthlyNews generated fallback industry recap for %04d-%02d."),
        ClosedYear,
        ClosedMonth);

    return NewEvent;
}
