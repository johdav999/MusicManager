#include "RecordManagerSubsystem.h"

#include "GameTimeSubsystem.h"
#include "SongManagerSubsystem.h"
#include "Song.h"
#include "Engine/World.h"
#include "Misc/Guid.h"

namespace
{
    /** Soft clamp used to stabilize decay curves. */
    float DecayWithFloor(float Value, float Floor)
    {
        return FMath::Max(Value, Floor);
    }
}

void URecordManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Initialize format rules spanning the full simulation timeline.
    FormatRules.Add(ERecordFormat::Vinyl, {ERecordFormat::Vinyl, 1955, 1995, 12.0f, 0.35f});
    FormatRules.Add(ERecordFormat::Cassette, {ERecordFormat::Cassette, 1965, 2005, 9.0f, 0.30f});
    FormatRules.Add(ERecordFormat::CD, {ERecordFormat::CD, 1985, 2015, 14.0f, 0.28f});
    FormatRules.Add(ERecordFormat::DigitalDownload, {ERecordFormat::DigitalDownload, 2003, 2026, 10.0f, 0.1f});
    FormatRules.Add(ERecordFormat::Streaming, {ERecordFormat::Streaming, 2010, 2026, 1.0f, 0.02f});

    // GameTimeSubsystem orchestrates monthly flow explicitly; no event binding needed here.
}

void URecordManagerSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

FString URecordManagerSubsystem::CreateRecord(const FRecordData& Data)
{
    FString NewRecordId = Data.RecordId;
    if (NewRecordId.IsEmpty())
    {
        NewRecordId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
    }

    FRecordData StoredRecord = Data;
    StoredRecord.RecordId = NewRecordId;

    Records.Add(NewRecordId, StoredRecord);

    return NewRecordId;
}

bool URecordManagerSubsystem::GetRecordById(const FString& RecordId, FRecordData& OutData) const
{
    if (const FRecordData* Found = Records.Find(RecordId))
    {
        OutData = *Found;
        return true;
    }

    return false;
}

void URecordManagerSubsystem::GetRecentlyReleasedArtists(const FDateTime& CurrentDate, int32 MonthsBack, TArray<FString>& OutArtistIds) const
{
    OutArtistIds.Reset();

    const int32 ClampedMonths = FMath::Max(0, MonthsBack);
    const FDateTime CutoffDate = CurrentDate - FTimespan::FromDays(ClampedMonths * 30);

    for (const TPair<FString, FRecordData>& Pair : Records)
    {
        const FRecordData& Record = Pair.Value;
        if (Record.ReleaseDate > CurrentDate)
        {
            continue; // Not out yet.
        }

        if (Record.ReleaseDate >= CutoffDate)
        {
            OutArtistIds.AddUnique(Record.ArtistId);
        }
    }
}

void URecordManagerSubsystem::HandleMonthAdvanced(const FDateTime& NewDate)
{
    check(IsInGameThread());
    SimulateMonthlySales(NewDate);
}

bool URecordManagerSubsystem::GetSalesHistory(const FString& RecordId, TArray<FRecordSalesEntry>& OutEntries) const
{
    if (const FRecordSalesHistory* Found = SalesHistory.Find(RecordId))
    {
        OutEntries = Found->Entries;
        return true;
    }

    return false;
}

int32 URecordManagerSubsystem::GetLifetimeUnits(const FString& RecordId) const
{
    if (const int32* Units = LifetimeUnits.Find(RecordId))
    {
        return *Units;
    }
    return 0;
}

void URecordManagerSubsystem::SimulateMonthlySales(const FDateTime& CurrentDate)
{
    UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        return;
    }

    UMarketManagerSubsystem* MarketSubsystem = GameInstance->GetSubsystem<UMarketManagerSubsystem>();
    UArtistManagerSubsystem* ArtistSubsystem = GameInstance->GetSubsystem<UArtistManagerSubsystem>();
    UFinanceManagerSubsystem* FinanceSubsystem = GameInstance->GetSubsystem<UFinanceManagerSubsystem>();

    if (!MarketSubsystem || !ArtistSubsystem || !FinanceSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("RecordManagerSubsystem: Missing dependent subsystems for sales simulation."));
        return;
    }

    TArray<FMarketDemandSnapshot> DemandSnapshots;
    MarketSubsystem->GetAllDemandSnapshots(DemandSnapshots);

    // Count concurrent releases per artist for cannibalization calculation.
    TMap<FString, int32> ReleasesPerArtist;
    for (const TPair<FString, FRecordData>& Pair : Records)
    {
        if (Pair.Value.ReleaseDate <= CurrentDate)
        {
            ReleasesPerArtist.FindOrAdd(Pair.Value.ArtistId)++;
        }
    }

    ArtistSubsystem->ClearConcurrentReleaseCache();
    for (const auto& ReleasePair : ReleasesPerArtist)
    {
        ArtistSubsystem->SetConcurrentReleaseCount(ReleasePair.Key, ReleasePair.Value);
    }

    TArray<FRecordSalesEntry> AllSalesEntries;

    for (const TPair<FString, FRecordData>& Pair : Records)
    {
        const FRecordData& Record = Pair.Value;
        if (Record.ReleaseDate > CurrentDate)
        {
            continue; // Not released yet.
        }

        // Evaluate artist impact per market and compute sales volume per format.
        for (const FMarketDemandSnapshot& Snapshot : DemandSnapshots)
        {
            FArtistMarketModifiers ArtistImpact;
            ArtistSubsystem->GetArtistMarketModifiers(Record.ArtistId, Snapshot.MarketId, Snapshot, ArtistImpact);

            ComputeRecordSalesForMarket(Record, Snapshot, ArtistImpact, CurrentDate, AllSalesEntries);
        }
    }

    // Persist sales and hand volumes to FinanceManager.
    for (const FRecordSalesEntry& Entry : AllSalesEntries)
    {
        SalesHistory.FindOrAdd(Entry.RecordId).Entries.Add(Entry);
        LifetimeUnits.FindOrAdd(Entry.RecordId) += Entry.UnitsSold;
    }

    FinanceSubsystem->ProcessRecordSalesEntries(AllSalesEntries, FormatRules, Records);
}

void URecordManagerSubsystem::ComputeRecordSalesForMarket(const FRecordData& Record, const FMarketDemandSnapshot& Demand, const FArtistMarketModifiers& ArtistImpact, const FDateTime& CurrentDate, TArray<FRecordSalesEntry>& OutEntries) const
{
    const float LifecycleFactor = ComputeLifecycleFactor(Record.ReleaseDate, CurrentDate);
    const float SongQuality = EvaluateSongQuality(Record);
    const float Exposure = Record.MarketingExposure;

    const float GenreDemand = Demand.GenreDemand.FindRef(Record.PrimaryGenre);
    if (GenreDemand <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    float BaseDemand = Demand.TotalReach * GenreDemand;
    BaseDemand *= ArtistImpact.PopularityMultiplier;
    BaseDemand *= ArtistImpact.MomentumMultiplier;
    BaseDemand *= ArtistImpact.ReputationMultiplier;
    BaseDemand *= ArtistImpact.GenreFitMultiplier;
    BaseDemand *= Record.RecordQuality * SongQuality * Exposure * LifecycleFactor;

    if (BaseDemand <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    for (ERecordFormat Format : Record.Formats)
    {
        if (!IsFormatEligible(Format, CurrentDate))
        {
            continue; // Respect era validity.
        }

        const float FormatWeight = Format == ERecordFormat::Streaming ? 0.35f : 1.0f;
        const int32 UnitsSold = FMath::Max(0, static_cast<int32>(BaseDemand * FormatWeight));

        if (UnitsSold <= 0)
        {
            continue;
        }

        FRecordSalesEntry Entry;
        Entry.RecordId = Record.RecordId;
        Entry.MarketId = Demand.MarketId;
        Entry.Format = Format;
        Entry.Month = CurrentDate;
        Entry.UnitsSold = UnitsSold;
        Entry.DemandScore = BaseDemand;

        OutEntries.Add(Entry);
    }
}

float URecordManagerSubsystem::ComputeLifecycleFactor(const FDateTime& ReleaseDate, const FDateTime& CurrentDate) const
{
    if (ReleaseDate > CurrentDate)
    {
        return 0.f;
    }

    const int32 MonthsSinceRelease = (CurrentDate.GetYear() - ReleaseDate.GetYear()) * 12 + (CurrentDate.GetMonth() - ReleaseDate.GetMonth());
    if (MonthsSinceRelease <= 0)
    {
        return 1.25f; // Launch spike.
    }

    const float Decay = FMath::Pow(0.94f, MonthsSinceRelease);
    return DecayWithFloor(Decay, 0.25f);
}

float URecordManagerSubsystem::EvaluateSongQuality(const FRecordData& Record) const
{
    UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        return 1.0f;
    }

    const USongManagerSubsystem* SongSubsystem = GameInstance->GetSubsystem<USongManagerSubsystem>();
    if (!SongSubsystem)
    {
        return 1.0f;
    }

    float AggregateQuality = 0.f;
    int32 Counted = 0;

    for (const FString& SongId : Record.SongIds)
    {
        if (USong* Song = SongSubsystem->GetSongById(SongId))
        {
            const FSongData& Data = Song->Data;
            const float Quality = (Data.HitPotential + Data.ProductionQuality + Data.Catchiness + Data.TrendAlignment + Data.Longevity) / 500.f;
            AggregateQuality += Quality;
            ++Counted;
        }
    }

    if (Counted == 0)
    {
        return 1.0f;
    }

    return FMath::Clamp(AggregateQuality / Counted, 0.5f, 1.5f);
}

bool URecordManagerSubsystem::IsFormatEligible(ERecordFormat Format, const FDateTime& CurrentDate) const
{
    if (const FRecordFormatRule* Rule = FormatRules.Find(Format))
    {
        return Rule->IsActiveForDate(CurrentDate);
    }

    return false;
}
