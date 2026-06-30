#include "ChartManagerSubsystem.h"

#include "ArtistManagerSubsystem.h"
#include "EventSubsystem.h"
#include "MarketManagerSubsystem.h"
#include "MusicSaveGame.h"
#include "PlayerLabelSubsystem.h"
#include "RecordManagerSubsystem.h"
#include "Engine/GameInstance.h"

DEFINE_LOG_CATEGORY(LogMusicCharts);

namespace
{
    constexpr int32 MaxChartEntries = 100;
    constexpr int32 MaxSpecialtyChartEntries = 50;

    int32 CompareEntries(const FChartEntry& A, const FChartEntry& B)
    {
        if (!FMath::IsNearlyEqual(A.ChartPoints, B.ChartPoints))
        {
            return A.ChartPoints > B.ChartPoints ? -1 : 1;
        }
        if (A.Units != B.Units)
        {
            return A.Units > B.Units ? -1 : 1;
        }
        return A.RecordId.Compare(B.RecordId);
    }

    FString SanitizeChartToken(const FString& Value)
    {
        FString Token = Value.ToLower();
        Token.ReplaceInline(TEXT(" "), TEXT("_"));
        Token.ReplaceInline(TEXT("-"), TEXT("_"));
        Token.ReplaceInline(TEXT("/"), TEXT("_"));
        return Token;
    }

    FString FormatToken(ERecordFormat Format)
    {
        switch (Format)
        {
        case ERecordFormat::Vinyl:
            return TEXT("vinyl");
        case ERecordFormat::Cassette:
            return TEXT("cassette");
        case ERecordFormat::CD:
            return TEXT("cd");
        case ERecordFormat::DigitalDownload:
            return TEXT("digital");
        case ERecordFormat::Streaming:
            return TEXT("streaming");
        default:
            return TEXT("format");
        }
    }

    FString FormatDisplayName(ERecordFormat Format)
    {
        switch (Format)
        {
        case ERecordFormat::Vinyl:
            return TEXT("Vinyl");
        case ERecordFormat::Cassette:
            return TEXT("Cassette");
        case ERecordFormat::CD:
            return TEXT("CD");
        case ERecordFormat::DigitalDownload:
            return TEXT("Digital");
        case ERecordFormat::Streaming:
            return TEXT("Streaming");
        default:
            return TEXT("Format");
        }
    }

    FChartDefinition BuildChartDefinition(
        const FString& ChartId,
        const FString& DisplayName,
        EChartScope Scope,
        const FString& RegionId,
        EChartRecordType RecordType,
        EChartFormulaProfile FormulaProfile,
        int32 MaxEntries)
    {
        FChartDefinition Definition;
        Definition.ChartId = ChartId;
        Definition.DisplayName = FText::FromString(DisplayName);
        Definition.Scope = Scope;
        Definition.RegionId = RegionId;
        Definition.RecordType = RecordType;
        Definition.FormulaProfile = FormulaProfile;
        Definition.MaxEntries = MaxEntries;
        return Definition;
    }

    void AddDefinition(TMap<FString, FChartDefinition>& Definitions, const FChartDefinition& Definition)
    {
        if (!Definition.ChartId.IsEmpty())
        {
            Definitions.Add(Definition.ChartId, Definition);
        }
    }
}

void UChartManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    RefreshChartDefinitions();
}

void UChartManagerSubsystem::GetChartDefinitions(TArray<FChartDefinition>& OutDefinitions) const
{
    OutDefinitions.Reset();
    ChartDefinitions.GenerateValueArray(OutDefinitions);
    OutDefinitions.Sort([](const FChartDefinition& A, const FChartDefinition& B)
    {
        return A.ChartId < B.ChartId;
    });
}

bool UChartManagerSubsystem::GetCurrentChart(const FString& ChartId, FWeeklyChartSnapshot& OutSnapshot) const
{
    if (const FString* SnapshotKey = CurrentSnapshotKeyByChartId.Find(ChartId))
    {
        if (const FWeeklyChartSnapshot* Snapshot = WeeklySnapshots.Find(*SnapshotKey))
        {
            OutSnapshot = *Snapshot;
            return true;
        }
    }

    return false;
}

bool UChartManagerSubsystem::GetChartByWeek(const FString& ChartId, const FDateTime& WeekStart, FWeeklyChartSnapshot& OutSnapshot) const
{
    if (const FWeeklyChartSnapshot* Snapshot = WeeklySnapshots.Find(BuildChartWeekKey(ChartId, WeekStart)))
    {
        OutSnapshot = *Snapshot;
        return true;
    }

    return false;
}

bool UChartManagerSubsystem::GetRecordChartHistory(const FString& RecordId, FRecordChartHistory& OutHistory) const
{
    if (const FRecordChartHistory* History = RecordHistory.Find(RecordId))
    {
        OutHistory = *History;
        return true;
    }

    return false;
}

bool UChartManagerSubsystem::GetTopRecord(const FString& ChartId, FChartEntry& OutEntry) const
{
    FWeeklyChartSnapshot Snapshot;
    if (!GetCurrentChart(ChartId, Snapshot) || Snapshot.Entries.Num() == 0)
    {
        return false;
    }

    OutEntry = Snapshot.Entries[0];
    return true;
}

bool UChartManagerSubsystem::BuildChartListView(const FString& ChartId, FChartListView& OutView) const
{
    OutView = FChartListView();
    GetChartDefinitions(OutView.ChartDefinitions);
    OutView.bHasCharts = OutView.ChartDefinitions.Num() > 0;

    if (!OutView.bHasCharts)
    {
        OutView.EmptyStateMessage = FText::FromString(TEXT("No chart definitions are available."));
        return true;
    }

    FString SelectedChartId = ChartId;
    if (SelectedChartId.IsEmpty())
    {
        SelectedChartId = OutView.ChartDefinitions[0].ChartId;
    }
    OutView.SelectedChartId = SelectedChartId;

    FWeeklyChartSnapshot Snapshot;
    if (!GetCurrentChart(SelectedChartId, Snapshot))
    {
        OutView.EmptyStateMessage = FText::FromString(TEXT("No chart has been generated for this chart yet."));
        return true;
    }

    OutView.SelectedWeekStart = Snapshot.WeekStart;
    for (const FChartEntry& Entry : Snapshot.Entries)
    {
        OutView.Entries.Add(BuildEntryView(Entry));
    }

    if (OutView.Entries.Num() == 0)
    {
        OutView.EmptyStateMessage = FText::FromString(TEXT("No records have enough real sales data to chart this week."));
    }

    return true;
}

bool UChartManagerSubsystem::BuildRecordChartHistoryView(const FString& RecordId, FRecordChartHistoryView& OutView) const
{
    OutView = FRecordChartHistoryView();
    OutView.RecordId = RecordId;

    if (const UGameInstance* GameInstance = GetGameInstance())
    {
        if (const URecordManagerSubsystem* RecordSubsystem = GameInstance->GetSubsystem<URecordManagerSubsystem>())
        {
            FRecordData Record;
            if (RecordSubsystem->GetRecordById(RecordId, Record))
            {
                OutView.RecordTitle = FText::FromString(Record.AlbumName);
            }
        }
    }

    FRecordChartHistory History;
    if (!GetRecordChartHistory(RecordId, History))
    {
        OutView.EmptyStateMessage = FText::FromString(TEXT("This record has not charted yet."));
        return true;
    }

    OutView.bHasHistory = History.Entries.Num() > 0;
    for (const FChartEntry& Entry : History.Entries)
    {
        OutView.Entries.Add(BuildEntryView(Entry));
    }

    if (!OutView.bHasHistory)
    {
        OutView.EmptyStateMessage = FText::FromString(TEXT("This record has not charted yet."));
    }

    return true;
}

bool UChartManagerSubsystem::BuildChartDashboardView(FChartDashboardView& OutView) const
{
    OutView = FChartDashboardView();

    TArray<FWeeklyChartSnapshot> CurrentSnapshots;
    for (const TPair<FString, FString>& Pair : CurrentSnapshotKeyByChartId)
    {
        if (const FWeeklyChartSnapshot* Snapshot = WeeklySnapshots.Find(Pair.Value))
        {
            if (Snapshot->Entries.Num() > 0)
            {
                CurrentSnapshots.Add(*Snapshot);
            }
        }
    }

    if (CurrentSnapshots.Num() == 0)
    {
        OutView.StatusMessage = FText::FromString(TEXT("No chart week has been resolved yet."));
        return true;
    }

    OutView.bHasChartData = true;
    OutView.StatusMessage = FText::FromString(TEXT("Charts are current."));

    auto BuildHighlight = [this](const FString& ChartId, const FChartEntry& Entry)
    {
        FChartDashboardHighlight Highlight;
        Highlight.bHasEntry = true;
        Highlight.ChartId = ChartId;
        Highlight.ChartName = GetChartDisplayName(ChartId);
        Highlight.Entry = BuildEntryView(Entry);
        return Highlight;
    };

    FChartEntry GlobalNumberOne;
    if (GetTopRecord(TEXT("global_records"), GlobalNumberOne))
    {
        OutView.CurrentNumberOne = BuildHighlight(TEXT("global_records"), GlobalNumberOne);
    }
    else
    {
        CurrentSnapshots.Sort([](const FWeeklyChartSnapshot& A, const FWeeklyChartSnapshot& B)
        {
            return A.ChartId < B.ChartId;
        });
        OutView.CurrentNumberOne = BuildHighlight(CurrentSnapshots[0].ChartId, CurrentSnapshots[0].Entries[0]);
    }

    int32 BestPlayerRank = MAX_int32;
    int32 BestMovement = MIN_int32;
    for (const FWeeklyChartSnapshot& Snapshot : CurrentSnapshots)
    {
        for (const FChartEntry& Entry : Snapshot.Entries)
        {
            const FChartEntryView View = BuildEntryView(Entry);
            if (!View.bPlayerOwned)
            {
                continue;
            }

            if (Entry.Rank > 0 && Entry.Rank < BestPlayerRank)
            {
                BestPlayerRank = Entry.Rank;
                OutView.TopPlayerOwnedRelease = BuildHighlight(Snapshot.ChartId, Entry);
            }

            if (View.RankMovement > BestMovement)
            {
                BestMovement = View.RankMovement;
                OutView.BiggestPlayerOwnedMovement = BuildHighlight(Snapshot.ChartId, Entry);
            }

            if ((Entry.PreviousRank == 0 || (Entry.PeakRank == Entry.Rank && Entry.PreviousRank > Entry.Rank) || Entry.Rank <= 10)
                && OutView.RecentMilestones.Num() < 5)
            {
                OutView.RecentMilestones.Add(BuildHighlight(Snapshot.ChartId, Entry));
            }
        }
    }

    return true;
}

void UChartManagerSubsystem::ResolveWeeklyCharts(const FDateTime& NewDate)
{
    check(IsInGameThread());

    RefreshChartDefinitions();
    const FDateTime WeekStart = GetWeekStart(NewDate);

    for (const TPair<FString, FChartDefinition>& Pair : ChartDefinitions)
    {
        const FString ChartWeekKey = BuildChartWeekKey(Pair.Key, WeekStart);
        if (ProcessedChartWeekKeys.Contains(ChartWeekKey))
        {
            continue;
        }

        FWeeklyChartSnapshot Snapshot = CalculateChart(Pair.Value, WeekStart, NewDate);
        WeeklySnapshots.Add(ChartWeekKey, Snapshot);
        CurrentSnapshotKeyByChartId.Add(Pair.Key, ChartWeekKey);
        ProcessedChartWeekKeys.Add(ChartWeekKey);
        UpdateRecordHistory(Snapshot);
        ProcessMilestones(Snapshot);

        UE_LOG(LogMusicCharts, Verbose, TEXT("Chart resolved: %s week %s entries=%d"), *Pair.Key, *WeekStart.ToString(), Snapshot.Entries.Num());
    }
}

void UChartManagerSubsystem::RefreshChartDefinitions()
{
    ChartDefinitions.Reset();

    AddDefinition(ChartDefinitions, BuildChartDefinition(TEXT("global_records"), TEXT("Global Records"), EChartScope::Global, TEXT(""), EChartRecordType::AllRecords, EChartFormulaProfile::EraDefault, MaxChartEntries));
    AddDefinition(ChartDefinitions, BuildChartDefinition(TEXT("global_singles"), TEXT("Global Singles"), EChartScope::Global, TEXT(""), EChartRecordType::Singles, EChartFormulaProfile::SinglesVelocity, MaxChartEntries));
    AddDefinition(ChartDefinitions, BuildChartDefinition(TEXT("global_albums"), TEXT("Global Albums"), EChartScope::Global, TEXT(""), EChartRecordType::Albums, EChartFormulaProfile::AlbumLongevity, MaxChartEntries));

    for (ERecordFormat Format : { ERecordFormat::Vinyl, ERecordFormat::Cassette, ERecordFormat::CD, ERecordFormat::DigitalDownload, ERecordFormat::Streaming })
    {
        FChartDefinition FormatChart = BuildChartDefinition(
            FString::Printf(TEXT("global_%s_records"), *FormatToken(Format)),
            FString::Printf(TEXT("Global %s Records"), *FormatDisplayName(Format)),
            EChartScope::Global,
            TEXT(""),
            EChartRecordType::AllRecords,
            Format == ERecordFormat::Streaming ? EChartFormulaProfile::StreamingEra : EChartFormulaProfile::FormatWeighted,
            MaxSpecialtyChartEntries);
        FormatChart.bFilterByFormat = true;
        FormatChart.FormatFilter = Format;
        AddDefinition(ChartDefinitions, FormatChart);
    }

    if (const UGameInstance* GameInstance = GetGameInstance())
    {
        if (const URecordManagerSubsystem* RecordSubsystem = GameInstance->GetSubsystem<URecordManagerSubsystem>())
        {
            TArray<FRecordData> Records;
            RecordSubsystem->GetAllRecords(Records);
            TSet<FString> Genres;
            for (const FRecordData& Record : Records)
            {
                if (!Record.PrimaryGenre.IsEmpty())
                {
                    Genres.Add(Record.PrimaryGenre);
                }
            }

            for (const FString& Genre : Genres)
            {
                FChartDefinition GenreChart = BuildChartDefinition(
                    FString::Printf(TEXT("global_genre_%s"), *SanitizeChartToken(Genre)),
                    FString::Printf(TEXT("Global %s Chart"), *Genre),
                    EChartScope::Global,
                    TEXT(""),
                    EChartRecordType::AllRecords,
                    EChartFormulaProfile::GenreSpecialist,
                    MaxSpecialtyChartEntries);
                GenreChart.GenreFilter = Genre;
                AddDefinition(ChartDefinitions, GenreChart);
            }
        }

        if (const UMarketManagerSubsystem* MarketSubsystem = GameInstance->GetSubsystem<UMarketManagerSubsystem>())
        {
            TArray<FMarketRegion> Regions;
            MarketSubsystem->GetAllRegions(Regions);
            for (const FMarketRegion& Region : Regions)
            {
                if (Region.RegionId.IsEmpty())
                {
                    continue;
                }

                AddDefinition(ChartDefinitions, BuildChartDefinition(
                    FString::Printf(TEXT("region_%s_records"), *Region.RegionId),
                    FString::Printf(TEXT("%s Records"), *Region.DisplayName),
                    EChartScope::Regional,
                    Region.RegionId,
                    EChartRecordType::AllRecords,
                    EChartFormulaProfile::EraDefault,
                    MaxChartEntries));
                AddDefinition(ChartDefinitions, BuildChartDefinition(
                    FString::Printf(TEXT("region_%s_singles"), *Region.RegionId),
                    FString::Printf(TEXT("%s Singles"), *Region.DisplayName),
                    EChartScope::Regional,
                    Region.RegionId,
                    EChartRecordType::Singles,
                    EChartFormulaProfile::SinglesVelocity,
                    MaxSpecialtyChartEntries));
                AddDefinition(ChartDefinitions, BuildChartDefinition(
                    FString::Printf(TEXT("region_%s_albums"), *Region.RegionId),
                    FString::Printf(TEXT("%s Albums"), *Region.DisplayName),
                    EChartScope::Regional,
                    Region.RegionId,
                    EChartRecordType::Albums,
                    EChartFormulaProfile::AlbumLongevity,
                    MaxSpecialtyChartEntries));
            }
        }
    }
}

FDateTime UChartManagerSubsystem::GetWeekStart(const FDateTime& Date) const
{
    const int32 DaysSinceMonday = static_cast<int32>(Date.GetDayOfWeek());
    return FDateTime(Date.GetYear(), Date.GetMonth(), Date.GetDay()) - FTimespan::FromDays(DaysSinceMonday);
}

FString UChartManagerSubsystem::BuildChartWeekKey(const FString& ChartId, const FDateTime& WeekStart) const
{
    return FString::Printf(TEXT("%s:%04d-%02d-%02d"), *ChartId, WeekStart.GetYear(), WeekStart.GetMonth(), WeekStart.GetDay());
}

FString UChartManagerSubsystem::BuildMilestoneKey(const FString& ChartId, const FString& RecordId, EChartMilestoneType MilestoneType, const FDateTime& WeekStart) const
{
    return FString::Printf(TEXT("%s:%s:%d:%04d-%02d-%02d"), *ChartId, *RecordId, static_cast<int32>(MilestoneType), WeekStart.GetYear(), WeekStart.GetMonth(), WeekStart.GetDay());
}

FWeeklyChartSnapshot UChartManagerSubsystem::CalculateChart(const FChartDefinition& Definition, const FDateTime& WeekStart, const FDateTime& GeneratedAt) const
{
    FWeeklyChartSnapshot Snapshot;
    Snapshot.ChartId = Definition.ChartId;
    Snapshot.WeekStart = WeekStart;
    Snapshot.GeneratedAt = GeneratedAt;

    const UGameInstance* GameInstance = GetGameInstance();
    const URecordManagerSubsystem* RecordSubsystem = GameInstance ? GameInstance->GetSubsystem<URecordManagerSubsystem>() : nullptr;
    if (!RecordSubsystem)
    {
        return Snapshot;
    }

    TArray<FRecordData> Records;
    RecordSubsystem->GetAllRecords(Records);

    TArray<FChartEntry> Entries;

    for (const FRecordData& Record : Records)
    {
        if (!DoesRecordMatchDefinition(Record, Definition))
        {
            continue;
        }

        TArray<FRecordSalesEntry> SalesHistory;
        if (!RecordSubsystem->GetSalesHistory(Record.RecordId, SalesHistory))
        {
            continue;
        }

        TArray<FRecordSalesEntry> WindowEntries;
        for (const FRecordSalesEntry& SalesEntry : SalesHistory)
        {
            if (Definition.Scope == EChartScope::Regional && SalesEntry.MarketId != Definition.RegionId)
            {
                continue;
            }

            if (Definition.bFilterByFormat && SalesEntry.Format != Definition.FormatFilter)
            {
                continue;
            }

            const int32 WeeklyUnits = CalculateWeeklyUnitsFromSalesEntry(SalesEntry, WeekStart);
            if (WeeklyUnits <= 0)
            {
                continue;
            }

            FRecordSalesEntry WeeklyEntry = SalesEntry;
            WeeklyEntry.Month = WeekStart;
            WeeklyEntry.UnitsSold = WeeklyUnits;
            WeeklyEntry.DemandScore = SalesEntry.UnitsSold > 0
                ? SalesEntry.DemandScore * (static_cast<float>(WeeklyUnits) / static_cast<float>(SalesEntry.UnitsSold))
                : 0.f;
            WindowEntries.Add(WeeklyEntry);
        }

        int32 Units = 0;
        int32 StreamEquivalentUnits = 0;
        const float Points = CalculateChartPoints(WindowEntries, Definition, WeekStart, Units, StreamEquivalentUnits);
        if (Points <= KINDA_SMALL_NUMBER)
        {
            continue;
        }

        FChartEntry Entry;
        Entry.ChartId = Definition.ChartId;
        Entry.RecordId = Record.RecordId;
        Entry.ArtistId = Record.ArtistId;
        Entry.WeekStart = WeekStart;
        Entry.ChartPoints = Points;
        Entry.Units = Units;
        Entry.StreamEquivalentUnits = StreamEquivalentUnits;
        Entries.Add(Entry);
    }

    Entries.Sort([](const FChartEntry& A, const FChartEntry& B)
    {
        return CompareEntries(A, B) < 0;
    });

    if (Entries.Num() > Definition.MaxEntries)
    {
        Entries.SetNum(Definition.MaxEntries);
    }

    FWeeklyChartSnapshot PreviousSnapshot;
    const bool bHasPrevious = GetCurrentChart(Definition.ChartId, PreviousSnapshot);
    TMap<FString, FChartEntry> PreviousByRecord;
    if (bHasPrevious)
    {
        for (const FChartEntry& PreviousEntry : PreviousSnapshot.Entries)
        {
            PreviousByRecord.Add(PreviousEntry.RecordId, PreviousEntry);
        }
    }

    for (int32 Index = 0; Index < Entries.Num(); ++Index)
    {
        FChartEntry& Entry = Entries[Index];
        Entry.Rank = Index + 1;

        const FChartEntry* PreviousEntry = PreviousByRecord.Find(Entry.RecordId);
        Entry.PreviousRank = PreviousEntry ? PreviousEntry->Rank : 0;
        Entry.WeeksOnChart = PreviousEntry ? PreviousEntry->WeeksOnChart + 1 : 1;
        Entry.PeakRank = PreviousEntry && PreviousEntry->PeakRank > 0
            ? FMath::Min(PreviousEntry->PeakRank, Entry.Rank)
            : Entry.Rank;
    }

    Snapshot.Entries = Entries;
    return Snapshot;
}

float UChartManagerSubsystem::CalculateChartPoints(const TArray<FRecordSalesEntry>& SalesEntries, const FChartDefinition& Definition, const FDateTime& WeekStart, int32& OutUnits, int32& OutStreamEquivalentUnits) const
{
    OutUnits = 0;
    OutStreamEquivalentUnits = 0;

    float Points = 0.f;
    for (const FRecordSalesEntry& Entry : SalesEntries)
    {
        if (Entry.UnitsSold <= 0)
        {
            continue;
        }

        const float FormulaWeight = CalculateFormulaWeight(Definition, Entry, WeekStart);
        if (Entry.Format == ERecordFormat::Streaming)
        {
            const int32 StreamEquivalentUnits = FMath::Max(1, Entry.UnitsSold / 150);
            OutStreamEquivalentUnits += StreamEquivalentUnits;
            Points += static_cast<float>(StreamEquivalentUnits) * FormulaWeight;
        }
        else
        {
            OutUnits += Entry.UnitsSold;
            Points += static_cast<float>(Entry.UnitsSold) * FormulaWeight;
        }

        Points += Entry.DemandScore * 0.01f * FormulaWeight;
    }

    return FMath::Max(0.f, Points);
}

int32 UChartManagerSubsystem::CalculateWeeklyUnitsFromSalesEntry(const FRecordSalesEntry& SalesEntry, const FDateTime& WeekStart)
{
    if (SalesEntry.UnitsSold <= 0)
    {
        return 0;
    }

    const FDateTime WeekBucketStart(WeekStart.GetYear(), WeekStart.GetMonth(), WeekStart.GetDay());
    const FDateTime WeekBucketEnd = WeekBucketStart + FTimespan::FromDays(7);
    const FDateTime MonthBucketStart(SalesEntry.Month.GetYear(), SalesEntry.Month.GetMonth(), 1);
    const FDateTime MonthBucketEnd = MonthBucketStart + FTimespan::FromDays(FDateTime::DaysInMonth(SalesEntry.Month.GetYear(), SalesEntry.Month.GetMonth()));

    const FDateTime OverlapStart = WeekBucketStart > MonthBucketStart ? WeekBucketStart : MonthBucketStart;
    const FDateTime OverlapEnd = WeekBucketEnd < MonthBucketEnd ? WeekBucketEnd : MonthBucketEnd;
    if (OverlapEnd <= OverlapStart)
    {
        return 0;
    }

    const double OverlapDays = (OverlapEnd - OverlapStart).GetTotalDays();
    const double MonthDays = (MonthBucketEnd - MonthBucketStart).GetTotalDays();
    if (MonthDays <= 0.0)
    {
        return 0;
    }

    return FMath::Max(0, FMath::RoundToInt(static_cast<float>(SalesEntry.UnitsSold * (OverlapDays / MonthDays))));
}

bool UChartManagerSubsystem::DoesRecordMatchDefinition(const FRecordData& Record, const FChartDefinition& Definition)
{
    switch (Definition.RecordType)
    {
    case EChartRecordType::Singles:
        if (Record.RecordType != ERecordType::Single && !Record.bIsSingle)
        {
            return false;
        }
        break;
    case EChartRecordType::Albums:
        if (Record.RecordType == ERecordType::Single || Record.bIsSingle)
        {
            return false;
        }
        break;
    case EChartRecordType::AllRecords:
    default:
        break;
    }

    if (!Definition.GenreFilter.IsEmpty() && !Record.PrimaryGenre.Equals(Definition.GenreFilter, ESearchCase::IgnoreCase))
    {
        return false;
    }

    if (Definition.bFilterByFormat && !Record.Formats.Contains(Definition.FormatFilter))
    {
        return false;
    }

    return true;
}

float UChartManagerSubsystem::CalculateFormulaWeight(const FChartDefinition& Definition, const FRecordSalesEntry& Entry, const FDateTime& WeekStart)
{
    EChartFormulaProfile Profile = Definition.FormulaProfile;
    if (Profile == EChartFormulaProfile::EraDefault)
    {
        if (WeekStart.GetYear() >= 2010)
        {
            Profile = EChartFormulaProfile::StreamingEra;
        }
        else if (WeekStart.GetYear() >= 2003)
        {
            Profile = EChartFormulaProfile::FormatWeighted;
        }
        else
        {
            Profile = EChartFormulaProfile::PhysicalSales;
        }
    }

    switch (Profile)
    {
    case EChartFormulaProfile::SinglesVelocity:
        return Entry.Format == ERecordFormat::Streaming || Entry.Format == ERecordFormat::DigitalDownload ? 1.15f : 1.05f;
    case EChartFormulaProfile::AlbumLongevity:
        return Entry.Format == ERecordFormat::Vinyl || Entry.Format == ERecordFormat::CD || Entry.Format == ERecordFormat::Cassette ? 1.10f : 0.95f;
    case EChartFormulaProfile::GenreSpecialist:
        return 1.08f;
    case EChartFormulaProfile::FormatWeighted:
        return Entry.Format == Definition.FormatFilter ? 1.25f : 1.0f;
    case EChartFormulaProfile::StreamingEra:
        return Entry.Format == ERecordFormat::Streaming ? 1.30f : 0.90f;
    case EChartFormulaProfile::PhysicalSales:
    default:
        return Entry.Format == ERecordFormat::Streaming ? 0.50f : 1.0f;
    }
}

void UChartManagerSubsystem::UpdateRecordHistory(const FWeeklyChartSnapshot& Snapshot)
{
    for (const FChartEntry& Entry : Snapshot.Entries)
    {
        FRecordChartHistory& History = RecordHistory.FindOrAdd(Entry.RecordId);
        History.RecordId = Entry.RecordId;
        History.Entries.RemoveAll([&Entry](const FChartEntry& Existing)
        {
            return Existing.ChartId == Entry.ChartId && Existing.WeekStart == Entry.WeekStart;
        });
        History.Entries.Add(Entry);
        History.Entries.Sort([](const FChartEntry& A, const FChartEntry& B)
        {
            if (A.WeekStart != B.WeekStart)
            {
                return A.WeekStart > B.WeekStart;
            }
            return A.ChartId < B.ChartId;
        });
    }
}

void UChartManagerSubsystem::ProcessMilestones(const FWeeklyChartSnapshot& Snapshot)
{
    for (const FChartEntry& Entry : Snapshot.Entries)
    {
        if (Entry.PreviousRank == 0)
        {
            EmitMilestoneNews(Entry, EChartMilestoneType::FirstEntry);
        }
        if (Entry.Rank <= 40)
        {
            EmitMilestoneNews(Entry, EChartMilestoneType::Top40);
        }
        if (Entry.Rank <= 10)
        {
            EmitMilestoneNews(Entry, EChartMilestoneType::Top10);
        }
        if (Entry.Rank == 1)
        {
            EmitMilestoneNews(Entry, EChartMilestoneType::NumberOne);
        }
        if (Entry.PeakRank == Entry.Rank && Entry.PreviousRank > 0 && Entry.Rank < Entry.PreviousRank)
        {
            EmitMilestoneNews(Entry, EChartMilestoneType::NewPeak);
        }
    }
}

void UChartManagerSubsystem::EmitMilestoneNews(const FChartEntry& Entry, EChartMilestoneType MilestoneType)
{
    const FString MilestoneKey = BuildMilestoneKey(Entry.ChartId, Entry.RecordId, MilestoneType, Entry.WeekStart);
    if (ProcessedMilestoneKeys.Contains(MilestoneKey))
    {
        return;
    }

    UEventSubsystem* EventSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEventSubsystem>() : nullptr;
    if (!EventSubsystem)
    {
        return;
    }

    FMusicNewsEvent Event;
    Event.NewsId = FGuid::NewGuid();
    Event.Timestamp = Entry.WeekStart;
    Event.NewsType = EMusicNewsType::ChartAchievement;
    Event.SourceName = Entry.ArtistId;
    Event.SubjectName = Entry.RecordId;
    Event.Metadata.Add(TEXT("ChartId"), Entry.ChartId);
    Event.Metadata.Add(TEXT("RecordId"), Entry.RecordId);
    Event.Metadata.Add(TEXT("ArtistId"), Entry.ArtistId);
    Event.Metadata.Add(TEXT("Rank"), FString::FromInt(Entry.Rank));
    Event.Metadata.Add(TEXT("MilestoneType"), FString::FromInt(static_cast<int32>(MilestoneType)));
    Event.Tags = { TEXT("Charts"), Entry.ChartId, Entry.RecordId };

    switch (MilestoneType)
    {
    case EChartMilestoneType::FirstEntry:
        Event.Headline = TEXT("New Record Enters The Charts");
        Event.BodyText = FString::Printf(TEXT("Record %s entered %s at #%d."), *Entry.RecordId, *Entry.ChartId, Entry.Rank);
        break;
    case EChartMilestoneType::Top40:
        Event.Headline = TEXT("Record Reaches Top 40");
        Event.BodyText = FString::Printf(TEXT("Record %s reached the Top 40 on %s."), *Entry.RecordId, *Entry.ChartId);
        break;
    case EChartMilestoneType::Top10:
        Event.Headline = TEXT("Record Breaks Into Top 10");
        Event.BodyText = FString::Printf(TEXT("Record %s reached the Top 10 on %s."), *Entry.RecordId, *Entry.ChartId);
        break;
    case EChartMilestoneType::NumberOne:
        Event.Headline = TEXT("Record Hits Number One");
        Event.BodyText = FString::Printf(TEXT("Record %s is #1 on %s."), *Entry.RecordId, *Entry.ChartId);
        break;
    case EChartMilestoneType::NewPeak:
        Event.Headline = TEXT("Record Sets New Chart Peak");
        Event.BodyText = FString::Printf(TEXT("Record %s climbed to a new peak of #%d on %s."), *Entry.RecordId, Entry.Rank, *Entry.ChartId);
        break;
    }

    if (EventSubsystem->PublishNewsEvent(Event, FString::Printf(TEXT("ChartMilestone:%s"), *MilestoneKey)))
    {
        ProcessedMilestoneKeys.Add(MilestoneKey);
    }
}

FChartEntryView UChartManagerSubsystem::BuildEntryView(const FChartEntry& Entry) const
{
    FChartEntryView View;
    View.Rank = Entry.Rank;
    View.RankMovement = Entry.PreviousRank > 0 ? Entry.PreviousRank - Entry.Rank : 0;
    View.RecordId = Entry.RecordId;
    View.ArtistId = Entry.ArtistId;
    View.ChartPoints = Entry.ChartPoints;
    View.Units = Entry.Units + Entry.StreamEquivalentUnits;
    View.PeakRank = Entry.PeakRank;
    View.WeeksOnChart = Entry.WeeksOnChart;

    const UGameInstance* GameInstance = GetGameInstance();
    const URecordManagerSubsystem* RecordSubsystem = GameInstance ? GameInstance->GetSubsystem<URecordManagerSubsystem>() : nullptr;
    const UArtistManagerSubsystem* ArtistSubsystem = GameInstance ? GameInstance->GetSubsystem<UArtistManagerSubsystem>() : nullptr;
    const UPlayerLabelSubsystem* LabelSubsystem = GameInstance ? GameInstance->GetSubsystem<UPlayerLabelSubsystem>() : nullptr;

    if (RecordSubsystem)
    {
        FRecordData Record;
        if (RecordSubsystem->GetRecordById(Entry.RecordId, Record))
        {
            View.RecordTitle = FText::FromString(Record.AlbumName);
            View.bPlayerOwned = LabelSubsystem && Record.LabelId == LabelSubsystem->GetPlayerLabelId();
        }
        else
        {
            View.RecordTitle = FText::FromString(Entry.RecordId);
        }
    }

    View.ArtistName = FText::FromString(Entry.ArtistId);
    if (ArtistSubsystem)
    {
        if (const FArtistContract* Contract = ArtistSubsystem->GetContractByArtistId(Entry.ArtistId))
        {
            View.ArtistName = FText::FromString(Contract->ArtistData.ArtistName);
        }
        else if (const FArtistContract* ContractByName = ArtistSubsystem->FindContractByArtistName(Entry.ArtistId))
        {
            View.ArtistName = FText::FromString(ContractByName->ArtistData.ArtistName);
        }
    }

    return View;
}

FText UChartManagerSubsystem::GetChartDisplayName(const FString& ChartId) const
{
    if (const FChartDefinition* Definition = ChartDefinitions.Find(ChartId))
    {
        return Definition->DisplayName;
    }

    return FText::FromString(ChartId);
}

void UChartManagerSubsystem::BuildSaveSnapshot(FChartSnapshot& OutSnapshot) const
{
    OutSnapshot.ChartDefinitions.Reset();
    ChartDefinitions.GenerateValueArray(OutSnapshot.ChartDefinitions);
    WeeklySnapshots.GenerateValueArray(OutSnapshot.WeeklySnapshots);
    OutSnapshot.RecordHistory = RecordHistory;
    OutSnapshot.CurrentSnapshotKeyByChartId = CurrentSnapshotKeyByChartId;
    OutSnapshot.ProcessedChartWeekKeys = ProcessedChartWeekKeys;
    OutSnapshot.ProcessedMilestoneKeys = ProcessedMilestoneKeys;
}

void UChartManagerSubsystem::ValidateSaveSnapshot(const FChartSnapshot& Snapshot, const TSet<FString>& KnownRecordIds, const TSet<FString>& KnownArtistIds, const TSet<FString>& KnownRegionIds, FMusicSaveValidationResult& Result) const
{
    TSet<FString> KnownChartIds;
    for (const FChartDefinition& Definition : Snapshot.ChartDefinitions)
    {
        if (Definition.ChartId.IsEmpty())
        {
            Result.AddError(TEXT("Chart definition has an empty chart id."));
            continue;
        }
        if (KnownChartIds.Contains(Definition.ChartId))
        {
            Result.AddError(FString::Printf(TEXT("Duplicate chart definition id %s."), *Definition.ChartId));
        }
        KnownChartIds.Add(Definition.ChartId);

        if (Definition.Scope == EChartScope::Regional && !KnownRegionIds.Contains(Definition.RegionId))
        {
            Result.AddError(FString::Printf(TEXT("Chart %s references missing region %s."), *Definition.ChartId, *Definition.RegionId));
        }
        if (Definition.MaxEntries <= 0)
        {
            Result.AddError(FString::Printf(TEXT("Chart %s has invalid max entries."), *Definition.ChartId));
        }
        if (!StaticEnum<EChartRecordType>()->IsValidEnumValue(static_cast<int64>(Definition.RecordType)))
        {
            Result.AddError(FString::Printf(TEXT("Chart %s has invalid record type."), *Definition.ChartId));
        }
        if (!StaticEnum<EChartFormulaProfile>()->IsValidEnumValue(static_cast<int64>(Definition.FormulaProfile)))
        {
            Result.AddError(FString::Printf(TEXT("Chart %s has invalid formula profile."), *Definition.ChartId));
        }
        if (Definition.bFilterByFormat && !StaticEnum<ERecordFormat>()->IsValidEnumValue(static_cast<int64>(Definition.FormatFilter)))
        {
            Result.AddError(FString::Printf(TEXT("Chart %s has invalid format filter."), *Definition.ChartId));
        }
    }

    TSet<FString> SeenSnapshotKeys;
    for (const FWeeklyChartSnapshot& SnapshotEntry : Snapshot.WeeklySnapshots)
    {
        if (SnapshotEntry.ChartId.IsEmpty() || !KnownChartIds.Contains(SnapshotEntry.ChartId))
        {
            Result.AddError(FString::Printf(TEXT("Weekly chart snapshot references missing chart %s."), *SnapshotEntry.ChartId));
        }
        if (SnapshotEntry.WeekStart.GetTicks() <= 0)
        {
            Result.AddError(FString::Printf(TEXT("Weekly chart snapshot %s has invalid week start."), *SnapshotEntry.ChartId));
        }

        const FString SnapshotKey = BuildChartWeekKey(SnapshotEntry.ChartId, SnapshotEntry.WeekStart);
        if (SeenSnapshotKeys.Contains(SnapshotKey))
        {
            Result.AddError(FString::Printf(TEXT("Duplicate weekly chart snapshot %s."), *SnapshotKey));
        }
        SeenSnapshotKeys.Add(SnapshotKey);

        TSet<int32> SeenRanks;
        for (const FChartEntry& Entry : SnapshotEntry.Entries)
        {
            if (Entry.Rank <= 0)
            {
                Result.AddError(FString::Printf(TEXT("Chart entry for record %s has invalid rank."), *Entry.RecordId));
            }
            if (SeenRanks.Contains(Entry.Rank))
            {
                Result.AddError(FString::Printf(TEXT("Duplicate rank %d in chart snapshot %s."), Entry.Rank, *SnapshotKey));
            }
            SeenRanks.Add(Entry.Rank);

            if (Entry.RecordId.IsEmpty() || !KnownRecordIds.Contains(Entry.RecordId))
            {
                Result.AddError(FString::Printf(TEXT("Chart entry references missing record %s."), *Entry.RecordId));
            }
            if (Entry.ArtistId.IsEmpty() || !KnownArtistIds.Contains(Entry.ArtistId))
            {
                Result.AddError(FString::Printf(TEXT("Chart entry references missing artist %s."), *Entry.ArtistId));
            }
            if (!FMath::IsFinite(Entry.ChartPoints) || Entry.ChartPoints < 0.f)
            {
                Result.AddError(FString::Printf(TEXT("Chart entry for record %s has invalid points."), *Entry.RecordId));
            }
            if (Entry.Units < 0 || Entry.StreamEquivalentUnits < 0)
            {
                Result.AddError(FString::Printf(TEXT("Chart entry for record %s has invalid units."), *Entry.RecordId));
            }
        }
    }

    for (const TPair<FString, FRecordChartHistory>& Pair : Snapshot.RecordHistory)
    {
        if (Pair.Key.IsEmpty() || !KnownRecordIds.Contains(Pair.Key))
        {
            Result.AddError(FString::Printf(TEXT("Chart history references missing record %s."), *Pair.Key));
        }
        if (Pair.Value.RecordId != Pair.Key)
        {
            Result.AddError(FString::Printf(TEXT("Chart history key %s does not match record id %s."), *Pair.Key, *Pair.Value.RecordId));
        }
    }

    for (const FString& Key : Snapshot.ProcessedChartWeekKeys)
    {
        if (Key.IsEmpty())
        {
            Result.AddError(TEXT("Processed chart week key set contains an empty key."));
        }
    }
    for (const FString& Key : Snapshot.ProcessedMilestoneKeys)
    {
        if (Key.IsEmpty())
        {
            Result.AddError(TEXT("Processed chart milestone key set contains an empty key."));
        }
    }
}

void UChartManagerSubsystem::ApplySaveSnapshot(const FChartSnapshot& Snapshot)
{
    ChartDefinitions.Reset();
    for (const FChartDefinition& Definition : Snapshot.ChartDefinitions)
    {
        ChartDefinitions.Add(Definition.ChartId, Definition);
    }

    WeeklySnapshots.Reset();
    for (const FWeeklyChartSnapshot& WeeklySnapshot : Snapshot.WeeklySnapshots)
    {
        WeeklySnapshots.Add(BuildChartWeekKey(WeeklySnapshot.ChartId, WeeklySnapshot.WeekStart), WeeklySnapshot);
    }

    RecordHistory = Snapshot.RecordHistory;
    CurrentSnapshotKeyByChartId = Snapshot.CurrentSnapshotKeyByChartId;
    ProcessedChartWeekKeys = Snapshot.ProcessedChartWeekKeys;
    ProcessedMilestoneKeys = Snapshot.ProcessedMilestoneKeys;

    if (ChartDefinitions.Num() == 0)
    {
        RefreshChartDefinitions();
    }
}
