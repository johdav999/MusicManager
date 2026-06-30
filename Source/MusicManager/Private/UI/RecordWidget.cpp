#include "UI/RecordWidget.h"

#include "ArtistManagerSubsystem.h"
#include "Async/Async.h"
#include "CommandDispatcherSubsystem.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
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
    SelectRecordType(ERecordType::Single);

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
    RefreshArtistHeader(ArtistId);
    ArtistSongIds.Reset();
    RecordSongIds.Reset();
    SongDataById.Reset();
    if (IsValid(RecordSongListView))
    {
        RecordSongListView->ClearListItems();
    }
    UE_LOG(LogTemp, Log, TEXT("RecordWidget: InitializeForArtist ArtistId='%s'."), *ArtistId);
    PopulateSongsForArtist(ArtistId);
    RefreshRecordingProjection();
}

void URecordWidget::OnConfirmPressed()
{
    const ERecordType RecordType = GetSelectedRecordType();

    FStartRecordingCommand Command;
    Command.ArtistId = CurrentArtistId;
    Command.RecordTitle = AlbumNameBox ? AlbumNameBox->GetText().ToString() : TEXT("");
    Command.RecordType = RecordType;
    Command.bIsSingle = RecordType == ERecordType::Single;
    Command.bIsLP = RecordType == ERecordType::LP;
    Command.SongIds = RecordSongIds;

    int32 CurrentYear = 0;
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>())
        {
            CurrentYear = TimeSubsystem->GetCurrentGameDate().GetYear();
        }
    }

    // Suggest coarse defaults based solely on the current era; validation remains in the subsystem.
    Command.RequestedFormats = GetDefaultFormatsForYear(CurrentYear);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UCommandDispatcherSubsystem* Dispatcher = GameInstance->GetSubsystem<UCommandDispatcherSubsystem>())
        {
            const FMusicCommandResult Result = Dispatcher->ExecuteStartRecording(Command);
            if (!Result.bSuccess)
            {
                UE_LOG(LogTemp, Warning, TEXT("RecordWidget: Failed to submit recording command - %s"), *Result.Message.ToString());
                if (RecordingWarningText)
                {
                    RecordingWarningText->SetText(Result.Message);
                }
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
    RefreshSelectedTrackStats();
    RefreshRecordingProjection();
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
    RefreshSelectedTrackStats();
    RefreshRecordingProjection();
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

void URecordWidget::HandleSingleChanged(bool bIsChecked)
{
    if (bIsChecked)
    {
        SelectRecordType(ERecordType::Single);
    }
}

void URecordWidget::HandleEPChanged(bool bIsChecked)
{
    if (bIsChecked)
    {
        SelectRecordType(ERecordType::EP);
    }
}

void URecordWidget::HandleLPChanged(bool bIsChecked)
{
    if (bIsChecked)
    {
        SelectRecordType(ERecordType::LP);
    }
}

void URecordWidget::HandleGenreSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    ActiveGenreFilter = SelectedItem.IsEmpty() ? TEXT("All Genres") : SelectedItem;
    RefreshAvailableSongsList();
}

void URecordWidget::HandleSearchTextChanged(const FText& NewText)
{
    ActiveSongSearch = NewText.ToString();
    RefreshAvailableSongsList();
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
    AvailableSongEntries.Reset();
    SongDataById.Reset();

    ActiveGenreFilter = TEXT("All Genres");
    ActiveSongSearch.Reset();
    if (GenreFilterBox)
    {
        GenreFilterBox->ClearOptions();
        GenreFilterBox->AddOption(TEXT("All Genres"));
        GenreFilterBox->SetSelectedOption(TEXT("All Genres"));
    }
    if (SearchTextBox)
    {
        SearchTextBox->SetText(FText::GetEmpty());
    }

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
    FString ArtistGenre;
    if (UArtistManagerSubsystem* ArtistSubsystem = GameInstance->GetSubsystem<UArtistManagerSubsystem>())
    {
        if (const FArtistContract* Contract = ArtistSubsystem->GetContractByArtistId(ArtistId))
        {
            ArtistGenre = Contract->ArtistData.Genre;
            UE_LOG(LogTemp, Log, TEXT("RecordWidget: Found artist contract ArtistId='%s' Name='%s' Genre='%s'."),
                *Contract->ArtistId,
                *Contract->ArtistData.ArtistName,
                *ArtistGenre);
        }
        else if (const FArtistContract* ContractByName = ArtistSubsystem->FindContractByArtistName(ArtistId))
        {
            ArtistGenre = ContractByName->ArtistData.Genre;
            UE_LOG(LogTemp, Warning, TEXT("RecordWidget: ArtistId '%s' matched contract by artist name. Stable ArtistId is '%s', Genre='%s'."),
                *ArtistId,
                *ContractByName->ArtistId,
                *ArtistGenre);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("RecordWidget: No active contract found for ArtistId='%s'; querying all eligible artist songs."), *ArtistId);
        }
    }

    if (!ArtistGenre.IsEmpty())
    {
        SongSubsystem->GetEligibleSongsForRecordingByGenre(ArtistId, ArtistGenre, Songs);
    }
    else
    {
        SongSubsystem->GetEligibleSongsForRecording(ArtistId, Songs);
    }

    UE_LOG(LogTemp, Log, TEXT("RecordWidget: Eligible song query returned %d songs for ArtistId='%s' Genre='%s'."),
        Songs.Num(),
        *ArtistId,
        *ArtistGenre);

    TSet<FString> GenreOptions;

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
        AvailableSongEntries.Add(Entry);
        if (!Data.Genre.IsEmpty())
        {
            GenreOptions.Add(Data.Genre);
        }
    }

    if (GenreFilterBox)
    {
        TArray<FString> SortedGenres = GenreOptions.Array();
        SortedGenres.Sort();
        for (const FString& Genre : SortedGenres)
        {
            GenreFilterBox->AddOption(Genre);
        }
        GenreFilterBox->SetSelectedOption(TEXT("All Genres"));
    }

    RefreshAvailableSongsList();

    if (Songs.IsEmpty() && RecordingWarningText)
    {
        RecordingWarningText->SetText(FText::FromString(ArtistGenre.IsEmpty()
            ? TEXT("No eligible songs are available for this artist.")
            : FString::Printf(TEXT("No eligible %s songs are available for this artist."), *ArtistGenre)));
    }
}

void URecordWidget::RefreshAvailableSongsList()
{
    if (!IsValid(SongListView))
    {
        return;
    }

    SongListView->ClearListItems();

    int32 VisibleCount = 0;
    for (URecordSongListEntryObject* Entry : AvailableSongEntries)
    {
        if (!Entry)
        {
            continue;
        }

        if (!SongMatchesCurrentFilters(Entry->SongData))
        {
            continue;
        }

        SongListView->AddItem(Entry);
        ++VisibleCount;
    }

    if (RecordingWarningText && AvailableSongEntries.Num() > 0 && VisibleCount == 0)
    {
        RecordingWarningText->SetText(FText::FromString(TEXT("No songs match the current genre/search filters.")));
    }
}

bool URecordWidget::SongMatchesCurrentFilters(const FSongData& SongData) const
{
    const bool bGenreMatches = ActiveGenreFilter.IsEmpty()
        || ActiveGenreFilter.Equals(TEXT("All Genres"), ESearchCase::IgnoreCase)
        || SongData.Genre.Equals(ActiveGenreFilter, ESearchCase::IgnoreCase);

    const FString Search = ActiveSongSearch.TrimStartAndEnd();
    const bool bSearchMatches = Search.IsEmpty()
        || SongData.SongName.Contains(Search, ESearchCase::IgnoreCase)
        || SongData.Genre.Contains(Search, ESearchCase::IgnoreCase);

    return bGenreMatches && bSearchMatches;
}

bool URecordWidget::IsSongSelected(const FString& SongId) const
{
    return RecordSongIds.Contains(SongId);
}
void URecordWidget::RefreshArtistHeader(const FString& ArtistId)
{
    UGameInstance* GameInstance = GetGameInstance();
    UArtistManagerSubsystem* ArtistSubsystem = GameInstance ? GameInstance->GetSubsystem<UArtistManagerSubsystem>() : nullptr;
    const FArtistContract* Contract = ArtistSubsystem ? ArtistSubsystem->GetContractByArtistId(ArtistId) : nullptr;
    if (!Contract && ArtistSubsystem)
    {
        Contract = ArtistSubsystem->FindContractByArtistName(ArtistId);
    }

    if (!Contract)
    {
        if (ArtistNameText) ArtistNameText->SetText(FText::FromString(TEXT("No artist selected")));
        if (ArtistGenreText) ArtistGenreText->SetText(FText::GetEmpty());
        if (ArtistPopularityText) ArtistPopularityText->SetText(FText::FromString(TEXT("0")));
        if (ArtistFansText) ArtistFansText->SetText(FText::FromString(TEXT("--")));
        if (ArtistReputationText) ArtistReputationText->SetText(FText::FromString(TEXT("0")));
        return;
    }

    const FArtistData& Artist = Contract->ArtistData;
    if (ArtistNameText) ArtistNameText->SetText(FText::FromString(Artist.ArtistName.IsEmpty() ? Contract->ArtistId : Artist.ArtistName));
    if (ArtistGenreText) ArtistGenreText->SetText(FText::FromString(Artist.Genre));
    if (ArtistPopularityText) ArtistPopularityText->SetText(FText::AsNumber(FMath::RoundToInt(Artist.AudienceEngagement)));
    if (ArtistFansText) ArtistFansText->SetText(FText::FromString(TEXT("--")));
    if (ArtistReputationText) ArtistReputationText->SetText(FText::AsNumber(FMath::RoundToInt((Artist.PerformanceScore + Artist.StagePresence) * 0.5f)));
}

FString URecordWidget::FormatSelectedDuration() const
{
    float TotalSeconds = 0.f;
    bool bHasDuration = false;
    for (const FString& SongId : RecordSongIds)
    {
        if (const FSongData* SongData = SongDataById.Find(SongId))
        {
            if (SongData->SoundWave)
            {
                TotalSeconds += SongData->SoundWave->Duration;
                bHasDuration = true;
            }
        }
    }

    if (!bHasDuration)
    {
        return TEXT("--:--");
    }

    const int32 RoundedSeconds = FMath::Max(0, FMath::RoundToInt(TotalSeconds));
    return FString::Printf(TEXT("%02d:%02d"), RoundedSeconds / 60, RoundedSeconds % 60);
}

void URecordWidget::RefreshSelectedTrackStats()
{
    const int32 TrackCount = RecordSongIds.Num();
    const int32 MaxTracks = GetSelectedRecordType() == ERecordType::Single ? 2 : (GetSelectedRecordType() == ERecordType::EP ? 6 : 14);
    const FString Duration = FormatSelectedDuration();

    if (SelectedTrackCountText)
    {
        SelectedTrackCountText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d TRACKS"), TrackCount, MaxTracks)));
    }
    if (TotalDurationText)
    {
        TotalDurationText->SetText(FText::FromString(Duration));
    }
    if (ReleaseTracksText)
    {
        ReleaseTracksText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), TrackCount, MaxTracks)));
    }
    if (ReleaseDurationText)
    {
        ReleaseDurationText->SetText(FText::FromString(Duration));
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

    if (IsValid(bIsSingle))
    {
        bIsSingle->OnCheckStateChanged.RemoveDynamic(this, &URecordWidget::HandleSingleChanged);
        bIsSingle->OnCheckStateChanged.AddDynamic(this, &URecordWidget::HandleSingleChanged);
    }

    if (IsValid(bIsEP))
    {
        bIsEP->OnCheckStateChanged.RemoveDynamic(this, &URecordWidget::HandleEPChanged);
        bIsEP->OnCheckStateChanged.AddDynamic(this, &URecordWidget::HandleEPChanged);
    }

    if (IsValid(bIsLP))
    {
        bIsLP->OnCheckStateChanged.RemoveDynamic(this, &URecordWidget::HandleLPChanged);
        bIsLP->OnCheckStateChanged.AddDynamic(this, &URecordWidget::HandleLPChanged);
    }
}

void URecordWidget::RefreshRecordingProjection()
{
    UGameInstance* GameInstance = GetGameInstance();
    URecordManagerSubsystem* RecordSubsystem = GameInstance ? GameInstance->GetSubsystem<URecordManagerSubsystem>() : nullptr;
    if (!RecordSubsystem)
    {
        return;
    }

    FRecordRecordingIntent Intent;
    Intent.ArtistId = CurrentArtistId;
    Intent.AlbumName = AlbumNameBox ? AlbumNameBox->GetText().ToString() : TEXT("");
    Intent.RecordType = GetSelectedRecordType();
    Intent.bIsSingle = Intent.RecordType == ERecordType::Single;
    Intent.bIsLP = Intent.RecordType == ERecordType::LP;
    Intent.SongIds = RecordSongIds;

    FRecordingProjection Projection;
    FString Error;
    RecordSubsystem->BuildRecordingProjection(Intent, Projection, Error);

    if (RecordingCostText)
    {
        RecordingCostText->SetText(FText::FromString(FString::Printf(TEXT("$%.0f"), Projection.EstimatedRecordingCost)));
    }
    if (RecordingDurationText)
    {
        RecordingDurationText->SetText(FText::FromString(Projection.EstimatedCompletionDate.ToString(TEXT("%b %d, %Y"))));
    }
    if (RecordingWarningText)
    {
        RecordingWarningText->SetText(Error.IsEmpty() ? FText::GetEmpty() : FText::FromString(Error));
    }
}

ERecordType URecordWidget::GetSelectedRecordType() const
{
    if (bIsEP && bIsEP->IsChecked())
    {
        return ERecordType::EP;
    }
    if (bIsLP && bIsLP->IsChecked())
    {
        return ERecordType::LP;
    }
    return ERecordType::Single;
}

void URecordWidget::SelectRecordType(ERecordType RecordType)
{
    if (bIsSingle)
    {
        bIsSingle->SetIsChecked(RecordType == ERecordType::Single);
    }
    if (bIsEP)
    {
        bIsEP->SetIsChecked(RecordType == ERecordType::EP);
    }
    if (bIsLP)
    {
        bIsLP->SetIsChecked(RecordType == ERecordType::LP);
    }
    if (ReleaseFormatText)
    {
        const FString FormatName = RecordType == ERecordType::Single ? TEXT("Single") : (RecordType == ERecordType::EP ? TEXT("EP") : TEXT("LP"));
        ReleaseFormatText->SetText(FText::FromString(FormatName));
    }
    RefreshSelectedTrackStats();
    RefreshRecordingProjection();
}
