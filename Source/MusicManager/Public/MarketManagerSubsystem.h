#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MarketManagerSubsystem.generated.h"

UENUM(BlueprintType)
enum class EMarketRegionType : uint8
{
    Country,
    State,
    CityCluster
};

USTRUCT(BlueprintType)
struct FMarketSegmentProfile
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString SegmentId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PopulationShare = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AvgIncome = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Trendiness = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, float> GenreAffinity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PriceSensitivity = 1.f;
};

USTRUCT(BlueprintType)
struct FMarketRegion
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RegionId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EMarketRegionType RegionType = EMarketRegionType::Country;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TotalPopulation = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FMarketSegmentProfile> Segments;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RadioReach = 0.5f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TMap<FString, float> RecentArtistExposure;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TMap<FString, float> RecentRecordExposure;
};

USTRUCT(BlueprintType)
struct FRecordSalesSnapshot
{
    GENERATED_BODY();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString LabelId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RegionId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 UnitsSold = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime PeriodStart;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime PeriodEnd;
};

UCLASS()
class MUSICMANAGER_API UMarketManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION()
    void HandleMonthAdvanced(const FDateTime& NewDate);

    UFUNCTION()
    void RegisterRecordRelease(const FString& RecordId, const FString& LabelId);

    UFUNCTION()
    void ApplyRadioExposure(const FString& RegionId, const FString& ArtistId, float Intensity);

    UFUNCTION()
    void SimulateMonthlyRecordSales(const FDateTime& PeriodStart, const FDateTime& PeriodEnd);

    UFUNCTION(BlueprintCallable)
    void GetLastSalesForRecord(const FString& RecordId, TArray<FRecordSalesSnapshot>& OutSales) const;

private:
    UPROPERTY()
    TMap<FString, FMarketRegion> Regions;

    UPROPERTY()
    TSet<FString> ActiveRecords;

    UPROPERTY()
    TArray<FRecordSalesSnapshot> SalesHistory;

    UFUNCTION()
    void BindToTimeSubsystem();

    TWeakObjectPtr<class UGameTimeSubsystem> TimeSubsystem;
};
