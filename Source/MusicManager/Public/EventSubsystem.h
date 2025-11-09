// File: Public/EventSubsystem.h
#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "TimerManager.h"
#include "EventSubsystem.generated.h"

class ULayout;
class UUserWidget;
class UWorld;

DECLARE_LOG_CATEGORY_EXTERN(LogEventSubsystem, Log, All);

/**
 * Subsystem that periodically notifies a child widget inside a registered layout widget.
 */
UCLASS(Config=Game)
class UEventSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UEventSubsystem();

    //~UGameInstanceSubsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * Minimal usage:
     * After adding your layout widget to the viewport, call
     * GetGameInstance()->GetSubsystem<UEventSubsystem>()->RegisterLayout(LayoutInstance);
     * When removing or destroying the layout, call UnregisterLayout with the same instance.
     * Register the root layout widget that contains the child widget to notify.
     */
    UFUNCTION(BlueprintCallable, Category="EventSubsystem")
    void RegisterLayout(ULayout* InLayout);

    /** Unregister a previously registered layout widget. */
    UFUNCTION(BlueprintCallable, Category="EventSubsystem")
    void UnregisterLayout(ULayout* InLayout);

protected:
    /** Timer callback executed on the game thread. */
    void OnTimerTick();

private:
    void HandleWorldBeginPlay(UWorld* InWorld);
    void HandleWorldBeginTearDown(UWorld* InWorld);

    void StartTimerForWorld(UWorld* InWorld);
    void StopTimer();
    void ResetCachedWidgets();
    void AttemptChildRefresh();
    void InvokeChildTick(UUserWidget& ChildWidget) const;

    bool IsTimerActiveForWorld(const UWorld* InWorld) const;

private:
    /** Preferred child widget name to search for when a layout is registered. */
    UPROPERTY(EditAnywhere, Config, Category="EventSubsystem")
    FName ChildWidgetName = TEXT("EventTicker");

    /** Optional child widget class to search for when the name lookup fails. */
    UPROPERTY(EditAnywhere, Config, Category="EventSubsystem")
    TSubclassOf<UUserWidget> ChildWidgetClass;

    /** Timer interval for notifying the child widget. */
    UPROPERTY(EditAnywhere, Config, Category="EventSubsystem", meta=(ClampMin="0.1", ClampMax="60.0"))
    float TickIntervalSeconds = 5.0f;

    /** Weak reference to the registered layout widget. */
    TWeakObjectPtr<ULayout> LayoutWeak;

    /** Weak reference to the child widget located within the layout. */
    TWeakObjectPtr<UUserWidget> ChildWeak;

    /** Weak reference to the active game world. */
    TWeakObjectPtr<UWorld> WorldWeak;

    /** Timer handle used to schedule notifications. */
    FTimerHandle EventTimerHandle;

    /** Delegate handles for world lifecycle events. */
    FDelegateHandle WorldBeginPlayHandle;
    FDelegateHandle WorldBeginTearDownHandle;
};
