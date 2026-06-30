#pragma once

#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "FSongData.h"
#include "RecordSongListItemWidget.generated.h"

class UButton;
class UTextBlock;
class UMusicSegmentedMeterWidget;

UCLASS()
class MUSICMANAGER_API URecordSongListItemWidget : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    URecordSongListItemWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    void Setup(const FString& InSongId, const FSongData& SongData);

    UFUNCTION()
    void OnPlayClicked();

    UFUNCTION()
    void OnAddClicked();

    UFUNCTION()
    void OnRemoveClicked();

    void SetOwningRecordWidget(class URecordWidget* InOwner);

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Record")
    void DisplaySongMetadata(const FSongData& SongData);

protected:
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* SongNameText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* SongMetadataText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* SongQualityText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* GenreColumnText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* DurationColumnText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* PlayButtonText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UTextBlock* AddButtonText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UMusicSegmentedMeterWidget* PopularityMeter;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* PlayButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* AddButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* RemoveButton;

    UPROPERTY(BlueprintReadWrite)
    FString SongId;

    UPROPERTY(BlueprintReadWrite)
    FString ArtistId;

    UPROPERTY(BlueprintReadWrite)
    bool bIsRecordListItem = false;

    UPROPERTY()
    FSongData CachedSongData;

    bool bPreviewPlaying = false;

private:
    void BindButtonDelegates();
    void RefreshInteractionVisuals();

    UPROPERTY()
    TWeakObjectPtr<URecordWidget> OwningRecordWidget;
};