#pragma once

#include "Components/Widget.h"
#include "MusicGoldSlider.generated.h"

class SMusicGoldSlider;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMusicGoldSliderValueChanged, float, Value);

/**
 * Custom-rendered gold-on-black slider for contract negotiation controls.
 */
UCLASS(BlueprintType, Blueprintable)
class MUSICMANAGER_API UMusicGoldSlider : public UWidget
{
    GENERATED_BODY()

public:
    UMusicGoldSlider(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(BlueprintAssignable, Category = "Music Slider|Event")
    FOnMusicGoldSliderValueChanged OnValueChanged;

    UFUNCTION(BlueprintCallable, Category = "Music Slider")
    void SetValue(float InValue);

    UFUNCTION(BlueprintCallable, Category = "Music Slider")
    float GetValue() const { return Value; }

    UFUNCTION(BlueprintCallable, Category = "Music Slider")
    void SetMinValue(float InMinValue);

    UFUNCTION(BlueprintCallable, Category = "Music Slider")
    void SetMaxValue(float InMaxValue);

    UFUNCTION(BlueprintCallable, Category = "Music Slider")
    void SetStepSize(float InStepSize);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music Slider", meta = (ClampMin = "0.0"))
    float Value = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music Slider")
    float MinValue = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music Slider")
    float MaxValue = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music Slider", meta = (ClampMin = "0.0"))
    float StepSize = 0.f;

    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void SynchronizeProperties() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
    void HandleSlateValueChanged(float InValue);
    float NormalizeValue(float InValue) const;

    TSharedPtr<SMusicGoldSlider> SlateSlider;
};
