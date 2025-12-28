#include "UI/RecordWidget.h"

#include "ArtistManagerSubsystem.h"
#include "Async/Async.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"
#include "GameTimeSubsystem.h"
#include "RecordManagerSubsystem.h"
#include "Song.h"
#include "SongManagerSubsystem.h"
#include "UI/RecordSongListItemWidget.h"

namespace
{
    /**
     * Temporary, coarse-grained mapping from game year to default record formats.
     * The widget only suggests defaults; RecordManagerSubsystem will validate and prune.
     */
    TArray<ERecordFormat> GetDefaultFormatsForYear(const int32 CurrentYear)
    {
        TArray<ERecordFormat> Formats;

        if (CurrentYear >= 1950 && CurrentYear <= 1969)
        {
            Formats.Add(ERecordFormat::Vinyl);
        }
        else if (CurrentYear >= 1970 && CurrentYear <= 1988)
        {
            Formats.Add(ERecordFormat::Vinyl);
            Formats.Add(ERecordFormat::Cassette);
        }
        else if (CurrentYear >= 1989 && CurrentYear <= 1999)
        {
            Formats.Add(ERecordFormat::Cassette);
            Formats.Add(ERecordFormat::CD);
        }
        else if (CurrentYear >= 2000 && CurrentYear <= 2009)
        {
            Formats.Add(ERecordFormat::CD);
            Formats.Add(ERecordFormat::DigitalDownload);
        }
        else
        {
            // 2010 and beyond default to digital-first distribution.
            Formats.Add(ERecordFormat::DigitalDownload);
            Formats.Add(ERecordFormat::Streaming);
        }

        if (Formats.IsEmpty())
        {
            // Fallback to a modern-friendly format if year data is unavailable.
            Formats.Add(ERecordFormat::DigitalDownload);
        }

        return Formats;
    }
}

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

    if (IsValid(RecordSongListView))
    {
        RecordSongListView->OnEntryWidgetGenerated().AddUObject(this, &URecordWidget::HandleEntryGenerated);
    }
}

void URecordWidget::InitializeForArtist(const FString& ArtistId)
{
    CurrentArtistId = ArtistId;
    ArtistSongIds.Reset();
    RecordSongIds.Reset();
    SongDataById.Reset();
    if (IsValid(RecordSongListView))
    {
        RecordSongListView->ClearListItems();
    }
    PopulateSongsForArtist(ArtistId);
}

void URecordWidget::OnConfirmPressed()
{
    const bool bSingleSelected = bIsSingle && bIsSingle->IsChecked();
    const bool bLPSelected = bIsLP && bIsLP->IsChecked();

    if (bSingleSelected == bLPSelected)
    {
        UE_LOG(LogTemp, Warning, TEXT("RecordWidget: Must choose Single or LP."));
        return;
    }

    if (bSingleSelected && RecordSongIds.Num() != 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("RecordWidget: Singles require exactly one song."));
        return;
    }

    if (bLPSelected && RecordSongIds.Num() < 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("RecordWidget: LPs require more than one song."));
        return;
    }

    FRecordRecordingIntent Intent;
    Intent.ArtistId = CurrentArtistId;
    Intent.AlbumName = AlbumNameBox ? AlbumNameBox->GetText().ToString() : TEXT("");
    Intent.bIsSingle = bSingleSelected;
    Intent.bIsLP = bLPSelected;
    Intent.SongIds = RecordSongIds;

    int32 CurrentYear = 0;
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>())
        {
            CurrentYear = TimeSubsystem->GetCurrentGameDate().GetYear();
        }
    }

    // Suggest coarse defaults based solely on the current era; validation remains in the subsystem.
    Intent.RequestedFormats = GetDefaultFormatsForYear(CurrentYear);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (URecordManagerSubsystem* RecordSubsystem = GameInstance->GetSubsystem<URecordManagerSubsystem>())
        {
            FString Error;
            if (!RecordSubsystem->SubmitRecordingIntent(Intent, Error))
            {
                UE_LOG(LogTemp, Warning, TEXT("RecordWidget: Failed to submit recording intent - %s"), *Error);
                return;
            }
        }
    }

    if (UWidget* Parent = GetParent())
    {
        SetVisibility(ESlateVisibility::Collapsed);
    }
    else
    {
        RemoveFromParent();
    }
}

void URecordWidget::OnCancelPressed()
{
    if (UWidget* Parent = GetParent())
    {
        SetVisibility(ESlateVisibility::Collapsed);
    }
    else
    {
        RemoveFromParent();
    }
}

void URecordWidget::AddSongToRecord(const FString& SongId)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<URecordWidget> WeakThis(this);
        const FString CopyId = SongId;

        AsyncTask(ENamedThreads::GameThread, [WeakThis, CopyId]()
        {
            if (URecordWidget* Strong = WeakThis.Get())
            {
                Strong->AddSongToRecord(CopyId);
            }
        });
        return;
    }

    if (RecordSongIds.Contains(SongId))
    {
        return;
    }

    if (!SongDataById.Contains(SongId))
    {
        UE_LOG(LogTemp, Warning, TEXT("RecordWidget: Tried to add unknown song %s."), *SongId);
        return;
    }

    RecordSongIds.Add(SongId);
    RefreshRecordSongList();
}

void URecordWidget::RemoveSongFromRecord(const FString& SongId)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<URecordWidget> WeakThis(this);
        const FString CopyId = SongId;

        AsyncTask(ENamedThreads::GameThread, [WeakThis, CopyId]()
        {
            if (URecordWidget* Strong = WeakThis.Get())
            {
                Strong->RemoveSongFromRecord(CopyId);
            }
        });
        return;
    }

    if (!RecordSongIds.Remove(SongId))
    {
        return;
    }

    RefreshRecordSongList();
}

void URecordWidget::RefreshRecordSongList()
{
    if (!IsValid(RecordSongListView))
    {
        UE_LOG(LogTemp, Warning, TEXT("RefreshRecordSongList: RecordSongListView not bound."));
        return;
    }

    RecordSongListView->ClearListItems();

    for (const FString& SongId : RecordSongIds)
    {
        const FSongData* SongData = SongDataById.Find(SongId);
        if (!SongData)
        {
            continue;
        }

        URecordSongListEntryObject* Entry = NewObject<URecordSongListEntryObject>(this);
        Entry->SongId = SongId;
        Entry->SongData = *SongData;
        Entry->ArtistId = CurrentArtistId;
        Entry->OwningWidget = this;
        Entry->bIsRecordList = true;

        RecordSongListView->AddItem(Entry);
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

void URecordWidget::PopulateSongsForArtist(const FString& ArtistId)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<URecordWidget> WeakThis(this);
        const FString CopyId = ArtistId;

        AsyncTask(ENamedThreads::GameThread, [WeakThis, CopyId]()
        {
            if (URecordWidget* Strong = WeakThis.Get())
            {
                Strong->PopulateSongsForArtist(CopyId);
            }
        });
        return;
    }

    if (!SongListView)
    {
        UE_LOG(LogTemp, Warning, TEXT("PopulateSongsForArtist: SongListView not bound."));
        return;
    }

    SongListView->ClearListItems();
    ArtistSongIds.Reset();
    SongDataById.Reset();

    UGameInstance* GameInstance = GetGameInstance();
    if (!IsValid(GameInstance))
    {
        return;
    }

    USongManagerSubsystem* SongSubsystem = GameInstance->GetSubsystem<USongManagerSubsystem>();
    if (!IsValid(SongSubsystem))
    {
        return;
    }

    TArray<USong*> Songs;
    SongSubsystem->GetEligibleSongsForRecording(ArtistId, Songs);

    for (USong* Song : Songs)
    {
        if (!IsValid(Song))
        {
            continue;
        }

        const FSongData& Data = Song->Data;

        // Create entry object for the list
        URecordSongListEntryObject* Entry = NewObject<URecordSongListEntryObject>(this);
        Entry->SongId = Song->SongId;
        Entry->SongData = Data;
        Entry->ArtistId = ArtistId;
        Entry->OwningWidget = this;
        Entry->bIsRecordList = false;

        ArtistSongIds.Add(Song->SongId);
        SongDataById.Add(Song->SongId, Data);
        SongListView->AddItem(Entry);
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
