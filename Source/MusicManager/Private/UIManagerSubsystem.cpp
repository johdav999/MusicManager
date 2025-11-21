// File: Private/UIManagerSubsystem.cpp
#include "UIManagerSubsystem.h"

#include "Layout.h"
#include "ArtistManagerSubsystem.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogUIManagerSubsystem, Log, All);

void UUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (UArtistManagerSubsystem* Artist = GetGameInstance()->GetSubsystem<UArtistManagerSubsystem>())
    {
        Artist->OnArtistSigned.AddDynamic(this, &UUIManagerSubsystem::HandleArtistSigned);
    }
}

void UUIManagerSubsystem::Deinitialize()
{
    if (UArtistManagerSubsystem* Artist = GetGameInstance()->GetSubsystem<UArtistManagerSubsystem>())
    {
        Artist->OnArtistSigned.RemoveDynamic(this, &UUIManagerSubsystem::HandleArtistSigned);
    }

    Super::Deinitialize();
}

void UUIManagerSubsystem::RegisterLayout(ULayout* InLayout)
{
    ActiveLayout = InLayout;
}

void UUIManagerSubsystem::ShowAudition(const FAuditionEvent& EventData)
{
    if (ULayout* Layout = ActiveLayout.Get())
    {
        Layout->ShowAuditionWidgetWithData(EventData);
    }
}

void UUIManagerSubsystem::HandleNewsCardSelected(const FMusicNewsEvent& EventData)
{
    ExecuteOnGameThread([this, EventData]()
    {
        OnNewsSelected.Broadcast(EventData);
    });
}

void UUIManagerSubsystem::HandleArtistSigned(const FArtistContract& Contract)
{
    if (ULayout* Layout = ActiveLayout.Get())
    {
        Layout->ShowContract(Contract);
    }
}

