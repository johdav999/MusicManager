#pragma once

#include "CoreMinimal.h"
#include "ArtistActionAvailability.generated.h"

/**
 * High-level actionability for signed artists.
 * This describes available actions (record/tour), not performance.
 */
UENUM(BlueprintType)
enum class EArtistActionAvailability : uint8
{
    None,
    RecordReady,
    TourReady,
    Multiple
};
