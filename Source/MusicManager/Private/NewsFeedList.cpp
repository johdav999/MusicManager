// File: Private/NewsFeedList.cpp
#include "NewsFeedList.h"

#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EventTickerWidget.h"
#include "Types/SlateEnums.h"

DEFINE_LOG_CATEGORY(LogNewsFeedList);

UNewsFeedList::UNewsFeedList(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UNewsFeedList::NativeConstruct()
{
    Super::NativeConstruct();

    if (!ensure(IsInGameThread()))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("NativeConstruct called off the game thread."));
        return;
    }

    UWorld* const World = GetWorld();
    if (!IsValid(World))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("NativeConstruct: World is invalid."));
        return;
    }

    if (UGameInstance* GameInstance = World->GetGameInstance())
    {
        if (UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>())
        {
            TimeSubsystem->OnMonthAdvanced.AddDynamic(this, &UNewsFeedList::HandleMonthAdvanced);
            TimeSubsystemWeak = TimeSubsystem;
        }
        else
        {
            UE_LOG(LogNewsFeedList, Warning, TEXT("NativeConstruct: Failed to locate UGameTimeSubsystem."));
        }
    }
    else
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("NativeConstruct: GameInstance is invalid."));
    }
}

void UNewsFeedList::NativeDestruct()
{
    if (!ensure(IsInGameThread()))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("NativeDestruct called off the game thread."));
    }

    if (UGameTimeSubsystem* TimeSubsystem = TimeSubsystemWeak.Get())
    {
        TimeSubsystem->OnMonthAdvanced.RemoveDynamic(this, &UNewsFeedList::HandleMonthAdvanced);
    }

    TimeSubsystemWeak.Reset();

    Super::NativeDestruct();
}

UEventTickerWidget* UNewsFeedList::AddNewsCard(const FMusicNewsEvent& Event)
{
    if (!ensure(IsInGameThread()))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("AddNewsCard called off the game thread."));
        return nullptr;
    }

    if (!IsValid(FeedContainer))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("AddNewsCard: FeedContainer is invalid."));
        return nullptr;
    }

    if (!EventTickerWidgetClass)
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("AddNewsCard: EventTickerWidgetClass is not set."));
        return nullptr;
    }

    UWorld* const World = GetWorld();
    if (!IsValid(World))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("AddNewsCard: World is invalid."));
        return nullptr;
    }

    UEventTickerWidget* const NewCard = CreateWidget<UEventTickerWidget>(World, EventTickerWidgetClass);
    if (!IsValid(NewCard))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("AddNewsCard: Failed to create event ticker widget."));
        return nullptr;
    }

    NewCard->SetNewsEvent(Event);

    if (UVerticalBoxSlot* const slot = FeedContainer->AddChildToVerticalBox(NewCard))
    {
       slot->SetHorizontalAlignment(HAlign_Fill);
        return NewCard;
    }

    UE_LOG(LogNewsFeedList, Warning, TEXT("AddNewsCard: Failed to add card to container."));
    if (IsValid(NewCard))
    {
        NewCard->RemoveFromParent();
    }

    return nullptr;
}

bool UNewsFeedList::RemoveNewsCard(UEventTickerWidget* Card)
{
    if (!ensure(IsInGameThread()))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("RemoveNewsCard called off the game thread."));
        return false;
    }

    if (!IsValid(FeedContainer))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("RemoveNewsCard: FeedContainer is invalid."));
        return false;
    }

    if (!IsValid(Card))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("RemoveNewsCard: Card is invalid."));
        return false;
    }

    if (!FeedContainer->HasChild(Card))
    {
        UE_LOG(LogNewsFeedList, Verbose, TEXT("RemoveNewsCard: Card is not a child of the feed."));
        return false;
    }

    return FeedContainer->RemoveChild(Card);
}

bool UNewsFeedList::MoveNewsCardToTop(UEventTickerWidget* Card)
{
    if (!ensure(IsInGameThread()))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("MoveNewsCardToTop called off the game thread."));
        return false;
    }

    if (!IsValid(FeedContainer))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("MoveNewsCardToTop: FeedContainer is invalid."));
        return false;
    }

    if (!IsValid(Card))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("MoveNewsCardToTop: Card is invalid."));
        return false;
    }

    const int32 ChildIndex = FeedContainer->GetChildIndex(Card);
    if (ChildIndex == INDEX_NONE)
    {
        UE_LOG(LogNewsFeedList, Verbose, TEXT("MoveNewsCardToTop: Card not found in feed."));
        return false;
    }

    if (ChildIndex == 0)
    {
        return true;
    }

    if (!FeedContainer->RemoveChild(Card))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("MoveNewsCardToTop: Failed to remove card from feed."));
        return false;
    }

    if (!IsValid(Card))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("MoveNewsCardToTop: Card became invalid after removal."));
        return false;
    }

    if (UPanelSlot* PanelSlot = FeedContainer->InsertChildAt(0, Card))
    {
        if (UVerticalBoxSlot* slot = Cast<UVerticalBoxSlot>(PanelSlot))
        {
            slot->SetHorizontalAlignment(HAlign_Fill);
        }
        return true;
    }



    UE_LOG(LogNewsFeedList, Warning, TEXT("MoveNewsCardToTop: Failed to insert card at top."));
    return false;
}

void UNewsFeedList::HandleMonthAdvanced(const FDateTime& NewDate)
{
    if (!ensure(IsInGameThread()))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("HandleMonthAdvanced called off the game thread."));
        return;
    }

    // Minimal integration example: create a synthetic news item whenever the centralized
    // time subsystem advances the simulation by one month.
    FMusicNewsEvent NewEvent;
    NewEvent.NewsId = FGuid::NewGuid();
    NewEvent.Timestamp = NewDate;
    NewEvent.NewsType = EMusicNewsType::IndustryTrend;

    const FString MonthYearString = NewDate.ToString(TEXT("%B %Y"));
    NewEvent.SourceName = TEXT("Global Market Desk");
    NewEvent.SubjectName = MonthYearString;
    NewEvent.Headline = FString::Printf(TEXT("%s Market Recap Released"), *MonthYearString);
    NewEvent.BodyText = FString::Printf(TEXT("UGameTimeSubsystem broadcasted the arrival of %s, triggering an automatic news feed refresh."), *MonthYearString);
    NewEvent.Tags = { TEXT("Auto"), TEXT("TimeSubsystem") };

    AddNewsCard(NewEvent);
}
