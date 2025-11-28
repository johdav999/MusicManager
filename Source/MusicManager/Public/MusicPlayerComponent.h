#pragma once

#include "AuditionTypes.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "MusicPlayerComponent.generated.h"

class USong;
class USoundBase;
class UAudioComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPerformanceFinished);

/**
 * Actor component that can play a song (real or simulated) for auditions and performances.
 */
UCLASS(ClassGroup = (Audio), meta = (BlueprintSpawnableComponent))
class MUSICMANAGER_API UMusicPlayerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMusicPlayerComponent();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Music")
    void PlaySong(USong* Song, const FArtistData& Artist);

    UFUNCTION(BlueprintCallable, Category = "Music")
    void PlayImprovisedPerformance(const FArtistData& Artist);

    UFUNCTION(BlueprintCallable, Category = "Music")
    void Stop();

    UFUNCTION(BlueprintPure, Category = "Music")
    bool IsPlaying() const;

    UPROPERTY(BlueprintAssignable, Category = "Music")
    FOnPerformanceFinished OnPerformanceFinished;

protected:
    void InitializeAudioComponent();
    void HandlePlayFinished();

    UFUNCTION()
    void OnAudioFinishedInternal();

private:
    UPROPERTY()
    UAudioComponent* AudioComponent;

    FTimerHandle SimulatedPlaybackHandle;
};
