#pragma once

#include "CoreMinimal.h"
#include "RecordManagerSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ChartManagerSubsystem.generated.h"

struct FChartSnapshot;
struct FMusicSaveValidationResult;

DECLARE_LOG_CATEGORY_EXTERN(LogMusicCharts, Log, All);

UENUM(BlueprintType)
enum class EChartScope : uint8
{
    Global,
    Regional
};

UENUM(BlueprintType)
enum class EChartRecordType : uint8
{
    AllRecords,
    Singles,
    Albums
};

UENUM(BlueprintType)
enum class EChartFormulaProfile : uint8
{
    EraDefault,
    PhysicalSales,
    SinglesVelocity,
    AlbumLongevity,
    GenreSpecialist,
    FormatWeighted,
    StreamingEra
};

UENUM(BlueprintType)
enum class EChartMilestoneType : uint8
{
    FirstEntry,
    Top40,
    Top10,
    NumberOne,
    NewPeak
};

USTRUCT(BlueprintType)
struct FChartDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ChartId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EChartScope Scope = EChartScope::Global;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RegionId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EChartRecordType RecordType = EChartRecordType::AllRecords;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString GenreFilter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bFilterByFormat = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERecordFormat FormatFilter = ERecordFormat::Vinyl;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EChartFormulaProfile FormulaProfile = EChartFormulaProfile::EraDefault;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 MaxEntries = 100;
};

USTRUCT(BlueprintType)
struct FChartEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ChartId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ArtistId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime WeekStart;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Rank = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 PreviousRank = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 PeakRank = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 WeeksOnChart = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ChartPoints = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Units = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 StreamEquivalentUnits = 0;
};

USTRUCT(BlueprintType)
struct FWeeklyChartSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ChartId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime WeekStart;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime GeneratedAt;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FChartEntry> Entries;
};

USTRUCT(BlueprintType)
struct FRecordChartHistory
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FChartEntry> Entries;
};

USTRUCT(BlueprintType)
struct FChartEntryView
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Rank = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RankMovement = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ArtistId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText RecordTitle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText ArtistName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ChartPoints = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Units = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 PeakRank = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 WeeksOnChart = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bPlayerOwned = false;
};

USTRUCT(BlueprintType)
struct FChartListView
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasCharts = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FChartDefinition> ChartDefinitions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString SelectedChartId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime SelectedWeekStart;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FChartEntryView> Entries;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText EmptyStateMessage;
};

USTRUCT(BlueprintType)
struct FRecordChartHistoryView
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText RecordTitle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasHistory = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FChartEntryView> Entries;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText EmptyStateMessage;
};

USTRUCT(BlueprintType)
struct FChartDashboardHighlight
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasEntry = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ChartId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText ChartName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FChartEntryView Entry;
};

USTRUCT(BlueprintType)
struct FChartDashboardView
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasChartData = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FChartDashboardHighlight CurrentNumberOne;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FChartDashboardHighlight TopPlayerOwnedRelease;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FChartDashboardHighlight BiggestPlayerOwnedMovement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FChartDashboardHighlight> RecentMilestones;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText StatusMessage;
};

UCLASS()
class MUSICMANAGER_API UChartManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category="Charts")
    void GetChartDefinitions(TArray<FChartDefinition>& OutDefinitions) const;

    UFUNCTION(BlueprintCallable, Category="Charts")
    bool GetCurrentChart(const FString& ChartId, FWeeklyChartSnapshot& OutSnapshot) const;

    UFUNCTION(BlueprintCallable, Category="Charts")
    bool GetChartByWeek(const FString& ChartId, const FDateTime& WeekStart, FWeeklyChartSnapshot& OutSnapshot) const;

    UFUNCTION(BlueprintCallable, Category="Charts")
    bool GetRecordChartHistory(const FString& RecordId, FRecordChartHistory& OutHistory) const;

    UFUNCTION(BlueprintCallable, Category="Charts")
    bool GetTopRecord(const FString& ChartId, FChartEntry& OutEntry) const;

    UFUNCTION(BlueprintCallable, Category="Charts")
    bool BuildChartListView(const FString& ChartId, FChartListView& OutView) const;

    UFUNCTION(BlueprintCallable, Category="Charts")
    bool BuildRecordChartHistoryView(const FString& RecordId, FRecordChartHistoryView& OutView) const;

    UFUNCTION(BlueprintCallable, Category="Charts")
    bool BuildChartDashboardView(FChartDashboardView& OutView) const;

    UFUNCTION(BlueprintCallable, Category="Charts")
    void ResolveWeeklyCharts(const FDateTime& NewDate);

    static int32 CalculateWeeklyUnitsFromSalesEntry(const FRecordSalesEntry& SalesEntry, const FDateTime& WeekStart);
    static bool DoesRecordMatchDefinition(const FRecordData& Record, const FChartDefinition& Definition);
    static float CalculateFormulaWeight(const FChartDefinition& Definition, const FRecordSalesEntry& Entry, const FDateTime& WeekStart);

    void BuildSaveSnapshot(FChartSnapshot& OutSnapshot) const;
    void ValidateSaveSnapshot(const FChartSnapshot& Snapshot, const TSet<FString>& KnownRecordIds, const TSet<FString>& KnownArtistIds, const TSet<FString>& KnownRegionIds, FMusicSaveValidationResult& Result) const;
    void ApplySaveSnapshot(const FChartSnapshot& Snapshot);

private:
    void RefreshChartDefinitions();
    FDateTime GetWeekStart(const FDateTime& Date) const;
    FString BuildChartWeekKey(const FString& ChartId, const FDateTime& WeekStart) const;
    FString BuildMilestoneKey(const FString& ChartId, const FString& RecordId, EChartMilestoneType MilestoneType, const FDateTime& WeekStart) const;
    FWeeklyChartSnapshot CalculateChart(const FChartDefinition& Definition, const FDateTime& WeekStart, const FDateTime& GeneratedAt) const;
    float CalculateChartPoints(const TArray<struct FRecordSalesEntry>& SalesEntries, const FChartDefinition& Definition, const FDateTime& WeekStart, int32& OutUnits, int32& OutStreamEquivalentUnits) const;
    void UpdateRecordHistory(const FWeeklyChartSnapshot& Snapshot);
    void ProcessMilestones(const FWeeklyChartSnapshot& Snapshot);
    void EmitMilestoneNews(const FChartEntry& Entry, EChartMilestoneType MilestoneType);
    FChartEntryView BuildEntryView(const FChartEntry& Entry) const;
    FText GetChartDisplayName(const FString& ChartId) const;

    UPROPERTY()
    TMap<FString, FChartDefinition> ChartDefinitions;

    UPROPERTY()
    TMap<FString, FWeeklyChartSnapshot> WeeklySnapshots;

    UPROPERTY()
    TMap<FString, FRecordChartHistory> RecordHistory;

    UPROPERTY()
    TMap<FString, FString> CurrentSnapshotKeyByChartId;

    UPROPERTY()
    TSet<FString> ProcessedChartWeekKeys;

    UPROPERTY()
    TSet<FString> ProcessedMilestoneKeys;
};
