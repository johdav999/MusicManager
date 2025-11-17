#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CommandItemWidget.generated.h"

class UButton;
class UImage;

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

protected:
    /** Image displayed with the command. Bound via the widget blueprint. */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* CommandImage;

    /** Button that triggers the command. Bound via the widget blueprint. */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* CommandButton;

    /** Stored name so the button knows what to broadcast. */
    UPROPERTY(BlueprintReadWrite, Category = "Command Item")
    FString CommandName;

private:
    /** Bound click handler. Kept private to avoid accidental external invocation. */
    UFUNCTION()
    void HandleButtonClicked();

    /** Helper that ensures bindings happen exactly once. */
    void BindButtonEvents();

    /** Helper to clear delegates when the widget goes away. */
    void UnbindButtonEvents();
};
