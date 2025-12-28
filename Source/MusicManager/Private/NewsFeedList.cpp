// File: Private/NewsFeedList.cpp
#include "NewsFeedList.h"

#include "Async/Async.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EventTickerWidget.h"
#include "Types/SlateEnums.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogNewsFeedList);

UNewsFeedList::UNewsFeedList(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UNewsFeedList::NativeConstruct()
{
    Super::NativeConstruct();

    if (!IsValid(FeedScrollBox))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("NativeConstruct: FeedScrollBox binding is missing."));
    }

    if (!IsValid(FeedContainer))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("NativeConstruct: FeedContainer binding is missing."));
    }

    if (IsValid(FeedScrollBox))
    {
        FeedScrollBox->SetAnimateWheelScrolling(true);
        FeedScrollBox->SetScrollBarVisibility(ESlateVisibility::Visible);
    }

    if (IsValid(FeedScrollBox) && IsValid(FeedContainer))
    {
        if (FeedContainer->GetParent() != FeedScrollBox)
        {
            if (UPanelWidget* const ExistingParent = FeedContainer->GetParent())
            {
                ExistingParent->RemoveChild(FeedContainer);
            }

            FeedScrollBox->AddChild(FeedContainer);
        }
    }
}

void UNewsFeedList::NativeDestruct()
{
    Super::NativeDestruct();
}

UEventTickerWidget* UNewsFeedList::AddNewsCard(const FMusicNewsEvent& Event)
{
    if (!ensure(IsInGameThread()))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("AddNewsCard called off the game thread."));
        TWeakObjectPtr<UNewsFeedList> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, Event]()
        {
            if (WeakThis.IsValid())
            {
                WeakThis->AddNewsCard(Event);
            }
        });
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

    if (!IsValid(FeedScrollBox))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("AddNewsCard: FeedScrollBox is invalid."));
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

    if (UPanelSlot* PanelSlot = FeedContainer->InsertChildAt(0, NewCard))
    {
        if (UVerticalBoxSlot* const slot = Cast<UVerticalBoxSlot>(PanelSlot))
        {
            slot->SetHorizontalAlignment(HAlign_Fill);
        }
        if (IsValid(FeedScrollBox))
        {
            FeedScrollBox->ScrollToStart();
        }
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
        TWeakObjectPtr<UNewsFeedList> WeakThis(this);
        TWeakObjectPtr<UEventTickerWidget> WeakCard(Card);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, WeakCard]()
        {
            if (WeakThis.IsValid())
            {
                WeakThis->RemoveNewsCard(WeakCard.Get());
            }
        });
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
        TWeakObjectPtr<UNewsFeedList> WeakThis(this);
        TWeakObjectPtr<UEventTickerWidget> WeakCard(Card);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, WeakCard]()
        {
            if (WeakThis.IsValid())
            {
                WeakThis->MoveNewsCardToTop(WeakCard.Get());
            }
        });
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
        if (IsValid(FeedScrollBox))
        {
            FeedScrollBox->ScrollToStart();
        }
        return true;
    }



    UE_LOG(LogNewsFeedList, Warning, TEXT("MoveNewsCardToTop: Failed to insert card at top."));
    return false;
}
