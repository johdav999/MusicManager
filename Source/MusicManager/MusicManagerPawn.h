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
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

class AMusicManagerPawn : public APawn
{
    GENERATED_BODY()

public:
    AMusicManagerPawn();

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

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

    /** Movement speed for the camera pawn. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    float MovementSpeed = 800.0f;

    /** Input mapping context that provides movement bindings. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputMappingContext* MovementMappingContext;

    /** Action used to move the pawn forward/backward. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* MoveForwardAction;

    /** Action used to move the pawn left/right. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    UInputAction* MoveRightAction;

private:
    void MoveForward(const FInputActionValue& Value);
    void MoveRight(const FInputActionValue& Value);
};
