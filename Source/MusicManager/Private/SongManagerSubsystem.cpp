#include "SongManagerSubsystem.h"

#include "Async/Async.h"
#include "Engine/DataTable.h"
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
    SongMap.Reset();

    LoadSongsFromDataTable();
}

void USongManagerSubsystem::Deinitialize()
{
    ensure(IsInGameThread());
    Songs.Reset();
    SongMap.Reset();
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

    AddSongToCollections(NewSong);
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

    if (const TObjectPtr<USong>* Found = SongMap.Find(InSongId))
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

    for (const TPair<FString, TObjectPtr<USong>>& Pair : SongMap)
    {
        if (const USong* Song = Pair.Value.Get())
        {
            if (Song->ArtistId == ArtistId)
            {
                OutSongs.Add(const_cast<USong*>(Song));
            }
        }
    }
}

void USongManagerSubsystem::GetAllSongs(TArray<USong*>& OutSongs) const
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<const USongManagerSubsystem> WeakThis(this);
        FEvent* SyncEvent = FPlatformProcess::GetSynchEventFromPool(true);

        AsyncTask(ENamedThreads::GameThread, [WeakThis, &OutSongs, SyncEvent]()
        {
            if (const USongManagerSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->GetAllSongs(OutSongs);
            }
            SyncEvent->Trigger();
        });

        SyncEvent->Wait();
        FPlatformProcess::ReturnSynchEventToPool(SyncEvent);
        return;
    }

    OutSongs.Reset();
    OutSongs.Reserve(SongMap.Num());
    for (const TPair<FString, TObjectPtr<USong>>& Pair : SongMap)
    {
        if (Pair.Value)
        {
            OutSongs.Add(Pair.Value);
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
    for (const TPair<FString, TObjectPtr<USong>>& Pair : SongMap)
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
    SongMap.Reset();
    for (const FSongSaveRecord& Record : Records)
    {
        if (USong* NewSong = CreateSongInternal(this, Record.ArtistId, Record.Data))
        {
            NewSong->SongId = Record.SongId;
            AddSongToCollections(NewSong);
        }
    }
}

void USongManagerSubsystem::LoadSongsFromDataTable()
{
    if (!SongDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("SongManagerSubsystem: SongDataTable is not set."));
        return;
    }

    static const FString ContextString(TEXT("SongManagerSubsystem::LoadSongsFromDataTable"));

    TArray<FSongData*> AllRows;
    SongDataTable->GetAllRows<FSongData>(ContextString, AllRows);

    Songs.Reserve(AllRows.Num());
    SongMap.Reserve(AllRows.Num());

    for (FSongData* Row : AllRows)
    {
        if (!Row)
        {
            continue;
        }

        USong* NewSong = NewObject<USong>(this);
        if (!NewSong)
        {
            continue;
        }

        NewSong->Initialize(TEXT(""), *Row);

        NewSong->SongId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensInBraces);

        AddSongToCollections(NewSong);
    }

    UE_LOG(LogTemp, Log, TEXT("SongManagerSubsystem: Loaded %d songs from SongDataTable."), Songs.Num());
}

void USongManagerSubsystem::AddSongToCollections(USong* NewSong)
{
    if (!NewSong)
    {
        return;
    }

    Songs.Add(NewSong);
    SongMap.Add(NewSong->SongId, NewSong);
}
