// File: Private/EventSubsystem.cpp
#include "EventSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "EventTickerWidget.h"
#include "Layout.h"
#include "Async/Async.h"
#include "Engine/World.h"
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

    LayoutWeak.Reset();
    ChildWeak.Reset();
    GameTimeSubsystem.Reset();

    Super::Deinitialize();
}

void UEventSubsystem::RegisterLayout(ULayout* InLayout)
{
    if (!ensure(IsInGameThread()))
    {
        return;
    }

    if (!IsValid(InLayout))
    {
        UE_LOG(LogEventSubsystem, Warning, TEXT("RegisterLayout called with invalid layout."));
        return;
    }

    LayoutWeak = InLayout;
    ChildWeak.Reset();

    UE_LOG(LogEventSubsystem, Display, TEXT("Registered layout %s."), *InLayout->GetName());

    ResolveChildWidget(*InLayout);
}

void UEventSubsystem::UnregisterLayout(ULayout* InLayout)
{
    if (!ensure(IsInGameThread()))
    {
        return;
    }

    if (!LayoutWeak.IsValid())
    {
        return;
    }

    if (InLayout && LayoutWeak.Get() != InLayout)
    {
        return;
    }

    UE_LOG(LogEventSubsystem, Display, TEXT("Unregistered layout."));

    LayoutWeak.Reset();
    ChildWeak.Reset();
}

//void UEventSubsystem::HandlePostWorldInit(UWorld* InWorld, const UWorld::InitializationValues IVS)
//{
//    (void)IVS;
//    if (!InWorld || !InWorld->IsGameWorld())
//    {
//        return;
//    }
//
//    if (!IsSameGameInstanceWorld(*InWorld))
//    {
//        return;
//    }
//
//    if (CachedWorld.IsValid() && CachedWorld.Get() != InWorld)
//    {
//        StopTimer();
//        CachedWorld.Reset();
//    }
//
//    UE_LOG(LogEventSubsystem, Verbose, TEXT("Post world initialization for %s."), *InWorld->GetName());
//
//    StartTimerForWorld(InWorld);
//   // SendDummyNews();
//}

//void UEventSubsystem::SendDummyNews()
//{
//
//    if (LayoutWeak.IsValid())
//    {
//        FMusicNewsEvent Dummy;
//        Dummy.NewsId = FGuid::NewGuid();
//        Dummy.Timestamp = FDateTime::Now();
//        Dummy.NewsType = EMusicNewsType::NewUpcomingArtistPerforming;
//        Dummy.SourceName = TEXT("The Wild Beats");
//        Dummy.SubjectName = TEXT("New Artist");
//        Dummy.Headline = TEXT("New artist on the block!");
//        Dummy.BodyText = TEXT("Johnny Rocker performs at the Mug");
//        Dummy.Tags = { TEXT("Live"), TEXT("Rockabilly"), TEXT("Performance") };
//
//        const TWeakObjectPtr<ULayout> LocalLayoutWeak = LayoutWeak;
//        AsyncTask(ENamedThreads::GameThread, [LocalLayoutWeak, Dummy]()
//            {
//                if (ULayout* LayoutPtr = LocalLayoutWeak.Get())
//                {
//                    if (IsValid(LayoutPtr))
//                    {
//                        LayoutPtr->AddNewsCardToFeed(Dummy);
//                    }
//                }
//            });
//    }
//}

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

void UEventSubsystem::ProcessMonthAdvanced(const FDateTime& NewDate)
{
    if (!ensure(IsInGameThread()))
    {
        return;
    }

    UE_LOG(LogEventSubsystem, Verbose, TEXT("Processing simulated date change to %s."), *NewDate.ToString());

    ULayout* Layout = LayoutWeak.Get();
    if (!IsValid(Layout))
    {
        UE_LOG(LogEventSubsystem, Verbose, TEXT("No layout registered; skipping simulated date processing."));
        ChildWeak.Reset();
        return;
    }

    UWorld* LayoutWorld = Layout->GetWorld();
    if (!IsValid(LayoutWorld) || !LayoutWorld->IsGameWorld())
    {
        UE_LOG(LogEventSubsystem, Verbose, TEXT("Layout world invalid; skipping simulated date processing."));
        ChildWeak.Reset();
        return;
    }

    UUserWidget* Child = ChildWeak.Get();
    if (!IsValid(Child))
    {
        Child = ResolveChildWidget(*Layout);
        if (!IsValid(Child))
        {
            UE_LOG(LogEventSubsystem, Verbose, TEXT("Unable to resolve child widget for layout %s."), *Layout->GetName());
            return;
        }
    }

    if (UEventTickerWidget* Ticker = Cast<UEventTickerWidget>(Child))
    {
        UE_LOG(LogEventSubsystem, Verbose, TEXT("Processed simulated date change for EventTickerWidget %s."), *Ticker->GetName());
        return;
    }

    UE_LOG(LogEventSubsystem, Verbose, TEXT("Child widget %s is not an EventTickerWidget; skipping."), *Child->GetName());
}

UUserWidget* UEventSubsystem::ResolveChildWidget(ULayout& Layout)
{
    if (!ensure(IsInGameThread()))
    {
        return nullptr;
    }

    UUserWidget* ResolvedChild = Layout.GetChildByNameOrClass(ChildWidgetName, ChildWidgetClass);
    if (IsValid(ResolvedChild))
    {
        ChildWeak = ResolvedChild;
        UE_LOG(LogEventSubsystem, Verbose, TEXT("Resolved child widget %s."), *ResolvedChild->GetName());
        return ResolvedChild;
    }

    ChildWeak.Reset();
    return nullptr;
}

bool UEventSubsystem::IsSameGameInstanceWorld(const UWorld& World) const
{
    return World.GetGameInstance() == GetGameInstance();
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
