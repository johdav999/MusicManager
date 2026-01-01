// File: Public/NewsFeedList.h
#pragma once

#include "Blueprint/UserWidget.h"
#include "EventSubsystem.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "NewsFeedList.generated.h"

class UVerticalBox;
class UScrollBox;
class UCanvasPanel;
class UNewsFeedItemWidget;
class UEventTickerWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogNewsFeedList, Log, All);

/**
 * News feed list widget that manages event ticker cards safely.
 */
UCLASS(BlueprintType, Blueprintable)
class UNewsFeedList : public UUserWidget
{
    GENERATED_BODY()

public:
    UNewsFeedList(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category="News")
    UNewsFeedItemWidget* AddNewsCard(const FMusicNewsEvent& Event);

    UFUNCTION(BlueprintCallable, Category="News")
    bool RemoveNewsCard(UNewsFeedItemWidget* Card);

    UFUNCTION(BlueprintCallable, Category="News")
    bool MoveNewsCardToTop(UNewsFeedItemWidget* Card);

    void HandleItemHovered(UNewsFeedItemWidget* Item);
    void HandleItemUnhovered(UNewsFeedItemWidget* Item);
    void HandleItemToggled(UNewsFeedItemWidget* Item);

    UEventTickerWidget* GetHoverTicker();

protected:
    UPROPERTY(meta=(BindWidget))
    UVerticalBox* FeedContainer;

    UPROPERTY(meta=(BindWidget))
    UScrollBox* FeedScrollBox;

    UPROPERTY(meta=(BindWidget))
    UCanvasPanel* HoverCanvas;

    UPROPERTY(EditAnywhere, Category="News")
    TSubclassOf<class UNewsFeedItemWidget> NewsFeedItemWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="News")
    TSubclassOf<UEventTickerWidget> HoverTickerWidgetClass;

private:
    void EnsureHoverTicker();
    void ShowHoverForItem(UNewsFeedItemWidget* Item, const FMusicNewsEvent& Event, const FGeometry& ItemGeometry);
    void HideHover();

    UPROPERTY(EditAnywhere)
    UEventTickerWidget* ActiveHoverTicker = nullptr;

    TWeakObjectPtr<UNewsFeedItemWidget> ActiveHoverItem;
};
