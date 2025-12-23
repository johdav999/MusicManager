#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CommandItemWidget.generated.h"

class UBorder;
class UButton;
class UImage;

UENUM(BlueprintType)
enum class ECommandItemVisualState : uint8
{
    Normal,
    Hovered,
    Selected
};

/** Delegate fired when the command button is clicked. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCommandItemClicked, const FString&, CommandName);

/**
 * Widget that represents a single command button in the panel.
 * Handles its own button click and exposes a multicast delegate to be picked up
 * by parent widgets without creating unsafe references.
 */
UCLASS()
class MUSICMANAGER_API UCommandItemWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UCommandItemWidget(const FObjectInitializer& ObjectInitializer);

    //~ Begin UUserWidget Interface
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    //~ End UUserWidget Interface

    /** Event that external listeners can bind to. */
    UPROPERTY(BlueprintAssignable, Category = "Command Item")
    FOnCommandItemClicked OnCommandItemClicked;

    /** Sets the command name that will be forwarded to listeners when clicked. */
    UFUNCTION(BlueprintCallable, Category = "Command Item")
    void SetCommandName(const FString& InCommandName);

    /** Sets the icon brush for the foreground image (can be nullptr for text-only buttons). */
    UFUNCTION(BlueprintCallable, Category = "Command Item")
    void SetCommandIcon(const FSlateBrush& InBrush);

    /** Allows external code to drive the visual state (e.g., selection). */
    UFUNCTION(BlueprintCallable, Category = "Command Item")
    void SetVisualState(ECommandItemVisualState NewState);

    /** Returns the command name for selection logic. */
    UFUNCTION(BlueprintCallable, Category = "Command Item")
    FString GetCommandName() const { return CommandName; }

protected:
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

protected:
    /** Optional background surface that can be styled in Blueprint. */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UBorder* BackgroundBorder;

    /** Optional border outline used to show hover/selection state. */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UBorder* OutlineBorder;

    /** Image displayed with the command. Bound via the widget blueprint. */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* CommandImage;

    /** Button that triggers the command. Bound via the widget blueprint. */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* CommandButton;

    /** Normal state background color. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
    FLinearColor NormalBackgroundColor;

    /** Hover state background color. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
    FLinearColor HoverBackgroundColor;

    /** Selected state background color. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
    FLinearColor SelectedBackgroundColor;

    /** Color to apply to the outline or border when highlighted. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
    FLinearColor BorderColor;

    /** Thickness of the outline when selected/hovered. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
    float SelectedBorderThickness;

    /** Stored name so the button knows what to broadcast. */
    UPROPERTY(BlueprintReadWrite, Category = "Command Item")
    FString CommandName;

private:
    /** Tracks the current visual state. */
    UPROPERTY(Transient)
    ECommandItemVisualState CurrentVisualState = ECommandItemVisualState::Normal;

    /** Bound click handler. Kept private to avoid accidental external invocation. */
    UFUNCTION()
    void HandleButtonClicked();

    /** Helper that ensures bindings happen exactly once. */
    void BindButtonEvents();

    /** Helper to clear delegates when the widget goes away. */
    void UnbindButtonEvents();

    void ApplyBackgroundColor(const FLinearColor& NewColor);
    void ApplyBorderStyle(bool bShowBorder) const;
    bool IsSelected() const { return CurrentVisualState == ECommandItemVisualState::Selected; }
};
