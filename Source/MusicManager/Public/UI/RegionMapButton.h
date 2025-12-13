#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "RegionMapButton.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRegionClicked, const FString&, RegionId);

/**
 * A simple widget representing a region button on the map.
 */
UCLASS(BlueprintType, Blueprintable)
class MUSICMANAGER_API URegionMapButton : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    /** Optional button bound in the designer. */
    UPROPERTY(meta = (BindWidgetOptional))
    UButton* RegionButton;

    /** Optional text bound in the designer. */
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* RegionText;

    /** Broadcast when this region button is clicked. */
    UPROPERTY(BlueprintAssignable, Category = "RegionMap")
    FOnRegionClicked OnRegionClicked;

    /** Initialize the region id and update the text. */
    UFUNCTION(BlueprintCallable, Category = "Region")
    void InitializeRegion(const FString& InRegionId);

    FString GetRegionId() const { return RegionId; }

protected:
    /** Handle button clicked event. */
    UFUNCTION()
    void HandleClicked();

private:
    UPROPERTY()
    FString RegionId;
};
