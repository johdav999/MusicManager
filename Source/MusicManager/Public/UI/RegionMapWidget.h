#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/RegionMapButton.h"
#include "RegionMapWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRegionSelected, const FString&, RegionId);

UCLASS()
class MUSICMANAGER_API URegionMapWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

    // Designer-bound region buttons
    UPROPERTY(meta=(BindWidgetOptional)) URegionMapButton* RegionMapButton_AL;
    UPROPERTY(meta=(BindWidgetOptional)) URegionMapButton* RegionMapButton_AK;
    UPROPERTY(meta=(BindWidgetOptional)) URegionMapButton* RegionMapButton_AZ;
    UPROPERTY(meta=(BindWidgetOptional)) URegionMapButton* RegionMapButton_AR;
    UPROPERTY(meta=(BindWidgetOptional)) URegionMapButton* RegionMapButton_CA;
    UPROPERTY(meta=(BindWidgetOptional)) URegionMapButton* RegionMapButton_CO;
    UPROPERTY(meta=(BindWidgetOptional)) URegionMapButton* RegionMapButton_CT;
    UPROPERTY(meta=(BindWidgetOptional)) URegionMapButton* RegionMapButton_DE;
    UPROPERTY(meta=(BindWidgetOptional)) URegionMapButton* RegionMapButton_FL;

    /** Runtime lookup: RegionId → Button */
    TMap<FString, URegionMapButton*> RegionButtons;

    UFUNCTION()
    void HandleButtonClicked(const FString& RegionId);

private:
    void RegisterRegionButton(const FString& RegionId, URegionMapButton* Button);
    void BindRegionButtons();

public:
    /** Broadcast when a region is selected. */
    UPROPERTY(BlueprintAssignable, Category = "RegionMap")
    FOnRegionSelected OnRegionSelected;

    UFUNCTION(BlueprintCallable)
    void RefreshRegions();
};
