#pragma once

#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
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

    void NotifySongSelectionChanged(const FString& SongId, bool bIsSelected);

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

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UListView* SongListView;

private:
    UFUNCTION()
    void HandleConfirmClicked();

    UFUNCTION()
    void HandleCancelClicked();

    UFUNCTION()

    void HandleEntryGenerated(UUserWidget* EntryWidget);


    void PopulateSongs(const FString& ArtistId);
    void BindButtonDelegates();

    FString CurrentArtistId;

    UPROPERTY()
    TSet<FString> SelectedSongIds;
};
