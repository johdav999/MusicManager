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
UENUM()
enum class ERecordLifecycleState : uint8
{
    Draft,
    Recording,
    Recorded,
    Scheduled,
    Released
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
};

UCLASS()
class MUSICMANAGER_API URecordManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION()
    FString CreateRecord(const FRecordData& Data);

    UFUNCTION(BlueprintCallable)
    bool GetRecordById(const FString& RecordId, FRecordData& OutData) const;

    /** Begin the recording lifecycle from player intent. */
    UFUNCTION(BlueprintCallable, Category="Records|Recording")
    bool SubmitRecordingIntent(const FRecordRecordingIntent& Intent, FString& OutError);

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

private:
    /** Track active recordings until the studio session is complete. */
    void ProcessActiveRecordings(const FDateTime& CurrentDate);
    void CompleteRecording(const FString& RecordingId, const FDateTime& CompletionDate);
    bool ValidateRecordingIntent(const FRecordRecordingIntent& Intent, FString& OutError);
    void ApplyFormatRules(const FDateTime& CurrentDate, TArray<ERecordFormat>& InOutFormats) const;
    FString DerivePrimaryGenre(const TArray<FString>& SongIds) const;
    float ComputeRecordQuality(const TArray<FString>& SongIds, const FString& ArtistId) const;
    FDateTime ResolveReleaseDate(const FRecordRecordingIntent& Intent, const FDateTime& DateRecorded) const;
    FString ResolveLabelForArtist(const FString& ArtistId) const;

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
};
