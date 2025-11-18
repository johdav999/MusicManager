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
    const int32 Year = NewDate.GetYear();
    const int32 Month = NewDate.GetMonth(); // 1–12

    static const FString MonthNames[12] = {
        TEXT("January"), TEXT("February"), TEXT("March"),
        TEXT("April"), TEXT("May"), TEXT("June"),
        TEXT("July"), TEXT("August"), TEXT("September"),
        TEXT("October"), TEXT("November"), TEXT("December")
    };

    // Safety check
    const FString MonthString = (Month >= 1 && Month <= 12)
        ? MonthNames[Month - 1]
        : TEXT("Unknown");

    DateText->SetText(FText::FromString(FString::Printf(TEXT("%s %d"), *MonthString, Year)));

}
