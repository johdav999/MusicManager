#include "RecordManagerSubsystem.h"

#include "ArtistManagerSubsystem.h"
#include "GameTimeSubsystem.h"
#include "MusicSaveGame.h"
#include "PlayerLabelSubsystem.h"
#include "SongManagerSubsystem.h"
#include "Song.h"
#include "EventSubsystem.h"
#include "Engine/World.h"
#include "Misc/Guid.h"

namespace
{
    /** Soft clamp used to stabilize decay curves. */
    float DecayWithFloor(float Value, float Floor)
    {
        return FMath::Max(Value, Floor);
    }

    const TCHAR* RecordTypeName(ERecordType RecordType)
    {
        switch (RecordType)
        {
        case ERecordType::Single:
            return TEXT("Single");
        case ERecordType::EP:
            return TEXT("EP");
        case ERecordType::LP:
            return TEXT("LP");
        default:
            return TEXT("Record");
        }
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
    RecordStates.FindOrAdd(NewRecordId) = StoredRecord.ReleaseDate.GetTicks() > 0
        ? ERecordLifecycleState::Recorded
        : ERecordLifecycleState::Draft;

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

int32 URecordManagerSubsystem::GetRecordCountForArtist(const FString& ArtistId) const
{
    if (ArtistId.IsEmpty())
    {
        return 0;
    }

    int32 Count = 0;
    for (const TPair<FString, FRecordData>& Pair : Records)
    {
        if (Pair.Value.ArtistId == ArtistId)
        {
            ++Count;
        }
    }

    return Count;
}

void URecordManagerSubsystem::GetRecentlyReleasedArtists(const FDateTime& CurrentDate, int32 MonthsBack, TArray<FString>& OutArtistIds) const
{
    OutArtistIds.Reset();

    const int32 ClampedMonths = FMath::Max(0, MonthsBack);
    const FDateTime CutoffDate = CurrentDate - FTimespan::FromDays(ClampedMonths * 30);

    for (const TPair<FString, FRecordData>& Pair : Records)
    {
        const FRecordData& Record = Pair.Value;
        const ERecordLifecycleState State = ResolveLifecycleStateForRecord(Record.RecordId);
        if ((State != ERecordLifecycleState::Released && State != ERecordLifecycleState::Catalog) || Record.ReleaseDate > CurrentDate)
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
    ProcessScheduledReleases(NewDate);
    SimulateMonthlySales(NewDate);
}

bool URecordManagerSubsystem::SubmitRecordingIntent(const FRecordRecordingIntent& Intent, FString& OutError)
{
    check(IsInGameThread());

    const FRecordRecordingIntent SanitizedIntent = NormalizeRecordingIntent(Intent);

    if (!ValidateRecordingIntent(SanitizedIntent, OutError))
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

    FRecordRecordingIntent SessionIntent = SanitizedIntent;
    if (SessionIntent.RequestedFormats.Num() == 0)
    {
        FormatRules.GenerateKeyArray(SessionIntent.RequestedFormats);
    }

    const FString RequiredGenre = DerivePrimaryGenre(SessionIntent.SongIds);
    if (!SongSubsystem->AssignSongsToArtist(SessionIntent.SongIds, SessionIntent.ArtistId, RequiredGenre, OutError))
    {
        return false;
    }

    if (!SongSubsystem->LockSongsForRecording(SessionIntent.SongIds, OutError))
    {
        return false;
    }

    if (UArtistManagerSubsystem* ArtistSubsystem = GameInstance->GetSubsystem<UArtistManagerSubsystem>())
    {
        ArtistSubsystem->RefreshArtistActionAvailability(SessionIntent.ArtistId);
    }

    const FString RecordingId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
    ActiveRecordingIntents.Add(RecordingId, SessionIntent);

    const FDateTime StartDate = TimeSubsystem->GetCurrentGameDate();
    const int32 DurationDays = EstimateRecordingDurationDays(SessionIntent);
    const FDateTime CompletionDate = StartDate + FTimespan::FromDays(DurationDays);
    RecordingStartDates.Add(RecordingId, StartDate);
    RecordingCompletionDates.Add(RecordingId, CompletionDate);
    RecordingCosts.Add(RecordingId, EstimateRecordingCost(SessionIntent));

    UE_LOG(LogTemp, Log, TEXT("Recording session started: RecordingId=%s ArtistId=%s Type=%s Songs=%d Cost=%.0f Completion=%s."),
        *RecordingId,
        *SessionIntent.ArtistId,
        RecordTypeName(SessionIntent.RecordType),
        SessionIntent.SongIds.Num(),
        RecordingCosts.FindRef(RecordingId),
        *CompletionDate.ToString());

    return true;
}

bool URecordManagerSubsystem::BuildRecordingProjection(const FRecordRecordingIntent& Intent, FRecordingProjection& OutProjection, FString& OutError) const
{
    const FRecordRecordingIntent NormalizedIntent = NormalizeRecordingIntent(Intent);

    OutProjection = FRecordingProjection();
    OutProjection.ArtistId = NormalizedIntent.ArtistId;
    OutProjection.RecordType = NormalizedIntent.RecordType;
    OutProjection.SongCount = NormalizedIntent.SongIds.Num();
    OutProjection.EstimatedRecordingCost = EstimateRecordingCost(NormalizedIntent);
    OutProjection.EstimatedDurationDays = EstimateRecordingDurationDays(NormalizedIntent);

    const UGameTimeSubsystem* TimeSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGameTimeSubsystem>() : nullptr;
    OutProjection.StartDate = TimeSubsystem ? TimeSubsystem->GetCurrentGameDate() : FDateTime(1955, 1, 1);
    OutProjection.EstimatedCompletionDate = OutProjection.StartDate + FTimespan::FromDays(OutProjection.EstimatedDurationDays);

    OutProjection.bCanRecord = ValidateRecordingIntent(NormalizedIntent, OutError);
    if (!OutProjection.bCanRecord && !OutError.IsEmpty())
    {
        OutProjection.ValidationWarnings.Add(OutError);
    }

    return OutProjection.bCanRecord;
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

void URecordManagerSubsystem::GetAllRecords(TArray<FRecordData>& OutRecords) const
{
    OutRecords.Reset();
    Records.GenerateValueArray(OutRecords);
}

bool URecordManagerSubsystem::ScheduleRelease(const FScheduleReleaseCommand& Command, FString& OutError)
{
    check(IsInGameThread());

    if (!ValidateReleaseCommand(Command, OutError))
    {
        return false;
    }

    FRecordData& Record = Records.FindChecked(Command.RecordId);
    Record.ReleaseDate = Command.ReleaseDate;
    Record.Formats = Command.Formats;
    ApplyFormatRules(Command.ReleaseDate, Record.Formats);
    Record.TargetRegionIds = Command.TargetRegionIds;

    UGameTimeSubsystem* TimeSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGameTimeSubsystem>() : nullptr;
    const FDateTime CurrentDate = TimeSubsystem ? TimeSubsystem->GetCurrentGameDate() : Command.ReleaseDate;
    RecordStates.FindOrAdd(Command.RecordId) = Command.ReleaseDate <= CurrentDate
        ? ERecordLifecycleState::Released
        : ERecordLifecycleState::Scheduled;

    return true;
}

bool URecordManagerSubsystem::ReleaseNow(const FString& RecordId, const FString& LabelId, FString& OutError)
{
    UGameTimeSubsystem* TimeSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGameTimeSubsystem>() : nullptr;
    UMarketManagerSubsystem* MarketSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMarketManagerSubsystem>() : nullptr;
    if (!TimeSubsystem || !MarketSubsystem)
    {
        OutError = TEXT("Required subsystems are unavailable for immediate release.");
        return false;
    }

    FRecordData ExistingRecord;
    if (!GetRecordById(RecordId, ExistingRecord))
    {
        OutError = TEXT("Record does not exist.");
        return false;
    }

    TArray<FString> RegionIds = ExistingRecord.TargetRegionIds;
    if (RegionIds.Num() == 0)
    {
        TArray<FMarketRegion> Regions;
        MarketSubsystem->GetAllRegions(Regions);
        for (const FMarketRegion& Region : Regions)
        {
            RegionIds.Add(Region.RegionId);
        }
    }

    FScheduleReleaseCommand Command;
    Command.RecordId = RecordId;
    Command.LabelId = LabelId;
    Command.ReleaseDate = TimeSubsystem->GetCurrentGameDate();
    Command.TargetRegionIds = RegionIds;
    Command.Formats = ExistingRecord.Formats;

    return ScheduleRelease(Command, OutError);
}

bool URecordManagerSubsystem::GetRecordLifecycleState(const FString& RecordId, ERecordLifecycleState& OutState) const
{
    if (!Records.Contains(RecordId))
    {
        return false;
    }

    OutState = ResolveLifecycleStateForRecord(RecordId);
    return true;
}

void URecordManagerSubsystem::GetRecordsAwaitingReleasePlanning(TArray<FRecordData>& OutRecords) const
{
    OutRecords.Reset();
    for (const TPair<FString, FRecordData>& Pair : Records)
    {
        const ERecordLifecycleState State = ResolveLifecycleStateForRecord(Pair.Key);
        if (State == ERecordLifecycleState::Recorded)
        {
            OutRecords.Add(Pair.Value);
        }
    }
}

void URecordManagerSubsystem::GetScheduledReleases(TArray<FRecordData>& OutRecords) const
{
    OutRecords.Reset();
    for (const TPair<FString, FRecordData>& Pair : Records)
    {
        if (ResolveLifecycleStateForRecord(Pair.Key) == ERecordLifecycleState::Scheduled)
        {
            OutRecords.Add(Pair.Value);
        }
    }
}

void URecordManagerSubsystem::GetReleasedOrCatalogRecords(TArray<FRecordData>& OutRecords) const
{
    OutRecords.Reset();
    for (const TPair<FString, FRecordData>& Pair : Records)
    {
        const ERecordLifecycleState State = ResolveLifecycleStateForRecord(Pair.Key);
        if (State == ERecordLifecycleState::Released || State == ERecordLifecycleState::Catalog)
        {
            OutRecords.Add(Pair.Value);
        }
    }
}

bool URecordManagerSubsystem::BuildReleasePlannerView(const FString& SelectedRecordId, FReleasePlannerView& OutView, FString& OutError) const
{
    OutView = FReleasePlannerView();

    TArray<FRecordData> PlannableRecords;
    GetRecordsAwaitingReleasePlanning(PlannableRecords);

    for (const FRecordData& Record : PlannableRecords)
    {
        FReleasePlannerRecordOption Option;
        Option.RecordId = Record.RecordId;
        Option.AlbumName = Record.AlbumName;
        Option.ArtistDisplayName = ResolveArtistDisplayName(Record.ArtistId);
        Option.LabelDisplayName = Record.LabelId;
        Option.ArtistId = Record.ArtistId;
        Option.LabelId = Record.LabelId;
        Option.DateRecorded = Record.DateRecorded;
        Option.LifecycleState = ResolveLifecycleStateForRecord(Record.RecordId);
        OutView.PlannableRecords.Add(Option);
    }

    OutView.bHasPlannableRecords = OutView.PlannableRecords.Num() > 0;
    if (!OutView.bHasPlannableRecords)
    {
        OutView.ValidationWarnings.Add(TEXT("No recorded music is awaiting release planning."));
        return true;
    }

    const FRecordData* SelectedRecord = nullptr;
    if (!SelectedRecordId.IsEmpty())
    {
        SelectedRecord = Records.Find(SelectedRecordId);
    }
    if (!SelectedRecord)
    {
        SelectedRecord = &PlannableRecords[0];
    }

    OutView.SelectedRecord = *SelectedRecord;
    OutView.SelectedRecordDisplayName = GetRecordDisplayName(SelectedRecord->RecordId);
    OutView.SelectedArtistDisplayName = ResolveArtistDisplayName(SelectedRecord->ArtistId);
    OutView.SelectedRecordState = ResolveLifecycleStateForRecord(SelectedRecord->RecordId);

    const UGameTimeSubsystem* TimeSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGameTimeSubsystem>() : nullptr;
    const FDateTime CurrentDate = TimeSubsystem ? TimeSubsystem->GetCurrentGameDate() : FDateTime(1955, 1, 1);
    for (const TPair<ERecordFormat, FRecordFormatRule>& Pair : FormatRules)
    {
        if (Pair.Value.IsActiveForDate(CurrentDate))
        {
            OutView.ValidFormats.Add(Pair.Value);
        }
    }

    const UMarketManagerSubsystem* MarketSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMarketManagerSubsystem>() : nullptr;
    if (MarketSubsystem)
    {
        MarketSubsystem->GetAllRegions(OutView.AvailableRegions);

        TArray<FMarketDemandSnapshot> Snapshots;
        MarketSubsystem->GetAllDemandSnapshots(Snapshots);
        for (const FMarketDemandSnapshot& Snapshot : Snapshots)
        {
            OutView.ProjectedReach += Snapshot.TotalReach;
        }
    }
    else
    {
        OutView.ValidationWarnings.Add(TEXT("Market data is unavailable."));
    }

    if (OutView.ValidFormats.Num() == 0)
    {
        OutView.ValidationWarnings.Add(TEXT("No formats are valid for the current date."));
    }
    if (OutView.AvailableRegions.Num() == 0)
    {
        OutView.ValidationWarnings.Add(TEXT("No release regions are available."));
    }

    return true;
}

void URecordManagerSubsystem::BuildReleaseDashboardSummary(FReleaseDashboardSummary& OutSummary) const
{
    OutSummary = FReleaseDashboardSummary();

    for (const TPair<FString, FRecordData>& Pair : Records)
    {
        const ERecordLifecycleState State = ResolveLifecycleStateForRecord(Pair.Key);
        FReleaseDashboardItem Item = BuildReleaseDashboardItem(Pair.Value);
        if (State == ERecordLifecycleState::Recorded)
        {
            OutSummary.AwaitingPlanning.Add(Item);
        }
        else if (State == ERecordLifecycleState::Scheduled)
        {
            OutSummary.ScheduledReleases.Add(Item);
        }
        else if (State == ERecordLifecycleState::Released || State == ERecordLifecycleState::Catalog)
        {
            OutSummary.RecentlyReleased.Add(Item);
        }
    }

    auto SortByReleaseDate = [](const FReleaseDashboardItem& A, const FReleaseDashboardItem& B)
    {
        return A.ReleaseDate < B.ReleaseDate;
    };
    OutSummary.AwaitingPlanning.Sort(SortByReleaseDate);
    OutSummary.ScheduledReleases.Sort(SortByReleaseDate);
    OutSummary.RecentlyReleased.Sort(SortByReleaseDate);
}

FString URecordManagerSubsystem::GetRecordDisplayName(const FString& RecordId) const
{
    if (const FRecordData* Record = Records.Find(RecordId))
    {
        return Record->AlbumName.IsEmpty() ? Record->RecordId : Record->AlbumName;
    }

    return RecordId;
}

FString URecordManagerSubsystem::GetArtistDisplayNameForRecord(const FString& RecordId) const
{
    if (const FRecordData* Record = Records.Find(RecordId))
    {
        return ResolveArtistDisplayName(Record->ArtistId);
    }

    return FString();
}

bool URecordManagerSubsystem::IsRecordMarketable(const FString& RecordId, FString& OutLabelId) const
{
    if (const FRecordData* Record = Records.Find(RecordId))
    {
        const ERecordLifecycleState State = ResolveLifecycleStateForRecord(RecordId);
        if (State == ERecordLifecycleState::Recorded || State == ERecordLifecycleState::Scheduled || State == ERecordLifecycleState::Released || State == ERecordLifecycleState::Catalog)
        {
            OutLabelId = Record->LabelId;
            return true;
        }
    }

    return false;
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
        const ERecordLifecycleState State = ResolveLifecycleStateForRecord(Pair.Key);
        if ((State == ERecordLifecycleState::Released || State == ERecordLifecycleState::Catalog) && Pair.Value.ReleaseDate <= CurrentDate)
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
        const ERecordLifecycleState State = ResolveLifecycleStateForRecord(Pair.Key);
        if ((State != ERecordLifecycleState::Released && State != ERecordLifecycleState::Catalog) || Record.ReleaseDate > CurrentDate)
        {
            continue; // Not released yet.
        }

        // Evaluate artist impact per market and compute sales volume per format.
        for (const FMarketDemandSnapshot& Snapshot : DemandSnapshots)
        {
            if (!IsRegionTargetedByRecord(Record, Snapshot.MarketId))
            {
                continue;
            }

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

    const float* RecordRadioBoostPtr = Demand.RecordRadioBoost.Find(Record.RecordId);
    const float RecordRadioBoost = RecordRadioBoostPtr ? *RecordRadioBoostPtr : 1.f;
    DemandScore *= FMath::Clamp(RecordRadioBoost, 0.8f, 2.0f);

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
    NewRecord.RecordType = Intent->RecordType;
    NewRecord.bIsSingle = Intent->RecordType == ERecordType::Single;
    NewRecord.bIsLP = Intent->RecordType == ERecordType::LP;
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
    RecordStates.FindOrAdd(NewRecordId) = Intent->DesiredReleaseDate.IsSet()
        ? ERecordLifecycleState::Scheduled
        : ERecordLifecycleState::Recorded;

    SongSubsystem->MarkSongsRecorded(Intent->SongIds, NewRecordId);
    SongSubsystem->UnlockSongs(Intent->SongIds);

    if (UEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<UEventSubsystem>())
    {
        FMusicNewsEvent Event;
        Event.NewsId = FGuid::NewGuid();
        Event.Timestamp = CompletionDate;
        Event.NewsType = EMusicNewsType::RecordingSession;
        Event.SourceName = ResolveArtistDisplayName(Intent->ArtistId);
        Event.SubjectName = NewRecord.AlbumName;
        Event.Headline = FString::Printf(TEXT("%s recording complete"), *NewRecord.AlbumName);
        Event.BodyText = FString::Printf(
            TEXT("%s has completed recording %s, a %s with %d track(s). It is ready for release planning."),
            *Event.SourceName,
            *NewRecord.AlbumName,
            RecordTypeName(Intent->RecordType),
            Intent->SongIds.Num());
        Event.Tags = { TEXT("Recording"), TEXT("Studio"), RecordTypeName(Intent->RecordType), NewRecord.PrimaryGenre };
        Event.Metadata.Add(TEXT("ArtistId"), Intent->ArtistId);
        Event.Metadata.Add(TEXT("RecordId"), NewRecordId);
        Event.Metadata.Add(TEXT("RecordType"), RecordTypeName(Intent->RecordType));
        Event.Metadata.Add(TEXT("SongCount"), FString::FromInt(Intent->SongIds.Num()));
        Event.Metadata.Add(TEXT("RecordingCost"), FString::SanitizeFloat(RecordingCosts.FindRef(RecordingId)));
        EventSubsystem->PublishNewsEvent(Event, FString::Printf(TEXT("RecordingComplete:%s"), *NewRecordId));
    }

    ensureMsgf(IsInGameThread(), TEXT("RecordManagerSubsystem: Record creation must occur on the game thread."));
    OnArtistRecordCreated.Broadcast(Intent->ArtistId);

    ActiveRecordingIntents.Remove(RecordingId);
    RecordingStartDates.Remove(RecordingId);
    RecordingCompletionDates.Remove(RecordingId);
    RecordingCosts.Remove(RecordingId);
    RecordStates.Remove(RecordingId);
}

bool URecordManagerSubsystem::ValidateRecordingIntent(const FRecordRecordingIntent& Intent, FString& OutError) const
{
    const FRecordRecordingIntent NormalizedIntent = NormalizeRecordingIntent(Intent);

    if (NormalizedIntent.ArtistId.IsEmpty())
    {
        OutError = TEXT("Artist must be selected.");
        return false;
    }

    if (!IsSongCountValidForType(NormalizedIntent.RecordType, NormalizedIntent.SongIds.Num(), OutError))
    {
        return false;
    }

    TSet<FString> UniqueSongs(NormalizedIntent.SongIds);
    if (UniqueSongs.Num() != NormalizedIntent.SongIds.Num())
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

    const FArtistContract* Contract = ArtistSubsystem->GetContractByArtistId(NormalizedIntent.ArtistId);
    if (!Contract || !Contract->bContractActive)
    {
        OutError = TEXT("Artist is not currently signed or eligible to record.");
        return false;
    }

    for (const FString& SongId : NormalizedIntent.SongIds)
    {
        if (USong* Song = SongSubsystem->GetSongById(SongId))
        {
            const bool bArtistOwned = Song->ArtistId == NormalizedIntent.ArtistId;
            const bool bUnownedGenreMatch = Song->ArtistId.IsEmpty()
                && !Contract->ArtistData.Genre.IsEmpty()
                && Song->Data.Genre.Equals(Contract->ArtistData.Genre, ESearchCase::IgnoreCase);
            if (!bArtistOwned && !bUnownedGenreMatch)
            {
                OutError = TEXT("All songs must belong to the selected artist or match the artist genre as unowned catalog songs.");
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
        for (const TPair<ERecordFormat, FRecordFormatRule>& Pair : FormatRules)
        {
            if (Pair.Value.IsActiveForDate(CurrentDate))
            {
                UniqueFormats.Add(Pair.Key);
                break;
            }
        }
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

ERecordLifecycleState URecordManagerSubsystem::ResolveLifecycleStateForRecord(const FString& RecordId) const
{
    if (const ERecordLifecycleState* State = RecordStates.Find(RecordId))
    {
        return *State;
    }

    if (const FRecordData* Record = Records.Find(RecordId))
    {
        return Record->ReleaseDate.GetTicks() > 0 ? ERecordLifecycleState::Recorded : ERecordLifecycleState::Draft;
    }

    return ERecordLifecycleState::Draft;
}

bool URecordManagerSubsystem::ValidateReleaseCommand(const FScheduleReleaseCommand& Command, FString& OutError) const
{
    const FRecordData* Record = Records.Find(Command.RecordId);
    if (!Record)
    {
        OutError = TEXT("Record does not exist.");
        return false;
    }

    if (Command.LabelId.IsEmpty() || Record->LabelId != Command.LabelId)
    {
        OutError = TEXT("Record is not owned by the selected label.");
        return false;
    }

    const ERecordLifecycleState State = ResolveLifecycleStateForRecord(Command.RecordId);
    if (State != ERecordLifecycleState::Recorded && State != ERecordLifecycleState::Scheduled)
    {
        OutError = TEXT("Only recorded or scheduled records can be scheduled for release.");
        return false;
    }

    if (Command.ReleaseDate.GetTicks() <= 0 || Command.ReleaseDate < Record->DateRecorded)
    {
        OutError = TEXT("Release date cannot be before the recording date.");
        return false;
    }

    const UGameTimeSubsystem* TimeSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGameTimeSubsystem>() : nullptr;
    if (TimeSubsystem && Command.ReleaseDate < TimeSubsystem->GetCurrentGameDate())
    {
        OutError = TEXT("Release date cannot be in the past. Use ReleaseNow for an immediate release.");
        return false;
    }

    if (Command.Formats.Num() == 0)
    {
        OutError = TEXT("At least one release format must be selected.");
        return false;
    }

    for (ERecordFormat Format : Command.Formats)
    {
        if (!IsFormatEligible(Format, Command.ReleaseDate))
        {
            OutError = TEXT("One or more selected formats are not valid for the release date.");
            return false;
        }
    }

    if (Command.TargetRegionIds.Num() == 0)
    {
        OutError = TEXT("At least one target region must be selected.");
        return false;
    }

    const UMarketManagerSubsystem* MarketSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMarketManagerSubsystem>() : nullptr;
    if (!MarketSubsystem)
    {
        OutError = TEXT("Market data is unavailable.");
        return false;
    }

    TSet<FString> UniqueRegions;
    for (const FString& RegionId : Command.TargetRegionIds)
    {
        FMarketRegion Region;
        if (RegionId.IsEmpty() || !MarketSubsystem->GetRegion(RegionId, Region))
        {
            OutError = TEXT("One or more selected regions are invalid.");
            return false;
        }
        if (UniqueRegions.Contains(RegionId))
        {
            OutError = TEXT("Duplicate target regions are not allowed.");
            return false;
        }
        UniqueRegions.Add(RegionId);
    }

    return true;
}

FRecordRecordingIntent URecordManagerSubsystem::NormalizeRecordingIntent(const FRecordRecordingIntent& Intent) const
{
    FRecordRecordingIntent Normalized = Intent;
    if (Intent.bIsSingle)
    {
        Normalized.RecordType = ERecordType::Single;
    }
    else if (Intent.bIsLP)
    {
        Normalized.RecordType = ERecordType::LP;
    }
    Normalized.bIsSingle = Normalized.RecordType == ERecordType::Single;
    Normalized.bIsLP = Normalized.RecordType == ERecordType::LP;
    return Normalized;
}

bool URecordManagerSubsystem::IsSongCountValidForType(ERecordType RecordType, int32 SongCount, FString& OutError) const
{
    if (SongCount <= 0)
    {
        OutError = TEXT("At least one song must be selected.");
        return false;
    }

    switch (RecordType)
    {
    case ERecordType::Single:
        if (SongCount != 1)
        {
            OutError = TEXT("Singles must contain exactly one track.");
            return false;
        }
        return true;
    case ERecordType::EP:
        if (SongCount < 2 || SongCount > 5)
        {
            OutError = TEXT("EPs must contain 2 to 5 tracks.");
            return false;
        }
        return true;
    case ERecordType::LP:
        if (SongCount < 6 || SongCount > 14)
        {
            OutError = TEXT("LPs must contain 6 to 14 tracks.");
            return false;
        }
        return true;
    default:
        OutError = TEXT("Invalid record type.");
        return false;
    }
}

float URecordManagerSubsystem::EstimateRecordingCost(const FRecordRecordingIntent& Intent) const
{
    const FRecordRecordingIntent NormalizedIntent = NormalizeRecordingIntent(Intent);
    const float BaseCost = NormalizedIntent.RecordType == ERecordType::Single ? 2500.f
        : NormalizedIntent.RecordType == ERecordType::EP ? 6500.f
        : 14000.f;
    const float PerSongCost = NormalizedIntent.RecordType == ERecordType::Single ? 1000.f
        : NormalizedIntent.RecordType == ERecordType::EP ? 1250.f
        : 1500.f;

    float EraMultiplier = 1.f;
    if (const UGameTimeSubsystem* TimeSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGameTimeSubsystem>() : nullptr)
    {
        const int32 Year = TimeSubsystem->GetCurrentGameDate().GetYear();
        EraMultiplier = FMath::Clamp(0.75f + static_cast<float>(Year - 1955) * 0.0125f, 0.75f, 1.75f);
    }

    float ArtistMultiplier = 1.f;
    if (const UArtistManagerSubsystem* ArtistSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UArtistManagerSubsystem>() : nullptr)
    {
        if (const FArtistContract* Contract = ArtistSubsystem->GetContractByArtistId(NormalizedIntent.ArtistId))
        {
            const float Quality = FMath::Clamp(
                (Contract->ArtistData.PerformanceScore + Contract->ArtistData.VocalQuality + Contract->ArtistData.SongwritingQuality) / 300.f,
                0.f,
                1.f);
            ArtistMultiplier = FMath::Lerp(0.9f, 1.35f, Quality);
        }
    }

    return FMath::RoundToFloat((BaseCost + PerSongCost * NormalizedIntent.SongIds.Num()) * EraMultiplier * ArtistMultiplier);
}

int32 URecordManagerSubsystem::EstimateRecordingDurationDays(const FRecordRecordingIntent& Intent) const
{
    const FRecordRecordingIntent NormalizedIntent = NormalizeRecordingIntent(Intent);
    const int32 BaseDays = NormalizedIntent.RecordType == ERecordType::Single ? 7
        : NormalizedIntent.RecordType == ERecordType::EP ? 21
        : 42;
    const int32 PerSongDays = NormalizedIntent.RecordType == ERecordType::Single ? 3
        : NormalizedIntent.RecordType == ERecordType::EP ? 4
        : 5;
    return FMath::Clamp(BaseDays + PerSongDays * NormalizedIntent.SongIds.Num(), 7, 140);
}

FString URecordManagerSubsystem::GetRecordTypeDisplayName(ERecordType RecordType) const
{
    return RecordTypeName(RecordType);
}

void URecordManagerSubsystem::ProcessScheduledReleases(const FDateTime& CurrentDate)
{
    for (TPair<FString, FRecordData>& Pair : Records)
    {
        ERecordLifecycleState& State = RecordStates.FindOrAdd(Pair.Key);
        if (State == ERecordLifecycleState::Scheduled && Pair.Value.ReleaseDate <= CurrentDate)
        {
            State = ERecordLifecycleState::Released;
        }
    }
}

bool URecordManagerSubsystem::IsRegionTargetedByRecord(const FRecordData& Record, const FString& RegionId) const
{
    return Record.TargetRegionIds.Contains(RegionId);
}

FString URecordManagerSubsystem::ResolveLabelForArtist(const FString& ArtistId) const
{
    if (const UGameInstance* GameInstance = GetGameInstance())
    {
        if (const UPlayerLabelSubsystem* LabelSubsystem = GameInstance->GetSubsystem<UPlayerLabelSubsystem>())
        {
            return LabelSubsystem->GetPlayerLabelId();
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("RecordManagerSubsystem: PlayerLabelSubsystem unavailable while resolving label for artist %s."), *ArtistId);
    return TEXT("label_player");
}

FString URecordManagerSubsystem::ResolveArtistDisplayName(const FString& ArtistId) const
{
    if (ArtistId.IsEmpty())
    {
        return FString();
    }

    if (const UGameInstance* GameInstance = GetGameInstance())
    {
        if (const UArtistManagerSubsystem* ArtistSubsystem = GameInstance->GetSubsystem<UArtistManagerSubsystem>())
        {
            if (const FArtistContract* Contract = ArtistSubsystem->GetContractByArtistId(ArtistId))
            {
                if (!Contract->ArtistData.ArtistName.IsEmpty())
                {
                    return Contract->ArtistData.ArtistName;
                }
                if (!Contract->ArtistId.IsEmpty())
                {
                    return Contract->ArtistId;
                }
            }
        }
    }

    return ArtistId;
}

FReleaseDashboardItem URecordManagerSubsystem::BuildReleaseDashboardItem(const FRecordData& Record) const
{
    FReleaseDashboardItem Item;
    Item.RecordId = Record.RecordId;
    Item.RecordDisplayName = Record.AlbumName.IsEmpty() ? Record.RecordId : Record.AlbumName;
    Item.ArtistId = Record.ArtistId;
    Item.ArtistDisplayName = ResolveArtistDisplayName(Record.ArtistId);
    Item.ReleaseDate = Record.ReleaseDate;
    Item.LifecycleState = ResolveLifecycleStateForRecord(Record.RecordId);
    Item.TargetRegionIds = Record.TargetRegionIds;
    Item.Formats = Record.Formats;
    return Item;
}

void URecordManagerSubsystem::BuildSaveSnapshot(FRecordManagerSnapshot& OutSnapshot) const
{
    OutSnapshot.Records.Reset();
    Records.GenerateValueArray(OutSnapshot.Records);
    OutSnapshot.SalesHistory = SalesHistory;
    OutSnapshot.LifetimeUnits = LifetimeUnits;
    OutSnapshot.RecordStates = RecordStates;
    OutSnapshot.ActiveRecordingSessions.Reset();
    for (const TPair<FString, FRecordRecordingIntent>& Pair : ActiveRecordingIntents)
    {
        FActiveRecordingSession Session;
        Session.RecordingId = Pair.Key;
        Session.Intent = Pair.Value;
        Session.StartDate = RecordingStartDates.FindRef(Pair.Key);
        Session.CompletionDate = RecordingCompletionDates.FindRef(Pair.Key);
        Session.RecordingCost = RecordingCosts.FindRef(Pair.Key);
        OutSnapshot.ActiveRecordingSessions.Add(Session);
    }
}

void URecordManagerSubsystem::ValidateSaveSnapshot(const FRecordManagerSnapshot& Snapshot, const TSet<FString>& KnownArtistIds, const TSet<FString>& KnownSongIds, const TSet<FString>& KnownLabelIds, FMusicSaveValidationResult& Result) const
{
    TSet<FString> SeenRecordIds;
    for (const FRecordData& Record : Snapshot.Records)
    {
        if (Record.RecordId.IsEmpty())
        {
            Result.AddError(TEXT("Saved record has an empty record id."));
            continue;
        }

        if (SeenRecordIds.Contains(Record.RecordId))
        {
            Result.AddError(FString::Printf(TEXT("Duplicate saved record id: %s."), *Record.RecordId));
        }
        SeenRecordIds.Add(Record.RecordId);

        if (Record.ArtistId.IsEmpty() || !KnownArtistIds.Contains(Record.ArtistId))
        {
            Result.AddError(FString::Printf(TEXT("Record %s references missing artist %s."), *Record.RecordId, *Record.ArtistId));
        }

        if (Record.LabelId.IsEmpty() || !KnownLabelIds.Contains(Record.LabelId))
        {
            Result.AddError(FString::Printf(TEXT("Record %s references missing label %s."), *Record.RecordId, *Record.LabelId));
        }

        TSet<FString> RecordSongIds;
        for (const FString& SongId : Record.SongIds)
        {
            if (SongId.IsEmpty())
            {
                Result.AddError(FString::Printf(TEXT("Record %s contains an empty song id."), *Record.RecordId));
            }
            else if (!KnownSongIds.Contains(SongId))
            {
                Result.AddError(FString::Printf(TEXT("Record %s references missing song %s."), *Record.RecordId, *SongId));
            }

            if (RecordSongIds.Contains(SongId))
            {
                Result.AddError(FString::Printf(TEXT("Record %s contains duplicate song %s."), *Record.RecordId, *SongId));
            }
            RecordSongIds.Add(SongId);
        }

        if (Record.DateRecorded.GetTicks() > 0 && Record.ReleaseDate.GetTicks() > 0 && Record.ReleaseDate < Record.DateRecorded)
        {
            Result.AddError(FString::Printf(TEXT("Record %s has release date before recorded date."), *Record.RecordId));
        }

        if ((Record.ReleaseDate.GetTicks() > 0 || Snapshot.RecordStates.FindRef(Record.RecordId) == ERecordLifecycleState::Scheduled || Snapshot.RecordStates.FindRef(Record.RecordId) == ERecordLifecycleState::Released)
            && Record.TargetRegionIds.Num() == 0)
        {
            Result.AddWarning(FString::Printf(TEXT("Record %s has no target regions and will not sell until scheduled with regions."), *Record.RecordId));
        }

        for (ERecordFormat Format : Record.Formats)
        {
            if (!StaticEnum<ERecordFormat>()->IsValidEnumValue(static_cast<int64>(Format)))
            {
                Result.AddError(FString::Printf(TEXT("Record %s contains invalid format value."), *Record.RecordId));
            }
        }
    }

    for (const TPair<FString, ERecordLifecycleState>& StatePair : Snapshot.RecordStates)
    {
        if (!SeenRecordIds.Contains(StatePair.Key))
        {
            Result.AddError(FString::Printf(TEXT("Record lifecycle state references missing record %s."), *StatePair.Key));
        }
        if (!StaticEnum<ERecordLifecycleState>()->IsValidEnumValue(static_cast<int64>(StatePair.Value)))
        {
            Result.AddError(FString::Printf(TEXT("Record %s contains invalid lifecycle state."), *StatePair.Key));
        }
    }

    TSet<FString> SeenRecordingIds;
    for (const FActiveRecordingSession& Session : Snapshot.ActiveRecordingSessions)
    {
        if (Session.RecordingId.IsEmpty())
        {
            Result.AddError(TEXT("Active recording session has an empty id."));
            continue;
        }
        if (SeenRecordingIds.Contains(Session.RecordingId))
        {
            Result.AddError(FString::Printf(TEXT("Duplicate active recording session id: %s."), *Session.RecordingId));
        }
        SeenRecordingIds.Add(Session.RecordingId);

        if (Session.Intent.ArtistId.IsEmpty() || !KnownArtistIds.Contains(Session.Intent.ArtistId))
        {
            Result.AddError(FString::Printf(TEXT("Active recording %s references missing artist %s."), *Session.RecordingId, *Session.Intent.ArtistId));
        }
        if (Session.StartDate.GetTicks() <= 0 || Session.CompletionDate.GetTicks() <= 0 || Session.CompletionDate < Session.StartDate)
        {
            Result.AddError(FString::Printf(TEXT("Active recording %s has invalid dates."), *Session.RecordingId));
        }
        if (Session.RecordingCost < 0.f || !FMath::IsFinite(Session.RecordingCost))
        {
            Result.AddError(FString::Printf(TEXT("Active recording %s has invalid cost."), *Session.RecordingId));
        }
        for (const FString& SongId : Session.Intent.SongIds)
        {
            if (SongId.IsEmpty() || !KnownSongIds.Contains(SongId))
            {
                Result.AddError(FString::Printf(TEXT("Active recording %s references missing song %s."), *Session.RecordingId, *SongId));
            }
        }
    }

    for (const TPair<FString, FRecordSalesHistory>& Pair : Snapshot.SalesHistory)
    {
        if (!SeenRecordIds.Contains(Pair.Key))
        {
            Result.AddError(FString::Printf(TEXT("Sales history references missing record %s."), *Pair.Key));
        }

        int32 SummedUnits = 0;
        for (const FRecordSalesEntry& Entry : Pair.Value.Entries)
        {
            if (Entry.RecordId != Pair.Key)
            {
                Result.AddError(FString::Printf(TEXT("Sales history key %s contains entry for record %s."), *Pair.Key, *Entry.RecordId));
            }
            if (Entry.MarketId.IsEmpty())
            {
                Result.AddError(FString::Printf(TEXT("Sales entry for record %s has empty market id."), *Entry.RecordId));
            }
            if (Entry.UnitsSold < 0)
            {
                Result.AddError(FString::Printf(TEXT("Sales entry for record %s has negative units."), *Entry.RecordId));
            }
            if (Entry.Month.GetTicks() <= 0)
            {
                Result.AddError(FString::Printf(TEXT("Sales entry for record %s has unset month."), *Entry.RecordId));
            }
            SummedUnits += FMath::Max(0, Entry.UnitsSold);
        }

        const int32 Lifetime = Snapshot.LifetimeUnits.FindRef(Pair.Key);
        if (Lifetime < SummedUnits)
        {
            Result.AddError(FString::Printf(TEXT("Lifetime units for record %s are less than summed sales history."), *Pair.Key));
        }
    }
}

void URecordManagerSubsystem::ApplySaveSnapshot(const FRecordManagerSnapshot& Snapshot)
{
    Records.Reset();
    for (const FRecordData& Record : Snapshot.Records)
    {
        Records.Add(Record.RecordId, Record);
    }

    SalesHistory = Snapshot.SalesHistory;
    LifetimeUnits = Snapshot.LifetimeUnits;
    RecordStates = Snapshot.RecordStates;
    ActiveRecordingIntents.Reset();
    RecordingStartDates.Reset();
    RecordingCompletionDates.Reset();
    RecordingCosts.Reset();

    TArray<FString> SongsToLock;
    for (const FActiveRecordingSession& Session : Snapshot.ActiveRecordingSessions)
    {
        if (Session.RecordingId.IsEmpty())
        {
            continue;
        }
        ActiveRecordingIntents.Add(Session.RecordingId, NormalizeRecordingIntent(Session.Intent));
        RecordingStartDates.Add(Session.RecordingId, Session.StartDate);
        RecordingCompletionDates.Add(Session.RecordingId, Session.CompletionDate);
        RecordingCosts.Add(Session.RecordingId, Session.RecordingCost);
        SongsToLock.Append(Session.Intent.SongIds);
    }

    if (SongsToLock.Num() > 0)
    {
        if (USongManagerSubsystem* SongSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<USongManagerSubsystem>() : nullptr)
        {
            FString LockError;
            SongSubsystem->LockSongsForRecording(SongsToLock, LockError);
            if (!LockError.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("RecordManagerSubsystem: failed to relock songs after load: %s"), *LockError);
            }
        }
    }
}
