#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LayoutWidget.generated.h"

/**
 * Base layout widget for the MusicManager game's user interface.
 */
UCLASS(Abstract, Blueprintable)
class ULayoutWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    ULayoutWidget(const FObjectInitializer& ObjectInitializer);

    /** Called once the widget has been added to the viewport. */
    UFUNCTION(BlueprintNativeEvent, Category = "Layout")
    void HandleLayoutInitialized();
    virtual void HandleLayoutInitialized_Implementation();

protected:
    virtual void NativeConstruct() override;
};
