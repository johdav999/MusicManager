#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SongManagerSubsystem.generated.h"

class USong;

/**
 * Subsystem that owns and simulates all song instances for the project.
 */
UCLASS()
class MUSICMANAGER_API USongManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // UGameInstanceSubsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Create a new song owned by an artist.
    UFUNCTION(BlueprintCallable, Category = "Songs")
    USong* CreateSong(const FString& ArtistId, const FString& SongName, const FString& Genre);

    // Mark a song as released at a specific date.
    UFUNCTION(BlueprintCallable, Category = "Songs")
    void ReleaseSong(USong* Song, const FDateTime& ReleaseDate);

    // Handle monthly time advancement from UGameTimeSubsystem.
    UFUNCTION()
    void HandleMonthAdvanced(const FDateTime& NewDate);

    // Query helpers for UI and gameplay.
    UFUNCTION(BlueprintCallable, Category = "Songs")
    TArray<USong*> GetTopSongs(int32 Count) const;

    UFUNCTION(BlueprintCallable, Category = "Songs")
    TArray<USong*> GetSongsByArtist(const FString& ArtistId) const;

    // Access to all active songs (read-only).
    const TArray<TObjectPtr<USong>>& GetAllActiveSongs() const
    {
        ensure(IsInGameThread());
        return ActiveSongs;
    }

    // Delegate fired when a song is released (for NewsFeed, UI, etc.).
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSongReleased, USong*, Song);

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSongReleased OnSongReleased;

private:
    // Songs currently active in the simulation (charting / relevant).
    UPROPERTY()
    TArray<TObjectPtr<USong>> ActiveSongs;

    // Songs that have fallen out of relevance / archived.
    UPROPERTY()
    TArray<TObjectPtr<USong>> ArchivedSongs;

    // Internal helper to update popularity and chart stats.
    void UpdateSongForNewMonth(USong* Song);

    // Internal helper to move a song to archive.
    void ArchiveSong(USong* Song);
};
