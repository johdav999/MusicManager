#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MusicManagerPawn.generated.h"

class UCameraComponent;
class USceneComponent;
class USpringArmComponent;

/**
 * Simple pawn used as the player's in-world representative for the strategy game.
 */
UCLASS()
class AMusicManagerPawn : public APawn
{
    GENERATED_BODY()

public:
    AMusicManagerPawn();

protected:
    /** Root scene component for the pawn. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* RootSceneComponent;

    /** Spring arm to position the camera. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USpringArmComponent* SpringArmComponent;

    /** Camera component used to view the board. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UCameraComponent* CameraComponent;
};
