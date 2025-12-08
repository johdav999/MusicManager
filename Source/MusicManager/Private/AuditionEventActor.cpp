#include "AuditionEventActor.h"

#include "ArtistManagerSubsystem.h"
#include "Async/Async.h"
#include "MusicPlayerComponent.h"
#include "UIManagerSubsystem.h"
#include "SongManagerSubsystem.h"
#include "Song.h"
#include "Math/UnrealMathUtility.h"

AAuditionEventActor::AAuditionEventActor()
{
    PrimaryActorTick.bCanEverTick = false;

    MusicPlayer = CreateDefaultSubobject<UMusicPlayerComponent>(TEXT("MusicPlayer"));
}

void AAuditionEventActor::BeginPlay()
{
    Super::BeginPlay();

    if (UMusicPlayerComponent* MPC = MusicPlayer)
    {
        if (UGameInstance* GI = GetGameInstance())
        {
            if (UUIManagerSubsystem* UI = GI->GetSubsystem<UUIManagerSubsystem>())
            {
                UI->RegisterMusicPlayerComponent(MPC);
            }
        }
    }
}

void AAuditionEventActor::StartAudition()
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<AAuditionEventActor> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (AAuditionEventActor* StrongThis = WeakThis.Get())
            {
                StrongThis->StartAudition();
            }
        });
        return;
    }

    bool bRegisterMusicPlayer = false;

    if (!MusicPlayer)
    {
        MusicPlayer = NewObject<UMusicPlayerComponent>(this, TEXT("MusicPlayerRuntime"));
        bRegisterMusicPlayer = true;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return;
    }

    if (bRegisterMusicPlayer)
    {
        if (UUIManagerSubsystem* UI = GameInstance->GetSubsystem<UUIManagerSubsystem>())
        {
            UI->RegisterMusicPlayerComponent(MusicPlayer);
        }
    }

    UArtistManagerSubsystem* ArtistManager = GameInstance->GetSubsystem<UArtistManagerSubsystem>();
    USongManagerSubsystem* SongManager = GameInstance->GetSubsystem<USongManagerSubsystem>();

    if (!ArtistManager || !SongManager)
    {
        return;
    }

    FArtistData Artist;
    if (!ArtistManager->GetNextUnsignedArtist(Artist))
    {
        return;
    }

    CurrentArtistId = Artist.ArtistName;

    TArray<USong*> ArtistSongs;
    SongManager->GetSongsForArtist(CurrentArtistId, ArtistSongs);

    if (ArtistSongs.Num() > 0)
    {
        CurrentSong = ArtistSongs[FMath::RandRange(0, ArtistSongs.Num() - 1)];
    }
    else
    {
        FSongData ImprovisedData;
        ImprovisedData.SongName = FString::Printf(TEXT("%s – Live Improvisation"), *Artist.ArtistName);
        ImprovisedData.Genre = Artist.Genre;

        CurrentSong = SongManager->CreateSong(CurrentArtistId, ImprovisedData);
    }

    if (MusicPlayer && !MusicPlayer->OnPerformanceFinished.IsAlreadyBound(this, &AAuditionEventActor::HandlePerformanceFinished))
    {
        MusicPlayer->OnPerformanceFinished.AddDynamic(this, &AAuditionEventActor::HandlePerformanceFinished);
    }

    if (MusicPlayer)
    {
        if (USong* Song = CurrentSong.Get())
        {
            MusicPlayer->PlaySong(Song, Artist);
        }
        else
        {
            MusicPlayer->PlayImprovisedPerformance(Artist);
        }
    }

    BeginPerformanceScoring(Artist, CurrentSong.Get());
}

void AAuditionEventActor::FinalizeDeal(bool bAcceptDeal)
{
    // Stop the audition music
    StopAuditionMusic();

    AuditionData.bSignedArtist = bAcceptDeal;
    AuditionData.Outcome = bAcceptDeal ? TEXT("Deal Accepted") : TEXT("Deal Rejected");
    OnNegotiationUpdated.Broadcast();
}

void AAuditionEventActor::BeginPerformanceScoring(const FArtistData& /*Artist*/, USong* /*Song*/)
{
    // Placeholder for gameplay tuning. Hook into analytics or scoring logic here.
}

void AAuditionEventActor::HandlePerformanceFinished()
{
    FinalizePerformance();
}

void AAuditionEventActor::FinalizePerformance()
{
    FinalizePerformanceResults();
}

void AAuditionEventActor::FinalizePerformanceResults()
{
    // update ArtistManagerSubsystem with scoring results
    // increase stage presence, fan engagement, etc.
    // trigger news/event subsystem if it exists
}

void AAuditionEventActor::StopAuditionMusic()
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<AAuditionEventActor> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (AAuditionEventActor* StrongThis = WeakThis.Get())
            {
                StrongThis->StopAuditionMusic();
            }
        });
        return;
    }

    if (MusicPlayer)
    {
        MusicPlayer->Stop();
    }
}
