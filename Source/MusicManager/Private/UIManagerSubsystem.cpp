// File: Private/UIManagerSubsystem.cpp
#include "UIManagerSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Layout.h"
#include "ArtistManagerSubsystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Logging/LogMacros.h"
#include "MusicPlayerComponent.h"
#include "UI/StatusWidget.h"
#include "UI/InspectorPanelWidget.h"
#include "UI/MainCanvasHost.h"

DEFINE_LOG_CATEGORY_STATIC(LogUIManagerSubsystem, Log, All);

void UUIManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (UEventSubsystem* EventSubsystem = GetGameInstance()->GetSubsystem<UEventSubsystem>())
    {
        EventSubsystem->OnNewsEventGenerated.AddDynamic(this, &UUIManagerSubsystem::HandleNewsEventGenerated);
    }

    if (UArtistManagerSubsystem* Artist = GetGameInstance()->GetSubsystem<UArtistManagerSubsystem>())
    {
        Artist->OnArtistSigned.AddDynamic(this, &UUIManagerSubsystem::HandleArtistSigned);
        Artist->OnArtistListChanged.AddDynamic(this, &UUIManagerSubsystem::HandleArtistListChanged);
    }
}

void UUIManagerSubsystem::Deinitialize()
{
    if (UEventSubsystem* EventSubsystem = GetGameInstance()->GetSubsystem<UEventSubsystem>())
    {
        EventSubsystem->OnNewsEventGenerated.RemoveDynamic(this, &UUIManagerSubsystem::HandleNewsEventGenerated);
    }

    if (UArtistManagerSubsystem* Artist = GetGameInstance()->GetSubsystem<UArtistManagerSubsystem>())
    {
        Artist->OnArtistSigned.RemoveDynamic(this, &UUIManagerSubsystem::HandleArtistSigned);
        Artist->OnArtistListChanged.RemoveDynamic(this, &UUIManagerSubsystem::HandleArtistListChanged);
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

    // Stop audition music when signing a deal
    StopAuditionMusic();

    if (ULayout* Layout = ActiveLayout.Get())
    {
        Layout->CloseAuditionWidget();
        Layout->ShowContract(Contract);
    }
}

void UUIManagerSubsystem::HandleArtistListChanged()
{
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

    if (ULayout* Layout = ActiveLayout.Get())
    {
        Layout->AddNewsCardToFeed(EventData);
    }
    else
    {
        PendingNewsEvents.Add(EventData);
    }
}

void UUIManagerSubsystem::HandleNewsEventGenerated(const FMusicNewsEvent& EventData)
{
    HandleNewsEvent(EventData);
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

        if (CommandName == TEXT("Contracts"))
        {
            UGameInstance* GameInstance = Self->GetGameInstance();
            if (!IsValid(GameInstance))
            {
                UE_LOG(LogUIManagerSubsystem, Warning, TEXT("HandleCommandAction: GameInstance is invalid."));
                return;
            }

            UArtistManagerSubsystem* ArtistSubsystem = GameInstance->GetSubsystem<UArtistManagerSubsystem>();
            if (!IsValid(ArtistSubsystem))
            {
                UE_LOG(LogUIManagerSubsystem, Warning, TEXT("HandleCommandAction: ArtistManagerSubsystem is unavailable."));
                return;
            }

            if (ArtistSubsystem->ActiveContracts.Num() <= 0)
            {
                UE_LOG(LogUIManagerSubsystem, Warning, TEXT("HandleCommandAction: No active contracts to display."));
                return;
            }

            const FArtistContract& Contract = ArtistSubsystem->ActiveContracts[0];

            ULayout* Layout = Self->ActiveLayout.Get();
            if (!IsValid(Layout))
            {
                UE_LOG(LogUIManagerSubsystem, Warning, TEXT("HandleCommandAction: No active layout registered to show contracts."));
                return;
            }
            UE_LOG(LogTemp, Display, TEXT("Show Contract"));
            Layout->ShowContract(Contract);
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
