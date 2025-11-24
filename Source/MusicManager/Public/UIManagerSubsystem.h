// File: Public/UIManagerSubsystem.h
#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "AuditionTypes.h"
#include "EventSubsystem.h"
#include "FArtistContract.h"
#include "Async/Async.h"
#include "Templates/UnrealTemplate.h"
#include "UIManagerSubsystem.generated.h"

class ULayout;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnNewsSelected, const FMusicNewsEvent&);

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

    /** Handle outcomes from audition flow to keep unsigned roster in sync. */
    UFUNCTION()
    void HandleAuditionResult(bool bPassed);

    /** Rebuilds the UI layout after a load to avoid stale references. */
    UFUNCTION(BlueprintCallable, Category="UI")
    void RebuildUI();

    void RefreshSignedArtistPanel();

    void ShowContractForArtist(const FString& ArtistName);

    /** Raised when a news card is selected anywhere in the UI. */
    FOnNewsSelected OnNewsSelected;

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

private:
    UPROPERTY(EditAnywhere, Category="UI")
    TSubclassOf<ULayout> LayoutClass;

    /** Weak pointer to the active layout to avoid ownership over widgets. */
    UPROPERTY()
    TWeakObjectPtr<ULayout> ActiveLayout;

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

