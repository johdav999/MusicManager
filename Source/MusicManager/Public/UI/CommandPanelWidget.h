#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CommandPanelWidget.generated.h"

class UCommandItemWidget;
class UHorizontalBox;
class UImage;

/**
 * Widget that hosts a set of command buttons. Responsible for dynamically
 * creating child widgets and safely handling their click events.
 */
UCLASS()
class MUSICMANAGER_API UCommandPanelWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UCommandPanelWidget(const FObjectInitializer& ObjectInitializer);

    //~ Begin UUserWidget Interface
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    //~ End UUserWidget Interface

    /** Blueprint event to react to command selections. Designers can implement switch logic in Blueprint. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Command Panel")
    void OnCommandClicked(const FString& CommandName);

    /** Optional Blueprint native function to allow C++ or BP overrides if desired. */
    UFUNCTION(BlueprintCallable, Category = "Command Panel")
    virtual void HandleCommandClicked(const FString& CommandName);

protected:
    /** Panel that stores the command item widgets. */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UHorizontalBox* CommandPanel;

    /** Background image closest to the user. */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* BackgroundImagePrimary;

    /** Secondary background for layered look. */
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UImage* BackgroundImageSecondary;

    /** Class used to spawn command items. Exposed for design-time override. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Command Panel")
    TSubclassOf<UCommandItemWidget> CommandItemWidgetClass;

private:
    /** Track spawned widgets through weak pointers to avoid lifetime issues. */
    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<UCommandItemWidget>> SpawnedCommandItems;

    /** List of default command names to seed the panel. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Command Panel", meta = (AllowPrivateAccess = "true"))
    TArray<FString> DefaultCommands;

    /** Called when a child notifies us of a click. */
    UFUNCTION()
    void HandleChildCommandClicked(const FString& CommandName);

    /** Helper to build the default list safely. */
    void BuildDefaultCommands();

    /** Create widgets for all commands in DefaultCommands. */
    void GenerateCommandItems();

    /** Utility to remove bindings when this widget goes away. */
    void CleanupChildBindings();
};
