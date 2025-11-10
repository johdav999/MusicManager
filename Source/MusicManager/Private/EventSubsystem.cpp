// File: Private/EventSubsystem.cpp
#include "EventSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "EventTickerWidget.h"
#include "Layout.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"
#include "TimerManager.h"
#include "Async/Async.h"

DEFINE_LOG_CATEGORY(LogEventSubsystem);

namespace
{
    constexpr float MinTickInterval = 0.1f;
    constexpr float MaxTickInterval = 60.0f;
}

void UEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (!WorldInitHandle.IsValid())
    {
        WorldInitHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UEventSubsystem::HandlePostWorldInit);
        UE_LOG(LogEventSubsystem, Display, TEXT("Bound to OnPostWorldInitialization."));
    }

    if (!WorldCleanupHandle.IsValid())
    {
        WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UEventSubsystem::HandleWorldCleanup);
        UE_LOG(LogEventSubsystem, Display, TEXT("Bound to OnWorldCleanup."));
    }

    if (UWorld* CurrentWorld = GetWorld())
    {
        if (CurrentWorld->IsGameWorld() && IsSameGameInstanceWorld(*CurrentWorld))
        {
            StartTimerForWorld(CurrentWorld);
        }
    }
}

void UEventSubsystem::Deinitialize()
{
    StopTimer();

    if (WorldInitHandle.IsValid())
    {
        FWorldDelegates::OnPostWorldInitialization.Remove(WorldInitHandle);
        WorldInitHandle = FDelegateHandle();
    }

    if (WorldCleanupHandle.IsValid())
    {
        FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
        WorldCleanupHandle = FDelegateHandle();
    }

    LayoutWeak.Reset();
    ChildWeak.Reset();
    CachedWorld.Reset();

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

void UEventSubsystem::HandlePostWorldInit(UWorld* InWorld, const UWorld::InitializationValues IVS)
{
    (void)IVS;
    if (!InWorld || !InWorld->IsGameWorld())
    {
        return;
    }

    if (!IsSameGameInstanceWorld(*InWorld))
    {
        return;
    }

    if (CachedWorld.IsValid() && CachedWorld.Get() != InWorld)
    {
        StopTimer();
        CachedWorld.Reset();
    }

    UE_LOG(LogEventSubsystem, Verbose, TEXT("Post world initialization for %s."), *InWorld->GetName());

    StartTimerForWorld(InWorld);

    if (LayoutWeak.IsValid())
    {
        FMusicNewsEvent Dummy;
        Dummy.NewsId = FGuid::NewGuid();
        Dummy.Timestamp = FDateTime::Now();
        Dummy.NewsType = EMusicNewsType::ArtistPerformance;
        Dummy.SourceName = TEXT("The Wild Beats");
        Dummy.SubjectName = TEXT("Summer Jam 1985");
        Dummy.Headline = TEXT("The Wild Beats electrify the crowd at Summer Jam!");
        Dummy.BodyText = TEXT("A stunning live show earns rave reviews and a surge in record sales.");
        Dummy.Tags = { TEXT("Live"), TEXT("Rock"), TEXT("Performance") };

        const TWeakObjectPtr<ULayout> LocalLayoutWeak = LayoutWeak;
        AsyncTask(ENamedThreads::GameThread, [LocalLayoutWeak, Dummy]()
        {
            if (ULayout* LayoutPtr = LocalLayoutWeak.Get())
            {
                if (IsValid(LayoutPtr))
                {
                    LayoutPtr->AddNewsCardToFeed(Dummy);
                }
            }
        });
    }
}

void UEventSubsystem::HandleWorldCleanup(UWorld* InWorld, bool /*bSessionEnded*/, bool /*bCleanupResources*/)
{
    if (!InWorld || !InWorld->IsGameWorld())
    {
        return;
    }

    if (!IsSameGameInstanceWorld(*InWorld))
    {
        return;
    }

    if (CachedWorld.IsValid() && CachedWorld.Get() == InWorld)
    {
        UE_LOG(LogEventSubsystem, Verbose, TEXT("World cleanup for %s."), *InWorld->GetName());
        StopTimer();
        CachedWorld.Reset();
        ChildWeak.Reset();
        LayoutWeak.Reset();
    }
}

void UEventSubsystem::StartTimerForWorld(UWorld* InWorld)
{
    if (!InWorld || !InWorld->IsGameWorld())
    {
        return;
    }

    if (!IsSameGameInstanceWorld(*InWorld))
    {
        return;
    }

    const float ClampedInterval = FMath::Clamp(TickIntervalSeconds, MinTickInterval, MaxTickInterval);
    if (!FMath::IsNearlyEqual(ClampedInterval, TickIntervalSeconds))
    {
        TickIntervalSeconds = ClampedInterval;
    }

    FTimerManager& TimerManager = InWorld->GetTimerManager();
    TimerManager.SetTimer(EventTimerHandle, this, &UEventSubsystem::OnTimerTick, ClampedInterval, true);

    CachedWorld = InWorld;

    UE_LOG(LogEventSubsystem, Display, TEXT("Started timer for world %s (interval %.2f seconds)."), *InWorld->GetName(), ClampedInterval);
}

void UEventSubsystem::StopTimer()
{
    if (UWorld* World = CachedWorld.Get())
    {
        if (World->IsGameWorld())
        {
            FTimerManager& TimerManager = World->GetTimerManager();
            if (TimerManager.TimerExists(EventTimerHandle) || TimerManager.IsTimerActive(EventTimerHandle))
            {
                TimerManager.ClearTimer(EventTimerHandle);
                UE_LOG(LogEventSubsystem, Display, TEXT("Stopped timer for world %s."), *World->GetName());
            }
            else
            {
                TimerManager.ClearTimer(EventTimerHandle);
            }
        }
    }

    EventTimerHandle.Invalidate();
}

void UEventSubsystem::OnTimerTick()
{
    check(IsInGameThread());

    UE_LOG(LogEventSubsystem, Verbose, TEXT("Event subsystem tick."));

    UWorld* World = CachedWorld.Get();
    if (!IsValid(World) || !World->IsGameWorld())
    {
        UE_LOG(LogEventSubsystem, Verbose, TEXT("Skipping tick due to invalid world."));
        StopTimer();
        CachedWorld.Reset();
        ChildWeak.Reset();
        return;
    }

    ULayout* Layout = LayoutWeak.Get();
    if (!IsValid(Layout))
    {
        UE_LOG(LogEventSubsystem, Verbose, TEXT("No layout registered; skipping tick."));
        ChildWeak.Reset();
        return;
    }

    if (Layout->GetWorld() != World)
    {
        UE_LOG(LogEventSubsystem, Verbose, TEXT("Layout world mismatch; skipping tick."));
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
        /*Ticker->OnEventSubsystemTick();
        return;*/
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
