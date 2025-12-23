#include "UI/RecordSongListItemWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Async/Async.h"
#include "MusicPlayerComponent.h"
#include "UI/RecordWidget.h"
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

        Setup(Entry->SongId, Entry->SongData);
    }
}

void URecordSongListItemWidget::Setup(const FString& InSongId, const FSongData& SongData)
{
    SongId = InSongId;
    CachedSongData = SongData;
    bSelected = false;

    if (IsValid(SongNameText))
    {
        SongNameText->SetText(FText::FromString(SongData.SongName));
    }

    DisplaySongMetadata(SongData);
    UpdateSelectionVisuals(bSelected);

    if (PlayButton)
    {
        PlayButton->OnClicked.Clear();
        PlayButton->OnClicked.AddDynamic(this, &URecordSongListItemWidget::OnPlayClicked);
    }

    BindButtonDelegates();
}

void URecordSongListItemWidget::NotifySelectionChanged(bool bIsSelected)
{
    bSelected = bIsSelected;

    UpdateSelectionVisuals(bSelected);

    if (URecordWidget* OwnerPtr = OwningRecordWidget.Get())
    {
        OwnerPtr->NotifySongSelectionChanged(SongId, bSelected);
    }
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

                    Player->PlaySongData(CachedSongData, ArtistIdToUse);
                    return;
                }
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("OnPlayClicked: Could not resolve MusicPlayerComponent."));
}

void URecordSongListItemWidget::OnAddClicked()
{
    NotifySelectionChanged(!bSelected);
}

void URecordSongListItemWidget::SetOwningRecordWidget(URecordWidget* InOwner)
{
    OwningRecordWidget = InOwner;
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
}
