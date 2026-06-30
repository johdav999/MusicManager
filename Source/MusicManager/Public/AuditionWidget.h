#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AuditionTypes.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "UI/MusicGoldSlider.h"
#include "AuditionWidget.generated.h"

class UImage;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAuditionDecision);

UCLASS(Blueprintable)
class UAuditionWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FAuditionEvent AuditionData;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextArtistName = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextGenre = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextVenue = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextCity = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextPerformanceScore = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextStagePresence = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextAudienceEngagement = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextVocalQuality = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextSongwritingQuality = nullptr;

    UPROPERTY(meta = (BindWidget))
    UMusicGoldSlider* SliderSignUpBonus = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextSignUpBonusValue = nullptr;

    UPROPERTY(meta = (BindWidget))
    UMusicGoldSlider* SliderNumOfRecords = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextNumOfRecordsValue = nullptr;

    UPROPERTY(meta = (BindWidget))
    UMusicGoldSlider* SliderRoyaltyRate = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextRoyaltyRateValue = nullptr;

    UPROPERTY(meta = (BindWidget))
    UMusicGoldSlider* SliderContractYears = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextContractYearsValue = nullptr;

    UPROPERTY(meta = (BindWidget))
    UButton* ButtonSignArtist = nullptr;

    UPROPERTY(meta = (BindWidget))
    UButton* ButtonPass = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UImage* PanelBackgroundImage = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UImage* ArtistPortraitImage = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UImage* VinylFrameImage = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Audition|Visuals")
    UTexture2D* DefaultPortraitTexture = nullptr;

    UPROPERTY(BlueprintAssignable, Category = "Audition")
    FOnAuditionDecision OnSignArtist;

    UPROPERTY(BlueprintAssignable, Category = "Audition")
    FOnAuditionDecision OnPass;

    UFUNCTION(BlueprintCallable)
    void RefreshDisplay();

    UFUNCTION(BlueprintCallable)
    void CreateAuditionFromArtist(const FArtistData& Artist);

protected:
    virtual void NativeConstruct() override;




    UFUNCTION()
    void HandleSignUpBonusChanged(float Value);

    UFUNCTION()
    void HandleNumOfRecordsChanged(float Value);

    UFUNCTION()
    void HandleRoyaltyRateChanged(float Value);

    UFUNCTION()
    void HandleContractYearsChanged(float Value);

    UFUNCTION()
    void HandleSignArtistClicked();

    UFUNCTION()
    void HandlePassClicked();

    UFUNCTION(BlueprintImplementableEvent)
    void OnNegotiationValueChanged();

private:
    void ConfigureSliderRanges();
    void RefreshStatMeters();
    void RefreshPortrait();
    void RefreshDealValueTexts();
    static FText FormatCurrency(float Value);
    static FText FormatWholeNumber(float Value);
    static FText FormatRoyalty(float Value);
    static FText FormatYears(int32 Value);
};
