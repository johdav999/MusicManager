#include "UI/RecordSongListItemWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "AuditionTypes.h"
#include "MusicManagerPlayerController.h"
#include "MusicPlayerComponent.h"
#include "RecordWidget.h"
#include "Song.h"
#include "SongManagerSubsystem.h"

URecordSongListItemWidget::URecordSongListItemWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void URecordSongListItemWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    Super::NativeOnListItemObjectSet(ListItemObject);

    if (const URecordSongListEntryObject* Entry = Cast<URecordSongListEntryObject>(ListItemObject))
    {
        OwningRecordWidget = Entry->OwningWidget;
        Setup(Entry->SongId, Entry->SongData);
        ArtistId = Entry->ArtistId;
    }
}

void URecordSongListItemWidget::Setup(const FString& InSongId, const FSongData& SongData)
{
    SongId = InSongId;
    bSelected = false;

    if (IsValid(SongNameText))
    {
        SongNameText->SetText(FText::FromString(SongData.SongName));
    }

    BindButtonDelegates();
}

void URecordSongListItemWidget::OnPlayClicked()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!IsValid(GameInstance))
    {
        return;
    }

    USongManagerSubsystem* SongManager = GameInstance->GetSubsystem<USongManagerSubsystem>();
    if (!SongManager)
    {
        return;
    }

    USong* Song = SongManager->GetSongById(SongId);
    if (!Song)
    {
        return;
    }

    if (APlayerController* PlayerController = World->GetFirstPlayerController())
    {
        if (AMusicManagerPlayerController* MusicController = Cast<AMusicManagerPlayerController>(PlayerController))
        {
            if (UMusicPlayerComponent* Player = MusicController->FindComponentByClass<UMusicPlayerComponent>())
            {
                FArtistData ArtistData;
                ArtistData.ArtistId = ArtistId;
                Player->PlaySong(Song, ArtistData);
            }
        }
    }
}

void URecordSongListItemWidget::OnAddClicked()
{
    bSelected = !bSelected;
    if (URecordWidget* OwnerPtr = OwningRecordWidget.Get())
    {
        OwnerPtr->NotifySongSelectionChanged(SongId, bSelected);
    }
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
