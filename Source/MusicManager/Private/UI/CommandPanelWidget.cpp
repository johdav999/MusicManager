#include "UI/CommandPanelWidget.h"

#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "UI/CommandItemWidget.h"

UCommandPanelWidget::UCommandPanelWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    BuildDefaultCommands();
}

void UCommandPanelWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!IsValid(CommandPanel))
    {
        UE_LOG(LogTemp, Error, TEXT("CommandPanelWidget: CommandPanel binding is missing. Cannot add commands."));
        return;
    }

    if (!CommandItemWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("CommandPanelWidget: CommandItemWidgetClass is not set."));
        return;
    }

    // Clear any previous widgets to avoid duplicates when the widget is reconstructed (e.g., PIE).
    CommandPanel->ClearChildren();
    CleanupChildBindings();

    GenerateCommandItems();
}

void UCommandPanelWidget::NativeDestruct()
{
    // Ensure delegates are removed before base destruct is called to avoid callbacks into destroyed widgets.
    CleanupChildBindings();

    Super::NativeDestruct();
}

void UCommandPanelWidget::HandleCommandClicked(const FString& CommandName)
{
    // Fire Blueprint event on the game thread so BP logic can safely switch on names.
    if (IsInGameThread())
    {
        OnCommandClicked(CommandName);
    }
}

void UCommandPanelWidget::HandleChildCommandClicked(const FString& CommandName)
{
    HandleCommandClicked(CommandName);
}

void UCommandPanelWidget::BuildDefaultCommands()
{
    if (DefaultCommands.Num() == 0)
    {
        DefaultCommands = {
            TEXT("Artists"),
            TEXT("Studio"),
            TEXT("Tours"),
            TEXT("Genre Tree"),
            TEXT("Radio"),
            TEXT("Charts"),
            TEXT("Financials"),
            TEXT("Marketing"),
            TEXT("Contracts")
        };
    }
}

void UCommandPanelWidget::GenerateCommandItems()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("CommandPanelWidget: World is null, cannot generate command items."));
        return;
    }

    SpawnedCommandItems.Reset();

    for (const FString& CommandName : DefaultCommands)
    {
        UCommandItemWidget* NewItem = CreateWidget<UCommandItemWidget>(World, CommandItemWidgetClass);
        if (!IsValid(NewItem))
        {
            UE_LOG(LogTemp, Warning, TEXT("CommandPanelWidget: Failed to create CommandItemWidget for %s."), *CommandName);
            continue;
        }

        NewItem->SetCommandName(CommandName);

        // Register for the click event in a thread-safe way by using UObject delegates instead of raw lambdas.
        NewItem->OnCommandItemClicked.RemoveDynamic(this, &UCommandPanelWidget::HandleChildCommandClicked);
        NewItem->OnCommandItemClicked.AddDynamic(this, &UCommandPanelWidget::HandleChildCommandClicked);

        CommandPanel->AddChildToHorizontalBox(NewItem);

        SpawnedCommandItems.Add(NewItem);
    }
}

void UCommandPanelWidget::CleanupChildBindings()
{
    for (TWeakObjectPtr<UCommandItemWidget>& ItemPtr : SpawnedCommandItems)
    {
        if (UCommandItemWidget* Item = ItemPtr.Get())
        {
            Item->OnCommandItemClicked.RemoveDynamic(this, &UCommandPanelWidget::HandleChildCommandClicked);
        }
    }

    SpawnedCommandItems.Reset();
}
