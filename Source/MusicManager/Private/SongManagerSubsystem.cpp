#include "SongManagerSubsystem.h"

#include "Algo/Sort.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "FSongData.h"
#include "GameTimeSubsystem.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/DateTime.h"
#include "Song.h"

void USongManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    ensure(IsInGameThread());

    Super::Initialize(Collection);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>())
        {
            // Listen for global month advancement events to drive popularity simulation.
            TimeSubsystem->OnMonthAdvanced.AddDynamic(this, &USongManagerSubsystem::HandleMonthAdvanced);
        }
    }
}

void USongManagerSubsystem::Deinitialize()
{
    ensure(IsInGameThread());

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>())
        {
            // Clean up bindings so the subsystem can be garbage collected correctly.
            TimeSubsystem->OnMonthAdvanced.RemoveDynamic(this, &USongManagerSubsystem::HandleMonthAdvanced);
        }
    }

    Super::Deinitialize();
}

USong* USongManagerSubsystem::CreateSong(const FString& ArtistId, const FString& SongName, const FString& Genre)
{
    ensure(IsInGameThread());

    if (!GetGameInstance())
    {
        return nullptr;
    }

    // Use the time subsystem if available to stamp the creation year.
    const int32 CurrentYear = [this]() -> int32
    {
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            if (UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>())
            {
                return TimeSubsystem->GetCurrentGameDate().GetYear();
            }
        }
        return FDateTime::Now().GetYear();
    }();

    // Construct the base data with a few randomized attributes for variety.
    FSongData SongData;
    SongData.SongName = SongName;
    SongData.Genre = Genre;
    SongData.YearCreated = CurrentYear;
    SongData.HitPotential = FMath::FRandRange(40.f, 90.f);
    SongData.Authenticity = FMath::FRandRange(30.f, 100.f);
    SongData.LyricsQuality = FMath::FRandRange(20.f, 100.f);
    SongData.Innovation = FMath::FRandRange(10.f, 90.f);
    SongData.ProductionQuality = FMath::FRandRange(30.f, 95.f);
    SongData.ArrangementQuality = FMath::FRandRange(25.f, 95.f);
    SongData.Energy = FMath::FRandRange(10.f, 100.f);
    SongData.Catchiness = FMath::FRandRange(20.f, 100.f);
    SongData.TrendAlignment = FMath::FRandRange(10.f, 100.f);
    SongData.Longevity = FMath::FRandRange(10.f, 100.f);
    SongData.ViralPotential = FMath::FRandRange(0.f, 100.f);
    SongData.CurrentPopularity = 0.f;
    SongData.ChartWeeks = 0;

    // UObjects must be created with NewObject so that the engine can manage their lifetime.
    USong* NewSong = NewObject<USong>(GetGameInstance());
    if (!NewSong)
    {
        return nullptr;
    }

    NewSong->Initialize(ArtistId, SongData);

    // Store the reference so every subsystem can query the authoritative list.
    ActiveSongs.Add(NewSong);

    return NewSong;
}

void USongManagerSubsystem::ReleaseSong(USong* Song, const FDateTime& ReleaseDate)
{
    ensure(IsInGameThread());

    if (!Song)
    {
        return;
    }

    // Keep the core identity information synchronized with the release date.
    Song->Data.YearCreated = ReleaseDate.GetYear();
    Song->Data.ReleaseYear = ReleaseDate.GetYear();
    Song->Data.ReleaseMonth = ReleaseDate.GetMonth();
    Song->Data.bIsReleased = true;

    // Notify any listeners (UI, news feed, etc.).
    OnSongReleased.Broadcast(Song);
}

void USongManagerSubsystem::HandleMonthAdvanced(const FDateTime& /*NewDate*/)
{
    ensure(IsInGameThread());

    // Run the simulation step for every active song.
    for (const TObjectPtr<USong>& SongPtr : ActiveSongs)
    {
        if (USong* Song = SongPtr.Get())
        {
            UpdateSongForNewMonth(Song);
        }
    }

    // Determine which songs should be archived after updates.
    TArray<USong*> SongsToArchive;
    for (const TObjectPtr<USong>& SongPtr : ActiveSongs)
    {
        if (USong* Song = SongPtr.Get())
        {
            if (Song->Data.CurrentPopularity < 5.f)
            {
                SongsToArchive.Add(Song);
            }
        }
    }

    for (USong* Song : SongsToArchive)
    {
        ArchiveSong(Song);
    }
}

TArray<USong*> USongManagerSubsystem::GetTopSongs(int32 Count) const
{
    ensure(IsInGameThread());

    TArray<USong*> SortedSongs;
    SortedSongs.Reserve(ActiveSongs.Num());

    for (const TObjectPtr<USong>& SongPtr : ActiveSongs)
    {
        if (USong* Song = SongPtr.Get())
        {
            SortedSongs.Add(Song);
        }
    }

    SortedSongs.Sort([](const USong& A, const USong& B)
        {
            return A.Data.CurrentPopularity > B.Data.CurrentPopularity;
        });

    if (Count <= 0)
    {
        return {};
    }

    if (SortedSongs.Num() <= Count)
    {
        return SortedSongs;
    }

    TArray<USong*> Result;
    Result.Reserve(Count);
    for (int32 Index = 0; Index < Count; ++Index)
    {
        Result.Add(SortedSongs[Index]);
    }
    return Result;
}

TArray<USong*> USongManagerSubsystem::GetSongsByArtist(const FString& ArtistId) const
{
    ensure(IsInGameThread());

    TArray<USong*> Result;

    const auto Collect = [&Result, &ArtistId](const TArray<TObjectPtr<USong>>& Source)
    {
        for (const TObjectPtr<USong>& SongPtr : Source)
        {
            if (USong* Song = SongPtr.Get())
            {
                if (Song->ArtistId == ArtistId)
                {
                    Result.Add(Song);
                }
            }
        }
    };

    Collect(ActiveSongs);
    Collect(ArchivedSongs);

    return Result;
}

void USongManagerSubsystem::UpdateSongForNewMonth(USong* Song)
{
    ensure(IsInGameThread());

    if (!Song)
    {
        return;
    }

    // Core simulation step: adjust popularity based on creative quality and market factors.
    const float BaseGrowth = Song->Data.HitPotential * 0.05f;           // Great songs grow faster in general.
    const float InnovationBoost = Song->Data.Innovation * 0.02f;         // Innovation keeps the track exciting.
    const float TrendFactor = Song->Data.TrendAlignment * 0.03f;         // Trend alignment rides cultural waves.
    const float ViralBoost = Song->Data.ViralPotential * FMath::FRandRange(0.0f, 0.4f); // Random viral spikes.
    const float AgingDecay = Song->Data.ChartWeeks * 0.4f;               // Songs cool off over time.

    Song->Data.CurrentPopularity += BaseGrowth + InnovationBoost + TrendFactor + ViralBoost - AgingDecay;
    Song->Data.CurrentPopularity = FMath::Clamp(Song->Data.CurrentPopularity, 0.0f, 100.0f);

    if (Song->Data.CurrentPopularity > 20.f)
    {
        // Songs that remain relevant accumulate chart weeks.
        ++Song->Data.ChartWeeks;
    }
}

void USongManagerSubsystem::ArchiveSong(USong* Song)
{
    ensure(IsInGameThread());

    if (!Song)
    {
        return;
    }

    const int32 Index = ActiveSongs.IndexOfByKey(Song);
    if (Index != INDEX_NONE)
    {
        ActiveSongs.RemoveAtSwap(Index);
        ArchivedSongs.Add(Song);
    }
}
