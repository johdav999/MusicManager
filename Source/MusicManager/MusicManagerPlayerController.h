#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MusicManagerPlayerController.generated.h"

class ULayoutWidget;

/**
 * Custom player controller responsible for initializing player-specific systems.
 */
UCLASS()
class AMusicManagerPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AMusicManagerPlayerController();

protected:
    virtual void BeginPlay() override;

    /** Base layout widget class added to the viewport at BeginPlay. */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<ULayoutWidget> LayoutWidgetClass;

    /** Instance of the layout widget added to the viewport. */
    UPROPERTY()
    TObjectPtr<ULayoutWidget> LayoutWidgetInstance;
};
