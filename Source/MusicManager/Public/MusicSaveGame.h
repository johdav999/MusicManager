#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "FArtistContract.h"
#include "FSongData.h"
#include "MusicSaveGame.generated.h"

USTRUCT()
struct FSavedSong
{
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FString SongId;

    UPROPERTY(SaveGame)
    FString ArtistId;

    UPROPERTY(SaveGame)
    FSongData Data;
};

UCLASS()
class MUSICMANAGER_API UMusicSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame)
    TArray<FSavedSong> SavedSongs;

    UPROPERTY(SaveGame)
    TArray<FArtistContract> SavedContracts;

    UPROPERTY(SaveGame)
    FDateTime SavedGameDate;

    UPROPERTY(SaveGame)
    int32 PlayerMoney = 0;
};
