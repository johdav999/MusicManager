#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MusicManagerPlayerController.generated.h"

class ULayout;

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
    UPROPERTY(EditDefaultsOnly, Category="UI")
    TSubclassOf<class ULayout> LayoutClass = nullptr;

    /** Instance of the layout widget added to the viewport. */
    UPROPERTY(Transient)
    TObjectPtr<class ULayout> LayoutInstance = nullptr;
};
