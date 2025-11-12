#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MusicPlayerComponent.generated.h"

class USoundBase;
class UAudioComponent;

/**
 * Actor component that manages a dedicated audio component so that Blueprints can easily play sounds on an actor.
 */
UCLASS(ClassGroup = (Audio), meta = (BlueprintSpawnableComponent))
class MUSICMANAGER_API UMusicPlayerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UMusicPlayerComponent();

    virtual void BeginPlay() override;

    /** Plays the assigned sound (or a sound set via SetSound) using the managed audio component. */
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void Play();

    /** Stops any sound currently playing on the managed audio component. */
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void Stop();

    /** Assigns a new sound to play. The sound will automatically be used for future Play calls. */
    UFUNCTION(BlueprintCallable, Category = "Audio")
    void SetSound(USoundBase* InSound);

    /** Returns true when the managed audio component is actively playing a sound. */
    UFUNCTION(BlueprintPure, Category = "Audio")
    bool IsPlaying() const;

    /** Provides access to the managed audio component for advanced Blueprint use cases. */
    UFUNCTION(BlueprintPure, Category = "Audio")
    UAudioComponent* GetAudioComponent() const { return AudioComponent; }

    /** Sound that will be played when calling Play or automatically at BeginPlay if enabled. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundBase* Sound;

    /** When enabled, the component will start playing the assigned sound as soon as BeginPlay runs. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    bool bPlayOnBeginPlay;

    /** Volume multiplier applied to the managed audio component. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0.0"))
    float VolumeMultiplier;

    /** Pitch multiplier applied to the managed audio component. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0.0"))
    float PitchMultiplier;

protected:
    /** Ensures that an audio component exists and is registered with the owning actor. */
    void InitializeAudioComponent();

private:
    /** Audio component used to actually play the requested sound. */
    UPROPERTY(Transient)
    UAudioComponent* AudioComponent;
};
