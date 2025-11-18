#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "GameTimeSubsystem.h"
#include "DateWidget.generated.h"

/**
 * Widget that displays the current simulated month and year from the game time subsystem.
 */
UCLASS()
class MUSICMANAGER_API UDateWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

protected:
    /** Text block bound in the designer for displaying the formatted date. */
    UPROPERTY(meta = (BindWidget))
    UTextBlock* DateText;

    /** Optional image that can be set in the designer for styling. */
    UPROPERTY(meta = (BindWidgetOptional))
    UImage* DateImage;

    UFUNCTION()
    void HandleMonthAdvanced(const FDateTime& NewDate);
};
