// File: Private/EventTickerWidget.cpp
#include "EventTickerWidget.h"

UEventTickerWidget::UEventTickerWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UEventTickerWidget::OnEventSubsystemTick_Implementation()
{
    // Intentionally left blank for C++ subclasses without Blueprint implementations.
}
