#include "ChartManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "MusicSaveGame.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicChartWeeklySalesAllocationTest, "MusicManager.Charts.WeeklySalesAllocation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicChartWeeklySalesAllocationTest::RunTest(const FString& Parameters)
{
    FRecordSalesEntry MonthlyEntry;
    MonthlyEntry.RecordId = TEXT("record_001");
    MonthlyEntry.MarketId = TEXT("US");
    MonthlyEntry.Format = ERecordFormat::Vinyl;
    MonthlyEntry.Month = FDateTime(1985, 5, 1);
    MonthlyEntry.UnitsSold = 3100;

    const int32 FullWeekUnits = UChartManagerSubsystem::CalculateWeeklyUnitsFromSalesEntry(MonthlyEntry, FDateTime(1985, 5, 6));
    TestEqual(TEXT("A full week inside a 31-day month should receive seven days of units."), FullWeekUnits, 700);

    const int32 PartialWeekUnits = UChartManagerSubsystem::CalculateWeeklyUnitsFromSalesEntry(MonthlyEntry, FDateTime(1985, 4, 28));
    TestEqual(TEXT("A week crossing into the month should receive only overlapping month days."), PartialWeekUnits, 400);

    const int32 OutsideUnits = UChartManagerSubsystem::CalculateWeeklyUnitsFromSalesEntry(MonthlyEntry, FDateTime(1985, 6, 2));
    TestEqual(TEXT("A week outside the month bucket should receive no units."), OutsideUnits, 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicChartDefinitionMatchingTest, "MusicManager.Charts.DefinitionMatching", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicChartDefinitionMatchingTest::RunTest(const FString& Parameters)
{
    FRecordData Record;
    Record.RecordId = TEXT("record_single_001");
    Record.ArtistId = TEXT("artist_001");
    Record.bIsSingle = true;
    Record.PrimaryGenre = TEXT("Synthpop");
    Record.Formats = { ERecordFormat::Vinyl, ERecordFormat::Streaming };

    FChartDefinition SinglesChart;
    SinglesChart.RecordType = EChartRecordType::Singles;
    TestTrue(TEXT("Single should match singles chart."), UChartManagerSubsystem::DoesRecordMatchDefinition(Record, SinglesChart));

    FChartDefinition AlbumsChart;
    AlbumsChart.RecordType = EChartRecordType::Albums;
    TestFalse(TEXT("Single should not match albums chart."), UChartManagerSubsystem::DoesRecordMatchDefinition(Record, AlbumsChart));

    FChartDefinition GenreChart;
    GenreChart.RecordType = EChartRecordType::AllRecords;
    GenreChart.GenreFilter = TEXT("synthpop");
    TestTrue(TEXT("Genre matching should be case-insensitive."), UChartManagerSubsystem::DoesRecordMatchDefinition(Record, GenreChart));

    FChartDefinition FormatChart;
    FormatChart.RecordType = EChartRecordType::AllRecords;
    FormatChart.bFilterByFormat = true;
    FormatChart.FormatFilter = ERecordFormat::CD;
    TestFalse(TEXT("Record should not match a format chart for unsupported formats."), UChartManagerSubsystem::DoesRecordMatchDefinition(Record, FormatChart));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicChartFormulaProfileTest, "MusicManager.Charts.FormulaProfiles", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicChartFormulaProfileTest::RunTest(const FString& Parameters)
{
    FRecordSalesEntry StreamingEntry;
    StreamingEntry.Format = ERecordFormat::Streaming;
    StreamingEntry.UnitsSold = 1500;

    FRecordSalesEntry VinylEntry;
    VinylEntry.Format = ERecordFormat::Vinyl;
    VinylEntry.UnitsSold = 1500;

    FChartDefinition StreamingChart;
    StreamingChart.FormulaProfile = EChartFormulaProfile::StreamingEra;
    TestTrue(
        TEXT("Streaming-era formula should weight streaming above physical units."),
        UChartManagerSubsystem::CalculateFormulaWeight(StreamingChart, StreamingEntry, FDateTime(2020, 1, 6))
        > UChartManagerSubsystem::CalculateFormulaWeight(StreamingChart, VinylEntry, FDateTime(2020, 1, 6)));

    FChartDefinition AlbumsChart;
    AlbumsChart.FormulaProfile = EChartFormulaProfile::AlbumLongevity;
    TestTrue(
        TEXT("Album longevity formula should favor physical album formats over streaming."),
        UChartManagerSubsystem::CalculateFormulaWeight(AlbumsChart, VinylEntry, FDateTime(1991, 1, 7))
        > UChartManagerSubsystem::CalculateFormulaWeight(AlbumsChart, StreamingEntry, FDateTime(1991, 1, 7)));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicChartDashboardViewTest, "MusicManager.Charts.DashboardView", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicChartDashboardViewTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    UChartManagerSubsystem* ChartManager = NewObject<UChartManagerSubsystem>(GameInstance);

    FChartSnapshot Snapshot;
    FChartDefinition Definition;
    Definition.ChartId = TEXT("global_records");
    Definition.DisplayName = FText::FromString(TEXT("Global Records"));
    Definition.MaxEntries = 100;
    Snapshot.ChartDefinitions.Add(Definition);

    FWeeklyChartSnapshot WeeklySnapshot;
    WeeklySnapshot.ChartId = TEXT("global_records");
    WeeklySnapshot.WeekStart = FDateTime(1985, 5, 6);
    WeeklySnapshot.GeneratedAt = FDateTime(1985, 5, 6);

    FChartEntry Entry;
    Entry.ChartId = TEXT("global_records");
    Entry.RecordId = TEXT("record_001");
    Entry.ArtistId = TEXT("artist_001");
    Entry.WeekStart = FDateTime(1985, 5, 6);
    Entry.Rank = 1;
    Entry.PeakRank = 1;
    Entry.WeeksOnChart = 1;
    Entry.Units = 1200;
    Entry.ChartPoints = 1200.f;
    WeeklySnapshot.Entries.Add(Entry);
    Snapshot.WeeklySnapshots.Add(WeeklySnapshot);
    Snapshot.CurrentSnapshotKeyByChartId.Add(TEXT("global_records"), TEXT("global_records:1985-05-06"));

    ChartManager->ApplySaveSnapshot(Snapshot);

    FChartDashboardView View;
    TestTrue(TEXT("Dashboard view should build from current chart snapshots."), ChartManager->BuildChartDashboardView(View));
    TestTrue(TEXT("Dashboard should report chart data when a current snapshot exists."), View.bHasChartData);
    TestTrue(TEXT("Dashboard should expose current number one."), View.CurrentNumberOne.bHasEntry);
    TestEqual(TEXT("Current number one should preserve the record id."), View.CurrentNumberOne.Entry.RecordId, FString(TEXT("record_001")));

    return true;
}

#endif
