#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AuditionTypes.h"
#include "ArtistHoverDetailWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * Layer-2 hover widget for signed artist details. Informational only.
 */
UCLASS()
class MUSICMANAGER_API UArtistHoverDetailWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void SetupFromArtistData(const FArtistData& ArtistData);

protected:
    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* ArtistNameText;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* GenreText;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* PopularityValueText;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* MomentumText;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* RecordsCountText;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* ContractStatusText;

    UPROPERTY(meta=(BindWidgetOptional))
    UProgressBar* PopularityBar;

    UPROPERTY(meta=(BindWidgetOptional))
    UProgressBar* PerformanceBar;

private:
    float ComputePopularityScore(const FArtistData& ArtistData) const;
};
