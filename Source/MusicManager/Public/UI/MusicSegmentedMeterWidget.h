#pragma once

#include "Components/Widget.h"
#include "MusicSegmentedMeterWidget.generated.h"

/**
 * Compact AAA-style segmented meter used by HUD panels instead of a smooth progress bar.
 */
UCLASS(BlueprintType, Blueprintable)
class MUSICMANAGER_API UMusicSegmentedMeterWidget : public UWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Meter")
    void SetPercent(float InPercent);

    UFUNCTION(BlueprintCallable, Category="Meter")
    float GetPercent() const { return Percent; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meter", meta=(ClampMin="1", ClampMax="24"))
    int32 SegmentCount = 12;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meter")
    FLinearColor FilledColor = FLinearColor(1.f, 0.827f, 0.353f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meter")
    FLinearColor EmptyColor = FLinearColor(0.13f, 0.125f, 0.105f, 0.95f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Meter")
    FLinearColor OutlineColor = FLinearColor(0.48f, 0.32f, 0.09f, 0.85f);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void SynchronizeProperties() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
    UPROPERTY(EditAnywhere, Category="Meter", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Percent = 0.f;

    TSharedPtr<class SMusicSegmentedMeter> MeterWidget;
};
