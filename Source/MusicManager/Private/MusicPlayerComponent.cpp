#include "MusicPlayerComponent.h"

#include "Components/AudioComponent.h"
#include "Components/SceneComponent.h"
#include "Song.h"
#include "TimerManager.h"
#include "Async/Async.h"
#include "Math/UnrealMathUtility.h"

UMusicPlayerComponent::UMusicPlayerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    AudioComponent = nullptr;
}

void UMusicPlayerComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeAudioComponent();
}

void UMusicPlayerComponent::PlaySong(USong* Song, const FArtistData& Artist)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<UMusicPlayerComponent> WeakThis(this);
        TWeakObjectPtr<USong> WeakSong(Song);
        const FArtistData ArtistCopy = Artist;

        AsyncTask(ENamedThreads::GameThread, [WeakThis, WeakSong, ArtistCopy]()
        {
            if (UMusicPlayerComponent* StrongThis = WeakThis.Get())
            {
                StrongThis->PlaySong(WeakSong.Get(), ArtistCopy);
            }
        });
        return;
    }

    InitializeAudioComponent();

    if (AudioComponent && Song && Song->Data.SoundWave)
    {
        AudioComponent->SetSound(Song->Data.SoundWave);

        AudioComponent->OnAudioFinished.Clear();
        AudioComponent->OnAudioFinished.AddDynamic(
            this,
            &UMusicPlayerComponent::OnAudioFinishedInternal
        );

        AudioComponent->Play();
        return;
    }

    PlayImprovisedPerformance(Artist);
}

void UMusicPlayerComponent::OnAudioFinishedInternal()
{
    HandlePlayFinished();
}

void UMusicPlayerComponent::PlayImprovisedPerformance(const FArtistData& Artist)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<UMusicPlayerComponent> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, Artist]()
        {
            if (UMusicPlayerComponent* StrongThis = WeakThis.Get())
            {
                StrongThis->PlayImprovisedPerformance(Artist);
            }
        });
        return;
    }

    InitializeAudioComponent();

    if (AudioComponent)
    {
        AudioComponent->Stop();
    }

    const float SimulatedDuration = FMath::Clamp(Artist.PerformanceScore / 10.f, 2.f, 8.f);
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(SimulatedPlaybackHandle, this, &UMusicPlayerComponent::HandlePlayFinished, SimulatedDuration, false);
    }
}

void UMusicPlayerComponent::Stop()
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<UMusicPlayerComponent> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (UMusicPlayerComponent* StrongThis = WeakThis.Get())
            {
                StrongThis->Stop();
            }
        });
        return;
    }

    if (AudioComponent && AudioComponent->IsPlaying())
    {
        AudioComponent->Stop();
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SimulatedPlaybackHandle);
    }
}

bool UMusicPlayerComponent::IsPlaying() const
{
    if (!IsInGameThread())
    {
        return false;
    }

    const bool bAudioPlaying = AudioComponent && AudioComponent->IsPlaying();
    const bool bTimerActive = SimulatedPlaybackHandle.IsValid();
    return bAudioPlaying || bTimerActive;
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

            if (USceneComponent* RootComponent = Owner->GetRootComponent())
            {
                AudioComponent->SetupAttachment(RootComponent);
            }

            AudioComponent->RegisterComponent();
        }
    }
}

void UMusicPlayerComponent::HandlePlayFinished()
{
    if (AudioComponent && AudioComponent->IsPlaying())
    {
        AudioComponent->Stop();
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SimulatedPlaybackHandle);
    }

    OnPerformanceFinished.Broadcast();
}
