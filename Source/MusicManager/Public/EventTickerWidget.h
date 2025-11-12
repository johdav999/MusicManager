// File: Public/EventTickerWidget.h
#pragma once

#include "Blueprint/UserWidget.h"
#include "EventSubsystem.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "EventTickerWidget.generated.h"

class UHorizontalBox;
class UImage;
class UTextBlock;
class UButton;
class ULayout;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNewsCardClicked, UEventTickerWidget*, ClickedCard);

/**
 * Widget that presents a single music news event.
 */
UCLASS(BlueprintType, Blueprintable)
class UEventTickerWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UEventTickerWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;

    /** Raised when the button inside this card is clicked */
    UPROPERTY(BlueprintAssignable, Category="News")
    FOnNewsCardClicked OnNewsCardClicked;

    UFUNCTION(BlueprintCallable, Category="News")
    void SetNewsEvent(const FMusicNewsEvent& InEvent);

    UFUNCTION(BlueprintCallable, Category="Layout")
    void SetLayoutReference(ULayout* InLayout);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="News", meta=(ExposeOnSpawn=true))
    FMusicNewsEvent NewsEvent;

protected:
    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* HeadlineText;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* SourceText;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* TimestampText;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* SubjectText;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* BodyText;

    UPROPERTY(meta=(BindWidgetOptional))
    UHorizontalBox* TagContainer;

    UPROPERTY(meta=(BindWidgetOptional))
    UImage* CategoryIcon;

    /** Button in the widget (bound from UMG Blueprint) */
    UPROPERTY(meta=(BindWidget))
    UButton* ClickButton;

    UFUNCTION(BlueprintNativeEvent, Category="News")
    void Refresh();
    virtual void Refresh_Implementation();

private:
    UFUNCTION()
    void OnClickButton();

    EMusicNewsType CurrentNewsType = EMusicNewsType::None;

    TWeakObjectPtr<ULayout> LayoutRef;
};
