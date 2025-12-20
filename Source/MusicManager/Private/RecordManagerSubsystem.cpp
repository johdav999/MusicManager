#include "RecordManagerSubsystem.h"

#include "ArtistManagerSubsystem.h"
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

    ProcessActiveRecordings(NewDate);
    SimulateMonthlySales(NewDate);
}

bool URecordManagerSubsystem::SubmitRecordingIntent(const FRecordRecordingIntent& Intent, FString& OutError)
{
    check(IsInGameThread());

    if (!ValidateRecordingIntent(Intent, OutError))
    {
        return false;
    }

    UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        OutError = TEXT("Invalid game instance for recording.");
        return false;
    }

    USongManagerSubsystem* SongSubsystem = GameInstance->GetSubsystem<USongManagerSubsystem>();
    UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>();
    if (!SongSubsystem || !TimeSubsystem)
    {
        OutError = TEXT("Missing dependent subsystems for recording.");
        return false;
    }

    FRecordRecordingIntent SanitizedIntent = Intent;
    if (SanitizedIntent.RequestedFormats.Num() == 0)
    {
        FormatRules.GenerateKeyArray(SanitizedIntent.RequestedFormats);
    }

    if (!SongSubsystem->LockSongsForRecording(SanitizedIntent.SongIds, OutError))
    {
        return false;
    }

    const FString RecordingId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
    ActiveRecordingIntents.Add(RecordingId, SanitizedIntent);

    const FDateTime StartDate = TimeSubsystem->GetCurrentGameDate();
    RecordingStartDates.Add(RecordingId, StartDate);

    // For now recordings complete within the same month; future work can add durations.
    RecordingCompletionDates.Add(RecordingId, StartDate);
    RecordStates.Add(RecordingId, ERecordLifecycleState::Recording);

    // Immediately process if completion date is now.
    ProcessActiveRecordings(StartDate);

    return true;
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

    float DemandScore = Demand.TotalReach * GenreDemand;
    DemandScore *= ArtistImpact.PopularityMultiplier;
    DemandScore *= ArtistImpact.MomentumMultiplier;
    DemandScore *= ArtistImpact.ReputationMultiplier;
    DemandScore *= ArtistImpact.GenreFitMultiplier;
    DemandScore *= Record.RecordQuality * SongQuality * Exposure * LifecycleFactor;

    const float* ArtistRadioBoostPtr = Demand.ArtistRadioBoost.Find(Record.ArtistId);
    const float ArtistRadioBoost = ArtistRadioBoostPtr ? *ArtistRadioBoostPtr : 1.f;
    const float RadioLifecycleWeight = FMath::GetMappedRangeValueClamped(FVector2D(0.25f, 1.25f), FVector2D(0.25f, 1.0f), LifecycleFactor);
    const float RadioFormatBias = Record.bIsSingle ? 1.1f : 1.0f;
    const float EffectiveRadioBoost = FMath::Clamp(1.f + (ArtistRadioBoost * RadioFormatBias - 1.f) * RadioLifecycleWeight, 0.8f, 1.8f);
    DemandScore *= EffectiveRadioBoost;

    if (DemandScore <= KINDA_SMALL_NUMBER)
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
        const int32 UnitsSold = FMath::Max(0, static_cast<int32>(DemandScore * FormatWeight));

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
        Entry.DemandScore = DemandScore;

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

void URecordManagerSubsystem::ProcessActiveRecordings(const FDateTime& CurrentDate)
{
    TArray<FString> CompletedIds;
    for (const auto& Pair : RecordingCompletionDates)
    {
        if (Pair.Value <= CurrentDate)
        {
            CompletedIds.Add(Pair.Key);
        }
    }

    for (const FString& RecordingId : CompletedIds)
    {
        CompleteRecording(RecordingId, CurrentDate);
    }
}

void URecordManagerSubsystem::CompleteRecording(const FString& RecordingId, const FDateTime& CompletionDate)
{
    FRecordRecordingIntent* Intent = ActiveRecordingIntents.Find(RecordingId);
    if (!Intent)
    {
        return;
    }

    UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        return;
    }

    USongManagerSubsystem* SongSubsystem = GameInstance->GetSubsystem<USongManagerSubsystem>();
    UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>();
    if (!SongSubsystem || !TimeSubsystem)
    {
        return;
    }

    const FDateTime RecordDate = CompletionDate;

    FRecordData NewRecord;
    NewRecord.ArtistId = Intent->ArtistId;
    NewRecord.AlbumName = Intent->AlbumName.IsEmpty() ? TEXT("Untitled Record") : Intent->AlbumName;
    NewRecord.bIsSingle = Intent->bIsSingle;
    NewRecord.bIsLP = Intent->bIsLP;
    NewRecord.SongIds = Intent->SongIds;
    NewRecord.DateRecorded = RecordDate;
    NewRecord.ReleaseDate = ResolveReleaseDate(*Intent, RecordDate);
    NewRecord.LabelId = ResolveLabelForArtist(Intent->ArtistId);
    NewRecord.PrimaryGenre = DerivePrimaryGenre(Intent->SongIds);
    NewRecord.Formats = Intent->RequestedFormats;
    ApplyFormatRules(RecordDate, NewRecord.Formats);
    NewRecord.RecordQuality = ComputeRecordQuality(Intent->SongIds, Intent->ArtistId);
    NewRecord.MarketingExposure = 1.0f;

    const FString NewRecordId = CreateRecord(NewRecord);
    RecordStates.FindOrAdd(NewRecordId) = ERecordLifecycleState::Recorded;

    SongSubsystem->MarkSongsRecorded(Intent->SongIds, NewRecordId);
    SongSubsystem->UnlockSongs(Intent->SongIds);

    ActiveRecordingIntents.Remove(RecordingId);
    RecordingStartDates.Remove(RecordingId);
    RecordingCompletionDates.Remove(RecordingId);
    RecordStates.Remove(RecordingId);
}

bool URecordManagerSubsystem::ValidateRecordingIntent(const FRecordRecordingIntent& Intent, FString& OutError)
{
    if (Intent.ArtistId.IsEmpty())
    {
        OutError = TEXT("Artist must be selected.");
        return false;
    }

    if (Intent.bIsSingle == Intent.bIsLP)
    {
        OutError = TEXT("Choose either Single or LP.");
        return false;
    }

    if (Intent.SongIds.Num() == 0)
    {
        OutError = TEXT("At least one song must be selected.");
        return false;
    }

    if (Intent.bIsSingle && Intent.SongIds.Num() != 1)
    {
        OutError = TEXT("Singles must contain exactly one track.");
        return false;
    }

    if (Intent.bIsLP && Intent.SongIds.Num() < 2)
    {
        OutError = TEXT("LPs require multiple tracks.");
        return false;
    }

    TSet<FString> UniqueSongs(Intent.SongIds);
    if (UniqueSongs.Num() != Intent.SongIds.Num())
    {
        OutError = TEXT("Duplicate songs are not allowed.");
        return false;
    }

    UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        OutError = TEXT("No game instance available for validation.");
        return false;
    }

    UArtistManagerSubsystem* ArtistSubsystem = GameInstance->GetSubsystem<UArtistManagerSubsystem>();
    USongManagerSubsystem* SongSubsystem = GameInstance->GetSubsystem<USongManagerSubsystem>();
    if (!ArtistSubsystem || !SongSubsystem)
    {
        OutError = TEXT("Required subsystems missing for recording.");
        return false;
    }

    const FArtistContract* Contract = ArtistSubsystem->GetContractByArtistId(Intent.ArtistId);
    if (!Contract || !Contract->bContractActive)
    {
        OutError = TEXT("Artist is not currently signed or eligible to record.");
        return false;
    }

    for (const FString& SongId : Intent.SongIds)
    {
        if (USong* Song = SongSubsystem->GetSongById(SongId))
        {
            if (Song->ArtistId != Intent.ArtistId)
            {
                OutError = TEXT("All songs must belong to the selected artist.");
                return false;
            }

            if (Song->Data.bIsReleased)
            {
                OutError = TEXT("Songs that are already released cannot be recorded again.");
                return false;
            }
        }
        else
        {
            OutError = TEXT("Invalid song selection detected.");
            return false;
        }
    }

    return true;
}

void URecordManagerSubsystem::ApplyFormatRules(const FDateTime& CurrentDate, TArray<ERecordFormat>& InOutFormats) const
{
    TSet<ERecordFormat> UniqueFormats;
    for (ERecordFormat Format : InOutFormats)
    {
        if (IsFormatEligible(Format, CurrentDate))
        {
            UniqueFormats.Add(Format);
        }
    }

    if (UniqueFormats.Num() == 0)
    {
        UniqueFormats.Add(ERecordFormat::DigitalDownload);
    }

    InOutFormats = UniqueFormats.Array();
}

FString URecordManagerSubsystem::DerivePrimaryGenre(const TArray<FString>& SongIds) const
{
    UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        return TEXT("Unknown");
    }

    const USongManagerSubsystem* SongSubsystem = GameInstance->GetSubsystem<USongManagerSubsystem>();
    if (!SongSubsystem)
    {
        return TEXT("Unknown");
    }

    TMap<FString, int32> GenreCounts;
    for (const FString& SongId : SongIds)
    {
        if (USong* Song = SongSubsystem->GetSongById(SongId))
        {
            GenreCounts.FindOrAdd(Song->Data.Genre)++;
        }
    }

    FString SelectedGenre = TEXT("Unknown");
    int32 BestCount = 0;
    for (const auto& Pair : GenreCounts)
    {
        if (Pair.Value > BestCount)
        {
            BestCount = Pair.Value;
            SelectedGenre = Pair.Key;
        }
    }

    return SelectedGenre;
}

float URecordManagerSubsystem::ComputeRecordQuality(const TArray<FString>& SongIds, const FString& ArtistId) const
{
    FRecordData TempRecord;
    TempRecord.ArtistId = ArtistId;
    TempRecord.SongIds = SongIds;

    const float SongQuality = EvaluateSongQuality(TempRecord);
    const float ProductionFloor = 0.6f;
    return DecayWithFloor(SongQuality, ProductionFloor);
}

FDateTime URecordManagerSubsystem::ResolveReleaseDate(const FRecordRecordingIntent& Intent, const FDateTime& DateRecorded) const
{
    if (Intent.DesiredReleaseDate.IsSet() && Intent.DesiredReleaseDate.GetValue() >= DateRecorded)
    {
        return Intent.DesiredReleaseDate.GetValue();
    }

    return DateRecorded;
}

FString URecordManagerSubsystem::ResolveLabelForArtist(const FString& ArtistId) const
{
    // Placeholder until labels are fully modeled. Using artist id helps downstream finance lookups stay deterministic.
    return ArtistId;
}
