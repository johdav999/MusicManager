#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/SoftObjectPath.h"
#include "CommandPanelWidget.generated.h"

/** Description of a single command entry shown in the panel. */
USTRUCT(BlueprintType)
struct FCommandDefinition
{
    GENERATED_BODY()

    /** Display name of the command used for binding click actions. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
    FString CommandName;

    /** Optional icon that can be streamed asynchronously via soft reference. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
    TSoftObjectPtr<UTexture2D> IconTexture;
};

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

protected:
    /** Data describing each command along with its optional icon. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Commands")
    TArray<FCommandDefinition> CommandDefinitions;

private:
    /** Track spawned widgets through weak pointers to avoid lifetime issues. */
    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<UCommandItemWidget>> SpawnedCommandItems;

    /** Indicates whether the panel is currently constructed and ready to accept UI updates. */
    UPROPERTY(Transient)
    bool bPanelActive = false;

    /** Called when a child notifies us of a click. */
    UFUNCTION()
    void HandleChildCommandClicked(const FString& CommandName);

    /** Helper to build the default list safely. */
    void BuildDefaultCommands();

    /** Create widgets for all commands in CommandDefinitions. */
    void GenerateCommandItems();

    /** Internal callback used to update the definition once an icon load completes. */
    void OnIconLoadedInternal(const FSoftObjectPath& LoadedPath, UObject* LoadedObject, FCommandDefinition Definition);

    /** Callback when an icon asset finishes streaming in. */
    void HandleIconLoaded(FCommandDefinition Definition);

    /** Applies the resolved texture to an item using a transient brush. */
    void ApplyIconToItem(UCommandItemWidget* Item, UTexture2D* Texture);

    /** Utility to remove bindings when this widget goes away. */
    void CleanupChildBindings();
};
