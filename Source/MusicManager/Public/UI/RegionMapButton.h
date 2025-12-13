#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
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

    /** Base fill color for the button. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
    FLinearColor ButtonColor = FLinearColor::White;

    /** Hover color for the button. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
    FLinearColor HoverColor = FLinearColor::White;

    /** Color applied to the button text. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
    FLinearColor TextColor = FLinearColor::White;

    /** Font size for the button text. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
    int32 FontSize = 24;

    /** Optional button size applied when a root SizeBox is present. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
    FVector2D ButtonSize = FVector2D::ZeroVector;

    /** Optional button bound in the designer. */
    UPROPERTY(meta = (BindWidgetOptional))
    UButton* RegionButton;

    /** Optional text bound in the designer. */
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* RegionText;

    /** Optional root size box to control the button dimensions. */
    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* RootSizeBox;

    /** Broadcast when this region button is clicked. */
    UPROPERTY(BlueprintAssignable, Category = "RegionMap")
    FOnRegionClicked OnRegionClicked;

    /** Initialize the region id and update the text. */
    UFUNCTION(BlueprintCallable, Category = "Region")
    void InitializeRegion(const FString& InRegionId);

    /** Apply all configured style properties to the underlying widgets. */
    UFUNCTION(BlueprintCallable, Category = "Style")
    void ApplyStyle();

    /** Set the button base color and update the style. */
    UFUNCTION(BlueprintCallable, Category = "Style")
    void SetButtonColor(const FLinearColor& InColor);

    /** Set the button hover color and update the style. */
    UFUNCTION(BlueprintCallable, Category = "Style")
    void SetHoverColor(const FLinearColor& InColor);

    /** Set the text color and update the style. */
    UFUNCTION(BlueprintCallable, Category = "Style")
    void SetTextColor(const FLinearColor& InColor);

    /** Set the text font size and update the style. */
    UFUNCTION(BlueprintCallable, Category = "Style")
    void SetFontSize(int32 InSize);

    /** Set the desired button size and update the style. */
    UFUNCTION(BlueprintCallable, Category = "Style")
    void SetButtonSize(const FVector2D& InSize);

    FString GetRegionId() const { return RegionId; }

protected:
    /** Handle button clicked event. */
    UFUNCTION()
    void HandleClicked();

private:
    UPROPERTY()
    FString RegionId;
};
