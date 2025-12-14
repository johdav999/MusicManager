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

    // Bind to the monthly tick to orchestrate sales simulation.
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>())
        {
            TimeSubsystem->OnMonthAdvanced.AddDynamic(this, &URecordManagerSubsystem::HandleMonthAdvanced);
        }
    }
}

void URecordManagerSubsystem::Deinitialize()
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>())
        {
            TimeSubsystem->OnMonthAdvanced.RemoveDynamic(this, &URecordManagerSubsystem::HandleMonthAdvanced);
        }
    }

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

void URecordManagerSubsystem::HandleMonthAdvanced(const FDateTime& NewDate)
{
    check(IsInGameThread());
    SimulateMonthlySales(NewDate);
}

void URecordManagerSubsystem::GetSalesHistory(const FString& RecordId, TArray<FRecordSalesEntry>& OutEntries) const
{
    if (const TArray<FRecordSalesEntry>* Found = SalesHistory.Find(RecordId))
    {
        OutEntries = *Found;
    }
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

    TArray<FMarketRegion> Regions;
    MarketSubsystem->GetAllRegions(Regions);

    // Count concurrent releases per artist for cannibalization calculation.
    TMap<FString, int32> ReleasesPerArtist;
    for (const TPair<FString, FRecordData>& Pair : Records)
    {
        if (Pair.Value.ReleaseDate <= CurrentDate)
        {
            ReleasesPerArtist.FindOrAdd(Pair.Value.ArtistId)++;
        }
    }

    TArray<FRecordSalesEntry> AllSalesEntries;

    for (const TPair<FString, FRecordData>& Pair : Records)
    {
        const FRecordData& Record = Pair.Value;
        if (Record.ReleaseDate > CurrentDate)
        {
            continue; // Not released yet.
        }

        const int32 ConcurrentReleases = ReleasesPerArtist.FindRef(Record.ArtistId);

        // Evaluate artist impact per market and compute sales volume per format.
        for (const FMarketRegion& Region : Regions)
        {
            FMarketDemandSnapshot Demand;
            if (!MarketSubsystem->EvaluateDemandSnapshot(Region.RegionId, Record.PrimaryGenre, CurrentDate, Demand))
            {
                continue;
            }

            const FArtistMarketModifiers ArtistImpact = ArtistSubsystem->EvaluateMarketModifiers(Record.ArtistId, Record.PrimaryGenre, Region.RegionId, CurrentDate, ConcurrentReleases);

            ComputeRecordSalesForMarket(Record, Region.RegionId, Demand, ArtistImpact, CurrentDate, AllSalesEntries);
        }
    }

    // Persist sales and hand volumes to FinanceManager.
    for (const FRecordSalesEntry& Entry : AllSalesEntries)
    {
        SalesHistory.FindOrAdd(Entry.RecordId).Add(Entry);
        LifetimeUnits.FindOrAdd(Entry.RecordId) += Entry.UnitsSold;

        const FRecordFormatRule* Rule = FormatRules.Find(Entry.Format);
        if (Rule)
        {
            const float GrossRevenue = Entry.UnitsSold * Rule->BasePrice;
            const float Cost = GrossRevenue * Rule->CostRate;
            const float Net = GrossRevenue - Cost;

            FinanceSubsystem->RegisterRecordSalesRevenue(Records[Entry.RecordId].LabelId, Entry.RecordId, Net, Entry.Month);

            // Record distribution cost separately to keep ledger transparent.
            FCashFlowEntry CostEntry;
            CostEntry.LabelId = Records[Entry.RecordId].LabelId;
            CostEntry.Type = ETransactionType::MarketingCost;
            CostEntry.Amount = -Cost;
            CostEntry.Timestamp = Entry.Month;
            CostEntry.RefId = Entry.RecordId;
            FinanceSubsystem->RegisterTransaction(CostEntry);
        }
    }
}

void URecordManagerSubsystem::ComputeRecordSalesForMarket(const FRecordData& Record, const FString& MarketId, const FMarketDemandSnapshot& Demand, const FArtistMarketModifiers& ArtistImpact, const FDateTime& CurrentDate, TArray<FRecordSalesEntry>& OutEntries) const
{
    const float LifecycleFactor = ComputeLifecycleFactor(Record.ReleaseDate, CurrentDate);
    const float SongQuality = EvaluateSongQuality(Record);
    const float Exposure = Record.MarketingExposure;

    const float BaseDemand = Demand.GetCompositeDemand() * ArtistImpact.GetComposite() * Record.RecordQuality * SongQuality * Exposure * LifecycleFactor;

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
        Entry.MarketId = MarketId;
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
