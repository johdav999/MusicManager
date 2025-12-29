// File: Public/UI/InspectorPanelWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectorPanelWidget.generated.h"

class UInspectable;

/**
 * Persistent inspector panel that mirrors the currently selected entity.
 * It is read-only and only exposes navigation actions into Layer-3 screens.
 */
UCLASS(BlueprintType, Blueprintable)
class UInspectorPanelWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="UI|Inspector")
    void SetSelectedEntity(UObject* InEntity);

    UFUNCTION(BlueprintCallable, Category="UI|Inspector")
    void ClearPanel();

protected:
    UPROPERTY(BlueprintReadOnly, Category="UI|Inspector")
    TWeakObjectPtr<UObject> SelectedEntity;

    /** Blueprint hook to update layout when selection changes. */
    UFUNCTION(BlueprintImplementableEvent, Category="UI|Inspector")
    void OnSelectionChanged(UObject* InEntity);

    /** Blueprint hook to clear inspector visuals. */
    UFUNCTION(BlueprintImplementableEvent, Category="UI|Inspector")
    void OnSelectionCleared();
};
