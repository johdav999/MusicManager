#include "UI/CommandItemWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"

UCommandItemWidget::UCommandItemWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCommandItemWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Widgets can be null in cooked builds if bindings were removed, so always validate.
    BindButtonEvents();
}

void UCommandItemWidget::NativeDestruct()
{
    // Clean up any bindings to avoid dangling delegates or thread safety issues.
    UnbindButtonEvents();
    Super::NativeDestruct();
}

void UCommandItemWidget::SetCommandName(const FString& InCommandName)
{
    CommandName = InCommandName;
}

void UCommandItemWidget::SetCommandIcon(const FSlateBrush& InBrush)
{
    if (CommandImage)
    {
        // Slate brushes are value types, so copying them is safe and thread-aware on the game thread.
        CommandImage->SetBrush(InBrush);
    }
}

void UCommandItemWidget::HandleButtonClicked()
{
    // Ensure the event is fired on the game thread to keep UI state consistent.
    if (!IsInGameThread())
    {
        // If somehow triggered on another thread, just ignore to avoid race conditions.
        return;
    }

    OnCommandItemClicked.Broadcast(CommandName);
}

void UCommandItemWidget::BindButtonEvents()
{
    if (IsValid(CommandButton))
    {
        CommandButton->OnClicked.RemoveDynamic(this, &UCommandItemWidget::HandleButtonClicked);
        CommandButton->OnClicked.AddDynamic(this, &UCommandItemWidget::HandleButtonClicked);
    }
}

void UCommandItemWidget::UnbindButtonEvents()
{
    if (IsValid(CommandButton))
    {
        CommandButton->OnClicked.RemoveDynamic(this, &UCommandItemWidget::HandleButtonClicked);
    }
}
