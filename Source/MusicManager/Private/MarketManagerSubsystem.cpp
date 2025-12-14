#include "MarketManagerSubsystem.h"

void UMarketManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadRegions();
}

void UMarketManagerSubsystem::LoadRegions()
{
    LoadedRegions.Empty();

    if (!RegionDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("MarketManagerSubsystem: No RegionDataTable assigned."));
        return;
    }

    static const FString Context = TEXT("RegionDataTable Load");

    TArray<FMarketRegion*> Rows;
    RegionDataTable->GetAllRows(Context, Rows);

    for (FMarketRegion* Row : Rows)
    {
        if (!Row) continue;
        LoadedRegions.Add(Row->RegionId, *Row);
    }

    UE_LOG(LogTemp, Log, TEXT("MarketManagerSubsystem: Loaded %d regions."), LoadedRegions.Num());
}

bool UMarketManagerSubsystem::GetRegion(const FString& RegionId, FMarketRegion& OutRegion) const
{
    if (const FMarketRegion* Found = LoadedRegions.Find(RegionId))
    {
        OutRegion = *Found;
        return true;
    }
    return false;
}

void UMarketManagerSubsystem::GetAllRegions(TArray<FMarketRegion>& OutRegions) const
{
    LoadedRegions.GenerateValueArray(OutRegions);
}

bool UMarketManagerSubsystem::EvaluateDemandSnapshot(const FString& RegionId, const FString& Genre, const FDateTime& ForDate, FMarketDemandSnapshot& OutSnapshot) const
{
    if (const FMarketRegion* Region = LoadedRegions.Find(RegionId))
    {
        OutSnapshot = FMarketDemandSnapshot();

        // Normalize population against a soft baseline of one million to keep multipliers tame.
        OutSnapshot.PopulationReach = FMath::Clamp(static_cast<float>(Region->TotalPopulation) / 1'000'000.0f, 0.1f, 5.0f);

        // Economic health: average income across all segments relative to $35k baseline.
        float IncomeTotal = 0.f;
        float PopulationShareTotal = 0.f;
        float GenreAffinityTotal = 0.f;
        for (const FMarketSegmentProfile& Segment : Region->Segments)
        {
            IncomeTotal += Segment.AvgIncome * Segment.PopulationShare;
            PopulationShareTotal += Segment.PopulationShare;

            if (const float* Affinity = Segment.GenreAffinity.Find(Genre))
            {
                GenreAffinityTotal += *Affinity * Segment.PopulationShare;
            }
        }

        if (PopulationShareTotal > KINDA_SMALL_NUMBER)
        {
            const float WeightedIncome = IncomeTotal / PopulationShareTotal;
            OutSnapshot.EconomicHealth = FMath::Clamp(WeightedIncome / 35000.f, 0.25f, 2.0f);
            OutSnapshot.GenreAppetite = GenreAffinityTotal / PopulationShareTotal;
        }

        // Saturation is derived from recent record exposure. Higher exposure means heavier competition.
        float RegionExposure = 0.f;
        TArray<float> ExposureValues;
        Region->RecentRecordExposure.GenerateValueArray(ExposureValues);
        for (float Value : ExposureValues)
        {
            RegionExposure += Value;
        }

        const float ExposureClamp = FMath::Clamp(RegionExposure / 10.f, 0.f, 1.5f);
        OutSnapshot.Saturation = 1.0f - FMath::Min(0.5f, ExposureClamp * 0.5f);

        OutSnapshot.SeasonalEffect = GetSeasonalDemandMultiplier(ForDate.GetMonth());
        OutSnapshot.Exposure = Region->RadioReach;

        return true;
    }

    return false;
}

float UMarketManagerSubsystem::GetSeasonalDemandMultiplier(int32 MonthIndex) const
{
    // Holiday season boosts physical sales; midsummer dip otherwise.
    switch (MonthIndex)
    {
    case 12:
    case 11:
        return 1.2f;
    case 6:
    case 7:
        return 0.9f;
    default:
        return 1.0f;
    }
}
