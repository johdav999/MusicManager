#include "GameTimeSubsystem.h"

#include "ArtistManagerSubsystem.h"
#include "Engine/World.h"
#include "FinanceManagerSubsystem.h"
#include "MarketManagerSubsystem.h"
#include "MusicSaveGame.h"
#include "RecordManagerSubsystem.h"

UGameTimeSubsystem::UGameTimeSubsystem()
    : CurrentGameDate(1955, 1, 1)
    , bIsTimeRunning(false)
    , bHasReachedSimulationEnd(false)
{
}

void UGameTimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    CurrentGameDate = FDateTime(1955, 1, 1);
    bHasReachedSimulationEnd = false;
    bIsTimeRunning = false;

    StartTimer();
}

void UGameTimeSubsystem::Deinitialize()
{
    StopTimer();
    Super::Deinitialize();
}

void UGameTimeSubsystem::AdvanceMonth()
{
    check(IsInGameThread());

    if (HasSimulationEnded())
    {
        StopTimer();
        return;
    }

    int32 NewYear = CurrentGameDate.GetYear();
    int32 NewMonth = CurrentGameDate.GetMonth() + 1;

    if (NewMonth > 12)
    {
        NewMonth = 1;
        ++NewYear;
    }

    if (NewYear > 2026)
    {
        bHasReachedSimulationEnd = true;
        StopTimer();
        return;
    }

    CurrentGameDate = FDateTime(NewYear, NewMonth, 1);

    // Orchestrate monthly flow explicitly to keep deterministic ordering across subsystems.
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UMarketManagerSubsystem* MarketSubsystem = GameInstance->GetSubsystem<UMarketManagerSubsystem>())
        {
            MarketSubsystem->HandleMonthAdvanced(CurrentGameDate);
        }

        if (UArtistManagerSubsystem* ArtistSubsystem = GameInstance->GetSubsystem<UArtistManagerSubsystem>())
        {
            ArtistSubsystem->HandleMonthAdvanced(CurrentGameDate);
        }

        if (URecordManagerSubsystem* RecordSubsystem = GameInstance->GetSubsystem<URecordManagerSubsystem>())
        {
            RecordSubsystem->HandleMonthAdvanced(CurrentGameDate);
        }

        if (UFinanceManagerSubsystem* FinanceSubsystem = GameInstance->GetSubsystem<UFinanceManagerSubsystem>())
        {
            FinanceSubsystem->HandleMonthAdvanced(CurrentGameDate);
        }
    }

    OnMonthAdvanced.Broadcast(CurrentGameDate);
}

void UGameTimeSubsystem::PauseTime(bool bPause)
{
    check(IsInGameThread());

    if (bPause)
    {
        if (bIsTimeRunning)
        {
            StopTimer();
        }
    }
    else
    {
        StartTimer();
    }
}

void UGameTimeSubsystem::StartTimer()
{
    if (HasSimulationEnded())
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(TimeAdvanceHandle, this, &UGameTimeSubsystem::AdvanceMonth, 4.0f, true);
        bIsTimeRunning = true;
    }
}

void UGameTimeSubsystem::StopTimer()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TimeAdvanceHandle);
    }

    bIsTimeRunning = false;
}

bool UGameTimeSubsystem::HasSimulationEnded() const
{
    if (bHasReachedSimulationEnd)
    {
        return true;
    }

    return CurrentGameDate.GetYear() > 2026;
}

void UGameTimeSubsystem::SaveState(UMusicSaveGame* SaveObject)
{
    if (!ensure(IsInGameThread()))
    {
        return;
    }

    if (SaveObject)
    {
        SaveObject->SavedGameDate = CurrentGameDate;
    }
}

void UGameTimeSubsystem::LoadState(const UMusicSaveGame* SaveObject)
{
    if (!ensure(IsInGameThread()))
    {
        return;
    }

    if (SaveObject)
    {
        CurrentGameDate = SaveObject->SavedGameDate;
    }
}
