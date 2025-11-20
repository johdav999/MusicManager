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
#include "ArtistManagerSubsystem.h"
#include "Engine/GameInstance.h"

ULayout::ULayout(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void ULayout::NativeConstruct()
{
    Super::NativeConstruct();

    // Ensure our UI is listening for contract updates as soon as it is ready.
    InitializeContractSubscriptions();
}

void ULayout::NativeDestruct()
{
    // Clean up delegates before the widget is destroyed to prevent dangling callbacks.
    CleanupContractSubscriptions();

    Super::NativeDestruct();
}

void ULayout::InitializeContractSubscriptions()
{
    if (!IsInGameThread())
    {
        // Re-run on the game thread to avoid touching UObjects from worker threads.
        const TWeakObjectPtr<ULayout> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (ULayout* StrongThis = WeakThis.Get())
            {
                StrongThis->InitializeContractSubscriptions();
            }
        });
        return;
    }

    if (!IsValid(GetGameInstance()))
    {
        return;
    }

    if (UArtistManagerSubsystem* ArtistSubsystem = GetGameInstance()->GetSubsystem<UArtistManagerSubsystem>())
    {
        ArtistManagerSubsystemWeak = ArtistSubsystem;

        if (!ArtistSubsystem->OnArtistSigned.IsAlreadyBound(this, &ULayout::HandleArtistSigned))
        {
            // Listen for new contract events so the UI can update automatically.
            ArtistSubsystem->OnArtistSigned.AddDynamic(this, &ULayout::HandleArtistSigned);
        }
    }
}

void ULayout::CleanupContractSubscriptions()
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<ULayout> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (ULayout* StrongThis = WeakThis.Get())
            {
                StrongThis->CleanupContractSubscriptions();
            }
        });
        return;
    }

    if (UArtistManagerSubsystem* ArtistSubsystem = ArtistManagerSubsystemWeak.Get())
    {
        if (ArtistSubsystem->OnArtistSigned.IsAlreadyBound(this, &ULayout::HandleArtistSigned))
        {
            ArtistSubsystem->OnArtistSigned.RemoveDynamic(this, &ULayout::HandleArtistSigned);
        }
    }

    ArtistManagerSubsystemWeak.Reset();
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

void ULayout::HandleArtistSigned(const FArtistContract& SignedContract)
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<ULayout> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, SignedContract]()
        {
            if (ULayout* StrongThis = WeakThis.Get())
            {
                StrongThis->HandleArtistSigned(SignedContract);
            }
        });
        return;
    }

    if (!IsValid(ContractWidget))
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleArtistSigned: ContractWidget is not bound on layout %s."), *GetName());
        return;
    }

    // Update the widget with the newly signed contract and ensure it is visible to the player.
    ContractWidget->SetContractData(SignedContract);

    if (!ContractWidget->IsVisible())
    {
        ContractWidget->SetVisibility(ESlateVisibility::Visible);
    }
}

void ULayout::ShowAuditionWidgetWithData(const FAuditionEvent& EventData)
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<ULayout> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, EventData]()
        {
            if (ULayout* Self = WeakThis.Get())
            {
                Self->ShowAuditionWidgetWithData(EventData);
            }
        });
        return;
    }

    if (!IsValid(this))
    {
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
    if (IsInGameThread())
    {
        ShowAuditionWidget_Internal();
        return;
    }

    TWeakObjectPtr<ULayout> WeakThis(this);
    AsyncTask(ENamedThreads::GameThread, [WeakThis]()
    {
        if (ULayout* Self = WeakThis.Get())
        {
            Self->ShowAuditionWidget_Internal();
        }
    });
}

void ULayout::ShowAuditionWidget_Internal()
{
    if (!IsValid(this))
    {
        return;
    }

    if (!IsValid(AuditionWidget))
    {
        return;
    }

    AuditionWidget->CreateDummyAudition();

    if (!AuditionWidget->IsVisible())
    {
        AuditionWidget->SetVisibility(ESlateVisibility::Visible);
    }
}

UAuditionWidget* ULayout::GetAuditionWidget() const
{
    // The layout owns the widget through the blueprint hierarchy, so no extra validation is required here.
    return AuditionWidget;
}
