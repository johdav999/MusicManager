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

    /**
     * Blueprint widgets should call this when their selection UI changes.
     */
    UFUNCTION(BlueprintCallable, Category = "Record")
    void NotifySelectionChanged(bool bIsSelected);

    UFUNCTION()
    void OnPlayClicked();

    UFUNCTION()
    void OnAddClicked();

    void SetOwningRecordWidget(class URecordWidget* InOwner);

protected:
    /**
     * Allow Blueprints to render song metadata however they like.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Record")
    void DisplaySongMetadata(const FSongData& SongData);

    /**
     * Allow Blueprints to update visual state (checkboxes, highlights, etc.).
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Record")
    void UpdateSelectionVisuals(bool bIsSelected);

protected:
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UTextBlock* SongNameText;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
    UButton* PlayButton;

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    UButton* AddButton;

    UPROPERTY(BlueprintReadWrite)
    FString SongId;

    UPROPERTY(BlueprintReadWrite)
    FString ArtistId;

    UPROPERTY(BlueprintReadWrite)
    bool bSelected = false;

    UPROPERTY()
    FSongData CachedSongData;

private:
    void BindButtonDelegates();

    UPROPERTY()
    TWeakObjectPtr<URecordWidget> OwningRecordWidget;
};
