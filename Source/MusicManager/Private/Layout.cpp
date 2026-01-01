// File: Private/Layout.cpp
#include "Layout.h"

#include "Async/Async.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "NewsFeedList.h"
#include "EventTickerWidget.h"
#include "NewsFeedItemWidget.h"
#include "AuditionWidget.h"
#include "Types/SlateEnums.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Engine/GameInstance.h"
#include "ArtistManagerSubsystem.h"
#include "UIManagerSubsystem.h"
#include "UI/HoverTooltipManagerWidget.h"
#include "UI/ArtistHoverDetailWidget.h"
#include "UI/InspectorPanelWidget.h"
#include "UI/MainCanvasHost.h"
#include "UI/SignedArtistPanelWidget.h"
#include "UI/RegionMapWidget.h"
#include "Components/CanvasPanelSlot.h"

ULayout::ULayout(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void ULayout::NativeConstruct()
{
    Super::NativeConstruct();

    // Root layout registers with the UI manager; widgets should not spawn other widgets directly.
    if (UUIManagerSubsystem* UIManager = GetUIManagerSubsystem())
    {
        if (IsValid(UIManager))
        {
            UIManager->RegisterLayout(this);
        }
    }

    if (Layer2_Root)
    {
        Layer2_Root->SetVisibility(ESlateVisibility::Visible);
    }

    if (MainCanvasHost)
    {
        MainCanvasHost->SetCanvasState(ECanvasState::Overview);
    }

    if (!RegionMapWidget && WidgetTree)
    {
        if (UWidget* FoundWidget = WidgetTree->FindWidget(TEXT("RegionMapWidget")))
        {
            RegionMapWidget = Cast<URegionMapWidget>(FoundWidget);
        }
    }

    if (RegionMapWidget)
    {
        RegionMapWidget->RefreshRegions();
    }

    if (IsValid(SignedArtistsPanel))
    {
        SignedArtistsPanel->OnArtistSelected.AddDynamic(this, &ULayout::HandleArtistSelected);
    }
}

void ULayout::NativeDestruct()
{
    if (IsValid(SignedArtistsPanel))
    {
        SignedArtistsPanel->OnArtistSelected.RemoveDynamic(this, &ULayout::HandleArtistSelected);
    }

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
    UE_LOG(LogTemp, Warning, TEXT("Trying to add NewsCard"));
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

    if (UNewsFeedItemWidget* NewItem = NewsFeedList->AddNewsCard(Event))
    {
        BindTickerEvents(NewItem);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AddNewsCardToFeed: Failed to add news card."));
    }
}

void ULayout::RemoveNewsCardFromFeed(UNewsFeedItemWidget* Card)
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

void ULayout::BindTickerEvents(UNewsFeedItemWidget* NewItem)
{
    UE_LOG(LogTemp, Display, TEXT("Bind ticker event!"));
    if (!ensure(IsInGameThread()))
    {
        return;
    }

    if (!IsValid(NewItem))
    {
        return;
    }

    if (!IsValid(NewsFeedList))
    {
        return;
    }

    UEventTickerWidget* Ticker = NewsFeedList->GetHoverTicker();
    if (!IsValid(Ticker))
    {
        UE_LOG(LogTemp, Warning, TEXT("BindTickerEvents: Hover ticker missing for news feed item."));
        return;
    }

    Ticker->OnNewsCardClicked.Clear();
    Ticker->OnNewsCardClicked.AddDynamic(this, &ULayout::HandleTickerClicked);
    Ticker->SetLayoutReference(this);
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

void ULayout::ShowRegionMap()
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<ULayout> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (ULayout* Strong = WeakThis.Get())
            {
                Strong->ShowRegionMap();
            }
        });
        return;
    }

    if (!RegionMapWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("RegionMapWidget is not initialized"));
        return;
    }

    RegionMapWidget->SetVisibility(ESlateVisibility::Visible);
    RegionMapWidget->RefreshRegions();
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

    UArtistManagerSubsystem* ArtistSub = GetArtistManagerSubsystem();
    if (!ArtistSub)
    {
        return;
    }

    FArtistData ArtistForAudition;
    if (!ArtistSub->GetNextUnsignedArtist(ArtistForAudition))
    {
        UE_LOG(LogTemp, Warning, TEXT("No unsigned artists available for audition."));
        return;
    }

    AuditionWidget->CreateAuditionFromArtist(ArtistForAudition);
    const FAuditionEvent EventData = AuditionWidget->AuditionData;

    if (UUIManagerSubsystem* UI = GetUIManagerSubsystem())
    {
        UI->ShowAudition(EventData);
    }
}

void ULayout::CloseAuditionWidget()
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<ULayout> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (ULayout* Strong = WeakThis.Get())
            {
                Strong->CloseAuditionWidget();
            }
        });
        return;
    }

    if (IsValid(AuditionWidget))
    {
        if (AuditionWidget->IsVisible())
        {
            AuditionWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void ULayout::ShowRecordWidget()
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<ULayout> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (ULayout* Strong = WeakThis.Get())
            {
                Strong->ShowRecordWidget();
            }
        });
        return;
    }

    if (!IsValid(RecordWidget))
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowRecordWidget: RecordWidget is invalid."));
        return;
    }

    // Resolve selected artist
    FString ArtistId;
    if (UArtistManagerSubsystem* ArtistSub = GetArtistManagerSubsystem())
    {
        ArtistId = ArtistSub->GetSelectedArtist();
    }

    if (ArtistId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowRecordWidget: No selected artist. Unable to populate songs."));
    }
    else
    {
        // Initialize RecordWidget for selected artist
        RecordWidget->InitializeForArtist(ArtistId);
    }

    UE_LOG(LogTemp, Warning, TEXT("Making RecordWidget visible"));
    RecordWidget->SetVisibility(ESlateVisibility::Visible);
}

void ULayout::CloseRecordWidget()
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<ULayout> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (ULayout* Strong = WeakThis.Get())
            {
                Strong->CloseRecordWidget();
            }
        });
        return;
    }

    if (IsValid(RecordWidget))
    {
        RecordWidget->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void ULayout::RefreshSignedArtists(const TArray<FArtistData>& Artists)
{
    if (!IsValid(SignedArtistsPanel))
    {
        return;
    }

    SignedArtistsPanel->PopulateArtistList(Artists);
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

void ULayout::ShowHoverTooltip(const FTooltipData& Data)
{
    if (IsValid(HoverTooltipManager))
    {
        HoverTooltipManager->ShowTooltip(Data);
    }
}

void ULayout::HideHoverTooltip()
{
    if (IsValid(HoverTooltipManager))
    {
        HoverTooltipManager->HideTooltip();
    }
}

void ULayout::ShowArtistHoverDetail(const FArtistData& ArtistData, const FVector2D& ScreenPosition)
{
    if (!IsValid(ArtistHoverDetailWidget))
    {
        return;
    }

    // Layer-2 only: informational hover details, no actions.
    ArtistHoverDetailWidget->SetupFromArtistData(ArtistData);
    ArtistHoverDetailWidget->SetVisibility(ESlateVisibility::Visible);

    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ArtistHoverDetailWidget->Slot))
    {
        CanvasSlot->SetPosition(ScreenPosition);
    }
    else
    {
        ArtistHoverDetailWidget->SetPositionInViewport(ScreenPosition, false);
    }
}

void ULayout::HideArtistHoverDetail()
{
    if (!IsValid(ArtistHoverDetailWidget))
    {
        return;
    }

    ArtistHoverDetailWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void ULayout::SetLayer2Enabled(bool bEnabled)
{
    if (!Layer2_Root)
    {
        return;
    }

    Layer2_Root->SetVisibility(bEnabled ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void ULayout::HandleArtistSelected(FString ArtistId)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<ULayout> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, ArtistId]()
        {
            if (ULayout* Self = WeakThis.Get())
            {
                Self->HandleArtistSelected(ArtistId);
            }
        });
        return;
    }

    if (UArtistManagerSubsystem* ArtistSubsystem = GetArtistManagerSubsystem())
    {
        ArtistSubsystem->SetSelectedArtist(ArtistId);
    }

    if (UUIManagerSubsystem* UI = GetUIManagerSubsystem())
    {
        // Route selection through the UI manager so the inspector panel can update.
        UI->SetSelectedEntity(GetArtistManagerSubsystem());
    }
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

UArtistManagerSubsystem* ULayout::GetArtistManagerSubsystem() const
{
    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        return nullptr;
    }

    return GI->GetSubsystem<UArtistManagerSubsystem>();
}
