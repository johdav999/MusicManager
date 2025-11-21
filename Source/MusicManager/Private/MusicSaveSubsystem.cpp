#include "MusicSaveSubsystem.h"

#include "ArtistManagerSubsystem.h"
#include "Async/Async.h"
#include "GameTimeSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "MusicSaveGame.h"
#include "SongManagerSubsystem.h"
#include "UIManagerSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMusicSaveSubsystem, Log, All);

void UMusicSaveSubsystem::SaveGame(const FString& SlotName)
{
    if (!IsInGameThread())
    {
        const FString SafeSlot = SlotName;
        TWeakObjectPtr<UMusicSaveSubsystem> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, SafeSlot]()
        {
            if (UMusicSaveSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->SaveGame(SafeSlot);
            }
        });
        return;
    }

    if (SlotName.IsEmpty())
    {
        UE_LOG(LogMusicSaveSubsystem, Warning, TEXT("SaveGame called with empty slot name."));
        return;
    }

    UMusicSaveGame* SaveObject = NewObject<UMusicSaveGame>();
    if (!SaveObject)
    {
        UE_LOG(LogMusicSaveSubsystem, Error, TEXT("Failed to allocate save game object."));
        return;
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (USongManagerSubsystem* SongManager = GameInstance->GetSubsystem<USongManagerSubsystem>())
        {
            SongManager->SaveState(SaveObject);
        }

        if (UArtistManagerSubsystem* ArtistManager = GameInstance->GetSubsystem<UArtistManagerSubsystem>())
        {
            ArtistManager->SaveState(SaveObject);
        }

        if (UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>())
        {
            TimeSubsystem->SaveState(SaveObject);
        }
    }

    if (WriteToSlot(SaveObject, SlotName))
    {
        UE_LOG(LogMusicSaveSubsystem, Log, TEXT("Saved game to slot '%s'."), *SlotName);
    }
}

void UMusicSaveSubsystem::LoadGame(const FString& SlotName)
{
    if (!IsInGameThread())
    {
        const FString SafeSlot = SlotName;
        TWeakObjectPtr<UMusicSaveSubsystem> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, SafeSlot]()
        {
            if (UMusicSaveSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->LoadGame(SafeSlot);
            }
        });
        return;
    }

    if (SlotName.IsEmpty())
    {
        UE_LOG(LogMusicSaveSubsystem, Warning, TEXT("LoadGame called with empty slot name."));
        return;
    }

    UMusicSaveGame* SaveObject = ReadFromSlot(SlotName);
    if (!SaveObject)
    {
        UE_LOG(LogMusicSaveSubsystem, Warning, TEXT("No save object could be loaded from slot '%s'."), *SlotName);
        return;
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (USongManagerSubsystem* SongManager = GameInstance->GetSubsystem<USongManagerSubsystem>())
        {
            SongManager->LoadState(SaveObject);
        }

        if (UArtistManagerSubsystem* ArtistManager = GameInstance->GetSubsystem<UArtistManagerSubsystem>())
        {
            ArtistManager->LoadState(SaveObject);
        }

        if (UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>())
        {
            TimeSubsystem->LoadState(SaveObject);
        }

        if (UUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UUIManagerSubsystem>())
        {
            UIManager->RebuildUI();
        }
    }
}

bool UMusicSaveSubsystem::WriteToSlot(UMusicSaveGame* SaveObject, const FString& SlotName)
{
    if (!ensure(IsInGameThread()))
    {
        return false;
    }

    if (!SaveObject)
    {
        return false;
    }

    return UGameplayStatics::SaveGameToSlot(SaveObject, SlotName, 0);
}

UMusicSaveGame* UMusicSaveSubsystem::ReadFromSlot(const FString& SlotName)
{
    if (!ensure(IsInGameThread()))
    {
        return nullptr;
    }

    UObject* RawObject = UGameplayStatics::LoadGameFromSlot(SlotName, 0);
    return Cast<UMusicSaveGame>(RawObject);
}
