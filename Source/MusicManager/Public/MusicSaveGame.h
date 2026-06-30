#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ChartManagerSubsystem.h"
#include "FArtistContract.h"
#include "EventSubsystem.h"
#include "FinanceManagerSubsystem.h"
#include "MarketingManagerSubsystem.h"
#include "MarketManagerSubsystem.h"
#include "RecordManagerSubsystem.h"
#include "SongManagerSubsystem.h"
#include "MusicSaveGame.generated.h"

USTRUCT(BlueprintType)
struct FMusicSaveValidationResult
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Save")
    bool bIsValid = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Save")
    TArray<FString> Errors;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Save")
    TArray<FString> Warnings;

    void AddError(const FString& Message)
    {
        bIsValid = false;
        Errors.Add(Message);
    }

    void AddWarning(const FString& Message)
    {
        Warnings.Add(Message);
    }
};

USTRUCT(BlueprintType)
struct FCampaignMetaSnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString SlotName;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FDateTime CreatedAt;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FDateTime LastSavedAt;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FSoftObjectPath ThumbnailAsset;
};

USTRUCT(BlueprintType)
struct FPlayerLabelSnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString LabelId = TEXT("label_player");

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString DisplayName = TEXT("Player Label");

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FDateTime FoundedDate = FDateTime(1955, 1, 1);

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    float Reputation = 1.0f;
};

USTRUCT(BlueprintType)
struct FTimeSnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FDateTime CurrentGameDate = FDateTime(1955, 1, 1);

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    bool bHasReachedSimulationEnd = false;
};

USTRUCT(BlueprintType)
struct FArtistManagerSnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    TArray<FArtistData> UnsignedArtists;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    TArray<FArtistContract> ActiveContracts;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    TArray<FArtistContract> ExpiredContracts;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString SelectedArtistId;

    UPROPERTY(SaveGame)
    TMap<FString, FArtistSongList> ArtistToSongs;

    UPROPERTY(SaveGame)
    TMap<FString, float> ArtistMomentum;

    UPROPERTY(SaveGame)
    TMap<FString, float> ArtistReputation;
};

USTRUCT(BlueprintType)
struct FRecordManagerSnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    TArray<FRecordData> Records;

    UPROPERTY(SaveGame)
    TMap<FString, FRecordSalesHistory> SalesHistory;

    UPROPERTY(SaveGame)
    TMap<FString, int32> LifetimeUnits;

    UPROPERTY(SaveGame)
    TMap<FString, ERecordLifecycleState> RecordStates;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    TArray<FActiveRecordingSession> ActiveRecordingSessions;
};

USTRUCT(BlueprintType)
struct FFinanceSnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    TMap<FString, FLabelAccount> LabelAccounts;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    TArray<FMonthlyFinanceSummary> MonthlySummaries;

    UPROPERTY(SaveGame)
    TSet<FString> ClosedMonthlySummaryKeys;
};

USTRUCT(BlueprintType)
struct FMarketSnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    TMap<FString, FRegionArtistExposureState> RegionArtistExposure;

    UPROPERTY(SaveGame)
    TMap<FString, FRegionRecordExposureState> RegionRecordExposure;
};

USTRUCT(BlueprintType)
struct FMarketingSnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    TArray<FMarketingCampaign> Campaigns;

    UPROPERTY(SaveGame)
    TSet<FString> AppliedCampaignMonthKeys;
};

USTRUCT(BlueprintType)
struct FChartSnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    TArray<FChartDefinition> ChartDefinitions;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    TArray<FWeeklyChartSnapshot> WeeklySnapshots;

    UPROPERTY(SaveGame)
    TMap<FString, FRecordChartHistory> RecordHistory;

    UPROPERTY(SaveGame)
    TMap<FString, FString> CurrentSnapshotKeyByChartId;

    UPROPERTY(SaveGame)
    TSet<FString> ProcessedChartWeekKeys;

    UPROPERTY(SaveGame)
    TSet<FString> ProcessedMilestoneKeys;
};

USTRUCT(BlueprintType)
struct FNewsSnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    TArray<FMonthlyNewsSummary> MonthlyNewsSummaries;

    UPROPERTY(SaveGame)
    TSet<FString> ProcessedNewsKeys;

    UPROPERTY(SaveGame)
    TSet<FString> ClosedMonthlyNewsKeys;
};

USTRUCT(BlueprintType)
struct FSaveSlotDescriptor
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
    FString SlotName;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
    FText DisplayName;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
    FString PlayerLabelName;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
    FDateTime InGameDate;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
    FDateTime CreatedAt;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
    FDateTime LastSavedAt;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
    int32 SaveVersion = 0;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
    bool bIsAutosave = false;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
    bool bIsBackup = false;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
    FString ParentSlotName;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
    FSoftObjectPath ThumbnailAsset;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
    bool bIsLoadable = false;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
    TArray<FString> ValidationMessages;
};

USTRUCT(BlueprintType)
struct FSaveSlotRegistrySnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
    TArray<FSaveSlotDescriptor> Slots;
};

UCLASS()
class MUSICMANAGER_API UMusicSaveSlotRegistry : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
    FSaveSlotRegistrySnapshot Registry;
};

USTRUCT(BlueprintType)
struct FFutureTourSaveRecord
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString TourId;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString ArtistId;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString Status;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FDateTime StartDate;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FDateTime EndDate;
};

USTRUCT(BlueprintType)
struct FFutureAwardSaveRecord
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString AwardSeasonId;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString AwardId;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString WinnerArtistId;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString WinnerRecordId;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    int32 Year = 0;
};

USTRUCT(BlueprintType)
struct FFutureCriticReviewSaveRecord
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString ReviewId;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString RecordId;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString PublicationId;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    float Score = 0.f;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FDateTime PublishedAt;
};

USTRUCT(BlueprintType)
struct FFutureRivalLabelSaveRecord
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString LabelId;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString DisplayName;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    float Cash = 0.f;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    float Reputation = 0.f;
};

USTRUCT(BlueprintType)
struct FFutureStaffSaveRecord
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString StaffId;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString RoleId;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    int32 Level = 1;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FDateTime HiredAt;
};

USTRUCT(BlueprintType)
struct FFutureUnlockSaveRecord
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString UnlockId;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FDateTime UnlockedAt;
};

USTRUCT(BlueprintType)
struct FSettingsUISnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString SelectedScreenId;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString SelectedArtistId;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FString SelectedRecordId;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    TMap<FString, FString> UserSettings;
};

USTRUCT(BlueprintType)
struct FFutureSystemsSnapshot
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    TArray<FFutureTourSaveRecord> Tours;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    TArray<FFutureAwardSaveRecord> Awards;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    TArray<FFutureCriticReviewSaveRecord> CriticReviews;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    TArray<FFutureRivalLabelSaveRecord> RivalLabels;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    TArray<FFutureStaffSaveRecord> Staff;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    TArray<FFutureUnlockSaveRecord> Unlocks;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite)
    FSettingsUISnapshot SettingsUIState;
};

UCLASS()
class MUSICMANAGER_API UMusicSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    static constexpr int32 CurrentSaveVersion = 5;
    static constexpr int32 MinimumSupportedSaveVersion = 1;

    UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly, Category="Save")
    int32 SaveVersion = CurrentSaveVersion;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
    FCampaignMetaSnapshot CampaignMeta;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
    FPlayerLabelSnapshot PlayerLabelSnapshot;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
    FTimeSnapshot TimeSnapshot;

    UPROPERTY(SaveGame)
    TArray<FSongSaveRecord> SavedSongs;

    UPROPERTY(SaveGame)
    FArtistManagerSnapshot ArtistSnapshot;

    UPROPERTY(SaveGame)
    FRecordManagerSnapshot RecordSnapshot;

    UPROPERTY(SaveGame)
    FFinanceSnapshot FinanceSnapshot;

    UPROPERTY(SaveGame)
    FMarketSnapshot MarketSnapshot;

    UPROPERTY(SaveGame)
    FMarketingSnapshot MarketingSnapshot;

    UPROPERTY(SaveGame)
    FChartSnapshot ChartSnapshot;

    UPROPERTY(SaveGame)
    FNewsSnapshot NewsSnapshot;

    UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
    FFutureSystemsSnapshot FutureSystemsSnapshot;

    /** Legacy v1 field retained only to reject/migrate old save objects cleanly. */
    UPROPERTY(SaveGame)
    TArray<FArtistContract> SavedContracts;

    /** Legacy v1 field retained only to reject/migrate old save objects cleanly. */
    UPROPERTY(SaveGame)
    FDateTime SavedGameDate;

    /** Legacy v1 field retained only to reject/migrate old save objects cleanly. */
    UPROPERTY(SaveGame)
    int32 PlayerMoney = 0;

    void InitializeNewSave(const FString& SlotName);
    FMusicSaveValidationResult ValidateTopLevel() const;
    FMusicSaveValidationResult MigrateToCurrentVersion();
    FMusicSaveValidationResult ValidateFutureSystemsSnapshot() const;
    FSaveSlotDescriptor BuildSlotDescriptor(const FString& RequestedSlotName, bool bIsAutosave = false, bool bIsBackup = false, const FString& ParentSlotName = FString()) const;
};
