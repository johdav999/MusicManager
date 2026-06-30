#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameTimeSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMusicSimTime, Log, All);

UENUM(BlueprintType)
enum class EMusicWeeklySimulationPhase : uint8
{
    TrendDrift UMETA(DisplayName = "Trend Drift"),
    ArtistStateUpdate UMETA(DisplayName = "Artist State Update"),
    ProductionProgress UMETA(DisplayName = "Production Progress"),
    ReleaseLaunchProcessing UMETA(DisplayName = "Release Launch Processing"),
    MarketExposureUpdate UMETA(DisplayName = "Market Exposure Update"),
    ChartCalculation UMETA(DisplayName = "Chart Calculation"),
    TourResolution UMETA(DisplayName = "Tour Resolution"),
    FinanceSettlement UMETA(DisplayName = "Finance Settlement"),
    CriticNewsGeneration UMETA(DisplayName = "Critic News Generation"),
    Notifications UMETA(DisplayName = "Notifications")
};

USTRUCT(BlueprintType)
struct FMonthlyCloseSummary
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ClosedYear = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ClosedMonth = 0;

    /** Inclusive first day of the closed month. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime PeriodStart;

    /** Exclusive first day of the next month. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime PeriodEnd;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime PreviousDate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime NewDate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString MonthKey;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeekAdvanced, const FDateTime&, PreviousDate, const FDateTime&, NewDate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnMonthClosed, int32, ClosedYear, int32, ClosedMonth, const FDateTime&, PreviousDate, const FDateTime&, NewDate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMonthlySummaryClosed, const FMonthlyCloseSummary&, Summary);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnYearAdvanced, int32, NewYear, const FDateTime&, PreviousDate, const FDateTime&, NewDate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMonthAdvanced, const FDateTime&, NewDate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimeBatchAdvanced, int32, WeeksAdvanced, const FDateTime&, NewDate);

class UMusicSaveGame;
struct FMusicSaveValidationResult;
struct FTimeSnapshot;

/**
 * Centralized time simulation subsystem that directs deterministic weekly advancement.
 * Month-end delegates remain as compatibility hooks for UI/listeners while monthly systems migrate.
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
     * Advance the simulation by one deterministic weekly step.
     */
    UFUNCTION(BlueprintCallable, Category="Time")
    void AdvanceOneWeek();

    /**
     * Advance multiple deterministic weekly steps in fixed order.
     */
    UFUNCTION(BlueprintCallable, Category="Time")
    int32 AdvanceWeeks(int32 NumWeeks);

    /**
     * Compatibility entry point for existing monthly callers. Advances weekly until the next month boundary is crossed.
     */
    UFUNCTION(BlueprintCallable, Category="Time")
    void AdvanceMonth();

    /**
     * Pauses or resumes the automatic month timer.
     */
    UFUNCTION(BlueprintCallable, Category="Time")
    void PauseTime(bool bPause);

    /**
     * Starts automatic month advancement at the configured interval.
     */
    UFUNCTION(BlueprintCallable, Category="Time")
    void StartAutoAdvance();

    /**
     * Stops automatic month advancement.
     */
    UFUNCTION(BlueprintCallable, Category="Time")
    void StopAutoAdvance();

    /**
     * Returns the current simulated date for UI display or logic.
     */
    UFUNCTION(BlueprintPure, Category="Time")
    FDateTime GetCurrentGameDate() const { return CurrentGameDate; }

    /**
     * True while AdvanceWeeks is processing more than one weekly step in a single deterministic batch.
     */
    UFUNCTION(BlueprintPure, Category="Time")
    bool IsBatchAdvancing() const { return bIsBatchAdvancing; }

    UFUNCTION(BlueprintPure, Category="Time")
    bool IsTimeRunning() const { return bIsTimeRunning; }

    UFUNCTION(BlueprintPure, Category="Time")
    float GetAutoAdvanceMonthIntervalSeconds() const { return AutoAdvanceMonthIntervalSeconds; }

    const TArray<EMusicWeeklySimulationPhase>& GetWeeklyPhaseOrder() const { return WeeklyPhaseOrder; }
    const TArray<EMusicWeeklySimulationPhase>& GetLastExecutedWeeklyPhases() const { return LastExecutedWeeklyPhases; }

    void SaveState(class UMusicSaveGame* SaveObject);
    void LoadState(const class UMusicSaveGame* SaveObject);
    void BuildSaveSnapshot(FTimeSnapshot& OutSnapshot) const;
    void ValidateSaveSnapshot(const FTimeSnapshot& Snapshot, FMusicSaveValidationResult& Result) const;
    void ApplySaveSnapshot(const FTimeSnapshot& Snapshot);

    /**
     * Fired each time the subsystem successfully advances one week.
     */
    UPROPERTY(BlueprintAssignable, Category="Time")
    FOnWeekAdvanced OnWeekAdvanced;

    /**
     * Fired when a weekly step crosses into a new month. ClosedYear and ClosedMonth identify the month being closed.
     */
    UPROPERTY(BlueprintAssignable, Category="Time")
    FOnMonthClosed OnMonthClosed;

    /**
     * Fired once per closed month with a stable month key and date range for summary/report consumers.
     */
    UPROPERTY(BlueprintAssignable, Category="Time")
    FOnMonthlySummaryClosed OnMonthlySummaryClosed;

    /**
     * Compatibility delegate for existing month-end listeners. Mirrors OnMonthClosed with the new date only.
     */
    UPROPERTY(BlueprintAssignable, Category="Time")
    FOnMonthAdvanced OnMonthAdvanced;

    /**
     * Fired when a weekly step crosses into a new year.
     */
    UPROPERTY(BlueprintAssignable, Category="Time")
    FOnYearAdvanced OnYearAdvanced;

    /**
     * Fired after a multi-week batch completes so UI can refresh once after deferred monthly summaries.
     */
    UPROPERTY(BlueprintAssignable, Category="Time")
    FOnTimeBatchAdvanced OnTimeBatchAdvanced;

#if WITH_AUTOMATION_TESTS
    /** Test-only date hook for boundary-focused cadence tests. */
    void AdvanceToDateForTesting(const FDateTime& NewDate);
#endif

protected:
    bool HasSimulationEnded() const;
    bool WouldEndSimulation(const FDateTime& CandidateDate) const;
    bool DidCrossMonthBoundary(const FDateTime& PreviousDate, const FDateTime& NewDate) const;
    void ProcessClosedMonths(const FDateTime& PreviousDate, const FDateTime& NewDate);
    void CloseMonth(const FMonthlyCloseSummary& Summary);
    FMonthlyCloseSummary BuildMonthlyCloseSummary(int32 ClosedYear, int32 ClosedMonth, const FDateTime& PreviousDate, const FDateTime& NewDate) const;
    bool DidCrossYearBoundary(const FDateTime& PreviousDate, const FDateTime& NewDate) const;
    void RunWeeklySimulation(const FDateTime& PreviousDate, const FDateTime& NewDate, bool bClosedMonth);
    void RunWeeklySimulationPhase(EMusicWeeklySimulationPhase Phase, const FDateTime& PreviousDate, const FDateTime& NewDate, bool bClosedMonth);
    void RunMonthlyCompatibilityPass(const FMonthlyCloseSummary& Summary);
    void HandleWorldInitialized(UWorld* World, const UWorld::InitializationValues IVS);
    void HandleAutoAdvanceTimerElapsed();
    void ClearAutoAdvanceTimer();

    /** Current simulated date. Weekly cadence means this may fall inside a month. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Time")
    FDateTime CurrentGameDate;

    /** Legacy state retained for Blueprint compatibility. Explicit advancement keeps this false. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Time")
    bool bIsTimeRunning;

    /** Real-time interval for automatic month advancement. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Time", meta=(ClampMin="0.1"))
    float AutoAdvanceMonthIntervalSeconds = 5.f;

    /** Timer that drives automatic month advancement. */
    FTimerHandle AutoAdvanceMonthTimerHandle;

    /** Suppresses repeated UI refreshes while deterministic fast-forward batches are still running. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Time")
    bool bIsBatchAdvancing;

    /** Fixed weekly phase order for save/load replay determinism. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Time")
    TArray<EMusicWeeklySimulationPhase> WeeklyPhaseOrder;

    /** Last weekly phase sequence executed; exposed for lightweight cadence validation. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Time")
    TArray<EMusicWeeklySimulationPhase> LastExecutedWeeklyPhases;

    /** True after the timeline surpasses the year 2026, preventing further advancement. */
    bool bHasReachedSimulationEnd;
};
