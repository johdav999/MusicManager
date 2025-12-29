// File: Public/UI/MainCanvasHost.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainCanvasHost.generated.h"

class UPanelWidget;
class UUserWidget;

UENUM(BlueprintType)
enum class ECanvasState : uint8
{
    Overview,
    ArtistScene,
    StudioScene,
    OfficeScene,
    FinanceScreen,
    MarketingScreen
};

/**
 * Hosts the main canvas content (3D scenes or Layer-3 decision screens).
 * This widget never spawns other widgets; it only owns what the UI manager assigns.
 */
UCLASS(BlueprintType, Blueprintable)
class UMainCanvasHost : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="UI|Canvas")
    void SetCanvasState(ECanvasState NewState);

    UFUNCTION(BlueprintCallable, Category="UI|Canvas")
    ECanvasState GetCanvasState() const { return CanvasState; }

    /** Assign the active layer-3 widget (created by the UI manager). */
    UFUNCTION(BlueprintCallable, Category="UI|Canvas")
    void SetLayer3Widget(UUserWidget* InWidget);

    UFUNCTION(BlueprintCallable, Category="UI|Canvas")
    void ClearLayer3Widget();

    UFUNCTION(BlueprintCallable, Category="UI|Canvas")
    bool IsLayer3Active() const;

protected:
    UPROPERTY(meta=(BindWidgetOptional))
    UPanelWidget* Layer3WidgetHost;

    UPROPERTY(meta=(BindWidgetOptional))
    UWidget* SceneViewport;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="UI|Canvas")
    ECanvasState CanvasState = ECanvasState::Overview;

    UPROPERTY()
    TWeakObjectPtr<UUserWidget> ActiveLayer3Widget;

    /** Blueprint hook to drive visuals when the canvas state changes. */
    UFUNCTION(BlueprintImplementableEvent, Category="UI|Canvas")
    void OnCanvasStateChanged(ECanvasState NewState);
};
