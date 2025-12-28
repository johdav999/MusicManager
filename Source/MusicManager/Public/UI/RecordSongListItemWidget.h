#pragma once

#include "Blueprint/UserWidget.h"

#include "Blueprint/IUserObjectListEntry.h"
#include "FSongData.h"
#include "RecordSongListItemWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class MUSICMANAGER_API URecordSongListItemWidget : public UUserWidget , public IUserObjectListEntry
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
    /**
     * Allow Blueprints to render song metadata however they like.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Record")
    void DisplaySongMetadata(const FSongData& SongData);

protected:
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* SongNameText;

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

private:
    void BindButtonDelegates();

    UPROPERTY()
    TWeakObjectPtr<URecordWidget> OwningRecordWidget;
};
