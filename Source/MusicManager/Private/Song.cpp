#include "Song.h"
#include "FSongData.h"
#include "Misc/Guid.h"

void USong::Initialize(const FString& InArtistId, const FSongData& InData)
{
    // All song initialization must run on the game thread for safety.
    ensure(IsInGameThread());

    ArtistId = InArtistId;
    Data = InData;

    // Generate a deterministic identifier that other systems can reference for save/load.
    SongId = FGuid::NewGuid().ToString(EGuidFormats::Short);
}

FString USong::GetDisplayName() const
{
    // Display helpers should also be called on the game thread to avoid data races.
    ensure(IsInGameThread());

    return FString::Printf(TEXT("%s (%d)"), *Data.SongName, Data.YearCreated);
}
