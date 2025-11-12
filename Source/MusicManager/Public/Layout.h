// File: Public/Layout.h
#pragma once

#include "Blueprint/UserWidget.h"
#include "EventSubsystem.h"
#include "EventTickerWidget.h"
#include "Layout.generated.h"

class UNewsFeedList;
class UUserWidget;
class UAuditionWidget;

/**
 * Layout widget that exposes helpers for locating child widgets by name or class.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNewsCardSelected, UEventTickerWidget*, SelectedCard);

UCLASS(BlueprintType, Blueprintable)
class ULayout final : public UUserWidget
{
    GENERATED_BODY()

public:
    ULayout(const FObjectInitializer& ObjectInitializer);

    /** Locate a user widget child by name or class, preferring the name when provided. */
    UFUNCTION(BlueprintCallable, Category="EventSubsystem")
    UUserWidget* GetChildByNameOrClass(FName WidgetName, TSubclassOf<UUserWidget> WidgetClass) const;

    UFUNCTION(BlueprintCallable, Category="News")
    void AddNewsCardToFeed(const FMusicNewsEvent& Event);

    UFUNCTION(BlueprintCallable, Category="News")
    void RemoveNewsCardFromFeed(UEventTickerWidget* Card);

    /** Raised when any card in the feed is clicked */
    UPROPERTY(BlueprintAssignable, Category="News")
    FOnNewsCardSelected OnNewsCardSelected;

    /** Called when a ticker widget is created and added to the feed */
    UFUNCTION(BlueprintCallable, Category="News")
    void BindTickerEvents(UEventTickerWidget* NewTicker);

    UFUNCTION(BlueprintCallable, Category="Layout")
    void ShowAuditionWidget();

protected:
    UPROPERTY(meta=(BindWidgetOptional))
    UNewsFeedList* NewsFeedList;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layout")
    UAuditionWidget* AuditionWidget;

private:
    UFUNCTION()
    void HandleTickerClicked(UEventTickerWidget* ClickedTicker);
};
