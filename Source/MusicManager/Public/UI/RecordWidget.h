#pragma once

#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "Containers/Map.h"
#include "FSongData.h"
#include "RecordWidget.generated.h"

class UButton;
class UCheckBox;
class UEditableTextBox;
class UListView;
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

protected:
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UCheckBox* bIsSingle;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UCheckBox* bIsLP;

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

private:
    UFUNCTION()
    void HandleConfirmClicked();

    UFUNCTION()
    void HandleCancelClicked();

    // NOTE: Not a UFUNCTION because UE delegates use a reference signature.
    void HandleEntryGenerated(UUserWidget& EntryWidget);

    UFUNCTION()
    void PopulateSongsForArtist(const FString& ArtistId);
    void RefreshRecordSongList();
    void BindButtonDelegates();

    FString CurrentArtistId;

    UPROPERTY()
    TArray<FString> ArtistSongIds;

    UPROPERTY()
    TArray<FString> RecordSongIds;

    UPROPERTY()
    TMap<FString, FSongData> SongDataById;
};
