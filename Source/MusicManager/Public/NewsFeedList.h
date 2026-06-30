// File: Public/NewsFeedList.h
#pragma once

#include "Blueprint/UserWidget.h"
#include "EventSubsystem.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "NewsFeedList.generated.h"

class UVerticalBox;
class UScrollBox;
class UCanvasPanel;
class UButton;
class UImage;
class UTextBlock;
class UNewsFeedItemWidget;
class UEventTickerWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogNewsFeedList, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNewsFeedCardAdded, UNewsFeedItemWidget*, Card, const FMusicNewsEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNewsFeedItemSelected, UNewsFeedItemWidget*, Card, const FMusicNewsEvent&, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnViewAllNewsRequested);

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

    /** Broadcast after a news card has been created, populated, and inserted into the feed. */
    UPROPERTY(BlueprintAssignable, Category="News")
    FOnNewsFeedCardAdded OnNewsFeedCardAdded;

    /** Broadcast when a runtime news feed item is clicked by the player. */
    UPROPERTY(BlueprintAssignable, Category="News")
    FOnNewsFeedItemSelected OnNewsFeedItemSelected;

    UPROPERTY(BlueprintAssignable, Category="News")
    FOnViewAllNewsRequested OnViewAllNewsRequested;

    UFUNCTION(BlueprintCallable, Category="News")
    bool RemoveNewsCard(UNewsFeedItemWidget* Card);

    UFUNCTION(BlueprintCallable, Category="News")
    bool MoveNewsCardToTop(UNewsFeedItemWidget* Card);

    void HandleItemHovered(UNewsFeedItemWidget* Item);
    void HandleItemUnhovered(UNewsFeedItemWidget* Item);
    void HandleItemToggled(UNewsFeedItemWidget* Item);

    UEventTickerWidget* GetHoverTicker();

protected:
    /** Override this in derived widget Blueprints to react when a card is added to the feed. */
    UFUNCTION(BlueprintImplementableEvent, Category="News", meta=(DisplayName="On News Card Added"))
    void BP_OnNewsCardAdded(UNewsFeedItemWidget* Card, const FMusicNewsEvent& Event);

    UPROPERTY(meta=(BindWidget))
    UVerticalBox* FeedContainer;

    UPROPERTY(meta=(BindWidgetOptional))
    UCanvasPanel* RootCanvas;

    UPROPERTY(meta=(BindWidgetOptional))
    UImage* PanelBackgroundImage;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* HeaderText;

    UPROPERTY(meta=(BindWidgetOptional))
    UButton* ViewAllNewsButton;

    UPROPERTY(meta=(BindWidgetOptional))
    UImage* ViewAllNewsIcon;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* ViewAllNewsText;

    UPROPERTY(meta=(BindWidgetOptional))
    UImage* ViewAllNewsChevron;

    UPROPERTY(meta=(BindWidget))
    UScrollBox* FeedScrollBox;

    UPROPERTY(meta=(BindWidget))
    UCanvasPanel* HoverCanvas;

    UPROPERTY(EditAnywhere, Category="News")
    TSubclassOf<class UNewsFeedItemWidget> NewsFeedItemWidgetClass;

    /** Removes designer placeholder children from FeedContainer when the runtime feed constructs. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="News")
    bool bClearFeedOnConstruct = true;

    UPROPERTY(EditDefaultsOnly, Category="News")
    TSubclassOf<UEventTickerWidget> HoverTickerWidgetClass;

private:
    UFUNCTION()
    void HandleViewAllNewsClicked();

    void EnsureHoverTicker();
    void ShowHoverForItem(UNewsFeedItemWidget* Item, const FMusicNewsEvent& Event, const FGeometry& ItemGeometry);
    void HideHover();

    UPROPERTY(EditAnywhere)
    UEventTickerWidget* ActiveHoverTicker = nullptr;

    TWeakObjectPtr<UNewsFeedItemWidget> ActiveHoverItem;
};
