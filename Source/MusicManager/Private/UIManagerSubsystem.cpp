// File: Private/UIManagerSubsystem.cpp
#include "UIManagerSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Layout.h"
#include "ArtistManagerSubsystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "CommandDispatcherSubsystem.h"
#include "Logging/LogMacros.h"
#include "MusicPlayerComponent.h"
#include "UI/StatusWidget.h"
#include "UI/InspectorPanelWidget.h"
#include "UI/MainCanvasHost.h"

DEFINE_LOG_CATEGORY_STATIC(LogUIManagerSubsystem, Log, All);

void UUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogUIManagerSubsystem, Warning, TEXT("UIManager initialized. News is routed directly by EventSubsystem; OnNewsEventGenerated remains available for non-UI listeners."));

    if (UArtistManagerSubsystem* Artist = GetGameInstance()->GetSubsystem<UArtistManagerSubsystem>())
    {
        Artist->OnArtistSigned.AddDynamic(this, &UUIManagerSubsystem::HandleArtistSigned);
        Artist->OnArtistListChanged.AddDynamic(this, &UUIManagerSubsystem::HandleArtistListChanged);
    }

    if (UCommandDispatcherSubsystem* Commands = GetGameInstance()->GetSubsystem<UCommandDispatcherSubsystem>())
    {
        Commands->OnCommandExecuted.AddDynamic(this, &UUIManagerSubsystem::HandleCommandExecuted);
    }
}

void UUIManagerSubsystem::Deinitialize()
{
    if (UArtistManagerSubsystem* Artist = GetGameInstance()->GetSubsystem<UArtistManagerSubsystem>())
    {
        Artist->OnArtistSigned.RemoveDynamic(this, &UUIManagerSubsystem::HandleArtistSigned);
        Artist->OnArtistListChanged.RemoveDynamic(this, &UUIManagerSubsystem::HandleArtistListChanged);
    }

    if (UCommandDispatcherSubsystem* Commands = GetGameInstance()->GetSubsystem<UCommandDispatcherSubsystem>())
    {
        Commands->OnCommandExecuted.RemoveDynamic(this, &UUIManagerSubsystem::HandleCommandExecuted);
    }

    Super::Deinitialize();
}

void UUIManagerSubsystem::RegisterLayout(ULayout* Layout)
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<UUIManagerSubsystem> WeakThis(this);
        TWeakObjectPtr<ULayout> WeakLayout(Layout);

        AsyncTask(ENamedThreads::GameThread, [WeakThis, WeakLayout]()
        {
            if (UUIManagerSubsystem* Self = WeakThis.Get())
            {
                if (ULayout* StrongLayout = WeakLayout.Get())
                {
                    Self->RegisterLayout(StrongLayout);
                }
            }
        });
        return;
    }

    ActiveLayout = Layout;
    UE_LOG(LogUIManagerSubsystem, Warning, TEXT("Registered active layout %s. Flushing PendingNewsEvents=%d."),
        *GetNameSafe(Layout),
        PendingNewsEvents.Num());

    // Flush any pending news events
    for (const FMusicNewsEvent& Event : PendingNewsEvents)
    {
        Layout->AddNewsCardToFeed(Event);
    }

    PendingNewsEvents.Empty();
}

void UUIManagerSubsystem::RegisterMusicPlayerComponent(UMusicPlayerComponent* InComponent)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<UUIManagerSubsystem> WeakThis(this);
        TWeakObjectPtr<UMusicPlayerComponent> WeakComp(InComponent);

        AsyncTask(ENamedThreads::GameThread, [WeakThis, WeakComp]()
        {
            if (UUIManagerSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->RegisterMusicPlayerComponent(WeakComp.Get());
            }
        });
        return;
    }

    if (!IsValid(InComponent))
    {
        UE_LOG(LogTemp, Warning, TEXT("RegisterMusicPlayerComponent: Provided component is invalid."));
        return;
    }

    MusicPlayerComponent = InComponent;
    UE_LOG(LogTemp, Log, TEXT("MusicPlayerComponent registered in UIManagerSubsystem."));
}

void UUIManagerSubsystem::RegisterStatusWidget(UStatusWidget* StatusWidget)
{
    const TWeakObjectPtr<UStatusWidget> WeakStatus(StatusWidget);
    ExecuteOnGameThread([this, WeakStatus]()
    {
        ActiveStatusWidget = WeakStatus;
    });
}

void UUIManagerSubsystem::UnregisterStatusWidget(UStatusWidget* StatusWidget)
{
    const TWeakObjectPtr<UStatusWidget> WeakStatus(StatusWidget);
    ExecuteOnGameThread([this, WeakStatus]()
    {
        if (ActiveStatusWidget == WeakStatus)
        {
            ActiveStatusWidget.Reset();
        }
    });
}

FString UUIManagerSubsystem::GetCurrentLabelId() const
{
    UE_LOG(LogUIManagerSubsystem, Verbose, TEXT("GetCurrentLabelId requested: %s"), *CurrentLabelId);
    return CurrentLabelId;
}

void UUIManagerSubsystem::SetCurrentLabelId(const FString& NewLabelId)
{
    ExecuteOnGameThread([this, NewLabelId]()
    {
        const bool bChanged = !CurrentLabelId.Equals(NewLabelId, ESearchCase::CaseSensitive);
        CurrentLabelId = NewLabelId;

        UE_LOG(LogUIManagerSubsystem, Log, TEXT("SetCurrentLabelId called: %s (changed: %s)"), *CurrentLabelId, bChanged ? TEXT("true") : TEXT("false"));

        if (bChanged)
        {
            OnCurrentLabelChanged.Broadcast(CurrentLabelId);
        }
    });
}

void UUIManagerSubsystem::StopAuditionMusic()
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<UUIManagerSubsystem> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (UUIManagerSubsystem* Strong = WeakThis.Get())
            {
                Strong->StopAuditionMusic();
            }
        });
        return;
    }

    if (MusicPlayerComponent)
    {
        MusicPlayerComponent->Stop();
        UE_LOG(LogTemp, Display, TEXT("UIManagerSubsystem: Stopped audition music."));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UIManagerSubsystem: No MusicPlayerComponent registered to stop."));
    }
}

void UUIManagerSubsystem::UnregisterLayout(ULayout* Layout)
{
    if (ActiveLayout.Get() == Layout)
    {
        ActiveLayout.Reset();
    }
}

void UUIManagerSubsystem::ShowAudition(const FAuditionEvent& EventData)
{
    if (ULayout* Layout = ActiveLayout.Get())
    {
        Layout->ShowAuditionWidgetWithData(EventData);
    }
}

void UUIManagerSubsystem::ShowMarketView()
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<UUIManagerSubsystem> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (UUIManagerSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->ShowMarketView();
            }
        });
        return;
    }

    if (ULayout* Layout = ActiveLayout.Get())
    {
        Layout->ShowRegionMap();
    }
}

void UUIManagerSubsystem::ShowRegionMap()
{
    const TWeakObjectPtr<UUIManagerSubsystem> WeakThis(this);

    if (!IsInGameThread())
    {
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (UUIManagerSubsystem* Strong = WeakThis.Get())
            {
                Strong->ShowRegionMap();
            }
        });
        return;
    }

    if (ULayout* Layout = ActiveLayout.Get())
    {
        Layout->ShowRegionMap();
    }
}

void UUIManagerSubsystem::HandleAuditionResult(bool bPassed)
{
    UGameInstance* GameInstance = GetGameInstance();
    if (!IsValid(GameInstance))
    {
        return;
    }

    UArtistManagerSubsystem* ArtistSub = GameInstance->GetSubsystem<UArtistManagerSubsystem>();
    if (!ArtistSub)
    {
        return;
    }

    if (!bPassed)
    {
        ArtistSub->RotateUnsignedArtist();
    }
}

void UUIManagerSubsystem::RefreshSignedArtistPanel()
{
    UGameInstance* GameInstance = GetGameInstance();
    if (!IsValid(GameInstance))
    {
        return;
    }

    if (bIsSimulationBatchUpdateActive)
    {
        bDeferredSignedArtistRefresh = true;
        ++SuppressedUIRefreshCount;
        return;
    }

    UArtistManagerSubsystem* ArtistManager = GameInstance->GetSubsystem<UArtistManagerSubsystem>();
    if (!IsValid(ArtistManager))
    {
        return;
    }

    TArray<FArtistData> ArtistData;
    ArtistManager->GetSignedArtistData(ArtistData);

    const TWeakObjectPtr<ULayout> LayoutWeak = ActiveLayout;
    AsyncTask(ENamedThreads::GameThread, [LayoutWeak, ArtistData]()
    {
        if (ULayout* Layout = LayoutWeak.Get())
        {
            Layout->RefreshSignedArtists(ArtistData);
        }
    });
}

void UUIManagerSubsystem::ShowContractForArtist(const FString& ArtistName)
{
    UGameInstance* GameInstance = GetGameInstance();
    if (!IsValid(GameInstance))
    {
        return;
    }

    UArtistManagerSubsystem* ArtistManager = GameInstance->GetSubsystem<UArtistManagerSubsystem>();
    if (!IsValid(ArtistManager))
    {
        return;
    }

    const FArtistContract* Found = ArtistManager->FindContractByArtistName(ArtistName);
    if (!Found)
    {
        return;
    }

    const TWeakObjectPtr<ULayout> LayoutWeak = ActiveLayout;
    AsyncTask(ENamedThreads::GameThread, [LayoutWeak, FoundContract = *Found]()
    {
        if (ULayout* Layout = LayoutWeak.Get())
        {
            Layout->ShowContract(FoundContract);
        }
    });
}

void UUIManagerSubsystem::HandleNewsCardSelected(const FMusicNewsEvent& EventData)
{
    ExecuteOnGameThread([this, EventData]()
    {
        UE_LOG(LogUIManagerSubsystem, Warning, TEXT("News card selected: Type=%d Headline='%s' Source='%s'."),
            static_cast<int32>(EventData.NewsType),
            *EventData.Headline,
            *EventData.SourceName);

        if (EventData.NewsType == EMusicNewsType::NewUpcomingArtistPerforming)
        {
            UGameInstance* GameInstance = GetGameInstance();
            UArtistManagerSubsystem* ArtistManager = IsValid(GameInstance)
                ? GameInstance->GetSubsystem<UArtistManagerSubsystem>()
                : nullptr;

            if (!IsValid(ArtistManager))
            {
                UE_LOG(LogUIManagerSubsystem, Warning, TEXT("News audition selection ignored: ArtistManagerSubsystem is unavailable."));
            }
            else
            {
                TArray<FArtistData> UnsignedArtists;
                ArtistManager->GetUnsignedArtists(UnsignedArtists);

                const FString* ArtistId = EventData.Metadata.Find(TEXT("ArtistId"));
                const FString ArtistKey = ArtistId && !ArtistId->IsEmpty() ? *ArtistId : EventData.SourceName;

                const FArtistData* MatchingArtist = UnsignedArtists.FindByPredicate([&ArtistKey, &EventData](const FArtistData& Artist)
                {
                    return (!ArtistKey.IsEmpty() && (Artist.ArtistId == ArtistKey || Artist.ArtistName == ArtistKey))
                        || (!EventData.SourceName.IsEmpty() && Artist.ArtistName == EventData.SourceName);
                });

                if (MatchingArtist)
                {
                    if (ULayout* Layout = ActiveLayout.Get())
                    {
                        UE_LOG(LogUIManagerSubsystem, Warning, TEXT("Opening audition panel from news event: ArtistId='%s' Name='%s'."),
                            *MatchingArtist->ArtistId,
                            *MatchingArtist->ArtistName);
                        Layout->ShowAuditionWidgetForArtist(*MatchingArtist);
                    }
                    else
                    {
                        UE_LOG(LogUIManagerSubsystem, Warning, TEXT("News audition selection resolved artist '%s' but no active layout is registered."), *MatchingArtist->ArtistName);
                    }
                }
                else
                {
                    UE_LOG(LogUIManagerSubsystem, Warning, TEXT("News audition selection could not resolve unsigned artist. ArtistKey='%s' Source='%s' UnsignedCount=%d."),
                        *ArtistKey,
                        *EventData.SourceName,
                        UnsignedArtists.Num());
                }
            }
        }

        OnNewsSelected.Broadcast(EventData);
    });
}

void UUIManagerSubsystem::HandleArtistSigned(const FArtistContract& Contract)
{
    const TWeakObjectPtr<UUIManagerSubsystem> WeakThis(this);

    if (!IsInGameThread())
    {
        AsyncTask(ENamedThreads::GameThread, [WeakThis, Contract]()
        {
            if (UUIManagerSubsystem* Strong = WeakThis.Get())
            {
                Strong->HandleArtistSigned(Contract);
            }
        });
        return;
    }

    if (bIsSimulationBatchUpdateActive)
    {
        bDeferredSignedArtistRefresh = true;
        ++SuppressedUIRefreshCount;
        return;
    }

    TArray<FArtistData> ArtistDataList;

    if (UArtistManagerSubsystem* ArtistSub = GetGameInstance()->GetSubsystem<UArtistManagerSubsystem>())
    {
        ArtistSub->GetSignedArtistData(ArtistDataList);
    }

    const TWeakObjectPtr<ULayout> RegisteredLayoutWeakPtr = ActiveLayout;
    AsyncTask(ENamedThreads::GameThread, [RegisteredLayoutWeakPtr, ArtistDataList]()
    {
            UE_LOG(LogTemp, Display, TEXT("Refresh panel started"));
        if (ULayout* Layout = RegisteredLayoutWeakPtr.Get())
        {
         
            Layout->RefreshSignedArtists(ArtistDataList);
            UE_LOG(LogTemp, Display, TEXT("Refresh panel Done"));
        }
    });
}

void UUIManagerSubsystem::HandleArtistListChanged()
{
    RefreshSignedArtistPanel();
}

void UUIManagerSubsystem::HandleNewsEvent(const FMusicNewsEvent& EventData)
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<UUIManagerSubsystem> WeakThis(this);

        AsyncTask(ENamedThreads::GameThread, [WeakThis, EventData]()
        {
            if (UUIManagerSubsystem* Self = WeakThis.Get())
            {
                Self->HandleNewsEvent(EventData);
            }
        });
        return;
    }

    if (bIsSimulationBatchUpdateActive)
    {
        DeferredBatchNewsEvents.Add(EventData);
        ++SuppressedUIRefreshCount;
        UE_LOG(LogUIManagerSubsystem, Warning, TEXT("Deferred news event during UI batch: Headline='%s' DeferredCount=%d."),
            *EventData.Headline,
            DeferredBatchNewsEvents.Num());
        return;
    }

    if (ULayout* Layout = ActiveLayout.Get())
    {
        UE_LOG(LogUIManagerSubsystem, Warning, TEXT("Routing news event to active layout %s: Headline='%s'."),
            *GetNameSafe(Layout),
            *EventData.Headline);
        Layout->AddNewsCardToFeed(EventData);
    }
    else
    {
        PendingNewsEvents.Add(EventData);
        UE_LOG(LogUIManagerSubsystem, Warning, TEXT("No active layout for news event. Queued Headline='%s' PendingNewsEvents=%d."),
            *EventData.Headline,
            PendingNewsEvents.Num());
    }
}

void UUIManagerSubsystem::HandleNewsEventGenerated(const FMusicNewsEvent& EventData)
{
    UE_LOG(LogUIManagerSubsystem, Warning, TEXT("Received generated news event: Headline='%s' Type=%d."),
        *EventData.Headline,
        static_cast<int32>(EventData.NewsType));
    HandleNewsEvent(EventData);
}

void UUIManagerSubsystem::HandleCommandExecuted(const FMusicCommandResult& Result)
{
    ExecuteOnGameThread([this, Result]()
    {
        FCommandNotification Notification;
        Notification.NotificationId = FGuid::NewGuid();
        Notification.Result = Result;
        Notification.Timestamp = Result.ResultDate.GetTicks() > 0 ? Result.ResultDate : FDateTime::UtcNow();

        if (Result.bSuccess)
        {
            Notification.Severity = ECommandNotificationSeverity::Success;
        }
        else if (Result.ErrorCode == EMusicCommandErrorCode::ValidationFailed || Result.ErrorCode == EMusicCommandErrorCode::InvalidState)
        {
            Notification.Severity = ECommandNotificationSeverity::Warning;
        }
        else
        {
            Notification.Severity = ECommandNotificationSeverity::Error;
        }

        PendingCommandNotifications.Add(Notification);
        constexpr int32 MaxPendingNotifications = 20;
        if (PendingCommandNotifications.Num() > MaxPendingNotifications)
        {
            PendingCommandNotifications.RemoveAt(0, PendingCommandNotifications.Num() - MaxPendingNotifications);
        }

        OnCommandNotification.Broadcast(Notification);
    });
}

void UUIManagerSubsystem::GetPendingCommandNotifications(TArray<FCommandNotification>& OutNotifications) const
{
    OutNotifications = PendingCommandNotifications;
}

void UUIManagerSubsystem::ClearPendingCommandNotifications()
{
    PendingCommandNotifications.Reset();
}

void UUIManagerSubsystem::HandleCommandAction(const FString& CommandName)
{
    UE_LOG(LogTemp, Display, TEXT("Handle Command"));
    TWeakObjectPtr<UUIManagerSubsystem> WeakThis(this);

    AsyncTask(ENamedThreads::GameThread, [WeakThis, CommandName]()
    {
        UUIManagerSubsystem* Self = WeakThis.Get();
        if (!IsValid(Self))
        {
            UE_LOG(LogTemp, Display, TEXT("Self not valid"));
            return;
        }

        if (CommandName == TEXT("Audition"))
        {
            ULayout* Layout = Self->ActiveLayout.Get();
            if (!IsValid(Layout))
            {
                UE_LOG(LogUIManagerSubsystem, Warning, TEXT("Audition command: No active layout available."));
                return;
            }

            UE_LOG(LogUIManagerSubsystem, Warning, TEXT("Showing Audition GUI"));
            Layout->ShowAuditionWidget();
        }
        else if (CommandName == TEXT("Market"))
        {
            ULayout* Layout = Self->ActiveLayout.Get();
            if (!IsValid(Layout))
            {
                UE_LOG(LogUIManagerSubsystem, Warning, TEXT("Market command: No active layout available."));
                return;
            }

            UE_LOG(LogUIManagerSubsystem, Warning, TEXT("Showing Market GUI"));
            Layout->ShowRegionMap();
        }
        else if (CommandName == TEXT("Contracts"))
        {
            ULayout* Layout = Self->ActiveLayout.Get();
            if (!IsValid(Layout))
            {
                UE_LOG(LogUIManagerSubsystem, Warning, TEXT("Contracts command: No active layout available."));
                return;
            }

            UE_LOG(LogUIManagerSubsystem, Warning, TEXT("Showing Active Contracts GUI"));
            Layout->ShowActiveContractsWidget();
        }
        else if (CommandName == TEXT("Studio"))
        {
            ULayout* Layout = Self->ActiveLayout.Get();
            if (!IsValid(Layout))
            {
                UE_LOG(LogUIManagerSubsystem, Warning, TEXT("Studio command: No active layout available."));
                return;
            }
            UE_LOG(LogUIManagerSubsystem, Warning, TEXT("Showing Recording GUI"));
            // Show the embedded RecordWidget
            Layout->ShowRecordWidget();
        }
        else if (CommandName == TEXT("Charts"))
        {
            UE_LOG(LogUIManagerSubsystem, Warning, TEXT("Charts command selected, but no production charts UMG screen is wired into Layout yet."));
        }
    });
}

void UUIManagerSubsystem::SetSelectedEntity(UObject* Entity)
{
    const TWeakObjectPtr<UObject> WeakEntity(Entity);
    ExecuteOnGameThread([this, WeakEntity]()
    {
        SelectedEntity = WeakEntity;

        if (ULayout* Layout = ActiveLayout.Get())
        {
            if (UInspectorPanelWidget* InspectorPanel = Layout->GetInspectorPanel())
            {
                InspectorPanel->SetSelectedEntity(WeakEntity.Get());
            }
        }
    });
}

void UUIManagerSubsystem::ShowLayer3Screen(TSubclassOf<UUserWidget> ScreenClass)
{
    ExecuteOnGameThread([this, ScreenClass]()
    {
        if (!ScreenClass)
        {
            return;
        }

        UGameInstance* GameInstance = GetGameInstance();
        if (!IsValid(GameInstance))
        {
            return;
        }

        ULayout* Layout = ActiveLayout.Get();
        if (!IsValid(Layout))
        {
            return;
        }

        UMainCanvasHost* CanvasHost = Layout->GetMainCanvasHost();
        if (!IsValid(CanvasHost))
        {
            return;
        }

        UUserWidget* Screen = CreateWidget<UUserWidget>(GameInstance, ScreenClass);
        if (!IsValid(Screen))
        {
            return;
        }

        // Layer-3 screens are only mounted by the UI manager to enforce strict layer boundaries.
        CanvasHost->SetLayer3Widget(Screen);
        Layout->SetLayer2Enabled(false);
    });
}

void UUIManagerSubsystem::CloseLayer3Screen()
{
    ExecuteOnGameThread([this]()
    {
        if (ULayout* Layout = ActiveLayout.Get())
        {
            if (UMainCanvasHost* CanvasHost = Layout->GetMainCanvasHost())
            {
                CanvasHost->ClearLayer3Widget();
            }

            // Re-enable Layer-2 hover once the decision screen is closed.
            Layout->SetLayer2Enabled(true);
        }
    });
}

void UUIManagerSubsystem::SetCanvasState(ECanvasState NewState)
{
    ExecuteOnGameThread([this, NewState]()
    {
        if (ULayout* Layout = ActiveLayout.Get())
        {
            if (UMainCanvasHost* CanvasHost = Layout->GetMainCanvasHost())
            {
                CanvasHost->SetCanvasState(NewState);
            }
        }
    });
}

void UUIManagerSubsystem::ShowHoverTooltip(const FTooltipData& Data)
{
    ExecuteOnGameThread([this, Data]()
    {
        if (ULayout* Layout = ActiveLayout.Get())
        {
            if (UMainCanvasHost* CanvasHost = Layout->GetMainCanvasHost())
            {
                if (CanvasHost->IsLayer3Active())
                {
                    return;
                }
            }

            Layout->ShowHoverTooltip(Data);
        }
    });
}

void UUIManagerSubsystem::HideHoverTooltip()
{
    ExecuteOnGameThread([this]()
    {
        if (ULayout* Layout = ActiveLayout.Get())
        {
            Layout->HideHoverTooltip();
        }
    });
}

void UUIManagerSubsystem::ShowArtistHover(const FArtistData& ArtistData)
{
    ExecuteOnGameThread([this, ArtistData]()
    {
        if (ULayout* Layout = ActiveLayout.Get())
        {
            if (UMainCanvasHost* CanvasHost = Layout->GetMainCanvasHost())
            {
                if (CanvasHost->IsLayer3Active())
                {
                    return;
                }
            }

            FVector2D MousePosition = UWidgetLayoutLibrary::GetMousePositionOnViewport(this);
            FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
            const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
            if (ViewportScale > 0.f)
            {
                MousePosition /= ViewportScale;
                ViewportSize /= ViewportScale;
            }

            const FVector2D HoverOffset(24.0f, 24.0f);
            const float SafePadding = 24.0f;
            FVector2D TargetPosition = MousePosition + HoverOffset;

            if (ViewportSize.X > 0.0f && ViewportSize.Y > 0.0f)
            {
                const float MaxX = FMath::Max(SafePadding, ViewportSize.X - SafePadding);
                const float MaxY = FMath::Max(SafePadding, ViewportSize.Y - SafePadding);
                TargetPosition.X = FMath::Clamp(TargetPosition.X, SafePadding, MaxX);
                TargetPosition.Y = FMath::Clamp(TargetPosition.Y, SafePadding, MaxY);
            }

            Layout->ShowArtistHoverDetail(ArtistData, TargetPosition);
        }
    });
}

void UUIManagerSubsystem::HideArtistHover()
{
    ExecuteOnGameThread([this]()
    {
        if (ULayout* Layout = ActiveLayout.Get())
        {
            Layout->HideArtistHoverDetail();
        }
    });
}

void UUIManagerSubsystem::RebuildUI()
{
    ExecuteOnGameThread([this]()
    {
        UGameInstance* GameInstance = GetGameInstance();
        if (!GameInstance)
        {
            UE_LOG(LogUIManagerSubsystem, Warning, TEXT("RebuildUI aborted: no GameInstance available."));
            return;
        }

        if (ULayout* ExistingLayout = ActiveLayout.Get())
        {
            ExistingLayout->RemoveFromParent();
            ActiveLayout.Reset();
        }

        TSubclassOf<ULayout> ClassToUse = LayoutClass;
        if (!ClassToUse || ClassToUse->HasAnyClassFlags(CLASS_Abstract))
        {
            UClass* DefaultClass = ULayout::StaticClass();
            if (DefaultClass && !DefaultClass->HasAnyClassFlags(CLASS_Abstract))
            {
                ClassToUse = DefaultClass;
            }
        }

        if (!ClassToUse || ClassToUse->HasAnyClassFlags(CLASS_Abstract))
        {
            UE_LOG(LogUIManagerSubsystem, Warning, TEXT("RebuildUI failed: No valid layout class available."));
            return;
        }

        ULayout* NewLayout = CreateWidget<ULayout>(GameInstance, ClassToUse);
        if (!NewLayout)
        {
            UE_LOG(LogUIManagerSubsystem, Warning, TEXT("RebuildUI failed: Could not create layout instance."));
            return;
        }

        ActiveLayout = NewLayout;
        LayoutClass = ClassToUse;
        NewLayout->AddToViewport();
    });
}

void UUIManagerSubsystem::BeginSimulationBatchUpdate(int32 RequestedWeeks, const FDateTime& StartDate)
{
    ExecuteOnGameThread([this, RequestedWeeks, StartDate]()
    {
        if (bIsSimulationBatchUpdateActive)
        {
            UE_LOG(LogUIManagerSubsystem, Warning, TEXT("BeginSimulationBatchUpdate called while another batch is active."));
        }

        bIsSimulationBatchUpdateActive = true;
        bDeferredSignedArtistRefresh = false;
        SuppressedUIRefreshCount = 0;
        DeferredBatchNewsEvents.Reset();

        UE_LOG(LogUIManagerSubsystem, Log, TEXT("UI batch update started: RequestedWeeks=%d StartDate=%s"),
            RequestedWeeks,
            *StartDate.ToString());
    });
}

void UUIManagerSubsystem::EndSimulationBatchUpdate(int32 WeeksProcessed, const FDateTime& EndDate)
{
    ExecuteOnGameThread([this, WeeksProcessed, EndDate]()
    {
        if (!bIsSimulationBatchUpdateActive)
        {
            return;
        }

        const bool bShouldRefreshSignedArtists = bDeferredSignedArtistRefresh;
        const int32 DeferredNewsCount = DeferredBatchNewsEvents.Num();
        const int32 DeferredRefreshCount = SuppressedUIRefreshCount;

        TArray<FMusicNewsEvent> NewsToFlush;
        NewsToFlush = MoveTemp(DeferredBatchNewsEvents);

        bIsSimulationBatchUpdateActive = false;
        bDeferredSignedArtistRefresh = false;
        SuppressedUIRefreshCount = 0;

        for (const FMusicNewsEvent& Event : NewsToFlush)
        {
            if (ULayout* Layout = ActiveLayout.Get())
            {
                UE_LOG(LogUIManagerSubsystem, Warning, TEXT("Flushing deferred batch news to layout %s: Headline='%s'."),
                    *GetNameSafe(Layout),
                    *Event.Headline);
                Layout->AddNewsCardToFeed(Event);
            }
            else
            {
                PendingNewsEvents.Add(Event);
                UE_LOG(LogUIManagerSubsystem, Warning, TEXT("No active layout while flushing batch news. Queued Headline='%s' PendingNewsEvents=%d."),
                    *Event.Headline,
                    PendingNewsEvents.Num());
            }
        }

        if (bShouldRefreshSignedArtists)
        {
            RefreshSignedArtistPanel();
        }

        UE_LOG(LogUIManagerSubsystem, Log, TEXT("UI batch update finished: WeeksProcessed=%d EndDate=%s DeferredNews=%d SuppressedRefreshes=%d RefreshedSignedArtists=%s"),
            WeeksProcessed,
            *EndDate.ToString(),
            DeferredNewsCount,
            DeferredRefreshCount,
            bShouldRefreshSignedArtists ? TEXT("true") : TEXT("false"));
    });
}
