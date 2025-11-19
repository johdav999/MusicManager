#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "FSongData.h"
#include "Song.generated.h"

/**
 * UObject wrapper that allows sharing a single song record between multiple systems.
 */
UCLASS(BlueprintType)
class MUSICMANAGER_API USong : public UObject
{
    GENERATED_BODY()

public:
    /** Core data for this song (reuse FSongData). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Song")
    FSongData Data;

    /** Unique ID for this song instance (for save/load and cross-system references). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Song")
    FString SongId;

    /** Optional: artist ID that primarily owns/created this song. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Song")
    FString ArtistId;

    /** Simple helper to initialize the song. */
    void Initialize(const FString& InArtistId, const FSongData& InData);

    /** Helper to compute a display string like "SongName (Year)". */
    UFUNCTION(BlueprintCallable, Category = "Song")
    FString GetDisplayName() const;
};
