#include "MusicManagerPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "UObject/UObjectGlobals.h"

AMusicManagerPawn::AMusicManagerPawn()
{
    PrimaryActorTick.bCanEverTick = false;

    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    SetRootComponent(RootSceneComponent);

    SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArmComponent->SetupAttachment(RootSceneComponent);
    SpringArmComponent->TargetArmLength = 800.0f;
    SpringArmComponent->bDoCollisionTest = false;
    SpringArmComponent->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));

    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);
    CameraComponent->bUsePawnControlRotation = false;

    MovementMappingContext = CreateDefaultSubobject<UInputMappingContext>(TEXT("MovementMappingContext"));

    MoveForwardAction = CreateDefaultSubobject<UInputAction>(TEXT("MoveForwardAction"));
    MoveForwardAction->ValueType = EInputActionValueType::Axis1D;
    MovementMappingContext->MapKey(MoveForwardAction, EKeys::W);
    FEnhancedInputActionKeyMapping& MoveBackwardMapping = MovementMappingContext->MapKey(MoveForwardAction, EKeys::S);
    MoveBackwardMapping.Modifiers.Add(NewObject<UInputModifierNegate>(this));

    MoveRightAction = CreateDefaultSubobject<UInputAction>(TEXT("MoveRightAction"));
    MoveRightAction->ValueType = EInputActionValueType::Axis1D;
    MovementMappingContext->MapKey(MoveRightAction, EKeys::D);
    FEnhancedInputActionKeyMapping& MoveLeftMapping = MovementMappingContext->MapKey(MoveRightAction, EKeys::A);
    MoveLeftMapping.Modifiers.Add(NewObject<UInputModifierNegate>(this));
}

void AMusicManagerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (MovementMappingContext)
    {
        if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
        {
            if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
            {
                if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
                {
                    Subsystem->AddMappingContext(MovementMappingContext, 0);
                }
            }
        }
    }

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (MoveForwardAction)
        {
            EnhancedInput->BindAction(MoveForwardAction, ETriggerEvent::Triggered, this, &AMusicManagerPawn::MoveForward);
            EnhancedInput->BindAction(MoveForwardAction, ETriggerEvent::Completed, this, &AMusicManagerPawn::MoveForward);
        }

        if (MoveRightAction)
        {
            EnhancedInput->BindAction(MoveRightAction, ETriggerEvent::Triggered, this, &AMusicManagerPawn::MoveRight);
            EnhancedInput->BindAction(MoveRightAction, ETriggerEvent::Completed, this, &AMusicManagerPawn::MoveRight);
        }
    }
}

void AMusicManagerPawn::MoveForward(const FInputActionValue& Value)
{
    const float AxisValue = Value.Get<float>();
    if (FMath::IsNearlyZero(AxisValue))
    {
        return;
    }

    const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
    if (DeltaSeconds <= 0.f)
    {
        return;
    }

    const FVector MovementDelta = GetActorForwardVector() * AxisValue * MovementSpeed * DeltaSeconds;
    AddActorWorldOffset(MovementDelta, false);
}

void AMusicManagerPawn::MoveRight(const FInputActionValue& Value)
{
    const float AxisValue = Value.Get<float>();
    if (FMath::IsNearlyZero(AxisValue))
    {
        return;
    }

    const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
    if (DeltaSeconds <= 0.f)
    {
        return;
    }

    const FVector MovementDelta = GetActorRightVector() * AxisValue * MovementSpeed * DeltaSeconds;
    AddActorWorldOffset(MovementDelta, false);
}
