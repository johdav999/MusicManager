// File: Private/UI/HoverTooltipManagerWidget.cpp
#include "UI/HoverTooltipManagerWidget.h"

void UHoverTooltipManagerWidget::ShowTooltip(const FTooltipData& Data)
{
    ActiveTooltip = Data;
    OnTooltipShown(Data);
}

void UHoverTooltipManagerWidget::HideTooltip()
{
    ActiveTooltip = FTooltipData();
    OnTooltipHidden();
}
