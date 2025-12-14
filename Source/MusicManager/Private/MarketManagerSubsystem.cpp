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

void UMarketManagerSubsystem::BuildMarketDemandSnapshot(const FMarketRegion& Region, FMarketDemandSnapshot& OutSnapshot) const
{
    OutSnapshot = FMarketDemandSnapshot();
    OutSnapshot.MarketId = Region.RegionId;

    float TotalEffectiveReach = 0.f;
    float MaxGenreDemand = 0.f;

    // Sum exposure once per region so each segment shares the same dampening.
    float ExposureSum = 0.f;
    for (const auto& Pair : Region.RecentArtistExposure)
    {
        ExposureSum += Pair.Value;
    }
    for (const auto& Pair : Region.RecentRecordExposure)
    {
        ExposureSum += Pair.Value;
    }

    const float ExposureDampen = 1.f / FMath::Max(1.f, 1.f + ExposureSum * 0.1f);

    // Aggregate each segment's contribution into per-genre demand buckets.
    for (const FMarketSegmentProfile& Segment : Region.Segments)
    {
        if (Segment.PopulationShare <= KINDA_SMALL_NUMBER)
        {
            continue;
        }

        const float EffectivePopulation = Region.TotalPopulation * Segment.PopulationShare;
        if (EffectivePopulation <= 0.f)
        {
            continue;
        }

        const float Trendiness = FMath::Clamp(Segment.Trendiness, 0.f, 2.f);
        const float Reach = EffectivePopulation * Region.RadioReach * ExposureDampen;
        TotalEffectiveReach += Reach;

        for (const TPair<FString, float>& AffinityPair : Segment.GenreAffinity)
        {
            const float Affinity = FMath::Max(0.f, AffinityPair.Value);
            if (Affinity <= 0.f)
            {
                continue;
            }

            const float DemandContribution = Reach * Affinity * (0.5f + Trendiness * 0.5f);
            float& Existing = OutSnapshot.GenreDemand.FindOrAdd(AffinityPair.Key);
            Existing += DemandContribution;
            MaxGenreDemand = FMath::Max(MaxGenreDemand, Existing);
        }
    }

    // Normalize per-genre demand into 0-1 while retaining relative shape.
    if (MaxGenreDemand > KINDA_SMALL_NUMBER)
    {
        for (auto& Pair : OutSnapshot.GenreDemand)
        {
            Pair.Value = FMath::Clamp(Pair.Value / MaxGenreDemand, 0.f, 1.f);
        }
    }

    // Scale total reach to a manageable number so downstream systems can convert to units without overflow.
    OutSnapshot.TotalReach = TotalEffectiveReach / 100000.f;
}

void UMarketManagerSubsystem::HandleMonthAdvanced(const FDateTime& NewDate)
{
    // Light decay on recent exposure to naturally clear competition over time.
    const float DecayRate = 0.9f;
    for (auto& RegionPair : LoadedRegions)
    {
        for (auto& ArtistExposure : RegionPair.Value.RecentArtistExposure)
        {
            ArtistExposure.Value *= DecayRate;
        }

        for (auto& RecordExposure : RegionPair.Value.RecentRecordExposure)
        {
            RecordExposure.Value *= DecayRate;
        }

        // NewDate currently unused but preserved for future seasonal/economic hooks.
        (void)NewDate;
    }
}

void UMarketManagerSubsystem::GetAllDemandSnapshots(TArray<FMarketDemandSnapshot>& OutSnapshots) const
{
    OutSnapshots.Reset();
    for (const auto& Pair : LoadedRegions)
    {
        FMarketDemandSnapshot Snapshot;
        BuildMarketDemandSnapshot(Pair.Value, Snapshot);
        OutSnapshots.Add(Snapshot);
    }
}
