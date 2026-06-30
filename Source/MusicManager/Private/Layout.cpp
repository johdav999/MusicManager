// File: Private/Layout.cpp
#include "Layout.h"

#include "Async/Async.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Components/CanvasPanel.h"
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
#include "UI/TopStatusBarWidget.h"
#include "UI/ActiveContractsWidget.h"
#include "UI/RegionMapWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"

namespace
{
    constexpr TCHAR PreferredArtistAuditionPanelClassPath[] = TEXT("/Game/GUI/Audition/ArtistAuditionPanelBP.ArtistAuditionPanelBP_C");
    constexpr TCHAR LegacyAuditionPanelClassPath[] = TEXT("/Game/GUI/AuditionBP.AuditionBP_C");
    constexpr TCHAR RecordingWidgetClassPath[] = TEXT("/Game/GUI/RecordingGUIBP.RecordingGUIBP_C");
    constexpr TCHAR ActiveContractsWidgetClassPath[] = TEXT("/Game/GUI/Contracts/ActiveContractsBP.ActiveContractsBP_C");

    bool IsLegacyAuditionPanelClass(const TSubclassOf<UAuditionWidget>& WidgetClass)
    {
        const UClass* Class = WidgetClass.Get();
        return Class && Class->GetPathName() == LegacyAuditionPanelClassPath;
    }
}

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

    EnsureTopStatusBarWidget();
    EnsureArtistAuditionPanelWidget();
    EnsureRecordWidget();
    EnsureActiveContractsWidget();

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

    if (IsValid(NewsFeedList))
    {
        NewsFeedList->OnNewsFeedItemSelected.RemoveDynamic(this, &ULayout::HandleNewsFeedItemSelected);
        NewsFeedList->OnNewsFeedItemSelected.AddDynamic(this, &ULayout::HandleNewsFeedItemSelected);
    }
}

void ULayout::NativeDestruct()
{
    if (IsValid(NewsFeedList))
    {
        NewsFeedList->OnNewsFeedItemSelected.RemoveDynamic(this, &ULayout::HandleNewsFeedItemSelected);
    }

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

void ULayout::EnsureTopStatusBarWidget()
{
    if (IsValid(TopStatusBarWidget))
    {
        TopStatusBarWidget->RefreshFromSubsystems();
        return;
    }

    if (!TopStatusBarWidgetClass)
    {
        TopStatusBarWidgetClass = LoadClass<UTopStatusBarWidget>(
            nullptr,
            TEXT("/Game/GUI/HUD/TopStatusBarBP.TopStatusBarBP_C"));
    }

    if (!TopStatusBarWidgetClass)
    {
        TopStatusBarWidgetClass = UTopStatusBarWidget::StaticClass();
    }

    UGameInstance* GameInstance = GetGameInstance();
    if (!IsValid(GameInstance) || !TopStatusBarWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("EnsureTopStatusBarWidget: Could not resolve GameInstance or widget class."));
        return;
    }

    TopStatusBarWidget = CreateWidget<UTopStatusBarWidget>(GameInstance, TopStatusBarWidgetClass);
    if (!IsValid(TopStatusBarWidget))
    {
        UE_LOG(LogTemp, Warning, TEXT("EnsureTopStatusBarWidget: Failed to create top status bar widget."));
        return;
    }

    if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(Layer1_Root))
    {
        UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(TopStatusBarWidget);
        CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
        CanvasSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 72.f));
        CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
    }
    else if (UPanelWidget* Panel = Cast<UPanelWidget>(Layer1_Root))
    {
        Panel->AddChild(TopStatusBarWidget);
    }
    else
    {
        TopStatusBarWidget->AddToViewport(50);
        UE_LOG(LogTemp, Warning, TEXT("EnsureTopStatusBarWidget: Layer1_Root is not a panel, added top status bar to viewport."));
    }

    TopStatusBarWidget->RefreshFromSubsystems();
}

void ULayout::EnsureArtistAuditionPanelWidget()
{
    if (!ArtistAuditionPanelWidgetClass || IsLegacyAuditionPanelClass(ArtistAuditionPanelWidgetClass))
    {
        if (ArtistAuditionPanelWidgetClass)
        {
            UE_LOG(LogTemp, Warning, TEXT("EnsureArtistAuditionPanelWidget: replacing legacy audition panel class '%s' with production panel '%s'."),
                *ArtistAuditionPanelWidgetClass->GetPathName(),
                PreferredArtistAuditionPanelClassPath);
        }

        ArtistAuditionPanelWidgetClass = LoadClass<UAuditionWidget>(
            nullptr,
            PreferredArtistAuditionPanelClassPath);
    }

    if (!ArtistAuditionPanelWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("EnsureArtistAuditionPanelWidget: ArtistAuditionPanelWidgetClass is not set."));
        return;
    }

    if (IsValid(AuditionWidget) && AuditionWidget->GetClass() == ArtistAuditionPanelWidgetClass)
    {
        AuditionWidget->OnSignArtist.RemoveDynamic(this, &ULayout::HandleAuditionSigned);
        AuditionWidget->OnSignArtist.AddDynamic(this, &ULayout::HandleAuditionSigned);
        AuditionWidget->OnPass.RemoveDynamic(this, &ULayout::HandleAuditionPassed);
        AuditionWidget->OnPass.AddDynamic(this, &ULayout::HandleAuditionPassed);
        return;
    }

    if (IsValid(AuditionWidget))
    {
        AuditionWidget->SetVisibility(ESlateVisibility::Collapsed);
    }

    UGameInstance* GameInstance = GetGameInstance();
    if (!IsValid(GameInstance))
    {
        UE_LOG(LogTemp, Warning, TEXT("EnsureArtistAuditionPanelWidget: GameInstance is invalid."));
        return;
    }

    UAuditionWidget* NewAuditionWidget = CreateWidget<UAuditionWidget>(GameInstance, ArtistAuditionPanelWidgetClass);
    if (!IsValid(NewAuditionWidget))
    {
        UE_LOG(LogTemp, Warning, TEXT("EnsureArtistAuditionPanelWidget: Failed to create %s."), *GetNameSafe(ArtistAuditionPanelWidgetClass));
        return;
    }

    if (UCanvasPanel* CanvasRoot = Cast<UCanvasPanel>(Layer1_Root))
    {
        UCanvasPanelSlot* CanvasSlot = CanvasRoot->AddChildToCanvas(NewAuditionWidget);
        CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 1.f));
        CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
        CanvasSlot->SetOffsets(FMargin(12.f, 84.f, 520.f, 20.f));
        CanvasSlot->SetZOrder(20);
    }
    else
    {
        NewAuditionWidget->AddToViewport(20);
        NewAuditionWidget->SetPositionInViewport(FVector2D(12.f, 84.f), false);
    }

    NewAuditionWidget->SetVisibility(ESlateVisibility::Collapsed);
    NewAuditionWidget->OnSignArtist.RemoveDynamic(this, &ULayout::HandleAuditionSigned);
    NewAuditionWidget->OnSignArtist.AddDynamic(this, &ULayout::HandleAuditionSigned);
    NewAuditionWidget->OnPass.RemoveDynamic(this, &ULayout::HandleAuditionPassed);
    NewAuditionWidget->OnPass.AddDynamic(this, &ULayout::HandleAuditionPassed);
    AuditionWidget = NewAuditionWidget;
    UE_LOG(LogTemp, Warning, TEXT("EnsureArtistAuditionPanelWidget: Created audition panel Widget='%s' Class='%s'."),
        *GetNameSafe(NewAuditionWidget),
        *GetPathNameSafe(NewAuditionWidget->GetClass()));
}

void ULayout::EnsureActiveContractsWidget()
{
    if (IsValid(ActiveContractsWidget))
    {
        ActiveContractsWidget->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    if (!ActiveContractsWidgetClass)
    {
        ActiveContractsWidgetClass = LoadClass<UActiveContractsWidget>(nullptr, ActiveContractsWidgetClassPath);
    }

    if (!ActiveContractsWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("EnsureActiveContractsWidget: ActiveContractsWidgetClass is not set and could not load %s."), ActiveContractsWidgetClassPath);
        return;
    }

    UGameInstance* GameInstance = GetGameInstance();
    if (!IsValid(GameInstance))
    {
        UE_LOG(LogTemp, Warning, TEXT("EnsureActiveContractsWidget: GameInstance is invalid."));
        return;
    }

    UActiveContractsWidget* NewContractsWidget = CreateWidget<UActiveContractsWidget>(GameInstance, ActiveContractsWidgetClass);
    if (!IsValid(NewContractsWidget))
    {
        UE_LOG(LogTemp, Warning, TEXT("EnsureActiveContractsWidget: Failed to create %s."), *GetNameSafe(ActiveContractsWidgetClass));
        return;
    }

    if (UCanvasPanel* CanvasRoot = Cast<UCanvasPanel>(Layer1_Root))
    {
        UCanvasPanelSlot* CanvasSlot = CanvasRoot->AddChildToCanvas(NewContractsWidget);
        CanvasSlot->SetAnchors(FAnchors(0.08f, 0.10f, 0.92f, 0.92f));
        CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
        CanvasSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
        CanvasSlot->SetZOrder(34);
    }
    else if (UPanelWidget* Panel = Cast<UPanelWidget>(Layer1_Root))
    {
        Panel->AddChild(NewContractsWidget);
    }
    else
    {
        NewContractsWidget->AddToViewport(34);
        UE_LOG(LogTemp, Warning, TEXT("EnsureActiveContractsWidget: Layer1_Root is not a panel, added contracts widget to viewport."));
    }

    NewContractsWidget->SetVisibility(ESlateVisibility::Collapsed);
    NewContractsWidget->OnCloseRequested.RemoveDynamic(this, &ULayout::HandleActiveContractsCloseRequested);
    NewContractsWidget->OnCloseRequested.AddDynamic(this, &ULayout::HandleActiveContractsCloseRequested);
    ActiveContractsWidget = NewContractsWidget;
    UE_LOG(LogTemp, Warning, TEXT("EnsureActiveContractsWidget: Created active contracts widget Widget='%s' Class='%s'."),
        *GetNameSafe(NewContractsWidget),
        *GetPathNameSafe(NewContractsWidget->GetClass()));
}
void ULayout::EnsureRecordWidget()
{
    if (IsValid(RecordWidget))
    {
        RecordWidget->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    if (!RecordWidgetClass)
    {
        RecordWidgetClass = LoadClass<URecordWidget>(nullptr, RecordingWidgetClassPath);
    }

    if (!RecordWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("EnsureRecordWidget: RecordWidgetClass is not set and could not load %s."), RecordingWidgetClassPath);
        return;
    }

    UGameInstance* GameInstance = GetGameInstance();
    if (!IsValid(GameInstance))
    {
        UE_LOG(LogTemp, Warning, TEXT("EnsureRecordWidget: GameInstance is invalid."));
        return;
    }

    URecordWidget* NewRecordWidget = CreateWidget<URecordWidget>(GameInstance, RecordWidgetClass);
    if (!IsValid(NewRecordWidget))
    {
        UE_LOG(LogTemp, Warning, TEXT("EnsureRecordWidget: Failed to create %s."), *GetNameSafe(RecordWidgetClass));
        return;
    }

    if (UCanvasPanel* CanvasRoot = Cast<UCanvasPanel>(Layer1_Root))
    {
        UCanvasPanelSlot* CanvasSlot = CanvasRoot->AddChildToCanvas(NewRecordWidget);
        CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
        CanvasSlot->SetOffsets(FMargin(24.f, 84.f, 24.f, 24.f));
        CanvasSlot->SetZOrder(30);
    }
    else if (UPanelWidget* Panel = Cast<UPanelWidget>(Layer1_Root))
    {
        Panel->AddChild(NewRecordWidget);
    }
    else
    {
        NewRecordWidget->AddToViewport(30);
        UE_LOG(LogTemp, Warning, TEXT("EnsureRecordWidget: Layer1_Root is not a panel, added recording widget to viewport."));
    }

    NewRecordWidget->SetVisibility(ESlateVisibility::Collapsed);
    RecordWidget = NewRecordWidget;
    UE_LOG(LogTemp, Warning, TEXT("EnsureRecordWidget: Created recording widget Widget='%s' Class='%s'."),
        *GetNameSafe(NewRecordWidget),
        *GetPathNameSafe(NewRecordWidget->GetClass()));
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
    UE_LOG(LogTemp, Warning, TEXT("Layout %s adding news card: Headline='%s' Type=%d."),
        *GetNameSafe(this),
        *Event.Headline,
        static_cast<int32>(Event.NewsType));
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
        UE_LOG(LogTemp, Warning, TEXT("AddNewsCardToFeed: Failed to add news card. Headline='%s'."), *Event.Headline);
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

void ULayout::HandleNewsFeedItemSelected(UNewsFeedItemWidget* Card, const FMusicNewsEvent& EventData)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<ULayout> WeakThis(this);
        TWeakObjectPtr<UNewsFeedItemWidget> WeakCard(Card);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, WeakCard, EventData]()
        {
            if (ULayout* Self = WeakThis.Get())
            {
                Self->HandleNewsFeedItemSelected(WeakCard.Get(), EventData);
            }
        });
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Layout selected news feed item: Card=%s Headline='%s' Type=%d."),
        *GetNameSafe(Card),
        *EventData.Headline,
        static_cast<int32>(EventData.NewsType));

    if (UUIManagerSubsystem* UI = GetUIManagerSubsystem())
    {
        UI->HandleNewsCardSelected(EventData);
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

    EnsureArtistAuditionPanelWidget();

    if (!IsValid(AuditionWidget))
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowAuditionWidgetWithData: AuditionWidget is invalid for artist '%s'."),
            *EventData.ArtistData.ArtistName);
        return;
    }

    AuditionWidget->AuditionData = EventData;
    AuditionWidget->RefreshDisplay();

    if (!AuditionWidget->IsVisible())
    {
        AuditionWidget->SetVisibility(ESlateVisibility::Visible);
    }
}

void ULayout::ShowAuditionWidgetForArtist(const FArtistData& ArtistData)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<ULayout> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, ArtistData]()
        {
            if (ULayout* Self = WeakThis.Get())
            {
                Self->ShowAuditionWidgetForArtist(ArtistData);
            }
        });
        return;
    }

    EnsureArtistAuditionPanelWidget();

    if (!IsValid(AuditionWidget))
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowAuditionWidgetForArtist: AuditionWidget is invalid for artist '%s'."), *ArtistData.ArtistName);
        return;
    }

    AuditionWidget->CreateAuditionFromArtist(ArtistData);
    AuditionWidget->SetVisibility(ESlateVisibility::Visible);

    UE_LOG(LogTemp, Warning, TEXT("ShowAuditionWidgetForArtist: opened audition panel for ArtistId='%s' Name='%s'."),
        *ArtistData.ArtistId,
        *ArtistData.ArtistName);
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

    EnsureArtistAuditionPanelWidget();

    UArtistManagerSubsystem* ArtistSub = GetArtistManagerSubsystem();
    if (!ArtistSub)
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowAuditionWidget: Artist subsystem is unavailable."));
        return;
    }

    if (!IsValid(AuditionWidget))
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowAuditionWidget: AuditionWidget is invalid after ensure."));
        return;
    }

    FArtistData ArtistForAudition;
    if (!ArtistSub->GetNextUnsignedArtist(ArtistForAudition))
    {
        UE_LOG(LogTemp, Warning, TEXT("No unsigned artists available for audition."));
        CloseAuditionWidget();
        return;
    }

    AuditionWidget->CreateAuditionFromArtist(ArtistForAudition);
    AuditionWidget->SetVisibility(ESlateVisibility::Visible);

    UE_LOG(LogTemp, Warning, TEXT("ShowAuditionWidget: opened next audition artist ArtistId='%s' Name='%s'."),
        *ArtistForAudition.ArtistId,
        *ArtistForAudition.ArtistName);
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

void ULayout::HandleAuditionSigned()
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<ULayout> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (ULayout* Strong = WeakThis.Get())
            {
                Strong->HandleAuditionSigned();
            }
        });
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Audition decision handled by layout: signed artist; closing audition panel."));
    CloseAuditionWidget();
}

void ULayout::HandleAuditionPassed()
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<ULayout> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (ULayout* Strong = WeakThis.Get())
            {
                Strong->HandleAuditionPassed();
            }
        });
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Audition decision handled by layout: passed artist; closing audition panel."));
    CloseAuditionWidget();
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

    EnsureRecordWidget();

    if (!IsValid(RecordWidget))
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowRecordWidget: RecordWidget is invalid."));
        return;
    }

    // Resolve selected artist. Older UI paths may still store artist names, but the
    // recording flow needs the stable contract ArtistId.
    FString ArtistId;
    if (UArtistManagerSubsystem* ArtistSub = GetArtistManagerSubsystem())
    {
        ArtistId = ArtistSub->GetSelectedArtist();
        if (!ArtistId.IsEmpty())
        {
            if (const FArtistContract* SelectedContract = ArtistSub->GetContractByArtistId(ArtistId))
            {
                ArtistId = SelectedContract->ArtistId;
            }
            else if (const FArtistContract* SelectedByName = ArtistSub->FindContractByArtistName(ArtistId))
            {
                ArtistId = SelectedByName->ArtistId;
                ArtistSub->SetSelectedArtist(ArtistId);
                UE_LOG(LogTemp, Log, TEXT("ShowRecordWidget: Resolved selected artist name to ArtistId '%s'."), *ArtistId);
            }
        }

        if (ArtistId.IsEmpty() && ArtistSub->ActiveContracts.Num() > 0)
        {
            ArtistId = ArtistSub->ActiveContracts[0].ArtistId;
            ArtistSub->SetSelectedArtist(ArtistId);
            UE_LOG(LogTemp, Warning, TEXT("ShowRecordWidget: No artist was selected; defaulting to first signed artist '%s'."), *ArtistId);
        }
    }

    if (ArtistId.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowRecordWidget: No selected/signed artist. Unable to populate songs."));
    }
    else
    {
        // Initialize RecordWidget for selected artist
        UE_LOG(LogTemp, Log, TEXT("ShowRecordWidget: Initializing recording widget for ArtistId '%s'."), *ArtistId);
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
    ShowActiveContractsWidgetForArtist(SignedContract.ArtistId);
}

void ULayout::ShowActiveContractsWidget()
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<ULayout> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (ULayout* Self = WeakThis.Get())
            {
                Self->ShowActiveContractsWidget();
            }
        });
        return;
    }

    EnsureActiveContractsWidget();

    if (!IsValid(ActiveContractsWidget))
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowActiveContractsWidget: ActiveContractsWidget is invalid."));
        return;
    }

    ActiveContractsWidget->RefreshContracts();
    ActiveContractsWidget->SetVisibility(ESlateVisibility::Visible);
}

void ULayout::ShowActiveContractsWidgetForArtist(const FString& ArtistId)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<ULayout> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, ArtistId]()
        {
            if (ULayout* Self = WeakThis.Get())
            {
                Self->ShowActiveContractsWidgetForArtist(ArtistId);
            }
        });
        return;
    }

    EnsureActiveContractsWidget();

    if (!IsValid(ActiveContractsWidget))
    {
        UE_LOG(LogTemp, Warning, TEXT("ShowActiveContractsWidgetForArtist: ActiveContractsWidget is invalid for ArtistId='%s'."), *ArtistId);
        return;
    }

    ActiveContractsWidget->ShowForArtist(ArtistId);
    ActiveContractsWidget->SetVisibility(ESlateVisibility::Visible);
}

void ULayout::CloseActiveContractsWidget()
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<ULayout> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (ULayout* Self = WeakThis.Get())
            {
                Self->CloseActiveContractsWidget();
            }
        });
        return;
    }

    if (IsValid(ActiveContractsWidget))
    {
        ActiveContractsWidget->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void ULayout::HandleActiveContractsCloseRequested()
{
    CloseActiveContractsWidget();
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
