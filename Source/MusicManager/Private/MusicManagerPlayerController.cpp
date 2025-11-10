#include "MusicManagerPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "EventSubsystem.h"
#include "Layout.h"

AMusicManagerPlayerController::AMusicManagerPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
}

void AMusicManagerPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController())
    {
        return;
    }

    TSubclassOf<ULayout> ClassToUse = LayoutClass;
    if (!ClassToUse)
    {
        UClass* DefaultClass = ULayout::StaticClass();
        if (DefaultClass && !DefaultClass->HasAnyClassFlags(CLASS_Abstract))
        {
            ClassToUse = DefaultClass;
        }
    }

    if (!ClassToUse || ClassToUse->HasAnyClassFlags(CLASS_Abstract))
    {
        return;
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        LayoutInstance = CreateWidget<ULayout>(GameInstance, ClassToUse);
        if (!LayoutInstance)
        {
            return;
        }

        LayoutInstance->AddToViewport();

        if (UEventSubsystem* EventSubsystem = GameInstance->GetSubsystem<UEventSubsystem>())
        {
            EventSubsystem->RegisterLayout(LayoutInstance);
        }

        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(InputMode);
        bShowMouseCursor = true;
    }
}
