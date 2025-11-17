#include "UI/CommandPanelWidget.h"

#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "UI/CommandItemWidget.h"
#include "Async/Async.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"
#include "Brushes/SlateBrush.h"
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
    if (CommandDefinitions.Num() > 0)
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
        MakeDefinition(TEXT("Artists"), TEXT("/Game/UI/Icons/Artists.Artists")),
        MakeDefinition(TEXT("Studio"), TEXT("/Game/UI/Icons/Studio.Studio")),
        MakeDefinition(TEXT("Tours"), TEXT("/Game/UI/Icons/Tours.Tours")),
        MakeDefinition(TEXT("Genre Tree"), TEXT("/Game/UI/Icons/GenreTree.GenreTree")),
        MakeDefinition(TEXT("Radio"), TEXT("/Game/UI/Icons/Radio.Radio")),
        MakeDefinition(TEXT("Charts"), TEXT("/Game/UI/Icons/Charts.Charts")),
        MakeDefinition(TEXT("Financials"), TEXT("/Game/UI/Icons/Financials.Financials")),
        MakeDefinition(TEXT("Marketing"), TEXT("/Game/UI/Icons/Marketing.Marketing")),
        MakeDefinition(TEXT("Contracts"), TEXT("/Game/UI/Icons/Contracts.Contracts"))
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
            FStreamableDelegate::CreateUObject(
                this,
                &UCommandPanelWidget::HandleIconLoaded,
                Definition));
    }
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

    if (UTexture2D* ResolvedTexture = Definition.IconTexture.Get())
    {
        ApplyIconToItem(NewItem, ResolvedTexture);
    }

    NewItem->OnCommandItemClicked.RemoveDynamic(this, &UCommandPanelWidget::HandleChildCommandClicked);
    NewItem->OnCommandItemClicked.AddDynamic(this, &UCommandPanelWidget::HandleChildCommandClicked);

    CommandPanel->AddChildToHorizontalBox(NewItem);

    SpawnedCommandItems.Add(NewItem);
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
}
