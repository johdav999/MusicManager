#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "UI/RegionMapButton.h"
#include "RegionMapWidget.generated.h"

class UMarketManagerSubsystem;

/**
 * Widget that creates region buttons for each market region.
 */
UCLASS(BlueprintType, Blueprintable)
class MUSICMANAGER_API URegionMapWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    URegionMapWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;

protected:
    /** Canvas panel used as the root container for region buttons. */
    UPROPERTY(meta = (BindWidget))
    UCanvasPanel* RootCanvas;

    /** Map of region ids to their corresponding buttons, instanced for Blueprint editing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "RegionMap")
    TMap<FString, URegionMapButton*> RegionButtons;

    /** Rebuild the widget tree and populate buttons for all regions. */
    void RebuildWidgetTree();
};
