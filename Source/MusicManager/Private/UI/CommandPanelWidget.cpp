#include "UI/CommandPanelWidget.h"

#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "UI/CommandItemWidget.h"
#include "Async/Async.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"
#include "UIManagerSubsystem.h"

#include "UObject/SoftObjectPath.h"

UCommandPanelWidget::UCommandPanelWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , bPanelActive(false)
{
    BuildDefaultCommands();
}

void UCommandPanelWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BuildDefaultCommands();

    bPanelActive = true;

    SelectedItem.Reset();

    if (!IsValid(CommandPanel))
    {
        UE_LOG(LogTemp, Error, TEXT("CommandPanelWidget: CommandPanel binding is missing. Cannot add commands."));
        bPanelActive = false;
        return;
    }

    if (!CommandItemWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("CommandPanelWidget: CommandItemWidgetClass is not set."));
        bPanelActive = false;
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

    bPanelActive = false;

    Super::NativeDestruct();
}

void UCommandPanelWidget::HandleCommandClicked(const FString& CommandName)
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<UCommandPanelWidget> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, CommandName]()
        {
            if (UCommandPanelWidget* Strong = WeakThis.Get())
            {
                Strong->HandleCommandClicked(CommandName);
            }
        });
        return;
    }

    UWorld* World = GetWorld();
    if (World)
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            if (UUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UUIManagerSubsystem>())
            {
                UIManager->HandleCommandAction(CommandName);
            }
        }
    }

    OnCommandClicked(CommandName);
}

void UCommandPanelWidget::HandleChildCommandClicked(const FString& CommandName)
{
    UCommandItemWidget* ClickedItem = nullptr;

    for (TWeakObjectPtr<UCommandItemWidget>& ItemPtr : SpawnedCommandItems)
    {
        if (UCommandItemWidget* Item = ItemPtr.Get())
        {
            if (Item->GetCommandName() == CommandName)
            {
                ClickedItem = Item;
                break;
            }
        }
    }

    UpdateSelection(ClickedItem);

    HandleCommandClicked(CommandName);
}

void UCommandPanelWidget::BuildDefaultCommands()
{
    const TArray<FString> ExpectedCommands = {
        TEXT("Audition"),
        TEXT("Market"),
        TEXT("Contracts"),
        TEXT("Studio"),
        TEXT("Charts")
    };

    bool bAlreadyHasDockCommands = CommandDefinitions.Num() == ExpectedCommands.Num();
    for (int32 Index = 0; bAlreadyHasDockCommands && Index < ExpectedCommands.Num(); ++Index)
    {
        bAlreadyHasDockCommands = CommandDefinitions[Index].CommandName == ExpectedCommands[Index];
    }

    if (bAlreadyHasDockCommands)
    {
        return;
    }

    auto MakeDefinition = [](const TCHAR* Name, const TCHAR* IconPath) -> FCommandDefinition
    {
        FCommandDefinition Definition;
        Definition.CommandName = Name;
        if (IconPath && *IconPath)
        {
            Definition.IconTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconPath));
        }
        return Definition;
    };

    CommandDefinitions = {
        MakeDefinition(TEXT("Audition"), TEXT("/Game/GUI/HUD/CommandDock/BottomCommandDockIcon_Audition.BottomCommandDockIcon_Audition")),
        MakeDefinition(TEXT("Market"), TEXT("/Game/GUI/HUD/CommandDock/BottomCommandDockIcon_Market.BottomCommandDockIcon_Market")),
        MakeDefinition(TEXT("Contracts"), TEXT("/Game/GUI/HUD/CommandDock/BottomCommandDockIcon_Contracts.BottomCommandDockIcon_Contracts")),
        MakeDefinition(TEXT("Studio"), TEXT("/Game/GUI/HUD/CommandDock/BottomCommandDockIcon_Studio.BottomCommandDockIcon_Studio")),
        MakeDefinition(TEXT("Charts"), TEXT("/Game/GUI/HUD/CommandDock/BottomCommandDockIcon_Charts.BottomCommandDockIcon_Charts"))
    };
}

void UCommandPanelWidget::GenerateCommandItems()
{
    if (CommandDefinitions.Num() == 0)
    {
        BuildDefaultCommands();
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("CommandPanelWidget: World is null, cannot generate command items."));
        return;
    }

    SpawnedCommandItems.Reset();

    for (FCommandDefinition& Definition : CommandDefinitions)
    {
        if (Definition.IconTexture.IsNull())
        {
            HandleIconLoaded(Definition);
            continue;
        }

        if (Definition.IconTexture.IsValid())
        {
            HandleIconLoaded(Definition);
            continue;
        }

        Definition.IconTexture.LoadAsync(
            FLoadSoftObjectPathAsyncDelegate::CreateUObject(
                this,
                &UCommandPanelWidget::OnIconLoadedInternal,
                Definition));
    }
}

void UCommandPanelWidget::OnIconLoadedInternal(const FSoftObjectPath& LoadedPath, UObject* LoadedObject, FCommandDefinition Definition)
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<UCommandPanelWidget> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, LoadedPath, LoadedObject, Definition]()
        {
            if (UCommandPanelWidget* StrongThis = WeakThis.Get())
            {
                StrongThis->OnIconLoadedInternal(LoadedPath, LoadedObject, Definition);
            }
        });
        return;
    }

    if (!IsValid(this) || !bPanelActive)
    {
        return;
    }

    UTexture2D* LoadedTexture = Cast<UTexture2D>(LoadedObject);
    if (!LoadedTexture)
    {
        UE_LOG(LogTemp, Warning, TEXT("CommandPanelWidget: Icon load failed for '%s' (%s)."), *Definition.CommandName, *LoadedPath.ToString());
    }

    Definition.IconTexture = LoadedTexture;

    HandleIconLoaded(Definition);
}

void UCommandPanelWidget::HandleIconLoaded(FCommandDefinition Definition)
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<UCommandPanelWidget> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, Definition]()
        {
            if (UCommandPanelWidget* StrongThis = WeakThis.Get())
            {
                StrongThis->HandleIconLoaded(Definition);
            }
        });
        return;
    }

    if (!bPanelActive || !IsValid(this) || !IsValid(CommandPanel) || !CommandItemWidgetClass)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UCommandItemWidget* NewItem = CreateWidget<UCommandItemWidget>(World, CommandItemWidgetClass);
    if (!IsValid(NewItem))
    {
        UE_LOG(LogTemp, Warning, TEXT("CommandPanelWidget: Failed to create CommandItemWidget for %s."), *Definition.CommandName);
        return;
    }

    NewItem->SetCommandName(Definition.CommandName);
    NewItem->SetVisualState(ECommandItemVisualState::Normal);

    if (UTexture2D* ResolvedTexture = Definition.IconTexture.Get())
    {
        ApplyIconToItem(NewItem, ResolvedTexture);
    }

    NewItem->OnCommandItemClicked.RemoveDynamic(this, &UCommandPanelWidget::HandleChildCommandClicked);
    NewItem->OnCommandItemClicked.AddDynamic(this, &UCommandPanelWidget::HandleChildCommandClicked);

    CommandPanel->AddChildToHorizontalBox(NewItem);

    SpawnedCommandItems.Add(NewItem);

    if (!LastSelectedCommandName.IsEmpty() && Definition.CommandName == LastSelectedCommandName)
    {
        UpdateSelection(NewItem);
    }
}

void UCommandPanelWidget::ApplyIconToItem(UCommandItemWidget* Item, UTexture2D* Texture)
{
    if (!IsValid(Item) || !IsValid(Texture))
    {
        return;
    }

    FSlateBrush Brush;
    Brush.SetResourceObject(Texture);
    Brush.ImageSize = FVector2D(128.f, 128.f);

    Item->SetCommandIcon(Brush);
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

    SelectedItem.Reset();
}

void UCommandPanelWidget::UpdateSelection(UCommandItemWidget* ClickedItem)
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<UCommandPanelWidget> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, ClickedItem]()
        {
            if (UCommandPanelWidget* StrongThis = WeakThis.Get())
            {
                StrongThis->UpdateSelection(ClickedItem);
            }
        });
        return;
    }

    if (!IsValid(ClickedItem))
    {
        if (UCommandItemWidget* CurrentSelected = SelectedItem.Get())
        {
            CurrentSelected->SetVisualState(ECommandItemVisualState::Normal);
        }

        SelectedItem.Reset();
        LastSelectedCommandName.Reset();
        return;
    }

    if (UCommandItemWidget* CurrentSelected = SelectedItem.Get())
    {
        if (CurrentSelected != ClickedItem)
        {
            CurrentSelected->SetVisualState(ECommandItemVisualState::Normal);
        }
    }

    ClickedItem->SetVisualState(ECommandItemVisualState::Selected);
    SelectedItem = ClickedItem;
    LastSelectedCommandName = ClickedItem->GetCommandName();
}
