#pragma once

#include "CoreMinimal.h"
#include "FSongData.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SongManagerSubsystem.generated.h"

class USong;

USTRUCT()
struct FSongSaveRecord
{
    GENERATED_BODY()

    UPROPERTY()
    FString SongId;

    UPROPERTY()
    FString ArtistId;

    UPROPERTY()
    FSongData Data;
};

/**
 * Thread-safe registry and factory for all song instances in the game.
 */
UCLASS()
class MUSICMANAGER_API USongManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Create a song for an artist using the provided data payload. */
    UFUNCTION(BlueprintCallable, Category = "Songs")
    USong* CreateSong(const FString& ArtistId, const FSongData& Data);

    /** Lookup a song by its identifier. */
    UFUNCTION(BlueprintCallable, Category = "Songs")
    USong* GetSongById(const FString& InSongId) const;

    /** Fetch all songs authored by the specified artist. */
    UFUNCTION(BlueprintCallable, Category = "Songs")
    void GetSongsForArtist(const FString& ArtistId, TArray<USong*>& OutSongs) const;

    /** Serialize the registry for a save game. */
    void SerializeForSave(TArray<FSongSaveRecord>& OutRecords) const;

    /** Restore the registry from saved data. */
    void DeserializeFromSave(const TArray<FSongSaveRecord>& Records);

private:
    UPROPERTY()
    TMap<FString, TObjectPtr<USong>> Songs;
};
