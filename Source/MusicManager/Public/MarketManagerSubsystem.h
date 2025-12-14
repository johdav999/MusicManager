#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "MarketRegion.h"
#include "MarketManagerSubsystem.generated.h"

/** Lightweight snapshot of demand multipliers for a single market at a point in time. */
USTRUCT(BlueprintType)
struct FMarketDemandSnapshot
{
    GENERATED_BODY();

    /** Multiplier representing reachable audience size (scaled around 1.0). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Market")
    float PopulationReach = 1.0f;

    /** Local economic confidence / discretionary spending strength. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Market")
    float EconomicHealth = 1.0f;

    /** How much the region currently wants the supplied genre. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Market")
    float GenreAppetite = 1.0f;

    /** Competition pressure and saturation. < 1.0 means suppressed demand. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Market")
    float Saturation = 1.0f;

    /** Seasonal bump or slump. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Market")
    float SeasonalEffect = 1.0f;

    /** Marketing reach proxy (radio/press). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Market")
    float Exposure = 1.0f;

    /** Convenience helper to combine all multipliers without computing sales directly. */
    float GetCompositeDemand() const
    {
        return PopulationReach * EconomicHealth * GenreAppetite * Saturation * SeasonalEffect * Exposure;
    }
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

    /** Loaded regions keyed by RegionId */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Market")
    TMap<FString, FMarketRegion> LoadedRegions;

    /** Load all rows from the DataTable */
    UFUNCTION(BlueprintCallable)
    void LoadRegions();

    /** Lookup region by RegionId */
    UFUNCTION(BlueprintCallable)
    bool GetRegion(const FString& RegionId, FMarketRegion& OutRegion) const;

    /** Get all loaded regions. */
    UFUNCTION(BlueprintCallable)
    void GetAllRegions(TArray<FMarketRegion>& OutRegions) const;

    /**
     * Build a read-only set of demand multipliers for the supplied market and genre.
     * The result is a purely data-oriented snapshot that other subsystems can use to
     * compute sales without duplicating market logic.
     */
    UFUNCTION(BlueprintCallable, Category="Market|Demand")
    bool EvaluateDemandSnapshot(const FString& RegionId, const FString& Genre, const FDateTime& ForDate, FMarketDemandSnapshot& OutSnapshot) const;

    /**
     * Utility: compute simple seasonal multiplier using month index (Decembers boost gifting, summers slow).
     */
    UFUNCTION(BlueprintCallable, Category="Market|Demand")
    float GetSeasonalDemandMultiplier(int32 MonthIndex) const;
};
