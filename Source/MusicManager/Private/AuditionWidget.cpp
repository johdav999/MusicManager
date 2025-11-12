#include "AuditionWidget.h"

void UAuditionWidget::NativeConstruct()
{
    Super::NativeConstruct();

    RefreshDisplay();

    if (SliderSignUpBonus)
    {
        SliderSignUpBonus->OnValueChanged.RemoveAll(this);
        SliderSignUpBonus->OnValueChanged.AddDynamic(this, &UAuditionWidget::HandleSignUpBonusChanged);
    }

    if (SliderNumOfRecords)
    {
        SliderNumOfRecords->OnValueChanged.RemoveAll(this);
        SliderNumOfRecords->OnValueChanged.AddDynamic(this, &UAuditionWidget::HandleNumOfRecordsChanged);
    }

    if (SliderRoyaltyRate)
    {
        SliderRoyaltyRate->OnValueChanged.RemoveAll(this);
        SliderRoyaltyRate->OnValueChanged.AddDynamic(this, &UAuditionWidget::HandleRoyaltyRateChanged);
    }

    if (SliderContractYears)
    {
        SliderContractYears->OnValueChanged.RemoveAll(this);
        SliderContractYears->OnValueChanged.AddDynamic(this, &UAuditionWidget::HandleContractYearsChanged);
    }
}

void UAuditionWidget::RefreshDisplay()
{
    if (TextArtistName)
    {
        TextArtistName->SetText(FText::FromString(AuditionData.ArtistData.ArtistName));
    }

    if (TextGenre)
    {
        TextGenre->SetText(FText::FromString(AuditionData.ArtistData.Genre));
    }

    if (TextVenue)
    {
        TextVenue->SetText(FText::FromString(AuditionData.VenueName));
    }

    if (TextCity)
    {
        TextCity->SetText(FText::FromString(AuditionData.City));
    }

    if (TextPerformanceScore)
    {
        TextPerformanceScore->SetText(FText::AsNumber(AuditionData.ArtistData.PerformanceScore));
    }

    if (TextStagePresence)
    {
        TextStagePresence->SetText(FText::AsNumber(AuditionData.ArtistData.StagePresence));
    }

    if (TextAudienceEngagement)
    {
        TextAudienceEngagement->SetText(FText::AsNumber(AuditionData.ArtistData.AudienceEngagement));
    }

    if (TextVocalQuality)
    {
        TextVocalQuality->SetText(FText::AsNumber(AuditionData.ArtistData.VocalQuality));
    }

    if (TextSongwritingQuality)
    {
        TextSongwritingQuality->SetText(FText::AsNumber(AuditionData.ArtistData.SongwritingQuality));
    }

    if (SliderSignUpBonus)
    {
        SliderSignUpBonus->SetValue(AuditionData.DealData.SignUpBonus);
    }

    if (SliderNumOfRecords)
    {
        SliderNumOfRecords->SetValue(static_cast<float>(AuditionData.DealData.NumOfRecords));
    }

    if (SliderRoyaltyRate)
    {
        SliderRoyaltyRate->SetValue(AuditionData.DealData.RoyaltyRate);
    }

    if (SliderContractYears)
    {
        SliderContractYears->SetValue(static_cast<float>(AuditionData.DealData.ContractYears));
    }
}

void UAuditionWidget::HandleSignUpBonusChanged(float Value)
{
    AuditionData.DealData.SignUpBonus = Value;
    OnNegotiationValueChanged();
}

void UAuditionWidget::HandleNumOfRecordsChanged(float Value)
{
    AuditionData.DealData.NumOfRecords = FMath::RoundToInt(Value);
    OnNegotiationValueChanged();
}

void UAuditionWidget::HandleRoyaltyRateChanged(float Value)
{
    AuditionData.DealData.RoyaltyRate = Value;
    OnNegotiationValueChanged();
}

void UAuditionWidget::HandleContractYearsChanged(float Value)
{
    AuditionData.DealData.ContractYears = FMath::RoundToInt(Value);
    OnNegotiationValueChanged();
}
