// File: Public/EventTickerWidget.h
#pragma once

#include "Blueprint/UserWidget.h"
#include "EventTickerWidget.generated.h"

/**
 * Widget notified by the event subsystem on each timer tick.
 */
UCLASS()
class UEventTickerWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UEventTickerWidget(const FObjectInitializer& ObjectInitializer);

    /** Called when the event subsystem fires its timer. Implement in Blueprint if desired. */
    UFUNCTION(BlueprintImplementableEvent, Category="EventSubsystem")
    void OnEventSubsystemTick();

protected:
    /** Default fallback invoked when no Blueprint override is provided. */
    virtual void OnEventSubsystemTick_Implementation();
};
