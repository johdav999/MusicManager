// File: Public/NewsFeedItemWidget.h
#pragma once

#include "Blueprint/UserWidget.h"
#include "EventSubsystem.h"
#include "NewsFeedItemWidget.generated.h"

class UImage;
class UPanelWidget;
class UTextBlock;
class UEventTickerWidget;
class UNewsFeedList;

/**
 * Lightweight news feed list item that owns a hover ticker detail widget.
 */
UCLASS(BlueprintType, Blueprintable)
class UNewsFeedItemWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UNewsFeedItemWidget(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable, Category="News")
    void SetupFromEvent(const FMusicNewsEvent& Event);

    UFUNCTION(BlueprintCallable, Category="News")
    UEventTickerWidget* GetHoverTicker() const;

    UFUNCTION(BlueprintCallable, Category="News")
    bool IsHoverTickerVisible() const;

    void SetOwnerList(UNewsFeedList* InOwnerList);
    void SetHoverTickerVisible(bool bVisible);

protected:
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    UPROPERTY(meta=(BindWidget))
    UTextBlock* HeadlineText;

    UPROPERTY(meta=(BindWidget))
    UImage* NewsTypeIcon;

    UPROPERTY(meta=(BindWidgetOptional))
    UPanelWidget* TickerContainer;

    UPROPERTY(EditDefaultsOnly, Category="News")
    TSubclassOf<UEventTickerWidget> HoverTickerWidgetClass;

private:
    void EnsureHoverTicker();
    void ApplyNewsTypeIcon(EMusicNewsType NewsType);

    bool bHoverVisible = false;

    UPROPERTY()
    UEventTickerWidget* HoverTicker;

    UPROPERTY()
    TWeakObjectPtr<UNewsFeedList> OwnerList;
};
