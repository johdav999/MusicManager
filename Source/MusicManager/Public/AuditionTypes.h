#pragma once

#include "CoreMinimal.h"
#include "AuditionTypes.generated.h"

USTRUCT(BlueprintType)
struct FArtistData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ArtistName = TEXT("");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float PerformanceScore = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float StagePresence = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AudienceEngagement = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float VocalQuality = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SongwritingQuality = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Genre = TEXT("");
};

USTRUCT(BlueprintType)
struct FDealData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ArtistName = TEXT("");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SignUpBonus = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 NumOfRecords = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RoyaltyRate = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ContractYears = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString DealNotes = TEXT("");
};

USTRUCT(BlueprintType)
struct FAuditionEvent
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString VenueName = TEXT("");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString City = TEXT("");

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime Date = FDateTime::Now();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FArtistData ArtistData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bSignedArtist = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDealData DealData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Outcome = TEXT("");
};
