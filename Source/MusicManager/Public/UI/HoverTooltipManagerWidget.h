// File: Public/UI/HoverTooltipManagerWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/TooltipData.h"
#include "HoverTooltipManagerWidget.generated.h"

/**
 * Layer-2 hover tooltip presenter. Contains only view logic.
 */
UCLASS(BlueprintType, Blueprintable)
class UHoverTooltipManagerWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="UI|Tooltip")
    void ShowTooltip(const FTooltipData& Data);

    UFUNCTION(BlueprintCallable, Category="UI|Tooltip")
    void HideTooltip();

protected:
    UPROPERTY(BlueprintReadOnly, Category="UI|Tooltip")
    FTooltipData ActiveTooltip;

    UFUNCTION(BlueprintImplementableEvent, Category="UI|Tooltip")
    void OnTooltipShown(const FTooltipData& Data);

    UFUNCTION(BlueprintImplementableEvent, Category="UI|Tooltip")
    void OnTooltipHidden();
};
