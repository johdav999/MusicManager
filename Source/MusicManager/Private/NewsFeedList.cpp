// File: Private/NewsFeedList.cpp
#include "NewsFeedList.h"

#include "Async/Async.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EventTickerWidget.h"
#include "NewsFeedItemWidget.h"
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

    if (!IsValid(HoverCanvas))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("NativeConstruct: HoverCanvas binding is missing."));
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
    UE_LOG(LogNewsFeedList, Warning,
        TEXT("NewsFeedItemWidgetClass = %s"),
        *GetNameSafe(NewsFeedItemWidgetClass));

    UE_LOG(LogNewsFeedList, Warning,
        TEXT("NewsFeedList runtime class = %s"),
        *GetClass()->GetPathName());

    EnsureHoverTicker();
}

void UNewsFeedList::NativeDestruct()
{
    HideHover();
    ActiveHoverItem = nullptr;
    Super::NativeDestruct();
}

UNewsFeedItemWidget* UNewsFeedList::AddNewsCard(const FMusicNewsEvent& Event)
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

    if (!NewsFeedItemWidgetClass)
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("AddNewsCard: NewsFeedItemWidgetClass is not set."));
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

    UNewsFeedItemWidget* const NewCard = CreateWidget<UNewsFeedItemWidget>(World, NewsFeedItemWidgetClass);
    if (!IsValid(NewCard))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("AddNewsCard: Failed to create news feed item widget."));
        return nullptr;
    }

    NewCard->SetOwnerList(this);
    NewCard->SetupFromEvent(Event);

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

bool UNewsFeedList::RemoveNewsCard(UNewsFeedItemWidget* Card)
{
    if (!ensure(IsInGameThread()))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("RemoveNewsCard called off the game thread."));
        TWeakObjectPtr<UNewsFeedList> WeakThis(this);
        TWeakObjectPtr<UNewsFeedItemWidget> WeakCard(Card);
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

    if (ActiveHoverItem.Get() == Card)
    {
        HideHover();
    }

    if (!FeedContainer->HasChild(Card))
    {
        UE_LOG(LogNewsFeedList, Verbose, TEXT("RemoveNewsCard: Card is not a child of the feed."));
        return false;
    }

    return FeedContainer->RemoveChild(Card);
}

bool UNewsFeedList::MoveNewsCardToTop(UNewsFeedItemWidget* Card)
{
    if (!ensure(IsInGameThread()))
    {
        UE_LOG(LogNewsFeedList, Warning, TEXT("MoveNewsCardToTop called off the game thread."));
        TWeakObjectPtr<UNewsFeedList> WeakThis(this);
        TWeakObjectPtr<UNewsFeedItemWidget> WeakCard(Card);
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

void UNewsFeedList::HandleItemHovered(UNewsFeedItemWidget* Item)
{
    if (!IsValid(Item))
    {
        return;
    }

    if (ActiveHoverItem.Get() == Item)
    {
        return;
    }

    ShowHoverForItem(Item, Item->GetNewsEvent(), Item->GetCachedGeometry());
}

void UNewsFeedList::HandleItemUnhovered(UNewsFeedItemWidget* Item)
{
    if (!IsValid(Item))
    {
        return;
    }

    if (ActiveHoverItem.Get() == Item)
    {
        HideHover();
    }
}

void UNewsFeedList::HandleItemToggled(UNewsFeedItemWidget* Item)
{
    if (!IsValid(Item))
    {
        return;
    }

    if (ActiveHoverItem.Get() == Item && ActiveHoverTicker && ActiveHoverTicker->IsVisible())
    {
        HideHover();
        return;
    }

    ShowHoverForItem(Item, Item->GetNewsEvent(), Item->GetCachedGeometry());
}

UEventTickerWidget* UNewsFeedList::GetHoverTicker()
{
    EnsureHoverTicker();
    return ActiveHoverTicker;
}

void UNewsFeedList::EnsureHoverTicker()
{
    if (ActiveHoverTicker || !HoverCanvas || !HoverTickerWidgetClass)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    ActiveHoverTicker = CreateWidget<UEventTickerWidget>(World, HoverTickerWidgetClass);
    if (!ActiveHoverTicker)
    {
        return;
    }

    UCanvasPanelSlot* Slot = HoverCanvas->AddChildToCanvas(ActiveHoverTicker);
    Slot->SetAutoSize(true);
    Slot->SetAnchors(FAnchors(0.f, 0.f));
    Slot->SetAlignment(FVector2D(0.f, 0.f));

    ActiveHoverTicker->SetVisibility(ESlateVisibility::Visible);
}

void UNewsFeedList::ShowHoverForItem(UNewsFeedItemWidget* Item, const FMusicNewsEvent& Event, const FGeometry& ItemGeometry)
{
    EnsureHoverTicker();

    if (!ActiveHoverTicker)
    {
        return;
    }

    ActiveHoverItem = Item;
    ActiveHoverTicker->SetNewsEvent(Event);

    const FVector2D ItemScreenPos = ItemGeometry.GetAbsolutePosition();
    const FVector2D HoverSize = ActiveHoverTicker->GetDesiredSize();

    constexpr float HoverOffsetX = 20.f;
    const FVector2D DesiredPos(
        ItemScreenPos.X - HoverSize.X - HoverOffsetX,
        ItemScreenPos.Y
    );

    if (UCanvasPanelSlot* slot = Cast<UCanvasPanelSlot>(ActiveHoverTicker->Slot))
    {
        Slot->SetPosition(DesiredPos);
    }


}

void UNewsFeedList::HideHover()
{
    if (ActiveHoverTicker)
    {
        ActiveHoverTicker->SetVisibility(ESlateVisibility::Collapsed);
    }

    ActiveHoverItem = nullptr;
}
