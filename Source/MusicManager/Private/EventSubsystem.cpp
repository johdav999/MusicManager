// File: Private/EventSubsystem.cpp
#include "EventSubsystem.h"

#include "Async/Async.h"
#include "Engine/GameInstance.h"
#include "GameTimeSubsystem.h"

DEFINE_LOG_CATEGORY(LogEventSubsystem);

void UEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (UGameTimeSubsystem* GameTime = GetOrCreateGameTimeSubsystem())
    {
        GameTime->OnMonthAdvanced.AddDynamic(this, &UEventSubsystem::HandleMonthAdvanced);
        UE_LOG(LogEventSubsystem, Display, TEXT("Subscribed to OnMonthAdvanced from UGameTimeSubsystem."));

        ProcessMonthAdvanced(GameTime->GetCurrentGameDate());
    }
    else
    {
        UE_LOG(LogEventSubsystem, Warning, TEXT("Unable to subscribe to UGameTimeSubsystem; event updates will be inactive."));
    }
}

void UEventSubsystem::Deinitialize()
{
    if (UGameTimeSubsystem* GameTime = GameTimeSubsystem.Get())
    {
        GameTime->OnMonthAdvanced.RemoveDynamic(this, &UEventSubsystem::HandleMonthAdvanced);
    }
    GameTimeSubsystem.Reset();

    Super::Deinitialize();
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

void UEventSubsystem::ProcessMonthAdvanced(const FDateTime& NewDate)
{
    if (!ensure(IsInGameThread()))
    {
        return;
    }

    UE_LOG(LogEventSubsystem, Verbose, TEXT("Processing simulated date change to %s."), *NewDate.ToString());

    const FMusicNewsEvent NewEvent = BuildMonthlyNews(NewDate);
    OnNewsEventGenerated.Broadcast(NewEvent);
}

UGameTimeSubsystem* UEventSubsystem::GetOrCreateGameTimeSubsystem()
{
    if (UGameTimeSubsystem* Existing = GameTimeSubsystem.Get())
    {
        return Existing;
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>();
        GameTimeSubsystem = TimeSubsystem;
        return TimeSubsystem;
    }

    return nullptr;
}

FMusicNewsEvent UEventSubsystem::BuildMonthlyNews(const FDateTime& NewDate) const
{
    FMusicNewsEvent NewEvent;
    NewEvent.NewsId = FGuid::NewGuid();
    NewEvent.Timestamp = NewDate;
    NewEvent.NewsType = EMusicNewsType::IndustryTrend;

    const FString MonthYearString = NewDate.ToString(TEXT("%B %Y"));
    NewEvent.SourceName = TEXT("Global Market Desk");
    NewEvent.SubjectName = MonthYearString;
    NewEvent.Headline = FString::Printf(TEXT("%s Market Recap Released"), *MonthYearString);
    NewEvent.BodyText = FString::Printf(TEXT("Simulated time advanced to %s, generating scheduled market coverage."), *MonthYearString);
    NewEvent.Tags = { TEXT("Auto"), TEXT("TimeSubsystem") };

    return NewEvent;
}
