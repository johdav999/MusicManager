#pragma once

#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "Containers/Map.h"
#include "FSongData.h"
#include "RecordManagerSubsystem.h"
#include "Types/SlateEnums.h"
#include "RecordWidget.generated.h"

class UButton;
class UCheckBox;
class UComboBoxString;
class UEditableTextBox;
class UListView;
class UTextBlock;
class URecordSongListItemWidget;
class URecordManagerSubsystem;

UCLASS(BlueprintType)
class MUSICMANAGER_API URecordSongListEntryObject : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY()
    FString SongId;

    UPROPERTY()
    FString ArtistId;

    UPROPERTY()
    FSongData SongData;

    UPROPERTY()
    TWeakObjectPtr<class URecordWidget> OwningWidget;

    UPROPERTY()
    bool bIsRecordList = false;
};

UCLASS(BlueprintType, Blueprintable)
class MUSICMANAGER_API URecordWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    URecordWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;

    void InitializeForArtist(const FString& ArtistId);

    UFUNCTION(BlueprintCallable)
    void OnConfirmPressed();

    UFUNCTION(BlueprintCallable)
    void OnCancelPressed();

    void AddSongToRecord(const FString& SongId);
    void RemoveSongFromRecord(const FString& SongId);
    bool IsSongSelected(const FString& SongId) const;

protected:
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UCheckBox* bIsSingle;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UCheckBox* bIsLP;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UCheckBox* bIsEP;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UEditableTextBox* AlbumNameBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* ConfirmButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* CancelButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UListView* SongListView;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UListView* RecordSongListView;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UComboBoxString* GenreFilterBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UEditableTextBox* SearchTextBox;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* RecordingCostText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* RecordingDurationText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* RecordingWarningText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* ArtistNameText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* ArtistGenreText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* ArtistPopularityText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* ArtistFansText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* ArtistReputationText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* ReleaseFormatText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* ReleaseTracksText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* ReleaseDurationText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* SelectedTrackCountText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* TotalDurationText;

private:
    UFUNCTION()
    void HandleConfirmClicked();

    UFUNCTION()
    void HandleCancelClicked();

    UFUNCTION()
    void HandleSingleChanged(bool bIsChecked);

    UFUNCTION()
    void HandleEPChanged(bool bIsChecked);

    UFUNCTION()
    void HandleLPChanged(bool bIsChecked);

    UFUNCTION()
    void HandleGenreSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void HandleSearchTextChanged(const FText& NewText);

    // NOTE: Not a UFUNCTION because UE delegates use a reference signature.
    void HandleEntryGenerated(UUserWidget& EntryWidget);

    UFUNCTION()
    void PopulateSongsForArtist(const FString& ArtistId);
    void RefreshAvailableSongsList();
    void RefreshRecordSongList();
    void RefreshRecordingProjection();
    void RefreshArtistHeader(const FString& ArtistId);
    void RefreshSelectedTrackStats();
    void BindButtonDelegates();
    void BindFilterDelegates();
    ERecordType GetSelectedRecordType() const;
    void SelectRecordType(ERecordType RecordType);
    FString FormatSelectedDuration() const;
    bool SongMatchesCurrentFilters(const FSongData& SongData) const;

    FString CurrentArtistId;

    UPROPERTY()
    TArray<FString> ArtistSongIds;

    UPROPERTY()
    TArray<FString> RecordSongIds;

    UPROPERTY()
    TArray<TObjectPtr<URecordSongListEntryObject>> AvailableSongEntries;

    UPROPERTY()
    TMap<FString, FSongData> SongDataById;

    FString ActiveGenreFilter = TEXT("All Genres");
    FString ActiveSongSearch;
};