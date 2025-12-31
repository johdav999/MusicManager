// File: Public/NewsFeedList.h
#pragma once

#include "Blueprint/UserWidget.h"
#include "EventSubsystem.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "NewsFeedList.generated.h"

class UVerticalBox;
class UScrollBox;
class UNewsFeedItemWidget;

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

protected:
    UPROPERTY(meta=(BindWidget))
    UVerticalBox* FeedContainer;

    UPROPERTY(meta=(BindWidget))
    UScrollBox* FeedScrollBox;

    UPROPERTY(EditDefaultsOnly, Category="News")
    TSubclassOf<class UNewsFeedItemWidget> NewsFeedItemWidgetClass;

private:
    TWeakObjectPtr<UNewsFeedItemWidget> ActiveHoverItem;
};
