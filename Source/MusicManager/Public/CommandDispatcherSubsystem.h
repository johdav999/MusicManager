#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FArtistDealTerms.h"
#include "EventSubsystem.h"
#include "MarketingManagerSubsystem.h"
#include "MusicCommandResult.h"
#include "RecordManagerSubsystem.h"
#include "CommandDispatcherSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMusicCommands, Log, All);

USTRUCT(BlueprintType)
struct FSignArtistCommand
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ArtistId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString LabelId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SignUpBonus = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RoyaltyRate = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RecordCommitment = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ContractYears = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bExclusive = true;
};

USTRUCT(BlueprintType)
struct FRejectArtistCommand
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ArtistId;
};

USTRUCT(BlueprintType)
struct FStartRecordingCommand
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ArtistId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordTitle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERecordType RecordType = ERecordType::Single;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsSingle = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsLP = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> SongIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<ERecordFormat> RequestedFormats;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime DesiredReleaseDate;
};

USTRUCT(BlueprintType)
struct FAdvanceTimeCommand
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 WeeksToAdvance = 1;
};

UENUM(BlueprintType)
enum class EMusicCommandType : uint8
{
    Unknown,
    SignArtist,
    RejectArtist,
    StartRecording,
    ScheduleRelease,
    LaunchMarketingCampaign,
    AdvanceTime
};

USTRUCT(BlueprintType)
struct FMusicCommandDomainEvent
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid EventId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMusicCommandType CommandType = EMusicCommandType::Unknown;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMusicCommandResult Result;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> AffectedEntityIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString CreatedEntityId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime Timestamp;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMusicCommandExecuted, const FMusicCommandResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMusicCommandDomainEvent, const FMusicCommandDomainEvent&, Event);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMusicCommandDomainEventNative, const FMusicCommandDomainEvent&);

UCLASS()
class MUSICMANAGER_API UCommandDispatcherSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UPROPERTY(BlueprintAssignable, Category="Commands")
    FOnMusicCommandExecuted OnCommandExecuted;

    UPROPERTY(BlueprintAssignable, Category="Commands")
    FOnMusicCommandDomainEvent OnCommandDomainEvent;

    FOnMusicCommandDomainEventNative OnCommandDomainEventNative;

    UFUNCTION(BlueprintCallable, Category="Commands|Artists")
    FMusicCommandResult ExecuteSignArtist(const FSignArtistCommand& Command);

    UFUNCTION(BlueprintCallable, Category="Commands|Artists")
    FMusicCommandResult ExecuteRejectArtist(const FRejectArtistCommand& Command);

    UFUNCTION(BlueprintCallable, Category="Commands|Records")
    FMusicCommandResult ExecuteStartRecording(const FStartRecordingCommand& Command);

    UFUNCTION(BlueprintCallable, Category="Commands|Records")
    FMusicCommandResult ExecuteScheduleRelease(const FScheduleReleaseCommand& Command);

    UFUNCTION(BlueprintCallable, Category="Commands|Marketing")
    FMusicCommandResult ExecuteLaunchMarketingCampaign(const FLaunchMarketingCampaignCommand& Command);

    UFUNCTION(BlueprintCallable, Category="Commands|Time")
    FMusicCommandResult ExecuteAdvanceTime(const FAdvanceTimeCommand& Command);

    static FMusicNewsEvent BuildReleaseScheduledNewsEvent(const FScheduleReleaseCommand& Command, const FString& RecordDisplayName, const FString& ArtistDisplayName);
    static FMusicNewsEvent BuildMarketingLaunchNewsEvent(const FLaunchMarketingCampaignCommand& Command, const FString& CampaignId, const FString& RecordDisplayName, const FString& ArtistDisplayName);

private:
    FMusicCommandResult FinishCommand(EMusicCommandType CommandType, const FMusicCommandResult& Result);
    FMusicCommandDomainEvent BuildDomainEvent(EMusicCommandType CommandType, const FMusicCommandResult& Result) const;
    bool RequireGameInstance(FMusicCommandResult& OutFailure) const;
    FString GetPlayerLabelId() const;
};
