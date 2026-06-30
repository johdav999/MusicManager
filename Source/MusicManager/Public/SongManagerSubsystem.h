#pragma once

#include "CoreMinimal.h"
#include "FSongData.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SongManagerSubsystem.generated.h"

class UDataTable;
class USong;
struct FMusicSaveValidationResult;

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

    UPROPERTY()
    bool bLockedForRecording = false;

    UPROPERTY()
    FString RecordedOnRecordId;
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

    /** DataTable containing all song rows (FSongData). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Songs")
    UDataTable* SongDataTable;

    /** All songs instantiated at game startup. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Songs")
    TArray<USong*> Songs;

    /** Create a song for an artist using the provided data payload. */
    UFUNCTION(BlueprintCallable, Category = "Songs")
    USong* CreateSong(const FString& ArtistId, const FSongData& Data);

    /** Lookup a song by its identifier. */
    UFUNCTION(BlueprintCallable, Category = "Songs")
    USong* GetSongById(const FString& InSongId) const;

    /** Fetch all songs authored by the specified artist. */
    UFUNCTION(BlueprintCallable, Category = "Songs")
    void GetSongsForArtist(const FString& ArtistId, TArray<USong*>& OutSongs) const;

    /** Retrieve all songs currently loaded in the subsystem. */
    void GetAllSongs(TArray<USong*>& OutSongs) const;

    /** Query unreleased and unlocked songs for an artist that can be recorded. */
    void GetEligibleSongsForRecording(const FString& ArtistId, TArray<USong*>& OutSongs) const;

    /** Query unreleased and unlocked songs from the catalog matching the artist or an unowned genre-compatible catalog entry. */
    void GetEligibleSongsForRecordingByGenre(const FString& ArtistId, const FString& Genre, TArray<USong*>& OutSongs) const;

    /** Assign unowned catalog songs to an artist when a recording session claims them. */
    bool AssignSongsToArtist(const TArray<FString>& SongIds, const FString& ArtistId, const FString& RequiredGenre, FString& OutError);

    /** Prevent a set of songs from being used in concurrent recordings. */
    bool LockSongsForRecording(const TArray<FString>& SongIds, FString& OutError);

    /** Release recording locks after completion or cancellation. */
    void UnlockSongs(const TArray<FString>& SongIds);

    /** Mark songs as attached to a recorded release (non-destructive to quality data). */
    void MarkSongsRecorded(const TArray<FString>& SongIds, const FString& RecordId);

    /** Serialize the registry for a save game. */
    void SerializeForSave(TArray<FSongSaveRecord>& OutRecords) const;

    void ValidateSaveRecords(const TArray<FSongSaveRecord>& Records, FMusicSaveValidationResult& Result) const;

    /** Restore the registry from saved data. */
    void DeserializeFromSave(const TArray<FSongSaveRecord>& Records);

private:
    /** Helper to load song rows from SongDataTable into Songs array. */
    UFUNCTION(BlueprintCallable, Category = "Songs")
    void LoadSongsFromDataTable();

    void AddSongToCollections(USong* NewSong);

    UPROPERTY()
    TMap<FString, TObjectPtr<USong>> SongMap;

    /** Simple lock table to avoid double-booking songs during recording. */
    UPROPERTY()
    TSet<FString> LockedSongIds;

    /** Historical mapping of songs to the record they ended up on. */
    UPROPERTY()
    TMap<FString, FString> SongToRecordMap;
};
