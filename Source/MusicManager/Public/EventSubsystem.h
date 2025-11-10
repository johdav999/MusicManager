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
 * Game-instance subsystem that periodically notifies a child widget on a registered layout widget.
 */
UCLASS(Config=Game)
class UEventSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="EventSubsystem")
    void RegisterLayout(ULayout* InLayout);

    UFUNCTION(BlueprintCallable, Category="EventSubsystem")
    void UnregisterLayout(ULayout* InLayout);


    void HandlePostWorldInit(UWorld* InWorld, const UWorld::InitializationValues IVS);
    void HandleWorldCleanup(UWorld* InWorld, bool bSessionEnded, bool bCleanupResources);
    void StartTimerForWorld(UWorld* InWorld);
    void StopTimer();
    void OnTimerTick();
    UUserWidget* ResolveChildWidget(ULayout& Layout);
    bool IsSameGameInstanceWorld(const UWorld& World) const;

    TWeakObjectPtr<UWorld> CachedWorld;
    FTimerHandle EventTimerHandle;
    FDelegateHandle WorldInitHandle;
    FDelegateHandle WorldCleanupHandle;

    UPROPERTY(EditAnywhere, Config, meta=(ClampMin="0.1", ClampMax="60.0"))
    float TickIntervalSeconds = 5.0f;

    UPROPERTY(EditAnywhere, Config)
    FName ChildWidgetName = TEXT("EventTicker");

    UPROPERTY(EditAnywhere,BlueprintReadWrite, Config)
    TSubclassOf<UUserWidget> ChildWidgetClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<ULayout> LayoutWeak;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<UUserWidget> ChildWeak;
};
