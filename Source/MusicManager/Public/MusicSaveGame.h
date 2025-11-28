#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "FArtistContract.h"
#include "SongManagerSubsystem.h"
#include "MusicSaveGame.generated.h"

UCLASS()
class MUSICMANAGER_API UMusicSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame)
    TArray<FSongSaveRecord> SavedSongs;

    UPROPERTY(SaveGame)
    TArray<FArtistContract> SavedContracts;

    UPROPERTY(SaveGame)
    FDateTime SavedGameDate;

    UPROPERTY(SaveGame)
    int32 PlayerMoney = 0;
};
