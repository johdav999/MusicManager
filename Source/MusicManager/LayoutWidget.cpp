#include "LayoutWidget.h"

ULayoutWidget::ULayoutWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void ULayoutWidget::NativeConstruct()
{
    Super::NativeConstruct();
    HandleLayoutInitialized();
}

void ULayoutWidget::HandleLayoutInitialized_Implementation()
{
    // Base implementation can be extended in Blueprint to build the game's UI.
}
