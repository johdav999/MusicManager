// File: Private/Layout.cpp
#include "Layout.h"

#include "Async/Async.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "EventTickerWidget.h"
#include "NewsFeedList.h"
#include "AuditionWidget.h"
#include "Types/SlateEnums.h"
#include "UObject/WeakObjectPtrTemplates.h"

ULayout::ULayout(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

UUserWidget* ULayout::GetChildByNameOrClass(FName WidgetName, TSubclassOf<UUserWidget> WidgetClass) const
{
    check(IsInGameThread());
    ensureMsgf(IsInGameThread(), TEXT("UI access must be on game thread"));

    if (!WidgetTree)
    {
        return nullptr;
    }

    if (!WidgetName.IsNone())
    {
        if (UWidget* NamedWidget = WidgetTree->FindWidget(WidgetName))
        {
            if (UUserWidget* NamedUserWidget = Cast<UUserWidget>(NamedWidget))
            {
                return NamedUserWidget;
            }
        }
    }

    if (WidgetClass)
    {
        UUserWidget* FoundWidget = nullptr;
        WidgetTree->ForEachWidget([WidgetClass, &FoundWidget](UWidget* Widget)
        {
            if (!FoundWidget && Widget && Widget->IsA(WidgetClass))
            {
                FoundWidget = Cast<UUserWidget>(Widget);
            }
        });

        if (FoundWidget)
        {
            return FoundWidget;
        }
    }

    return nullptr;
}

void ULayout::AddNewsCardToFeed(const FMusicNewsEvent& Event)
{
    if (!ensure(IsInGameThread()))
    {
        UE_LOG(LogTemp, Warning, TEXT("AddNewsCardToFeed called off the game thread."));
        return;
    }

    if (!IsValid(NewsFeedList))
    {
        UE_LOG(LogTemp, Warning, TEXT("AddNewsCardToFeed: NewsFeedList is invalid on layout %s."), *GetName());
        return;
    }

    if (UEventTickerWidget* NewTicker = NewsFeedList->AddNewsCard(Event))
    {
        BindTickerEvents(NewTicker);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AddNewsCardToFeed: Failed to add news card."));
    }
}

void ULayout::RemoveNewsCardFromFeed(UEventTickerWidget* Card)
{
    if (!ensure(IsInGameThread()))
    {
        UE_LOG(LogTemp, Warning, TEXT("RemoveNewsCardFromFeed called off the game thread."));
        return;
    }

    if (!IsValid(Card))
    {
        UE_LOG(LogTemp, Warning, TEXT("RemoveNewsCardFromFeed: Card is invalid."));
        return;
    }

    if (!IsValid(NewsFeedList))
    {
        UE_LOG(LogTemp, Warning, TEXT("RemoveNewsCardFromFeed: NewsFeedList is invalid on layout %s."), *GetName());
        return;
    }

    if (!NewsFeedList->RemoveNewsCard(Card))
    {
        UE_LOG(LogTemp, Warning, TEXT("RemoveNewsCardFromFeed: Failed to remove news card."));
    }
}

void ULayout::BindTickerEvents(UEventTickerWidget* NewTicker)
{
    if (!ensure(IsInGameThread()))
    {
        return;
    }

    if (!IsValid(NewTicker))
    {
        return;
    }

    NewTicker->OnNewsCardClicked.Clear();
    NewTicker->OnNewsCardClicked.AddDynamic(this, &ULayout::HandleTickerClicked);
    NewTicker->SetLayoutReference(this);
}

void ULayout::HandleTickerClicked(UEventTickerWidget* ClickedTicker)
{
    if (!IsInGameThread())
    {
        AsyncTask(ENamedThreads::GameThread, [WeakThis = TWeakObjectPtr<ULayout>(this), WeakTicker = TWeakObjectPtr<UEventTickerWidget>(ClickedTicker)]()
        {
            if (ULayout* Self = WeakThis.Get())
            {
                if (UEventTickerWidget* Ticker = WeakTicker.Get())
                {
                    Self->HandleTickerClicked(Ticker);
                }
            }
        });
        return;
    }

    if (IsValid(ClickedTicker))
    {
        OnNewsCardSelected.Broadcast(ClickedTicker);
    }
}

void ULayout::ShowAuditionWidget()
{
    const TWeakObjectPtr<ULayout> WeakThis(this);
    AsyncTask(ENamedThreads::GameThread, [WeakThis]()
    {
        if (ULayout* Self = WeakThis.Get())
        {
            if (!IsValid(Self))
            {
                return;
            }

            if (!IsValid(Self->AuditionWidget))
            {
                return;
            }

            if (!Self->AuditionWidget->IsVisible())
            {
                Self->AuditionWidget->SetVisibility(ESlateVisibility::Visible);
            }
        }
    });
}
