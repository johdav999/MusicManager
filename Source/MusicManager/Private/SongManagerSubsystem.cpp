#include "SongManagerSubsystem.h"

#include "Async/Async.h"
#include "Engine/GameInstance.h"
#include "Song.h"
#include "Misc/Guid.h"
#include "HAL/PlatformProcess.h"

namespace
{
USong* CreateSongInternal(USongManagerSubsystem* Subsystem, const FString& ArtistId, const FSongData& Data)
{
    if (!Subsystem || !IsInGameThread())
    {
        return nullptr;
    }

    UGameInstance* GameInstance = Subsystem->GetGameInstance();
    UObject* Outer = GameInstance ? static_cast<UObject*>(GameInstance) : GetTransientPackage();

    USong* NewSong = NewObject<USong>(Outer);
    if (!NewSong)
    {
        return nullptr;
    }

    NewSong->Initialize(ArtistId, Data);
    NewSong->SongId = FString::Printf(TEXT("SNG_%s"), *FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens));

    return NewSong;
}
} // namespace

void USongManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    ensure(IsInGameThread());
    Super::Initialize(Collection);
    Songs.Reset();
}

void USongManagerSubsystem::Deinitialize()
{
    ensure(IsInGameThread());
    Songs.Reset();
    Super::Deinitialize();
}

USong* USongManagerSubsystem::CreateSong(const FString& ArtistId, const FSongData& Data)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<USongManagerSubsystem> WeakThis(this);
        TWeakObjectPtr<USong> CreatedSong;

        FEvent* SyncEvent = FPlatformProcess::GetSynchEventFromPool(true);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, ArtistId, Data, &CreatedSong, SyncEvent]()
        {
            if (USongManagerSubsystem* StrongThis = WeakThis.Get())
            {
                CreatedSong = StrongThis->CreateSong(ArtistId, Data);
            }
            SyncEvent->Trigger();
        });

        SyncEvent->Wait();
        FPlatformProcess::ReturnSynchEventToPool(SyncEvent);
        return CreatedSong.Get();
    }

    if (!GetGameInstance())
    {
        return nullptr;
    }

    USong* NewSong = CreateSongInternal(this, ArtistId, Data);
    if (!NewSong)
    {
        return nullptr;
    }

    Songs.Add(NewSong->SongId, NewSong);
    return NewSong;
}

USong* USongManagerSubsystem::GetSongById(const FString& InSongId) const
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<const USongManagerSubsystem> WeakThis(this);
        TWeakObjectPtr<USong> FoundSong;
        FEvent* SyncEvent = FPlatformProcess::GetSynchEventFromPool(true);

        AsyncTask(ENamedThreads::GameThread, [WeakThis, InSongId, &FoundSong, SyncEvent]()
        {
            if (const USongManagerSubsystem* StrongThis = WeakThis.Get())
            {
                FoundSong = StrongThis->GetSongById(InSongId);
            }
            SyncEvent->Trigger();
        });

        SyncEvent->Wait();
        FPlatformProcess::ReturnSynchEventToPool(SyncEvent);
        return FoundSong.Get();
    }

    if (const TObjectPtr<USong>* Found = Songs.Find(InSongId))
    {
        return Found->Get();
    }
    return nullptr;
}

void USongManagerSubsystem::GetSongsForArtist(const FString& ArtistId, TArray<USong*>& OutSongs) const
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<const USongManagerSubsystem> WeakThis(this);
        FEvent* SyncEvent = FPlatformProcess::GetSynchEventFromPool(true);

        AsyncTask(ENamedThreads::GameThread, [WeakThis, ArtistId, &OutSongs, SyncEvent]()
        {
            if (const USongManagerSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->GetSongsForArtist(ArtistId, OutSongs);
            }
            SyncEvent->Trigger();
        });

        SyncEvent->Wait();
        FPlatformProcess::ReturnSynchEventToPool(SyncEvent);
        return;
    }

    OutSongs.Reset();
    for (const TPair<FString, TObjectPtr<USong>>& Pair : Songs)
    {
        if (const USong* Song = Pair.Value.Get())
        {
            if (Song->ArtistId == ArtistId)
            {
                OutSongs.Add(Pair.Value.Get());
            }
        }
    }
}

void USongManagerSubsystem::SerializeForSave(TArray<FSongSaveRecord>& OutRecords) const
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<const USongManagerSubsystem> WeakThis(this);
        FEvent* SyncEvent = FPlatformProcess::GetSynchEventFromPool(true);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, &OutRecords, SyncEvent]()
        {
            if (const USongManagerSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->SerializeForSave(OutRecords);
            }
            SyncEvent->Trigger();
        });
        SyncEvent->Wait();
        FPlatformProcess::ReturnSynchEventToPool(SyncEvent);
        return;
    }

    OutRecords.Reset();
    for (const TPair<FString, TObjectPtr<USong>>& Pair : Songs)
    {
        if (const USong* Song = Pair.Value.Get())
        {
            FSongSaveRecord Record;
            Record.SongId = Song->SongId;
            Record.ArtistId = Song->ArtistId;
            Record.Data = Song->Data;
            OutRecords.Add(Record);
        }
    }
}

void USongManagerSubsystem::DeserializeFromSave(const TArray<FSongSaveRecord>& Records)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<USongManagerSubsystem> WeakThis(this);
        FEvent* SyncEvent = FPlatformProcess::GetSynchEventFromPool(true);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, Records, SyncEvent]()
        {
            if (USongManagerSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->DeserializeFromSave(Records);
            }
            SyncEvent->Trigger();
        });

        SyncEvent->Wait();
        FPlatformProcess::ReturnSynchEventToPool(SyncEvent);
        return;
    }

    Songs.Reset();
    for (const FSongSaveRecord& Record : Records)
    {
        if (USong* NewSong = CreateSongInternal(this, Record.ArtistId, Record.Data))
        {
            NewSong->SongId = Record.SongId;
            Songs.Add(NewSong->SongId, NewSong);
        }
    }
}
