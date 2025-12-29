// File: Public/UI/TooltipData.h
#pragma once

#include "CoreMinimal.h"
#include "TooltipData.generated.h"

/**
 * Lightweight data payload for layer-2 hover tooltips.
 */
USTRUCT(BlueprintType)
struct FTooltipData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI")
    FText Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI")
    FText Body;
};
