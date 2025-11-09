#include "MusicManagerGameMode.h"

#include "MusicManagerPawn.h"
#include "MusicManagerPlayerController.h"

AMusicManagerGameMode::AMusicManagerGameMode()
{
    DefaultPawnClass = AMusicManagerPawn::StaticClass();
    PlayerControllerClass = AMusicManagerPlayerController::StaticClass();
}
