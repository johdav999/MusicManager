// File: Public/NewsFeedItemWidget.h
#pragma once

#include "Blueprint/UserWidget.h"
#include "EventSubsystem.h"
#include "NewsFeedItemWidget.generated.h"

class UImage;
class UTextBlock;
class UNewsFeedList;
class UTexture2D;

/**
 * Lightweight news feed list item that reports hover state to the owning list.
 */
UCLASS(BlueprintType, Blueprintable)
class UNewsFeedItemWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UNewsFeedItemWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category="News")
    void SetupFromEvent(const FMusicNewsEvent& Event);

    UFUNCTION(BlueprintCallable, Category="News")
    const FMusicNewsEvent& GetNewsEvent() const;

    void SetOwnerList(UNewsFeedList* InOwnerList);

protected:
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* HeadlineText;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* SourceText;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* DateText;

    UPROPERTY(meta=(BindWidgetOptional))
    UImage* NewsTypeIcon;

    UPROPERTY(meta=(BindWidgetOptional))
    UImage* DateIcon;

    UPROPERTY(meta=(BindWidgetOptional))
    UImage* AccentDivider;

private:
    void ResolveWidgetBindings();
    void ApplyCachedEventToWidgets();
    void ApplyNewsTypeIcon(EMusicNewsType NewsType);
    FString ResolveSourceText() const;
    static FText FormatNewsDate(const FDateTime& Timestamp);
    static UTexture2D* ResolveNewsTypeTexture(EMusicNewsType NewsType);

    FMusicNewsEvent CachedEvent;
    bool bHasCachedEvent = false;

    UPROPERTY()
    TWeakObjectPtr<UNewsFeedList> OwnerList;
};
