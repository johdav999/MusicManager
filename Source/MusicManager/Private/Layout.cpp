// File: Private/Layout.cpp
#include "Layout.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"

ULayout::ULayout(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

UUserWidget* ULayout::GetChildByNameOrClass(FName WidgetName, TSubclassOf<UUserWidget> WidgetClass) const
{
    check(IsInGameThread());
    ensureMsgf(IsInGameThread(), TEXT("UI access must be on game thread"));

    if (!WidgetTree)
    {
        return nullptr;
    }

    if (!WidgetName.IsNone())
    {
        if (UWidget* NamedWidget = WidgetTree->FindWidget(WidgetName))
        {
            if (UUserWidget* NamedUserWidget = Cast<UUserWidget>(NamedWidget))
            {
                return NamedUserWidget;
            }
        }
    }

    if (WidgetClass)
    {
        UUserWidget* FoundWidget = nullptr;
        WidgetTree->ForEachWidget([WidgetClass, &FoundWidget](UWidget* Widget)
        {
            if (!FoundWidget && Widget && Widget->IsA(WidgetClass))
            {
                FoundWidget = Cast<UUserWidget>(Widget);
            }
        });

        if (FoundWidget)
        {
            return FoundWidget;
        }
    }

    return nullptr;
}
