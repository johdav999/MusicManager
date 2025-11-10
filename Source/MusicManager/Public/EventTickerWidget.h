// File: Public/EventTickerWidget.h
#pragma once

#include "Blueprint/UserWidget.h"
#include "EventSubsystem.h"
#include "EventTickerWidget.generated.h"

class UHorizontalBox;
class UImage;
class UTextBlock;

/**
 * Widget that presents a single music news event.
 */
UCLASS()
class UEventTickerWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UEventTickerWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category="News")
    void SetNewsEvent(const FMusicNewsEvent& InEvent);

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

    UFUNCTION(BlueprintNativeEvent, Category="News")
    void Refresh();
    virtual void Refresh_Implementation();
};
