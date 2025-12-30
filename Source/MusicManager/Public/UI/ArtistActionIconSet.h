#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ArtistActionAvailability.h"
#include "ArtistActionIconSet.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct FArtistActionIconData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    EArtistActionAvailability Availability = EArtistActionAvailability::None;

    UPROPERTY(EditAnywhere)
    UTexture2D* IconTexture = nullptr;
};

/**
 * Data-driven icon set for layer-1 action availability.
 */
UCLASS(BlueprintType)
class MUSICMANAGER_API UArtistActionIconSet : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere)
    TArray<FArtistActionIconData> Icons;
};
