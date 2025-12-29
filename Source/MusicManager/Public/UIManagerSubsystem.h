// File: Public/UIManagerSubsystem.h
#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "AuditionTypes.h"
#include "EventSubsystem.h"
#include "FArtistContract.h"
#include "Async/Async.h"
#include "Templates/UnrealTemplate.h"
#include "UI/MainCanvasHost.h"
#include "UI/TooltipData.h"
#include "UIManagerSubsystem.generated.h"

class ULayout;
class UMusicPlayerComponent;
class UStatusWidget;
class UUserWidget;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnNewsSelected, const FMusicNewsEvent&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCurrentLabelChanged, const FString&, LabelId);

/**
 * Game-instance subsystem that orchestrates high-level UI interactions and ensures they run on the game thread.
 */
UCLASS()
class UUIManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Register the currently active layout so UI actions can be routed appropriately. */
    UFUNCTION(BlueprintCallable, Category="UI")
    void RegisterLayout(ULayout* Layout);

    /** Unregister the current layout when it's no longer active. */
    void UnregisterLayout(ULayout* Layout);

    /** Route or buffer news events until the layout is ready. */
    void HandleNewsEvent(const FMusicNewsEvent& EventData);

    bool IsLayoutAvailable() const { return ActiveLayout.IsValid(); }

    /** Displays the audition window for the supplied event data. */
    UFUNCTION(BlueprintCallable, Category="UI")
    void ShowAudition(const FAuditionEvent& EventData);

    UFUNCTION()
    void ShowMarketView();

    UFUNCTION(BlueprintCallable, Category = "UI")
    void ShowRegionMap();

    /** Handle outcomes from audition flow to keep unsigned roster in sync. */
    UFUNCTION()
    void HandleAuditionResult(bool bPassed);

    /** Rebuilds the UI layout after a load to avoid stale references. */
    UFUNCTION(BlueprintCallable, Category="UI")
    void RebuildUI();

    void RefreshSignedArtistPanel();

    void ShowContractForArtist(const FString& ArtistName);

    UFUNCTION()
    void RegisterMusicPlayerComponent(UMusicPlayerComponent* InComponent);

    UFUNCTION(BlueprintCallable)
    UMusicPlayerComponent* GetMusicPlayerComponent() const { return MusicPlayerComponent; }

    /** Register the active status widget so updates can be mediated through the UI manager. */
    void RegisterStatusWidget(UStatusWidget* StatusWidget);

    /** Unregister the status widget when it is removed from the layout. */
    void UnregisterStatusWidget(UStatusWidget* StatusWidget);

    /** Returns the label id to use for UI-bound financial displays. */
    FString GetCurrentLabelId() const;

    /** Sets the label id to use for UI-bound financial displays and notifies listeners. */
    void SetCurrentLabelId(const FString& NewLabelId);

    UFUNCTION()
    void StopAuditionMusic();

    /** Raised when a news card is selected anywhere in the UI. */
    FOnNewsSelected OnNewsSelected;

    /** Raised when the current label id changes. */
    UPROPERTY(BlueprintAssignable)
    FOnCurrentLabelChanged OnCurrentLabelChanged;

    /** Handle selection events coming from news cards. */

    UFUNCTION()
    void HandleNewsCardSelected(const FMusicNewsEvent& EventData);

    UFUNCTION()
    void HandleArtistSigned(const FArtistContract& Contract);

    UFUNCTION()
    void HandleArtistListChanged();

    UFUNCTION()
    void HandleNewsEventGenerated(const FMusicNewsEvent& EventData);

    /**
     * Entry point for handling command actions from the command panel.
     * Ensures all logic executes on the game thread before interacting with UObjects.
     */
    void HandleCommandAction(const FString& CommandName);

    /** Selects an entity and updates the persistent inspector panel. */
    UFUNCTION(BlueprintCallable, Category="UI|Selection")
    void SetSelectedEntity(UObject* Entity);

    /** Display a layer-3 screen (decision UI) via the main canvas host. */
    UFUNCTION(BlueprintCallable, Category="UI|Layer3")
    void ShowLayer3Screen(TSubclassOf<UUserWidget> ScreenClass);

    /** Close the active layer-3 screen. */
    UFUNCTION(BlueprintCallable, Category="UI|Layer3")
    void CloseLayer3Screen();

    /** Update the canvas state (scene + decision context). */
    UFUNCTION(BlueprintCallable, Category="UI|Canvas")
    void SetCanvasState(ECanvasState NewState);

    /** Show or hide hover tooltips on layer-2. */
    UFUNCTION(BlueprintCallable, Category="UI|Layer2")
    void ShowHoverTooltip(const FTooltipData& Data);

    UFUNCTION(BlueprintCallable, Category="UI|Layer2")
    void HideHoverTooltip();

private:
    /** Configurable label id used for finance lookups. */
    UPROPERTY(EditAnywhere, Category="UI")
    FString CurrentLabelId;

    UPROPERTY(EditAnywhere, Category="UI")
    TSubclassOf<ULayout> LayoutClass;

    /** Weak pointer to the active layout to avoid ownership over widgets. */
    UPROPERTY()
    TWeakObjectPtr<ULayout> ActiveLayout;

    /** Weak pointer to the active status widget displayed in the layout. */
    UPROPERTY()
    TWeakObjectPtr<UStatusWidget> ActiveStatusWidget;

    UPROPERTY()
    TWeakObjectPtr<UObject> SelectedEntity;

    UPROPERTY()
    UMusicPlayerComponent* MusicPlayerComponent;

    /** News events received before the layout exists */
    UPROPERTY()
    TArray<FMusicNewsEvent> PendingNewsEvents;

    template<typename Func>
    void ExecuteOnGameThread(Func&& Lambda)
    {
        if (IsInGameThread())
        {
            Lambda();
            return;
        }

        AsyncTask(ENamedThreads::GameThread, [CapturedLambda = Forward<Func>(Lambda)]()
        {
            CapturedLambda();
        });
    }
};
