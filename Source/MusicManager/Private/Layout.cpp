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
#include "Engine/GameInstance.h"
#include "UIManagerSubsystem.h"

ULayout::ULayout(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void ULayout::NativeConstruct()
{
    Super::NativeConstruct();

    if (UUIManagerSubsystem* UIManager = GetUIManagerSubsystem())
    {
        if (IsValid(UIManager))
        {
            UIManager->RegisterLayout(this);
        }
    }
}

void ULayout::NativeDestruct()
{
    if (UUIManagerSubsystem* UIManager = GetUIManagerSubsystem())
    {
        if (IsValid(UIManager))
        {
            UIManager->UnregisterLayout(this);
        }
    }

    Super::NativeDestruct();
}

UUserWidget* ULayout::GetChildByNameOrClass(FName WidgetName, TSubclassOf<UUserWidget> WidgetClass) const
{
    check(IsInGameThread());

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
        TWeakObjectPtr<ULayout> WeakThis(this);
        TWeakObjectPtr<UEventTickerWidget> WeakTicker(ClickedTicker);

        AsyncTask(ENamedThreads::GameThread, [WeakThis, WeakTicker]()
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

    if (!IsValid(ClickedTicker))
    {
        return;
    }

    if (UUIManagerSubsystem* UI = GetUIManagerSubsystem())
    {
        UI->HandleNewsCardSelected(ClickedTicker->NewsEvent);
    }
}

void ULayout::ShowAuditionWidgetWithData(const FAuditionEvent& EventData)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<ULayout> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, EventData]()
        {
            if (ULayout* Self = WeakThis.Get())
            {
                Self->ShowAuditionWidgetWithData(EventData);
            }
        });
        return;
    }

    if (!IsValid(AuditionWidget))
    {
        return;
    }

    AuditionWidget->AuditionData = EventData;

    if (!AuditionWidget->IsVisible())
    {
        AuditionWidget->SetVisibility(ESlateVisibility::Visible);
    }
}

void ULayout::ShowAuditionWidget()
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<ULayout> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (ULayout* Self = WeakThis.Get())
            {
                Self->ShowAuditionWidget();
            }
        });
        return;
    }

    if (!IsValid(AuditionWidget))
    {
        return;
    }

    AuditionWidget->CreateDummyAudition();
    const FAuditionEvent EventData = AuditionWidget->AuditionData;

    if (UUIManagerSubsystem* UI = GetUIManagerSubsystem())
    {
        UI->ShowAudition(EventData);
    }
}

void ULayout::ShowContract(const FArtistContract& SignedContract)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<ULayout> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, SignedContract]()
        {
            if (ULayout* Self = WeakThis.Get())
            {
                Self->ShowContract(SignedContract);
            }
        });
        return;
    }

    if (!IsValid(ContractWidget))
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowContract: ContractWidget is not bound on layout %s."), *GetName());
        return;
    }

    ContractWidget->SetContractData(SignedContract);

    if (!ContractWidget->IsVisible())
    {
        ContractWidget->SetVisibility(ESlateVisibility::Visible);
    }
}

UAuditionWidget* ULayout::GetAuditionWidget() const
{
    // The layout owns the widget through the blueprint hierarchy, so no extra validation is required here.
    return AuditionWidget;
}

UUIManagerSubsystem* ULayout::GetUIManagerSubsystem() const
{
    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        return nullptr;
    }

    return GI->GetSubsystem<UUIManagerSubsystem>();
}
