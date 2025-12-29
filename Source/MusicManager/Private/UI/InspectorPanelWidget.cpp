// File: Private/UI/InspectorPanelWidget.cpp
#include "UI/InspectorPanelWidget.h"

#include "UI/Inspectable.h"

void UInspectorPanelWidget::SetSelectedEntity(UObject* InEntity)
{
    if (SelectedEntity.Get() == InEntity)
    {
        return;
    }

    SelectedEntity = InEntity;

    if (!IsValid(InEntity))
    {
        ClearPanel();
        return;
    }

    OnSelectionChanged(InEntity);

    if (InEntity->GetClass()->ImplementsInterface(UInspectable::StaticClass()))
    {
        const IInspectable* Inspectable = Cast<IInspectable>(InEntity);
        if (Inspectable)
        {
            Inspectable->PopulateInspector(this);
        }
    }
}

void UInspectorPanelWidget::ClearPanel()
{
    SelectedEntity.Reset();
    OnSelectionCleared();
}
