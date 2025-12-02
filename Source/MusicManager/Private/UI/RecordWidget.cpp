#include "UI/RecordWidget.h"

#include "ArtistManagerSubsystem.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"
#include "RecordManagerSubsystem.h"
#include "Song.h"
#include "UI/RecordSongListItemWidget.h"

URecordWidget::URecordWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void URecordWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BindButtonDelegates();

    if (IsValid(SongListView))
    {
        SongListView->OnEntryWidgetGenerated().AddUObject(this, &URecordWidget::HandleEntryGenerated);
    }
}

void URecordWidget::InitializeForArtist(const FString& ArtistId)
{
    CurrentArtistId = ArtistId;
    SelectedSongIds.Reset();
    PopulateSongs(ArtistId);
}

void URecordWidget::OnConfirmPressed()
{
    const bool bSingleSelected = bIsSingle && bIsSingle->IsChecked();
    const bool bLPSelected = bIsLP && bIsLP->IsChecked();

    FRecordData NewRecord;
    NewRecord.ArtistId = CurrentArtistId;
    if (AlbumNameBox)
    {
        NewRecord.AlbumName = AlbumNameBox->GetText().ToString();
    }
    NewRecord.bIsSingle = bSingleSelected;
    NewRecord.bIsLP = bLPSelected;
    NewRecord.SongIds.Reset();
    for (const FString& Id : SelectedSongIds)
    {
        NewRecord.SongIds.Add(Id);
    }
    NewRecord.DateRecorded = FDateTime::Now();

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (URecordManagerSubsystem* RecordSubsystem = GameInstance->GetSubsystem<URecordManagerSubsystem>())
        {
            RecordSubsystem->CreateRecord(NewRecord);
        }
    }

    RemoveFromParent();
}

void URecordWidget::OnCancelPressed()
{
    RemoveFromParent();
}

void URecordWidget::NotifySongSelectionChanged(const FString& SongId, bool bIsSelected)
{
    if (bIsSelected)
    {
        SelectedSongIds.Add(SongId);
    }
    else
    {
        SelectedSongIds.Remove(SongId);
    }
}

void URecordWidget::HandleConfirmClicked()
{
    OnConfirmPressed();
}

void URecordWidget::HandleCancelClicked()
{
    OnCancelPressed();
}

void URecordWidget::HandleEntryGenerated(UUserWidget& EntryWidget)
{
    if (URecordSongListItemWidget* ListItem = Cast<URecordSongListItemWidget>(&EntryWidget))
    {
        ListItem->SetOwningRecordWidget(this);
    }
}

void URecordWidget::PopulateSongs(const FString& ArtistId)
{
    if (!IsValid(SongListView))
    {
        return;
    }

    SongListView->ClearListItems();

    UGameInstance* GameInstance = GetGameInstance();
    if (!IsValid(GameInstance))
    {
        return;
    }

    UArtistManagerSubsystem* ArtistSubsystem = GameInstance->GetSubsystem<UArtistManagerSubsystem>();
    if (!ArtistSubsystem)
    {
        return;
    }

    TArray<USong*> SongsForArtist;
    ArtistSubsystem->GetSongsForArtist(ArtistId, SongsForArtist);

    for (USong* Song : SongsForArtist)
    {
        if (!IsValid(Song))
        {
            continue;
        }

        URecordSongListEntryObject* EntryObject = NewObject<URecordSongListEntryObject>(this);
        EntryObject->SongId = Song->SongId;
        EntryObject->ArtistId = Song->ArtistId;
        EntryObject->SongData = Song->Data;
        EntryObject->OwningWidget = this;

        SongListView->AddItem(EntryObject);
    }
}

void URecordWidget::BindButtonDelegates()
{
    if (IsValid(ConfirmButton))
    {
        ConfirmButton->OnClicked.RemoveDynamic(this, &URecordWidget::HandleConfirmClicked);
        ConfirmButton->OnClicked.AddDynamic(this, &URecordWidget::HandleConfirmClicked);
    }

    if (IsValid(CancelButton))
    {
        CancelButton->OnClicked.RemoveDynamic(this, &URecordWidget::HandleCancelClicked);
        CancelButton->OnClicked.AddDynamic(this, &URecordWidget::HandleCancelClicked);
    }
}
