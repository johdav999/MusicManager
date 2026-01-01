// File: Public/NewsFeedItemWidget.h
#pragma once

#include "Blueprint/UserWidget.h"
#include "EventSubsystem.h"
#include "NewsFeedItemWidget.generated.h"

class UImage;
class UTextBlock;
class UNewsFeedList;

/**
 * Lightweight news feed list item that reports hover state to the owning list.
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
    const FMusicNewsEvent& GetNewsEvent() const;

    void SetOwnerList(UNewsFeedList* InOwnerList);

protected:
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    UPROPERTY(meta=(BindWidget))
    UTextBlock* HeadlineText;

    UPROPERTY(meta=(BindWidget))
    UImage* NewsTypeIcon;

private:
    void ApplyNewsTypeIcon(EMusicNewsType NewsType);

    FMusicNewsEvent CachedEvent;

    UPROPERTY()
    TWeakObjectPtr<UNewsFeedList> OwnerList;
};
