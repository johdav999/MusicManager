#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"

#include "MarketRegion.generated.h"

UENUM(BlueprintType)
enum class EMarketRegionType : uint8
{
    Country,
    State,
    CityCluster
};

USTRUCT(BlueprintType)
struct FMarketSegmentProfile : public FTableRowBase
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
struct FMarketRegion : public FTableRowBase
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
    TArray<FString> SegmentIds;

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly)
    TArray<FMarketSegmentProfile> Segments;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RadioReach = 0.5f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TMap<FString, float> RecentArtistExposure;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TMap<FString, float> RecentRecordExposure;
};
