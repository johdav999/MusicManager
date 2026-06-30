#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MarketingManagerSubsystem.generated.h"

class UFinanceManagerSubsystem;
class UMarketManagerSubsystem;
class URecordManagerSubsystem;
struct FMarketingSnapshot;
struct FMusicSaveValidationResult;

UENUM(BlueprintType)
enum class EMarketingChannel : uint8
{
    Radio,
    Press,
    Television,
    Posters,
    Social,
    Playlisting
};

UENUM(BlueprintType)
enum class EMarketingCampaignStatus : uint8
{
    Planned,
    Active,
    Completed,
    Cancelled
};

USTRUCT(BlueprintType)
struct FMarketingExposureEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RegionId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMarketingChannel Channel = EMarketingChannel::Radio;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Exposure = 0.f;
};

USTRUCT(BlueprintType)
struct FMarketingChannelRule
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMarketingChannel Channel = EMarketingChannel::Radio;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 EraStartYear = 1955;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 EraEndYear = 2026;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ExposurePerThousand = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MinimumBudget = 1000.f;

    bool IsActiveForDate(const FDateTime& Date) const
    {
        return Date.GetYear() >= EraStartYear && Date.GetYear() <= EraEndYear;
    }
};

USTRUCT(BlueprintType)
struct FLaunchMarketingCampaignCommand
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString LabelId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> TargetRegionIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<EMarketingChannel> Channels;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Budget = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime StartDate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime EndDate;
};

USTRUCT(BlueprintType)
struct FMarketingCampaign
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString CampaignId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString LabelId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> TargetRegionIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<EMarketingChannel> Channels;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Budget = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime StartDate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime EndDate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMarketingCampaignStatus Status = EMarketingCampaignStatus::Planned;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMarketingExposureEntry> GeneratedExposure;
};

USTRUCT(BlueprintType)
struct FMarketingCampaignROISummary
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString CampaignId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ArtistDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Spend = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float GeneratedExposure = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 EstimatedUnitLift = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float EstimatedGrossRevenue = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float EstimatedROI = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMarketingCampaignStatus Status = EMarketingCampaignStatus::Planned;
};

USTRUCT(BlueprintType)
struct FMarketingChannelOption
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMarketingChannel Channel = EMarketingChannel::Radio;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bAvailable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MinimumBudget = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float EstimatedExposurePerThousand = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString UnavailableReason;
};

USTRUCT(BlueprintType)
struct FMarketingPlannerView
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRecordIsMarketable = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ArtistDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString LabelId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float LabelCash = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaximumAffordableBudget = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> TargetRegionIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMarketingChannelOption> ChannelOptions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMarketingCampaign> ActiveCampaigns;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMarketingCampaignROISummary> CampaignROISummaries;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMarketingCampaignROISummary ForecastSummary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> ValidationWarnings;
};

USTRUCT(BlueprintType)
struct FMarketingDashboardSummary
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMarketingCampaignROISummary> ActiveCampaigns;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMarketingCampaignROISummary> CompletedCampaigns;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TotalActiveSpend = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TotalProjectedROI = 0.f;
};

UCLASS()
class MUSICMANAGER_API UMarketingManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category="Marketing")
    bool LaunchMarketingCampaign(const FLaunchMarketingCampaignCommand& Command, FString& OutCampaignId, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="Marketing")
    void GetCampaignsForRecord(const FString& RecordId, TArray<FMarketingCampaign>& OutCampaigns) const;

    UFUNCTION(BlueprintCallable, Category="Marketing")
    bool BuildMarketingPlannerView(const FString& RecordId, FMarketingPlannerView& OutView, FString& OutError) const;

    UFUNCTION(BlueprintCallable, Category="Marketing")
    bool BuildCampaignROISummary(const FString& CampaignId, FMarketingCampaignROISummary& OutSummary) const;

    UFUNCTION(BlueprintCallable, Category="Marketing")
    void BuildMarketingDashboardSummary(FMarketingDashboardSummary& OutSummary) const;

    static FMarketingCampaignROISummary CalculateROISummary(
        const FMarketingCampaign& Campaign,
        const FString& RecordDisplayName,
        const FString& ArtistDisplayName,
        float AverageUnitRevenue = 10.f);

    UFUNCTION()
    void HandleMonthAdvanced(const FDateTime& NewDate);

    void BuildSaveSnapshot(FMarketingSnapshot& OutSnapshot) const;
    void ValidateSaveSnapshot(const FMarketingSnapshot& Snapshot, const TSet<FString>& KnownRecordIds, const TSet<FString>& KnownLabelIds, const TSet<FString>& KnownRegionIds, FMusicSaveValidationResult& Result) const;
    void ApplySaveSnapshot(const FMarketingSnapshot& Snapshot);

private:
    bool ValidateLaunchCommand(const FLaunchMarketingCampaignCommand& Command, FString& OutError) const;
    float EstimateExposure(const FLaunchMarketingCampaignCommand& Command, EMarketingChannel Channel) const;
    float ResolveChannelExposure(const FMarketingCampaign& Campaign, EMarketingChannel Channel) const;
    FString BuildCampaignMonthKey(const FString& CampaignId, const FDateTime& Date) const;
    bool IsChannelValidForDate(EMarketingChannel Channel, const FDateTime& Date, FMarketingChannelRule& OutRule) const;
    FMarketingCampaignROISummary BuildForecastSummary(const FMarketingPlannerView& View) const;

    UPROPERTY()
    TMap<FString, FMarketingCampaign> Campaigns;

    UPROPERTY()
    TMap<EMarketingChannel, FMarketingChannelRule> ChannelRules;

    UPROPERTY()
    TSet<FString> AppliedCampaignMonthKeys;
};
