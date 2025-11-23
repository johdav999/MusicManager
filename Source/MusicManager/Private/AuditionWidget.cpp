#include "AuditionWidget.h"
#include "ArtistManagerSubsystem.h"
#include "FArtistDealTerms.h"

void UAuditionWidget::NativeConstruct()
{
    Super::NativeConstruct();

    RefreshDisplay();

    if (SliderSignUpBonus)
    {
        SliderSignUpBonus->OnValueChanged.RemoveAll(this);
        SliderSignUpBonus->OnValueChanged.AddDynamic(this, &UAuditionWidget::HandleSignUpBonusChanged);
        if (TextSignUpBonusValue)
        {
            TextSignUpBonusValue->SetText(FText::AsNumber(FMath::RoundToInt(SliderSignUpBonus->GetValue())));
        }
    }

    if (SliderNumOfRecords)
    {
        SliderNumOfRecords->OnValueChanged.RemoveAll(this);
        SliderNumOfRecords->OnValueChanged.AddDynamic(this, &UAuditionWidget::HandleNumOfRecordsChanged);
        if (TextNumOfRecordsValue)
        {
            TextNumOfRecordsValue->SetText(FText::AsNumber(FMath::RoundToInt(SliderNumOfRecords->GetValue())));
        }
    }

    if (SliderRoyaltyRate)
    {
        SliderRoyaltyRate->OnValueChanged.RemoveAll(this);
        SliderRoyaltyRate->OnValueChanged.AddDynamic(this, &UAuditionWidget::HandleRoyaltyRateChanged);
        if (TextRoyaltyRateValue)
        {
            TextRoyaltyRateValue->SetText(FText::AsNumber(FMath::RoundToInt(SliderRoyaltyRate->GetValue())));
        }
    }

    if (SliderContractYears)
    {
        SliderContractYears->OnValueChanged.RemoveAll(this);
        SliderContractYears->OnValueChanged.AddDynamic(this, &UAuditionWidget::HandleContractYearsChanged);
        if (TextContractYearsValue)
        {
            TextContractYearsValue->SetText(FText::AsNumber(FMath::RoundToInt(SliderContractYears->GetValue())));
        }
    }

    if (ButtonSignArtist)
    {
        ButtonSignArtist->OnClicked.RemoveAll(this);
        ButtonSignArtist->OnClicked.AddDynamic(this, &UAuditionWidget::HandleSignArtistClicked);
    }

    if (ButtonPass)
    {
        ButtonPass->OnClicked.RemoveAll(this);
        ButtonPass->OnClicked.AddDynamic(this, &UAuditionWidget::HandlePassClicked);
    }
}

void UAuditionWidget::CreateAuditionFromArtist(const FArtistData& Artist)
{
    AuditionData.ArtistData = Artist;
    AuditionData.DealData.ArtistName = Artist.ArtistName;

    AuditionData.DealData.SignUpBonus = 5000.f + Artist.PerformanceScore * 7500.f;
    AuditionData.DealData.NumOfRecords = 3;
    AuditionData.DealData.RoyaltyRate = FMath::Clamp(0.1f + Artist.StagePresence * 0.1f, 0.05f, 0.3f);
    AuditionData.DealData.ContractYears = 3;

    if (AuditionData.VenueName.IsEmpty())
    {
        AuditionData.VenueName = TEXT("Local Venue");
    }

    if (AuditionData.City.IsEmpty())
    {
        AuditionData.City = TEXT("Local City");
    }

    RefreshDisplay();
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
    if (TextSignUpBonusValue)
    {
        TextSignUpBonusValue->SetText(FText::AsNumber(FMath::RoundToInt(Value)));
    }
    OnNegotiationValueChanged();
}

void UAuditionWidget::HandleNumOfRecordsChanged(float Value)
{
    AuditionData.DealData.NumOfRecords = FMath::RoundToInt(Value);
    if (TextNumOfRecordsValue)
    {
        TextNumOfRecordsValue->SetText(FText::AsNumber(FMath::RoundToInt(Value)));
    }
    OnNegotiationValueChanged();
}

void UAuditionWidget::HandleRoyaltyRateChanged(float Value)
{
    AuditionData.DealData.RoyaltyRate = Value;
    if (TextRoyaltyRateValue)
    {
        TextRoyaltyRateValue->SetText(FText::AsNumber(FMath::RoundToInt(Value)));
    }
    OnNegotiationValueChanged();
}

void UAuditionWidget::HandleContractYearsChanged(float Value)
{
    AuditionData.DealData.ContractYears = FMath::RoundToInt(Value);
    if (TextContractYearsValue)
    {
        TextContractYearsValue->SetText(FText::AsNumber(FMath::RoundToInt(Value)));
    }
    OnNegotiationValueChanged();
}

void UAuditionWidget::HandleSignArtistClicked()
{
    FArtistDealTerms Deal;
    Deal.ArtistId = AuditionData.ArtistData.ArtistName;
    Deal.ContractYears = SliderContractYears ? static_cast<int32>(SliderContractYears->GetValue()) : AuditionData.DealData.ContractYears;
    Deal.NumRecords = SliderNumOfRecords ? static_cast<int32>(SliderNumOfRecords->GetValue()) : AuditionData.DealData.NumOfRecords;
    Deal.RoyaltyRate = SliderRoyaltyRate ? SliderRoyaltyRate->GetValue() : AuditionData.DealData.RoyaltyRate;
    Deal.SignUpBonus = SliderSignUpBonus ? SliderSignUpBonus->GetValue() : AuditionData.DealData.SignUpBonus;
    Deal.bExclusive = true;
    Deal.ProposedStartDate = FDateTime::Now();

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UArtistManagerSubsystem* Subsystem = GameInstance->GetSubsystem<UArtistManagerSubsystem>())
        {
            Subsystem->SignArtist(Deal);
        }
    }

    OnSignArtist.Broadcast();
}

void UAuditionWidget::HandlePassClicked()
{
    OnPass.Broadcast();
}
