#include "UI/RecordSongListItemWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Async/Async.h"
#include "MusicPlayerComponent.h"
#include "UI/RecordWidget.h"
#include "UI/MusicSegmentedMeterWidget.h"
#include "UIManagerSubsystem.h"

URecordSongListItemWidget::URecordSongListItemWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void URecordSongListItemWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    if (URecordSongListEntryObject* Entry = Cast<URecordSongListEntryObject>(ListItemObject))
    {
        OwningRecordWidget = Entry->OwningWidget;
        ArtistId = Entry->ArtistId;
        bIsRecordListItem = Entry->bIsRecordList;

        Setup(Entry->SongId, Entry->SongData);
    }
}

void URecordSongListItemWidget::Setup(const FString& InSongId, const FSongData& SongData)
{
    SongId = InSongId;
    CachedSongData = SongData;

    if (IsValid(SongNameText))
    {
        SongNameText->SetText(FText::FromString(SongData.SongName));
    }

    if (IsValid(SongMetadataText))
    {
        SongMetadataText->SetText(FText::FromString(FString::Printf(
            TEXT("%s - %d - %s"),
            *SongData.Genre,
            SongData.YearCreated,
            SongData.SoundWave ? TEXT("Preview ready") : TEXT("No preview"))));
    }

    if (IsValid(SongQualityText))
    {
        const int32 Hit = FMath::RoundToInt(SongData.HitPotential);
        const int32 Catchiness = FMath::RoundToInt(SongData.Catchiness);
        const int32 Production = FMath::RoundToInt(SongData.ProductionQuality);
        SongQualityText->SetText(FText::FromString(FString::Printf(
            TEXT("Hit %d  Hook %d  Prod %d"),
            Hit,
            Catchiness,
            Production)));
    }

    if (IsValid(PopularityMeter))
    {
        const float PopularityScore = (SongData.HitPotential + SongData.Catchiness + SongData.ProductionQuality) / 300.f;
        PopularityMeter->SetPercent(FMath::Clamp(PopularityScore, 0.f, 1.f));
    }

    DisplaySongMetadata(SongData);

    if (IsValid(AddButton))
    {
        AddButton->SetVisibility(bIsRecordListItem ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }

    if (IsValid(RemoveButton))
    {
        RemoveButton->SetVisibility(bIsRecordListItem ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (PlayButton)
    {
        PlayButton->OnClicked.Clear();
        PlayButton->OnClicked.AddDynamic(this, &URecordSongListItemWidget::OnPlayClicked);
    }

    BindButtonDelegates();
    RefreshInteractionVisuals();
}

void URecordSongListItemWidget::OnPlayClicked()
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<URecordSongListItemWidget> WeakThis(this);

        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (URecordSongListItemWidget* StrongThis = WeakThis.Get())
            {
                StrongThis->OnPlayClicked();
            }
        });
        return;
    }

    // Validate song data
    if (!CachedSongData.SoundWave)
    {
        UE_LOG(LogTemp, Warning, TEXT("OnPlayClicked: No SoundWave available for playback."));
        return;
    }

    // Forward playback to MusicPlayerComponent via UIManagerSubsystem
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            if (UUIManagerSubsystem* UI = GameInstance->GetSubsystem<UUIManagerSubsystem>())
            {
                if (UMusicPlayerComponent* Player = UI->GetMusicPlayerComponent())
                {
                    // ArtistId may come from EntryObject or OwningRecordWidget
                    FString ArtistIdToUse = ArtistId;

                    if (bPreviewPlaying && Player->IsPlaying())
                    {
                        Player->Stop();
                        bPreviewPlaying = false;
                    }
                    else
                    {
                        Player->PlaySongData(CachedSongData, ArtistIdToUse);
                        bPreviewPlaying = true;
                    }
                    RefreshInteractionVisuals();
                    return;
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("OnPlayClicked: Could not resolve MusicPlayerComponent."));
}

void URecordSongListItemWidget::OnAddClicked()
{
    if (URecordWidget* OwnerPtr = OwningRecordWidget.Get())
    {
        OwnerPtr->AddSongToRecord(SongId);
    }
}

void URecordSongListItemWidget::OnRemoveClicked()
{
    if (URecordWidget* OwnerPtr = OwningRecordWidget.Get())
    {
        OwnerPtr->RemoveSongFromRecord(SongId);
    }
}

void URecordSongListItemWidget::SetOwningRecordWidget(URecordWidget* InOwner)
{
    OwningRecordWidget = InOwner;
    RefreshInteractionVisuals();
}

void URecordSongListItemWidget::BindButtonDelegates()
{
    if (IsValid(PlayButton))
    {
        PlayButton->OnClicked.RemoveDynamic(this, &URecordSongListItemWidget::OnPlayClicked);
        PlayButton->OnClicked.AddDynamic(this, &URecordSongListItemWidget::OnPlayClicked);
    }

    if (IsValid(AddButton))
    {
        AddButton->OnClicked.RemoveDynamic(this, &URecordSongListItemWidget::OnAddClicked);
        AddButton->OnClicked.AddDynamic(this, &URecordSongListItemWidget::OnAddClicked);
    }

    if (IsValid(RemoveButton))
    {
        RemoveButton->OnClicked.RemoveDynamic(this, &URecordSongListItemWidget::OnRemoveClicked);
        RemoveButton->OnClicked.AddDynamic(this, &URecordSongListItemWidget::OnRemoveClicked);
    }
}

void URecordSongListItemWidget::RefreshInteractionVisuals()
{
    if (IsValid(PlayButtonText))
    {
        PlayButtonText->SetText(FText::FromString(bPreviewPlaying ? TEXT("STOP") : TEXT("PLAY")));
    }

    if (IsValid(AddButtonText))
    {
        const bool bSelected = OwningRecordWidget.IsValid() && OwningRecordWidget->IsSongSelected(SongId);
        AddButtonText->SetText(FText::FromString(bSelected ? TEXT("ON") : TEXT("")));
    }
}