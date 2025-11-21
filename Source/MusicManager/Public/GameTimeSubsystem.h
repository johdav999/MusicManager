#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TimerManager.h"
#include "GameTimeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMonthAdvanced, const FDateTime&, NewDate);

class UMusicSaveGame;

/**
 * Centralized time simulation subsystem that controls the passage of in-game months.
 */
UCLASS()
class UGameTimeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UGameTimeSubsystem();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * Manually advance the simulation by one month.
     */
    UFUNCTION(BlueprintCallable, Category="Time")
    void AdvanceMonth();

    /**
     * Pause or resume the automatic time advancement timer.
     */
    UFUNCTION(BlueprintCallable, Category="Time")
    void PauseTime(bool bPause);

    /**
     * Returns the current simulated date for UI display or logic.
     */
    UFUNCTION(BlueprintPure, Category="Time")
    FDateTime GetCurrentGameDate() const { return CurrentGameDate; }

    void SaveState(class UMusicSaveGame* SaveObject);
    void LoadState(const class UMusicSaveGame* SaveObject);

    /**
     * Fired each time the subsystem successfully advances one month.
     */
    UPROPERTY(BlueprintAssignable, Category="Time")
    FOnMonthAdvanced OnMonthAdvanced;

protected:
    void StartTimer();
    void StopTimer();

    bool HasSimulationEnded() const;

    /** Current simulated date. Always normalized to the first day of the month. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Time")
    FDateTime CurrentGameDate;

    /** Whether the automatic timer is actively advancing months. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Time")
    bool bIsTimeRunning;

    /** Internal repeating timer that fires once every ten real seconds. */
    FTimerHandle TimeAdvanceHandle;

    /** True after the timeline surpasses the year 2026, preventing further advancement. */
    bool bHasReachedSimulationEnd;
};
