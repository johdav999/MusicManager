#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "MarketRegion.h"
#include "MarketManagerSubsystem.generated.h"

/** Snapshot of all genre demand within a specific market at a single point in time. */
USTRUCT(BlueprintType)
struct FMarketDemandSnapshot
{
    GENERATED_BODY();

    /** Owning market identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Market")
    FString MarketId;

    /** Effective reachable population (scaled down to manageable magnitudes). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Market")
    float TotalReach = 0.f;

    /** Normalized demand per genre (0-1). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Market")
    TMap<FString, float> GenreDemand;

    /** Per-artist radio lift derived from recent exposure. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Market")
    TMap<FString, float> ArtistRadioBoost;

    /** Optional per-record radio lift hook. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Market")
    TMap<FString, float> RecordRadioBoost;
};

UCLASS()
class MUSICMANAGER_API UMarketManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /** Assign in BP or defaults: DataTable with FMarketRegion rows */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Market")
    UDataTable* RegionDataTable;

    /** Assign in BP or defaults: DataTable with FMarketSegmentProfile rows */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Market")
    UDataTable* SegmentProfileDataTable;

    /** Loaded regions keyed by RegionId */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Market")
    TMap<FString, FMarketRegion> LoadedRegions;

    /** Loaded market segments keyed by SegmentId */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Market")
    TMap<FString, FMarketSegmentProfile> LoadedSegmentProfiles;

    /** Runtime resolved market segments per region */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Market")
    TMap<FString, TArray<FMarketSegmentProfile>> RegionSegments;

    /** Load all rows from the DataTable */
    UFUNCTION(BlueprintCallable)
    void LoadRegions();

    /** Load all market segment profiles from the DataTable */
    UFUNCTION(BlueprintCallable)
    void LoadMarketSegmentProfiles();

    /** Lookup region by RegionId */
    UFUNCTION(BlueprintCallable)
    bool GetRegion(const FString& RegionId, FMarketRegion& OutRegion) const;

    UFUNCTION(BlueprintCallable)
    void AssignSegmentsToRegions();

    /** Get all loaded regions. */
    UFUNCTION(BlueprintCallable)
    void GetAllRegions(TArray<FMarketRegion>& OutRegions) const;

    /** Aggregate segment-level demand into a market snapshot. */
    void BuildMarketDemandSnapshot(const FMarketRegion& Region, FMarketDemandSnapshot& OutSnapshot) const;

    /** Convenience helper for monthly orchestration to refresh time-sensitive state. */
    void HandleMonthAdvanced(const FDateTime& NewDate);

    /** Utility: compute all market snapshots for the current month. */
    void GetAllDemandSnapshots(TArray<FMarketDemandSnapshot>& OutSnapshots) const;

private:


    /** TEMPORARY RADIO SIMULATION: REPLACE WITH REAL RADIO SYSTEM */
    void SimulateMonthlyRadioPlay(const FDateTime& CurrentDate);
};
