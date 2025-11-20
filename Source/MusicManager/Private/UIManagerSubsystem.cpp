// File: Private/UIManagerSubsystem.cpp
#include "UIManagerSubsystem.h"

#include "Layout.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogUIManagerSubsystem, Log, All);

void UUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    RegisteredLayout.Reset();
    OnNewsSelected.Clear();
}

void UUIManagerSubsystem::Deinitialize()
{
    RegisteredLayout.Reset();
    OnNewsSelected.Clear();

    Super::Deinitialize();
}

void UUIManagerSubsystem::RegisterLayout(ULayout* InLayout)
{
    if (!ensure(IsInGameThread()))
    {
        return;
    }

    if (!IsValid(InLayout))
    {
        UE_LOG(LogUIManagerSubsystem, Warning, TEXT("RegisterLayout called with invalid layout."));
        RegisteredLayout.Reset();
        return;
    }

    RegisteredLayout = InLayout;
    UE_LOG(LogUIManagerSubsystem, Display, TEXT("Registered layout: %s"), *InLayout->GetName());
}

ULayout* UUIManagerSubsystem::GetLayout() const
{
    return RegisteredLayout.Get();
}

void UUIManagerSubsystem::ShowAudition(const FAuditionEvent& EventData)
{
    if (!RegisteredLayout.IsValid())
    {
        UE_LOG(LogUIManagerSubsystem, Warning, TEXT("ShowAudition called without a valid layout registered."));
        return;
    }

    const TWeakObjectPtr<ULayout> LocalLayout = RegisteredLayout;

    ExecuteOnGameThread([LocalLayout, EventData]()
    {
        ULayout* LayoutPtr = LocalLayout.Get();
        if (!ensure(IsValid(LayoutPtr)))
        {
            return;
        }

        LayoutPtr->ShowAuditionWidgetWithData(EventData);
    });
}

void UUIManagerSubsystem::HandleNewsCardSelected(const FMusicNewsEvent& EventData)
{
    ExecuteOnGameThread([this, EventData]()
    {
        if (!ensure(IsInGameThread()))
        {
            return;
        }

        OnNewsSelected.Broadcast(EventData);
    });
}

