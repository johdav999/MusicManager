#include "MarketManagerSubsystem.h"

#include "Algo/RandomShuffle.h"
#include "ArtistManagerSubsystem.h"
#include "MusicSaveGame.h"
#include "RecordManagerSubsystem.h"

void UMarketManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadMarketSegmentProfiles();
    LoadRegions();
    AssignSegmentsToRegions();
}

void UMarketManagerSubsystem::LoadRegions()
{
    LoadedRegions.Empty();
    RegionArtistExposure.Empty();
    RegionRecordExposure.Empty();
    RegionSegments.Empty();

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
        RegionArtistExposure.Add(Row->RegionId, FRegionArtistExposureState());
        RegionRecordExposure.Add(Row->RegionId, FRegionRecordExposureState());
    }

    UE_LOG(LogTemp, Log, TEXT("MarketManagerSubsystem: Loaded %d regions."), LoadedRegions.Num());
}

void UMarketManagerSubsystem::LoadMarketSegmentProfiles()
{
    LoadedSegmentProfiles.Empty();

    if (!SegmentProfileDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("MarketManagerSubsystem: No SegmentProfileDataTable assigned."));
        return;
    }

    static const FString Context = TEXT("SegmentProfileDataTable Load");

    TArray<FMarketSegmentProfile*> Rows;
    SegmentProfileDataTable->GetAllRows(Context, Rows);

    for (FMarketSegmentProfile* Row : Rows)
    {
        if (!Row) continue;
        LoadedSegmentProfiles.Add(Row->SegmentId, *Row);
    }

    UE_LOG(LogTemp, Log, TEXT("MarketManagerSubsystem: Loaded %d market segment profiles."), LoadedSegmentProfiles.Num());
}

void UMarketManagerSubsystem::AssignSegmentsToRegions()
{
    RegionSegments.Empty();

    for (const auto& RegionPair : LoadedRegions)
    {
        const FMarketRegion& Region = RegionPair.Value;

        FResolvedMarketSegments ResolvedSegments;
        float TotalPopulationShare = 0.f;

        if (Region.SegmentIds.Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("MarketManagerSubsystem: Region %s has no segment ids configured."), *Region.RegionId);
        }

        for (const FString& SegmentId : Region.SegmentIds)
        {
            if (SegmentId.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("MarketManagerSubsystem: Region %s contains an empty segment id entry."), *Region.RegionId);
                continue;
            }

            if (const FMarketSegmentProfile* SegmentProfile = LoadedSegmentProfiles.Find(SegmentId))
            {
                ResolvedSegments.Segments.Add(*SegmentProfile);
                TotalPopulationShare += SegmentProfile->PopulationShare;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("MarketManagerSubsystem: Region %s references missing segment id '%s'."), *Region.RegionId, *SegmentId);
            }
        }

        if (ResolvedSegments.Segments.Num() == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("MarketManagerSubsystem: Region %s has no resolved market segments."), *Region.RegionId);
        }

        if (TotalPopulationShare > 1.0f + KINDA_SMALL_NUMBER || TotalPopulationShare < 0.95f)
        {
            UE_LOG(LogTemp, Warning, TEXT("MarketManagerSubsystem: Region %s population share sum %.2f outside recommended range [0.95, 1.0]."), *Region.RegionId, TotalPopulationShare);
        }

        RegionSegments.Add(Region.RegionId, MoveTemp(ResolvedSegments));
    }
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

bool UMarketManagerSubsystem::AddRecordExposure(const FString& RegionId, const FString& RecordId, float ExposureAmount)
{
    if (RegionId.IsEmpty() || RecordId.IsEmpty() || !LoadedRegions.Contains(RegionId))
    {
        return false;
    }

    if (!FMath::IsFinite(ExposureAmount) || ExposureAmount <= 0.f)
    {
        return false;
    }

    TMap<FString, float>& ExposureByRecord = RegionRecordExposure.FindOrAdd(RegionId).RecordExposure;
    float& CurrentExposure = ExposureByRecord.FindOrAdd(RecordId);
    CurrentExposure = FMath::Clamp(CurrentExposure + ExposureAmount, 0.f, 3.0f);
    return true;
}

float UMarketManagerSubsystem::GetRecordExposure(const FString& RegionId, const FString& RecordId) const
{
    if (const FRegionRecordExposureState* RegionExposure = RegionRecordExposure.Find(RegionId))
    {
        if (const float* Exposure = RegionExposure->RecordExposure.Find(RecordId))
        {
            return *Exposure;
        }
    }

    return 0.f;
}

void UMarketManagerSubsystem::BuildMarketDemandSnapshot(const FMarketRegion& Region, FMarketDemandSnapshot& OutSnapshot) const
{
    OutSnapshot = FMarketDemandSnapshot();
    OutSnapshot.MarketId = Region.RegionId;

    float TotalEffectiveReach = 0.f;
    float MaxGenreDemand = 0.f;

    auto ConvertExposureToBoost = [](float Exposure)
    {
        // Exposure is capped by radio play simulation; convert to a gentle multiplier.
        return FMath::Clamp(1.f + Exposure * 0.3f, 1.f, 1.5f);
    };

    // Sum exposure once per region so each segment shares the same dampening.
    float ExposureSum = 0.f;
    if (const FRegionArtistExposureState* ArtistExposure = RegionArtistExposure.Find(Region.RegionId))
    {
        for (const auto& Pair : ArtistExposure->ArtistExposure)
        {
            ExposureSum += Pair.Value;
            OutSnapshot.ArtistRadioBoost.Add(Pair.Key, ConvertExposureToBoost(Pair.Value));
        }
    }
    if (const FRegionRecordExposureState* RecordExposure = RegionRecordExposure.Find(Region.RegionId))
    {
        for (const auto& Pair : RecordExposure->RecordExposure)
        {
            ExposureSum += Pair.Value;
            OutSnapshot.RecordRadioBoost.Add(Pair.Key, ConvertExposureToBoost(Pair.Value));
        }
    }

    const float ExposureDampen = 1.f / FMath::Max(1.f, 1.f + ExposureSum * 0.1f);

    const FResolvedMarketSegments* ResolvedSegments = RegionSegments.Find(Region.RegionId);
    if (!ResolvedSegments || ResolvedSegments->Segments.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("MarketManagerSubsystem: No resolved segments found for region %s when building demand snapshot."), *Region.RegionId);
        return;
    }

    // Aggregate each segment's contribution into per-genre demand buckets.
    for (const FMarketSegmentProfile& Segment : ResolvedSegments->Segments)
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

namespace
{
    /** TEMPORARY RADIO SIMULATION: tuning values to keep exposure lightweight. */
    constexpr int32 MinArtistsPerRegion = 3;
    constexpr int32 MaxArtistsPerRegion = 8;
    constexpr float MinExposureMultiplier = 0.05f;
    constexpr float MaxExposureMultiplier = 0.2f;
    constexpr float MaxExposurePerArtist = 2.0f;
    constexpr int32 RecentReleaseWindowMonths = 3;
}

void UMarketManagerSubsystem::SimulateMonthlyRadioPlay(const FDateTime& CurrentDate)
{
    check(IsInGameThread());

    UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        return;
    }

    UArtistManagerSubsystem* ArtistSubsystem = GameInstance->GetSubsystem<UArtistManagerSubsystem>();
    URecordManagerSubsystem* RecordSubsystem = GameInstance->GetSubsystem<URecordManagerSubsystem>();

    TArray<FString> CandidateArtistIds;

    if (ArtistSubsystem)
    {
        TArray<FArtistData> SignedArtists;
        ArtistSubsystem->GetSignedArtistData(SignedArtists);
        for (const FArtistData& Artist : SignedArtists)
        {
            CandidateArtistIds.AddUnique(Artist.ArtistName);
        }
    }

    if (RecordSubsystem)
    {
        TArray<FString> RecentReleaseArtists;
        RecordSubsystem->GetRecentlyReleasedArtists(CurrentDate, RecentReleaseWindowMonths, RecentReleaseArtists);
        for (const FString& ArtistId : RecentReleaseArtists)
        {
            CandidateArtistIds.AddUnique(ArtistId);
        }
    }

    if (CandidateArtistIds.Num() == 0)
    {
        return;
    }

    for (auto& RegionPair : LoadedRegions)
    {
        const FMarketRegion& Region = RegionPair.Value;

        TMap<FString, float>& RegionExposure = RegionArtistExposure.FindOrAdd(Region.RegionId).ArtistExposure;

        // Seeded randomness keeps tests deterministic per month/region while remaining lightweight.
        const int32 Seed = CurrentDate.ToUnixTimestamp() ^ GetTypeHash(Region.RegionId);
        FRandomStream Stream(Seed);

        TArray<FString> ShuffledArtists = CandidateArtistIds;
        for (int32 i = ShuffledArtists.Num() - 1; i > 0; --i)
        {
            const int32 SwapIndex = Stream.RandRange(0, i);
            ShuffledArtists.Swap(i, SwapIndex);
        }

        const int32 ArtistsThisRegion = FMath::Clamp(Stream.RandRange(MinArtistsPerRegion, MaxArtistsPerRegion), 0, ShuffledArtists.Num());
        const float ClampedReach = FMath::Max(0.f, Region.RadioReach);

        for (int32 Index = 0; Index < ArtistsThisRegion; ++Index)
        {
            const FString& ArtistId = ShuffledArtists[Index];

            const float Increment = ClampedReach * Stream.FRandRange(MinExposureMultiplier, MaxExposureMultiplier);
            float& Exposure = RegionExposure.FindOrAdd(ArtistId);
            Exposure = FMath::Clamp(Exposure + Increment, 0.f, MaxExposurePerArtist);

            // TODO: Genre-based radio formatting once genre metadata is wired to stations.
            // TODO: Payola/promotion hooks to bias selection.
            // TODO: Chart feedback loops and touring boosts to influence rotations.
            // TODO: Record-specific exposure that differentiates singles vs. albums.
        }
    }
}

void UMarketManagerSubsystem::HandleMonthAdvanced(const FDateTime& NewDate)
{
    check(IsInGameThread());

    // Light decay on recent exposure to naturally clear competition over time.
    const float DecayRate = 0.9f;
    for (auto& RegionExposure : RegionArtistExposure)
    {
        for (auto& ArtistExposure : RegionExposure.Value.ArtistExposure)
        {
            ArtistExposure.Value *= DecayRate;
        }
    }

    for (auto& RecordExposurePair : RegionRecordExposure)
    {
        for (auto& RecordExposure : RecordExposurePair.Value.RecordExposure)
        {
            RecordExposure.Value *= DecayRate;
        }
    }

    // TEMPORARY RADIO SIMULATION: REPLACE WITH REAL RADIO SYSTEM
    SimulateMonthlyRadioPlay(NewDate);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UArtistManagerSubsystem* ArtistSubsystem = GameInstance->GetSubsystem<UArtistManagerSubsystem>())
        {
            struct FExposureAggregate
            {
                float ExposureSum = 0.f;
                int32 Markets = 0;
            };

            TMap<FString, FExposureAggregate> ExposureByArtist;

            for (const auto& RegionPair : RegionArtistExposure)
            {
                for (const auto& ArtistExposure : RegionPair.Value.ArtistExposure)
                {
                    //if (ArtistExposure.Value < 0.6f)
                    //{
                    //    continue; // Not enough spin to meaningfully move momentum.
                    //}

                    FExposureAggregate& Aggregate = ExposureByArtist.FindOrAdd(ArtistExposure.Key);
                    Aggregate.ExposureSum += ArtistExposure.Value;
                    Aggregate.Markets++;
                }
            }

            TMap<FString, float> MomentumBoosts;
            for (const auto& Pair : ExposureByArtist)
            {
                if (Pair.Value.Markets < 2)
                {
                    continue; // Require cross-market presence to nudge momentum.
                }

                const float AverageExposure = Pair.Value.ExposureSum / static_cast<float>(Pair.Value.Markets);
                const float BoostMagnitude = FMath::Clamp(1.f + AverageExposure * 0.08f + (Pair.Value.Markets - 1) * 0.04f, 1.f, 1.25f);
                MomentumBoosts.Add(Pair.Key, BoostMagnitude);
            }

            if (MomentumBoosts.Num() > 0)
            {
                ArtistSubsystem->ApplyRadioExposureMomentum(MomentumBoosts);
            }
        }
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

void UMarketManagerSubsystem::BuildSaveSnapshot(FMarketSnapshot& OutSnapshot) const
{
    OutSnapshot.RegionArtistExposure = RegionArtistExposure;
    OutSnapshot.RegionRecordExposure = RegionRecordExposure;
}

void UMarketManagerSubsystem::ValidateSaveSnapshot(const FMarketSnapshot& Snapshot, const TSet<FString>& KnownArtistIds, const TSet<FString>& KnownRecordIds, FMusicSaveValidationResult& Result) const
{
    for (const TPair<FString, FRegionArtistExposureState>& RegionPair : Snapshot.RegionArtistExposure)
    {
        if (!LoadedRegions.Contains(RegionPair.Key))
        {
            Result.AddError(FString::Printf(TEXT("Market artist exposure references unknown region %s."), *RegionPair.Key));
        }

        for (const TPair<FString, float>& ArtistExposure : RegionPair.Value.ArtistExposure)
        {
            if (ArtistExposure.Key.IsEmpty() || !KnownArtistIds.Contains(ArtistExposure.Key))
            {
                Result.AddError(FString::Printf(TEXT("Market artist exposure references unknown artist %s."), *ArtistExposure.Key));
            }
            if (!FMath::IsFinite(ArtistExposure.Value) || ArtistExposure.Value < 0.f)
            {
                Result.AddError(FString::Printf(TEXT("Market artist exposure for artist %s has invalid value."), *ArtistExposure.Key));
            }
        }
    }

    for (const TPair<FString, FRegionRecordExposureState>& RegionPair : Snapshot.RegionRecordExposure)
    {
        if (!LoadedRegions.Contains(RegionPair.Key))
        {
            Result.AddError(FString::Printf(TEXT("Market record exposure references unknown region %s."), *RegionPair.Key));
        }

        for (const TPair<FString, float>& RecordExposure : RegionPair.Value.RecordExposure)
        {
            if (RecordExposure.Key.IsEmpty() || !KnownRecordIds.Contains(RecordExposure.Key))
            {
                Result.AddError(FString::Printf(TEXT("Market record exposure references unknown record %s."), *RecordExposure.Key));
            }
            if (!FMath::IsFinite(RecordExposure.Value) || RecordExposure.Value < 0.f)
            {
                Result.AddError(FString::Printf(TEXT("Market record exposure for record %s has invalid value."), *RecordExposure.Key));
            }
        }
    }
}

void UMarketManagerSubsystem::ApplySaveSnapshot(const FMarketSnapshot& Snapshot)
{
    RegionArtistExposure = Snapshot.RegionArtistExposure;
    RegionRecordExposure = Snapshot.RegionRecordExposure;

    for (const TPair<FString, FMarketRegion>& RegionPair : LoadedRegions)
    {
        RegionArtistExposure.FindOrAdd(RegionPair.Key);
        RegionRecordExposure.FindOrAdd(RegionPair.Key);
    }
}
