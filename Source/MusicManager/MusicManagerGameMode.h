#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MusicManagerGameMode.generated.h"

class AMusicManagerPawn;
class AMusicManagerPlayerController;

/**
 * Game mode defining the default pawn and player controller for the MusicManager strategy game.
 */
UCLASS()
class AMusicManagerGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AMusicManagerGameMode();
};
