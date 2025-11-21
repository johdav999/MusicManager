#include "GameTimeSubsystem.h"

#include "Engine/World.h"
#include "MusicSaveGame.h"

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
        World->GetTimerManager().SetTimer(TimeAdvanceHandle, this, &UGameTimeSubsystem::AdvanceMonth, 2.0f, true);
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
