// File: Private/EventSubsystem.cpp
#include "EventSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "WorldDelegates.h"
#include "EventTickerWidget.h"
#include "Layout.h"
#include "Blueprint/UserWidget.h"
#include "Math/UnrealMathUtility.h"

DEFINE_LOG_CATEGORY(LogEventSubsystem);

namespace
{
    constexpr float kMinTickInterval = 0.1f;
    constexpr float kMaxTickInterval = 60.0f;
    const FName kTickFunctionName(TEXT("OnEventSubsystemTick"));
}

UEventSubsystem::UEventSubsystem() = default;

void UEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    check(IsInGameThread());
    ensureMsgf(IsInGameThread(), TEXT("UI access must be on game thread"));

    Super::Initialize(Collection);

    if (!WorldBeginPlayHandle.IsValid())
    {
        WorldBeginPlayHandle = FWorldDelegates::OnWorldBeginPlay.AddUObject(this, &UEventSubsystem::HandleWorldBeginPlay);
    }

    if (!WorldBeginTearDownHandle.IsValid())
    {
        WorldBeginTearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &UEventSubsystem::HandleWorldBeginTearDown);
    }

    UE_LOG(LogEventSubsystem, Verbose, TEXT("Event subsystem initialized."));
}

void UEventSubsystem::Deinitialize()
{
    check(IsInGameThread());
    ensureMsgf(IsInGameThread(), TEXT("UI access must be on game thread"));

    StopTimer();

    ResetCachedWidgets();

    if (WorldBeginPlayHandle.IsValid())
    {
        FWorldDelegates::OnWorldBeginPlay.Remove(WorldBeginPlayHandle);
        WorldBeginPlayHandle = FDelegateHandle();
    }

    if (WorldBeginTearDownHandle.IsValid())
    {
        FWorldDelegates::OnWorldBeginTearDown.Remove(WorldBeginTearDownHandle);
        WorldBeginTearDownHandle = FDelegateHandle();
    }

    WorldWeak.Reset();

    UE_LOG(LogEventSubsystem, Verbose, TEXT("Event subsystem deinitialized."));

    Super::Deinitialize();
}

void UEventSubsystem::RegisterLayout(ULayout* InLayout)
{
    check(IsInGameThread());
    ensureMsgf(IsInGameThread(), TEXT("UI access must be on game thread"));

    if (!IsValid(InLayout))
    {
        UE_LOG(LogEventSubsystem, Warning, TEXT("RegisterLayout called with invalid widget."));
        return;
    }

    LayoutWeak = InLayout;
    AttemptChildRefresh();

    if (UWorld* LayoutWorld = InLayout->GetWorld())
    {
        if (LayoutWorld->IsGameWorld() && LayoutWorld->HasBegunPlay())
        {
            WorldWeak = LayoutWorld;
            StartTimerForWorld(LayoutWorld);
        }
    }

    UE_LOG(LogEventSubsystem, Display, TEXT("Layout %s registered with event subsystem."), *InLayout->GetName());
}

void UEventSubsystem::UnregisterLayout(ULayout* InLayout)
{
    check(IsInGameThread());
    ensureMsgf(IsInGameThread(), TEXT("UI access must be on game thread"));

    if (!LayoutWeak.IsValid())
    {
        return;
    }

    if (InLayout != nullptr && LayoutWeak.Get() != InLayout)
    {
        return;
    }

    UE_LOG(LogEventSubsystem, Display, TEXT("Layout unregistered from event subsystem."));

    ResetCachedWidgets();
    StopTimer();
}

void UEventSubsystem::OnTimerTick()
{
    check(IsInGameThread());
    ensureMsgf(IsInGameThread(), TEXT("UI access must be on game thread"));

    UWorld* World = WorldWeak.Get();
    if (!IsValid(World) || !World->IsGameWorld())
    {
        UE_LOG(LogEventSubsystem, Verbose, TEXT("Timer tick skipped due to invalid world."));
        StopTimer();
        WorldWeak.Reset();
        ResetCachedWidgets();
        return;
    }

    ULayout* Layout = LayoutWeak.Get();
    if (!IsValid(Layout))
    {
        UE_LOG(LogEventSubsystem, Verbose, TEXT("Timer tick stopping due to invalid layout."));
        StopTimer();
        ResetCachedWidgets();
        return;
    }

    UUserWidget* Child = ChildWeak.Get();
    if (!IsValid(Child))
    {
        AttemptChildRefresh();
        Child = ChildWeak.Get();

        if (!IsValid(Child))
        {
            UE_LOG(LogEventSubsystem, Verbose, TEXT("Child widget is not available on timer tick."));
            return;
        }
    }

    InvokeChildTick(*Child);
}

void UEventSubsystem::HandleWorldBeginPlay(UWorld* InWorld)
{
    check(IsInGameThread());
    ensureMsgf(IsInGameThread(), TEXT("UI access must be on game thread"));

    if (!IsValid(InWorld) || !InWorld->IsGameWorld())
    {
        return;
    }

    if (InWorld->GetGameInstance() != GetGameInstance())
    {
        return;
    }

    if (WorldWeak.IsValid() && WorldWeak.Get() == InWorld && IsTimerActiveForWorld(InWorld))
    {
        return;
    }

    WorldWeak = InWorld;
    StartTimerForWorld(InWorld);

    UE_LOG(LogEventSubsystem, Verbose, TEXT("World %s began play; timer initialized."), *InWorld->GetName());
}

void UEventSubsystem::HandleWorldBeginTearDown(UWorld* InWorld)
{
    check(IsInGameThread());
    ensureMsgf(IsInGameThread(), TEXT("UI access must be on game thread"));

    if (!IsValid(InWorld) || !InWorld->IsGameWorld())
    {
        return;
    }

    if (InWorld->GetGameInstance() != GetGameInstance())
    {
        return;
    }

    UE_LOG(LogEventSubsystem, Verbose, TEXT("World %s is tearing down; stopping timer."), *InWorld->GetName());

    StopTimer();
    WorldWeak.Reset();
    ResetCachedWidgets();
}

void UEventSubsystem::StartTimerForWorld(UWorld* InWorld)
{
    check(IsInGameThread());
    ensureMsgf(IsInGameThread(), TEXT("UI access must be on game thread"));

    if (!IsValid(InWorld) || !InWorld->IsGameWorld())
    {
        return;
    }

    if (InWorld->GetGameInstance() != GetGameInstance())
    {
        return;
    }

    const float ClampedInterval = FMath::Clamp(TickIntervalSeconds, kMinTickInterval, kMaxTickInterval);
    if (!FMath::IsNearlyEqual(ClampedInterval, TickIntervalSeconds))
    {
        TickIntervalSeconds = ClampedInterval;
    }

    if (IsTimerActiveForWorld(InWorld))
    {
        return;
    }

    FTimerManager& TimerManager = InWorld->GetTimerManager();
    TimerManager.SetTimer(EventTimerHandle, this, &UEventSubsystem::OnTimerTick, ClampedInterval, true, ClampedInterval);

    UE_LOG(LogEventSubsystem, Verbose, TEXT("Event timer started with interval %.2f seconds."), ClampedInterval);
}

void UEventSubsystem::StopTimer()
{
    check(IsInGameThread());
    ensureMsgf(IsInGameThread(), TEXT("UI access must be on game thread"));

    if (UWorld* World = WorldWeak.Get())
    {
        if (World->IsGameWorld())
        {
            World->GetTimerManager().ClearTimer(EventTimerHandle);
        }
    }

    EventTimerHandle.Invalidate();
}

void UEventSubsystem::ResetCachedWidgets()
{
    ChildWeak.Reset();
    LayoutWeak.Reset();
}

void UEventSubsystem::AttemptChildRefresh()
{
    check(IsInGameThread());
    ensureMsgf(IsInGameThread(), TEXT("UI access must be on game thread"));

    ULayout* Layout = LayoutWeak.Get();
    if (!IsValid(Layout))
    {
        ChildWeak.Reset();
        return;
    }

    UUserWidget* FoundChild = Layout->GetChildByNameOrClass(ChildWidgetName, ChildWidgetClass);
    if (IsValid(FoundChild))
    {
        ChildWeak = FoundChild;
        UE_LOG(LogEventSubsystem, Verbose, TEXT("Cached child widget %s."), *FoundChild->GetName());
    }
    else
    {
        ChildWeak.Reset();
    }
}

void UEventSubsystem::InvokeChildTick(UUserWidget& ChildWidget) const
{
    check(IsInGameThread());
    ensureMsgf(IsInGameThread(), TEXT("UI access must be on game thread"));

    if (UEventTickerWidget* TickerWidget = Cast<UEventTickerWidget>(&ChildWidget))
    {
        TickerWidget->OnEventSubsystemTick();
        return;
    }

    if (UFunction* TickFunction = ChildWidget.FindFunction(kTickFunctionName))
    {
        ChildWidget.ProcessEvent(TickFunction, nullptr);
    }
    else
    {
        UE_LOG(LogEventSubsystem, Verbose, TEXT("Child widget %s has no OnEventSubsystemTick event."), *ChildWidget.GetName());
    }
}

bool UEventSubsystem::IsTimerActiveForWorld(const UWorld* InWorld) const
{
    if (!IsValid(InWorld) || !InWorld->IsGameWorld())
    {
        return false;
    }

    if (WorldWeak.Get() != InWorld)
    {
        return false;
    }

    const FTimerManager& TimerManager = InWorld->GetTimerManager();
    return TimerManager.TimerExists(EventTimerHandle) || TimerManager.IsTimerActive(EventTimerHandle);
}
