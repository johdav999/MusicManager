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
        GameTime->OnMonthAdvanced.RemoveAll(this);
    }
    GameTimeSubsystem.Reset();

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

        Existing->OnMonthAdvanced.RemoveAll(this);
    }

    GameTimeSubsystem = TimeSubsystem;

   /* TimeSubsystem->OnMonthAdvanced.AddUObject(this, &UEventSubsystem::HandleMonthAdvanced);*/
    TimeSubsystem->OnMonthAdvanced.AddDynamic(this, &UEventSubsystem::HandleMonthAdvanced);
    ProcessMonthAdvanced(TimeSubsystem->GetCurrentGameDate());
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

    ProcessMonthAdvanced(NewDate);
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

bool UEventSubsystem::HasNewsBeenProcessed(const FGuid& NewsId) const
{
    return ProcessedNewsIds.Contains(NewsId);
}

void UEventSubsystem::MarkNewsAsProcessed(const FGuid& NewsId)
{
    ProcessedNewsIds.Add(NewsId);
}

void UEventSubsystem::ProcessMonthAdvanced(const FDateTime& NewDate)
{
    if (!ensure(IsInGameThread()))
    {
        return;
    }

    UE_LOG(LogEventSubsystem, Verbose, TEXT("Processing simulated date change to %s."), *NewDate.ToString());

    const FMusicNewsEvent NewEvent = BuildMonthlyNews(NewDate);
    if (NewEvent.BodyText != "")
    {
        // Prevent duplicate triggers
        if (HasNewsBeenProcessed(NewEvent.NewsId))
        {
            UE_LOG(LogEventSubsystem, Verbose, TEXT("Skipping duplicate news event %s"), *NewEvent.NewsId.ToString());
            return;
        }

        MarkNewsAsProcessed(NewEvent.NewsId);

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
                                    TEXT("%s preparing for an upcoming performance"),
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

    NewEvent.NewsType = EMusicNewsType::IndustryTrend;

    const FString MonthYearString = NewDate.ToString(TEXT("%B %Y"));
    NewEvent.SourceName = TEXT("Global Market Desk");
    NewEvent.SubjectName = MonthYearString;
    NewEvent.Headline = FString::Printf(TEXT("%s Market Recap Released"), *MonthYearString);
    NewEvent.BodyText = FString::Printf(
        TEXT("Simulated time advanced to %s, generating scheduled market coverage."),
        *MonthYearString
    );
    NewEvent.Tags = { TEXT("Auto"), TEXT("TimeSubsystem") };

    return NewEvent;
}
