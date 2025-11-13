#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AuditionTypes.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "AuditionWidget.generated.h"

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
    USlider* SliderSignUpBonus = nullptr;

    UPROPERTY(meta = (BindWidget))
    USlider* SliderNumOfRecords = nullptr;

    UPROPERTY(meta = (BindWidget))
    USlider* SliderRoyaltyRate = nullptr;

    UPROPERTY(meta = (BindWidget))
    USlider* SliderContractYears = nullptr;

    UPROPERTY(meta = (BindWidget))
    UButton* ButtonSignArtist = nullptr;

    UPROPERTY(meta = (BindWidget))
    UButton* ButtonPass = nullptr;

    UPROPERTY(BlueprintAssignable, Category = "Audition")
    FOnAuditionDecision OnSignArtist;

    UPROPERTY(BlueprintAssignable, Category = "Audition")
    FOnAuditionDecision OnPass;

    UFUNCTION(BlueprintCallable)
    void RefreshDisplay();

    UFUNCTION(BlueprintCallable)
    void CreateDummyAudition();

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
    bool bHasInitializedTestData = false;
};
