#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "RegionMapButton.generated.h"

/**
 * A simple widget representing a region button on the map.
 */
UCLASS(BlueprintType, Blueprintable)
class MUSICMANAGER_API URegionMapButton : public UUserWidget
{
    GENERATED_BODY()

public:
    URegionMapButton(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;

    /** Optional button bound in the designer. */
    UPROPERTY(meta = (BindWidget))
    UButton* RegionButton;

    /** Optional text bound in the designer. */
    UPROPERTY(meta = (BindWidget))
    UTextBlock* RegionText;

    /** Identifier for this region. */
    UPROPERTY(BlueprintReadOnly, Category = "Region")
    FString RegionId;

    /** Initialize the region id and update the text. */
    UFUNCTION(BlueprintCallable, Category = "Region")
    void InitializeRegion(const FString& InRegionId);

protected:
    /** Create default widgets if not provided by Blueprint. */
    void EnsureDefaultWidgets();
};
