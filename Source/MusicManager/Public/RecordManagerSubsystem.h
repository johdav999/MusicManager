#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MarketRegion.h"
#include "MarketManagerSubsystem.h"
#include "ArtistManagerSubsystem.h"
#include "FinanceManagerSubsystem.h"
#include "RecordManagerSubsystem.generated.h"

class USongManagerSubsystem;
class UGameTimeSubsystem;
struct FMusicSaveValidationResult;
struct FRecordManagerSnapshot;

DECLARE_MULTICAST_DELEGATE_OneParam(
    FOnArtistRecordCreated,
    FString /* ArtistId */
);

UENUM(BlueprintType)
enum class ERecordFormat : uint8
{
    Vinyl,
    Cassette,
    CD,
    DigitalDownload,
    Streaming
};

/** High-level lifecycle stages for a record. */
UENUM(BlueprintType)
enum class ERecordLifecycleState : uint8
{
    Draft,
    Recording,
    Recorded,
    Scheduled,
    Released,
    Catalog
};

UENUM(BlueprintType)
enum class ERecordType : uint8
{
    Single,
    EP,
    LP
};

USTRUCT(BlueprintType)
struct FScheduleReleaseCommand
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString LabelId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime ReleaseDate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> TargetRegionIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<ERecordFormat> Formats;
};

/**
 * Player request to record a new release. This is intentionally light-weight and
 * never persisted; authoritative validation happens in subsystems.
 */
USTRUCT(BlueprintType)
struct FRecordRecordingIntent
{
    GENERATED_BODY();

    /** Owning artist for the release. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ArtistId;

    /** Album/record display name requested by the player. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString AlbumName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERecordType RecordType = ERecordType::Single;

    /** True if the player intends to release a single (one track). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsSingle = false;

    /** True if the player intends to release an LP (multiple tracks). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsLP = false;

    /** Ordered list of song ids in track order. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> SongIds;

    /** Formats the player wants to support, before era filtering. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<ERecordFormat> RequestedFormats;

    /** Optional requested release date; recording completion is still authoritative. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TOptional<FDateTime> DesiredReleaseDate;
};

USTRUCT(BlueprintType)
struct FRecordingProjection
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ArtistId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERecordType RecordType = ERecordType::Single;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 SongCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float EstimatedRecordingCost = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 EstimatedDurationDays = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime StartDate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime EstimatedCompletionDate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bCanRecord = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> ValidationWarnings;
};

USTRUCT(BlueprintType)
struct FActiveRecordingSession
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordingId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRecordRecordingIntent Intent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime StartDate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime CompletionDate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RecordingCost = 0.f;
};

/**
 * Era validity, pricing, and cost profile for a record format.
 */
USTRUCT(BlueprintType)
struct FRecordFormatRule
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERecordFormat Format = ERecordFormat::Vinyl;

    /** First year this format can be sold. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 EraStartYear = 1955;

    /** Last year this format is relevant (inclusive). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 EraEndYear = 2026;

    /** Suggested retail price baseline used by FinanceManager. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BasePrice = 10.f;

    /** Variable cost rate applied in FinanceManager when booking sales. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CostRate = 0.35f;

    bool IsActiveForDate(const FDateTime& Date) const
    {
        return Date.GetYear() >= EraStartYear && Date.GetYear() <= EraEndYear;
    }
};

/** Per-market and per-format unit sales for a single month. */
USTRUCT(BlueprintType)
struct FRecordSalesEntry
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString MarketId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERecordFormat Format = ERecordFormat::Vinyl;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime Month;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 UnitsSold = 0;

    /** Optional debug/analytics value for demand strength. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float DemandScore = 0.f;
};

USTRUCT(BlueprintType)
struct FRecordSalesHistory
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FRecordSalesEntry> Entries;
};

USTRUCT(BlueprintType)
struct FRecordData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ArtistId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString AlbumName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERecordType RecordType = ERecordType::Single;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsSingle = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsLP = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> SongIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime DateRecorded;

    /** First day the record is commercially available. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime ReleaseDate;

    /** Label owner used for finance booking. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString LabelId;

    /** Primary genre for market appetite alignment. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString PrimaryGenre;

    /** Formats this release should sell on (subject to era validity). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<ERecordFormat> Formats;

    /** Intrinsic quality baseline derived from production/mastering. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RecordQuality = 0.7f;

    /** Marketing exposure multiplier injected by campaign systems. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MarketingExposure = 1.0f;

    /** Regions where this record is commercially available. Empty means not scheduled for market sale. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> TargetRegionIds;
};

USTRUCT(BlueprintType)
struct FReleasePlannerRecordOption
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString AlbumName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ArtistDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString LabelDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ArtistId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString LabelId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime DateRecorded;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERecordLifecycleState LifecycleState = ERecordLifecycleState::Draft;
};

USTRUCT(BlueprintType)
struct FReleasePlannerView
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasPlannableRecords = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRecordData SelectedRecord;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString SelectedRecordDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString SelectedArtistDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERecordLifecycleState SelectedRecordState = ERecordLifecycleState::Draft;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FReleasePlannerRecordOption> PlannableRecords;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FRecordFormatRule> ValidFormats;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMarketRegion> AvailableRegions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ProjectedReach = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> ValidationWarnings;
};

USTRUCT(BlueprintType)
struct FReleaseDashboardItem
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ArtistId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ArtistDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime ReleaseDate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERecordLifecycleState LifecycleState = ERecordLifecycleState::Draft;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> TargetRegionIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<ERecordFormat> Formats;
};

USTRUCT(BlueprintType)
struct FReleaseDashboardSummary
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FReleaseDashboardItem> AwaitingPlanning;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FReleaseDashboardItem> ScheduledReleases;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FReleaseDashboardItem> RecentlyReleased;
};

UCLASS()
class MUSICMANAGER_API URecordManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    FOnArtistRecordCreated OnArtistRecordCreated;

    UFUNCTION()
    FString CreateRecord(const FRecordData& Data);

    UFUNCTION(BlueprintCallable)
    bool GetRecordById(const FString& RecordId, FRecordData& OutData) const;

    UFUNCTION(BlueprintCallable, Category="Records")
    int32 GetRecordCountForArtist(const FString& ArtistId) const;

    /** Begin the recording lifecycle from player intent. */
    UFUNCTION(BlueprintCallable, Category="Records|Recording")
    bool SubmitRecordingIntent(const FRecordRecordingIntent& Intent, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="Records|Recording")
    bool BuildRecordingProjection(const FRecordRecordingIntent& Intent, FRecordingProjection& OutProjection, FString& OutError) const;

    /** TEMPORARY RADIO SIMULATION SUPPORT: REPLACE WITH REAL RADIO SYSTEM */
    void GetRecentlyReleasedArtists(const FDateTime& CurrentDate, int32 MonthsBack, TArray<FString>& OutArtistIds) const;

    /** Entry point triggered from GameTimeSubsystem each month. */
    UFUNCTION()
    void HandleMonthAdvanced(const FDateTime& NewDate);

    /** Allows other systems to query persisted sales history. */
    UFUNCTION(BlueprintCallable, Category="Records|Sales")
    bool GetSalesHistory(const FString& RecordId, TArray<FRecordSalesEntry>& OutEntries) const;

    UFUNCTION(BlueprintCallable, Category="Records|Sales")
    int32 GetLifetimeUnits(const FString& RecordId) const;

    UFUNCTION(BlueprintCallable, Category="Records")
    void GetAllRecords(TArray<FRecordData>& OutRecords) const;

    UFUNCTION(BlueprintCallable, Category="Records|Release")
    bool ScheduleRelease(const FScheduleReleaseCommand& Command, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="Records|Release")
    bool ReleaseNow(const FString& RecordId, const FString& LabelId, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="Records|Release")
    bool GetRecordLifecycleState(const FString& RecordId, ERecordLifecycleState& OutState) const;

    UFUNCTION(BlueprintCallable, Category="Records|Release")
    void GetRecordsAwaitingReleasePlanning(TArray<FRecordData>& OutRecords) const;

    UFUNCTION(BlueprintCallable, Category="Records|Release")
    void GetScheduledReleases(TArray<FRecordData>& OutRecords) const;

    UFUNCTION(BlueprintCallable, Category="Records|Release")
    void GetReleasedOrCatalogRecords(TArray<FRecordData>& OutRecords) const;

    UFUNCTION(BlueprintCallable, Category="Records|Release")
    bool BuildReleasePlannerView(const FString& SelectedRecordId, FReleasePlannerView& OutView, FString& OutError) const;

    UFUNCTION(BlueprintCallable, Category="Records|Release")
    void BuildReleaseDashboardSummary(FReleaseDashboardSummary& OutSummary) const;

    UFUNCTION(BlueprintCallable, Category="Records")
    FString GetRecordDisplayName(const FString& RecordId) const;

    UFUNCTION(BlueprintCallable, Category="Records")
    FString GetArtistDisplayNameForRecord(const FString& RecordId) const;

    UFUNCTION(BlueprintCallable, Category="Records|Release")
    bool IsRecordMarketable(const FString& RecordId, FString& OutLabelId) const;

    void BuildSaveSnapshot(FRecordManagerSnapshot& OutSnapshot) const;
    void ValidateSaveSnapshot(const FRecordManagerSnapshot& Snapshot, const TSet<FString>& KnownArtistIds, const TSet<FString>& KnownSongIds, const TSet<FString>& KnownLabelIds, FMusicSaveValidationResult& Result) const;
    void ApplySaveSnapshot(const FRecordManagerSnapshot& Snapshot);

private:
    /** Track active recordings until the studio session is complete. */
    void ProcessActiveRecordings(const FDateTime& CurrentDate);
    void CompleteRecording(const FString& RecordingId, const FDateTime& CompletionDate);
    bool ValidateRecordingIntent(const FRecordRecordingIntent& Intent, FString& OutError) const;
    FRecordRecordingIntent NormalizeRecordingIntent(const FRecordRecordingIntent& Intent) const;
    bool IsSongCountValidForType(ERecordType RecordType, int32 SongCount, FString& OutError) const;
    float EstimateRecordingCost(const FRecordRecordingIntent& Intent) const;
    int32 EstimateRecordingDurationDays(const FRecordRecordingIntent& Intent) const;
    FString GetRecordTypeDisplayName(ERecordType RecordType) const;
    void ApplyFormatRules(const FDateTime& CurrentDate, TArray<ERecordFormat>& InOutFormats) const;
    FString DerivePrimaryGenre(const TArray<FString>& SongIds) const;
    float ComputeRecordQuality(const TArray<FString>& SongIds, const FString& ArtistId) const;
    FDateTime ResolveReleaseDate(const FRecordRecordingIntent& Intent, const FDateTime& DateRecorded) const;
    FString ResolveLabelForArtist(const FString& ArtistId) const;
    ERecordLifecycleState ResolveLifecycleStateForRecord(const FString& RecordId) const;
    bool ValidateReleaseCommand(const FScheduleReleaseCommand& Command, FString& OutError) const;
    void ProcessScheduledReleases(const FDateTime& CurrentDate);
    bool IsRegionTargetedByRecord(const FRecordData& Record, const FString& RegionId) const;
    FString ResolveArtistDisplayName(const FString& ArtistId) const;
    FReleaseDashboardItem BuildReleaseDashboardItem(const FRecordData& Record) const;

    void SimulateMonthlySales(const FDateTime& CurrentDate);
    void ComputeRecordSalesForMarket(const FRecordData& Record, const FMarketDemandSnapshot& Demand, const FArtistMarketModifiers& ArtistImpact, const FDateTime& CurrentDate, TArray<FRecordSalesEntry>& OutEntries) const;
    float ComputeLifecycleFactor(const FDateTime& ReleaseDate, const FDateTime& CurrentDate) const;
    float EvaluateSongQuality(const FRecordData& Record) const;
    bool IsFormatEligible(ERecordFormat Format, const FDateTime& CurrentDate) const;

    UPROPERTY()
    TMap<FString, FRecordData> Records;

    UPROPERTY()
    TMap<FString, FRecordSalesHistory> SalesHistory;

    UPROPERTY()
    TMap<FString, int32> LifetimeUnits;

    /** Era rules and pricing for each available format. */
    UPROPERTY()
    TMap<ERecordFormat, FRecordFormatRule> FormatRules;

    /** Internal tracking of lifecycle state per record id. */
    UPROPERTY()
    TMap<FString, ERecordLifecycleState> RecordStates;

    /** Lightweight recording session bookkeeping keyed by a generated recording id. */
    UPROPERTY()
    TMap<FString, FRecordRecordingIntent> ActiveRecordingIntents;
    UPROPERTY()
    TMap<FString, FDateTime> RecordingStartDates;
    UPROPERTY()
    TMap<FString, FDateTime> RecordingCompletionDates;

    UPROPERTY()
    TMap<FString, float> RecordingCosts;
};
