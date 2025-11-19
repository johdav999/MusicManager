#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundWave.h" // REQUIRED for USTRUCT member TObjectPtr<USoundWave>
#include "FSongData.generated.h"

/**
 * Describes a single song entry within the music management simulation.
 */
USTRUCT(BlueprintType)
struct FSongData
{
    GENERATED_BODY()

public:
    FSongData()
        : SongName(TEXT("Untitled"))
        , Genre(TEXT("Unknown"))
        , YearCreated(1955)
        , HitPotential(0.f)
        , Authenticity(0.f)
        , LyricsQuality(0.f)
        , Innovation(0.f)
        , ProductionQuality(0.f)
        , ArrangementQuality(0.f)
        , Energy(0.f)
        , Catchiness(0.f)
        , TrendAlignment(0.f)
        , Longevity(0.f)
        , ViralPotential(0.f)
        , CurrentPopularity(0.f)
        , ChartWeeks(0)
        , SoundWave(nullptr)
        , ReleaseYear(0)
        , ReleaseMonth(0)
        , bIsReleased(false)
    {
    }

    // --- Identity Fields ---

    /** Song display name. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    FString SongName;

    /** Primary genre categorization for sorting and filtering. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    FString Genre;

    /** Year the song was created or released. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Identity")
    int32 YearCreated;

    // --- Core Quality Metrics ---

    /** Likelihood of becoming a hit (0-100). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core Quality", meta = (ClampMin = "0", ClampMax = "100"))
    float HitPotential;

    /** How authentic the track feels (0-100). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core Quality", meta = (ClampMin = "0", ClampMax = "100"))
    float Authenticity;

    /** Lyric craftsmanship quality (0-100). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core Quality", meta = (ClampMin = "0", ClampMax = "100"))
    float LyricsQuality;

    /** Degree of innovation and uniqueness (0-100). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core Quality", meta = (ClampMin = "0", ClampMax = "100"))
    float Innovation;

    // --- Production / Arrangement Metrics ---

    /** Overall production polish (0-100). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production", meta = (ClampMin = "0", ClampMax = "100"))
    float ProductionQuality;

    /** Arrangement and structure quality (0-100). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production", meta = (ClampMin = "0", ClampMax = "100"))
    float ArrangementQuality;

    /** Perceived energy level (0-100). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production", meta = (ClampMin = "0", ClampMax = "100"))
    float Energy;

    /** Stickiness of hooks and melodies (0-100). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Production", meta = (ClampMin = "0", ClampMax = "100"))
    float Catchiness;

    // --- Market Dynamics ---

    /** Alignment with current trends (0-100). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market", meta = (ClampMin = "0", ClampMax = "100"))
    float TrendAlignment;

    /** Expected shelf life in the charts (0-100). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market", meta = (ClampMin = "0", ClampMax = "100"))
    float Longevity;

    /** Potential to go viral (0-100). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Market", meta = (ClampMin = "0", ClampMax = "100"))
    float ViralPotential;

    // --- Runtime / Simulation ---

    /** Current popularity score tracked during simulation. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
    float CurrentPopularity;

    /** Total weeks spent on the charts. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
    int32 ChartWeeks;

    /** Optional sound data associated with this song. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TObjectPtr<USoundWave> SoundWave;

    // --- Release Metadata ---

    /** Calendar year when the song officially released. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Release")
    int32 ReleaseYear;

    /** Calendar month when the song officially released. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Release")
    int32 ReleaseMonth;

    /** Flag indicating if the song has been released to the public. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Release")
    bool bIsReleased;
};
