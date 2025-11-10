// File: Public/NewsFeedList.h
#pragma once

#include "Blueprint/UserWidget.h"
#include "EventSubsystem.h"
#include "NewsFeedList.generated.h"

class UVerticalBox;
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

    UFUNCTION(BlueprintCallable, Category="News")
    UEventTickerWidget* AddNewsCard(const FMusicNewsEvent& Event);

    UFUNCTION(BlueprintCallable, Category="News")
    bool RemoveNewsCard(UEventTickerWidget* Card);

    UFUNCTION(BlueprintCallable, Category="News")
    bool MoveNewsCardToTop(UEventTickerWidget* Card);

protected:
    UPROPERTY(meta=(BindWidget))
    UVerticalBox* FeedContainer;

    UPROPERTY(EditDefaultsOnly, Category="News")
    TSubclassOf<class UEventTickerWidget> EventTickerWidgetClass;
};
