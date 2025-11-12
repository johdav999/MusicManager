#include "MusicPlayerComponent.h"

#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundBase.h"

UMusicPlayerComponent::UMusicPlayerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    Sound = nullptr;
    bPlayOnBeginPlay = false;
    VolumeMultiplier = 1.0f;
    PitchMultiplier = 1.0f;
    AudioComponent = nullptr;
}

void UMusicPlayerComponent::BeginPlay()
{
    Super::BeginPlay();

    InitializeAudioComponent();

    if (AudioComponent)
    {
        AudioComponent->SetVolumeMultiplier(VolumeMultiplier);
        AudioComponent->SetPitchMultiplier(PitchMultiplier);

        if (Sound)
        {
            AudioComponent->SetSound(Sound);

            if (bPlayOnBeginPlay && !AudioComponent->IsPlaying())
            {
                AudioComponent->Play();
            }
        }
    }
}

void UMusicPlayerComponent::Play()
{
    InitializeAudioComponent();

    if (AudioComponent)
    {
        if (Sound)
        {
            AudioComponent->SetSound(Sound);
        }

        if (!AudioComponent->IsPlaying())
        {
            AudioComponent->Play();
        }
    }
}

void UMusicPlayerComponent::Stop()
{
    if (AudioComponent && AudioComponent->IsPlaying())
    {
        AudioComponent->Stop();
    }
}

void UMusicPlayerComponent::SetSound(USoundBase* InSound)
{
    Sound = InSound;

    if (AudioComponent)
    {
        AudioComponent->SetSound(Sound);
    }
}

bool UMusicPlayerComponent::IsPlaying() const
{
    return AudioComponent && AudioComponent->IsPlaying();
}

void UMusicPlayerComponent::InitializeAudioComponent()
{
    if (AudioComponent && AudioComponent->IsRegistered())
    {
        return;
    }

    AudioComponent = nullptr;

    if (AActor* Owner = GetOwner())
    {
        AudioComponent = NewObject<UAudioComponent>(Owner, TEXT("MusicPlayer_AudioComponent"));

        if (AudioComponent)
        {
            AudioComponent->bAutoActivate = false;
            AudioComponent->bAutoDestroy = false;
            AudioComponent->SetVolumeMultiplier(VolumeMultiplier);
            AudioComponent->SetPitchMultiplier(PitchMultiplier);

            if (USceneComponent* RootComponent = Owner->GetRootComponent())
            {
                AudioComponent->SetupAttachment(RootComponent);
            }

            AudioComponent->RegisterComponent();
        }
    }
}
