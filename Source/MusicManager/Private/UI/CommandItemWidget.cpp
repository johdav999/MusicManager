#include "UI/CommandItemWidget.h"

#include "Async/Async.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

UCommandItemWidget::UCommandItemWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , NormalBackgroundColor(FLinearColor::White)
    , HoverBackgroundColor(FLinearColor(0.9f, 0.9f, 0.9f, 1.0f))
    , SelectedBackgroundColor(FLinearColor(0.8f, 0.8f, 1.0f, 1.0f))
    , BorderColor(FLinearColor(0.0f, 0.5f, 1.0f, 1.0f))
    , SelectedBorderThickness(2.0f)
{
}

void UCommandItemWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Widgets can be null in cooked builds if bindings were removed, so always validate.
    BindButtonEvents();

    SetVisualState(ECommandItemVisualState::Normal);
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
    if (CommandLabelText)
    {
        CommandLabelText->SetText(FText::FromString(CommandName));
    }
}

void UCommandItemWidget::SetCommandIcon(const FSlateBrush& InBrush)
{
    if (IsValid(CommandImage))
    {
        // Slate brushes are value types, so copying them is safe and thread-aware on the game thread.
        CommandImage->SetBrush(InBrush);
    }
}

void UCommandItemWidget::SetVisualState(ECommandItemVisualState NewState)
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<UCommandItemWidget> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, NewState]()
        {
            if (UCommandItemWidget* StrongThis = WeakThis.Get())
            {
                StrongThis->SetVisualState(NewState);
            }
        });
        return;
    }

    if (CurrentVisualState == NewState)
    {
        return;
    }

    CurrentVisualState = NewState;

    switch (NewState)
    {
    case ECommandItemVisualState::Hovered:
        ApplyBackgroundColor(HoverBackgroundColor);
        ApplyBorderStyle(true);
        break;
    case ECommandItemVisualState::Selected:
        ApplyBackgroundColor(SelectedBackgroundColor);
        ApplyBorderStyle(true);
        break;
    default:
        ApplyBackgroundColor(NormalBackgroundColor);
        ApplyBorderStyle(false);
        break;
    }
}

void UCommandItemWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    if (!IsSelected())
    {
        SetVisualState(ECommandItemVisualState::Hovered);
    }
}

void UCommandItemWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    if (!IsSelected())
    {
        SetVisualState(ECommandItemVisualState::Normal);
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

void UCommandItemWidget::ApplyBackgroundColor(const FLinearColor& NewColor)
{
    if (BackgroundBorder)
    {
        BackgroundBorder->SetBrushColor(NewColor);
    }
    else if (CommandButton)
    {
        CommandButton->SetBackgroundColor(NewColor);
    }
}

void UCommandItemWidget::ApplyBorderStyle(bool bShowBorder) const
{
    if (!OutlineBorder)
    {
        return;
    }

    FSlateBrush Brush = OutlineBorder->Background;
    Brush.DrawAs = ESlateBrushDrawType::Box;
    Brush.Margin = FMargin(bShowBorder ? SelectedBorderThickness : 0.f);
    Brush.TintColor = FSlateColor(bShowBorder ? BorderColor : FLinearColor::Transparent);
    OutlineBorder->SetBrush(Brush);
}
