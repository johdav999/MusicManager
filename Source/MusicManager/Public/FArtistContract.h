#pragma once

#include "CoreMinimal.h"
#include "AuditionTypes.h"
#include "FArtistDealTerms.h"
#include "FArtistContract.generated.h"

USTRUCT(BlueprintType)
struct FArtistContract
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contract")
    FString ArtistId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contract")
    FArtistData ArtistData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contract")
    FArtistDealTerms Terms;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contract")
    FDateTime StartDate = FDateTime();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contract")
    FDateTime EndDate = FDateTime();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contract")
    int32 RecordsDelivered = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contract")
    bool bContractActive = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contract")
    float LifetimeRevenue = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contract")
    float LifetimeCost = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contract")
    float LastRoyaltyPayment = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contract")
    float MonthlyUpkeepCost = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contract")
    float CumulativeRoyaltyPaid = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contract")
    int32 MonthsActive = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contract")
    float PerformanceMomentum = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Contract")
    float ProductionProgress = 0.f;
};
