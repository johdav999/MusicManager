#include "UI/DateWidget.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UDateWidget::NativeConstruct()
{
    ensure(IsInGameThread());

    Super::NativeConstruct();

    UGameTimeSubsystem* TimeSys = nullptr;

    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            TimeSys = GameInstance->GetSubsystem<UGameTimeSubsystem>();
        }
    }

    if (TimeSys)
    {
        TimeSys->OnMonthAdvanced.AddDynamic(this, &UDateWidget::HandleMonthAdvanced);
        HandleMonthAdvanced(TimeSys->GetCurrentGameDate());
    }
}

void UDateWidget::NativeDestruct()
{
    ensure(IsInGameThread());

    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            if (UGameTimeSubsystem* TimeSys = GameInstance->GetSubsystem<UGameTimeSubsystem>())
            {
                TimeSys->OnMonthAdvanced.RemoveDynamic(this, &UDateWidget::HandleMonthAdvanced);
            }
        }
    }

    Super::NativeDestruct();
}

void UDateWidget::HandleMonthAdvanced(const FDateTime& NewDate)
{
    ensure(IsInGameThread());

    if (!DateText)
    {
        return;
    }

    const FString MonthYear = NewDate.ToString(TEXT("MMMM yyyy"));
    DateText->SetText(FText::FromString(MonthYear));
}
