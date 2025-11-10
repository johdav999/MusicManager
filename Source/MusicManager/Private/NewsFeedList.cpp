// File: Private/NewsFeedList.cpp
#include "NewsFeedList.h"

#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EventTickerWidget.h"
#include "Engine/World.h"
#include "Types/SlateEnums.h"

DEFINE_LOG_CATEGORY(LogNewsFeedList);

UNewsFeedList::UNewsFeedList(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UNewsFeedList::NativeConstruct()
{
    Super::NativeConstruct();
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

    if (UVerticalBoxSlot* const Slot = FeedContainer->AddChildToVerticalBox(NewCard))
    {
        Slot->SetHorizontalAlignment(HAlign_Fill);
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

    if (UVerticalBoxSlot* const Slot = FeedContainer->InsertChildAt(0, Card))
    {
        Slot->SetHorizontalAlignment(HAlign_Fill);
        return true;
    }

    UE_LOG(LogNewsFeedList, Warning, TEXT("MoveNewsCardToTop: Failed to insert card at top."));
    return false;
}
