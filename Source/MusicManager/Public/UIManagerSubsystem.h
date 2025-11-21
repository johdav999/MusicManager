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
    void RegisterLayout(ULayout* InLayout);

    /** Unregister the current layout when it's no longer active. */
    void UnregisterLayout(ULayout* Layout);

    /** Displays the audition window for the supplied event data. */
    UFUNCTION(BlueprintCallable, Category="UI")
    void ShowAudition(const FAuditionEvent& EventData);

    /** Rebuilds the UI layout after a load to avoid stale references. */
    UFUNCTION(BlueprintCallable, Category="UI")
    void RebuildUI();

    /** Raised when a news card is selected anywhere in the UI. */
    FOnNewsSelected OnNewsSelected;

    /** Handle selection events coming from news cards. */
    void HandleNewsCardSelected(const FMusicNewsEvent& EventData);

    void HandleArtistSigned(const FArtistContract& Contract);

private:
    UPROPERTY(EditAnywhere, Category="UI")
    TSubclassOf<ULayout> LayoutClass;

    /** Weak pointer to the active layout to avoid ownership over widgets. */
    TWeakObjectPtr<ULayout> ActiveLayout;

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

