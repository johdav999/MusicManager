// File: Public/UIManagerSubsystem.h
#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "AuditionTypes.h"
#include "EventSubsystem.h"
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

    /** Returns the registered layout if it is still valid. */
    ULayout* GetLayout() const;

    ///**
    // * Helper to ensure UI work is executed on the game thread.
    // * Usage: ExecuteOnGameThread([this](){ /* game thread UI work */ });
    // */
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

    /** Displays the audition window for the supplied event data. */
    UFUNCTION(BlueprintCallable, Category="UI")
    void ShowAudition(const FAuditionEvent& EventData);

    /** Raised when a news card is selected anywhere in the UI. */
    FOnNewsSelected OnNewsSelected;

    /** Handle selection events coming from news cards. */
    void HandleNewsCardSelected(const FMusicNewsEvent& EventData);

private:
    /** Weak pointer to the active layout to avoid ownership over widgets. */
    TWeakObjectPtr<ULayout> RegisteredLayout;
};

