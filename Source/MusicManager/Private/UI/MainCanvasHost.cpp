// File: Private/UI/MainCanvasHost.cpp
#include "UI/MainCanvasHost.h"

#include "Components/PanelWidget.h"

void UMainCanvasHost::SetCanvasState(ECanvasState NewState)
{
    if (CanvasState == NewState)
    {
        return;
    }

    CanvasState = NewState;
    OnCanvasStateChanged(NewState);
}

void UMainCanvasHost::SetLayer3Widget(UUserWidget* InWidget)
{
    if (!Layer3WidgetHost)
    {
        return;
    }

    ClearLayer3Widget();

    if (!IsValid(InWidget))
    {
        return;
    }

    Layer3WidgetHost->AddChild(InWidget);
    ActiveLayer3Widget = InWidget;
}

void UMainCanvasHost::ClearLayer3Widget()
{
    if (!Layer3WidgetHost)
    {
        ActiveLayer3Widget.Reset();
        return;
    }

    Layer3WidgetHost->ClearChildren();
    ActiveLayer3Widget.Reset();
}

bool UMainCanvasHost::IsLayer3Active() const
{
    return ActiveLayer3Widget.IsValid();
}
