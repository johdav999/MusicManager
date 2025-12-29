// File: Public/UI/Inspectable.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Inspectable.generated.h"

class UInspectorPanelWidget;

UINTERFACE(Blueprintable)
class UInspectable : public UInterface
{
    GENERATED_BODY()
};

/**
 * Implemented by entities that can feed the persistent inspector panel.
 */
class IInspectable
{
    GENERATED_BODY()

public:
    /** Populate the inspector panel with read-only or navigation-focused data. */
    virtual void PopulateInspector(UInspectorPanelWidget* Panel) const = 0;
};
