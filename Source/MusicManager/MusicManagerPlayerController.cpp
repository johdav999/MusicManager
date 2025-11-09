#include "MusicManagerPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "LayoutWidget.h"

AMusicManagerPlayerController::AMusicManagerPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
}

void AMusicManagerPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalController())
    {
        if (!LayoutWidgetClass)
        {
            LayoutWidgetClass = ULayoutWidget::StaticClass();
        }

        if (LayoutWidgetClass)
        {
            LayoutWidgetInstance = CreateWidget<ULayoutWidget>(this, LayoutWidgetClass);
            if (LayoutWidgetInstance)
            {
                LayoutWidgetInstance->AddToViewport();
            }
        }
    }
}
