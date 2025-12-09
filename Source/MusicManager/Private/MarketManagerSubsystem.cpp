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
