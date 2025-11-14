#pragma once

#include "CoreMinimal.h"
#include "FArtistDealTerms.generated.h"

USTRUCT(BlueprintType)
struct FArtistDealTerms
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Deal")
    FString ArtistId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Deal")
    int32 ContractYears = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Deal")
    int32 NumRecords = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Deal")
    float RoyaltyRate = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Deal")
    float SignUpBonus = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Deal")
    bool bExclusive = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Deal")
    FDateTime ProposedStartDate = FDateTime();
};
