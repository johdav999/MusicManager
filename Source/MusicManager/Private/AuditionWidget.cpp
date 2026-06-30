#include "AuditionWidget.h"
#include "ArtistManagerSubsystem.h"
#include "CommandDispatcherSubsystem.h"
#include "FArtistDealTerms.h"
#include "PlayerLabelSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "UI/MusicSegmentedMeterWidget.h"

void UAuditionWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!DefaultPortraitTexture)
    {
        DefaultPortraitTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/GUI/Audition/ArtistAuditionDefaultPortrait.ArtistAuditionDefaultPortrait"));
    }

    ConfigureSliderRanges();
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

    AuditionData.DealData.SignUpBonus = 5000.f + FMath::Clamp(Artist.PerformanceScore, 0.f, 100.f) * 230.f;
    AuditionData.DealData.NumOfRecords = 3;
    AuditionData.DealData.RoyaltyRate = FMath::Clamp(0.08f + (FMath::Clamp(Artist.StagePresence, 0.f, 100.f) / 100.f) * 0.12f, 0.05f, 0.3f);
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
        TextVenue->SetText(FText::FromString(AuditionData.VenueName.IsEmpty() ? TEXT("A&R Showcase") : AuditionData.VenueName));
    }

    if (TextCity)
    {
        TextCity->SetText(FText::FromString(AuditionData.City.IsEmpty() ? TEXT("Local Market") : AuditionData.City));
    }

    if (TextPerformanceScore)
    {
        TextPerformanceScore->SetText(FormatWholeNumber(AuditionData.ArtistData.PerformanceScore));
    }

    if (TextStagePresence)
    {
        TextStagePresence->SetText(FormatWholeNumber(AuditionData.ArtistData.StagePresence));
    }

    if (TextAudienceEngagement)
    {
        TextAudienceEngagement->SetText(FormatWholeNumber(AuditionData.ArtistData.AudienceEngagement));
    }

    if (TextVocalQuality)
    {
        TextVocalQuality->SetText(FormatWholeNumber(AuditionData.ArtistData.VocalQuality));
    }

    if (TextSongwritingQuality)
    {
        TextSongwritingQuality->SetText(FormatWholeNumber(AuditionData.ArtistData.SongwritingQuality));
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

    RefreshStatMeters();
    RefreshPortrait();
    RefreshDealValueTexts();
}

void UAuditionWidget::HandleSignUpBonusChanged(float Value)
{
    AuditionData.DealData.SignUpBonus = Value;
    if (TextSignUpBonusValue)
    {
        TextSignUpBonusValue->SetText(FormatCurrency(Value));
    }
    OnNegotiationValueChanged();
}

void UAuditionWidget::HandleNumOfRecordsChanged(float Value)
{
    AuditionData.DealData.NumOfRecords = FMath::RoundToInt(Value);
    if (TextNumOfRecordsValue)
    {
        TextNumOfRecordsValue->SetText(FormatWholeNumber(Value));
    }
    OnNegotiationValueChanged();
}

void UAuditionWidget::HandleRoyaltyRateChanged(float Value)
{
    AuditionData.DealData.RoyaltyRate = Value;
    if (TextRoyaltyRateValue)
    {
        TextRoyaltyRateValue->SetText(FormatRoyalty(Value));
    }
    OnNegotiationValueChanged();
}

void UAuditionWidget::HandleContractYearsChanged(float Value)
{
    AuditionData.DealData.ContractYears = FMath::RoundToInt(Value);
    if (TextContractYearsValue)
    {
        TextContractYearsValue->SetText(FormatYears(FMath::RoundToInt(Value)));
    }
    OnNegotiationValueChanged();
}

void UAuditionWidget::HandleSignArtistClicked()
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UCommandDispatcherSubsystem* Dispatcher = GameInstance->GetSubsystem<UCommandDispatcherSubsystem>())
        {
            FSignArtistCommand Command;
            Command.ArtistId = AuditionData.ArtistData.ArtistId.IsEmpty()
                ? AuditionData.ArtistData.ArtistName
                : AuditionData.ArtistData.ArtistId;
            Command.LabelId = TEXT("label_player");
            if (const UPlayerLabelSubsystem* LabelSubsystem = GameInstance->GetSubsystem<UPlayerLabelSubsystem>())
            {
                Command.LabelId = LabelSubsystem->GetPlayerLabelId();
            }
            Command.ContractYears = SliderContractYears ? static_cast<int32>(SliderContractYears->GetValue()) : AuditionData.DealData.ContractYears;
            Command.RecordCommitment = SliderNumOfRecords ? static_cast<int32>(SliderNumOfRecords->GetValue()) : AuditionData.DealData.NumOfRecords;
            Command.RoyaltyRate = SliderRoyaltyRate ? SliderRoyaltyRate->GetValue() : AuditionData.DealData.RoyaltyRate;
            Command.SignUpBonus = SliderSignUpBonus ? SliderSignUpBonus->GetValue() : AuditionData.DealData.SignUpBonus;
            Command.bExclusive = true;

            const FMusicCommandResult Result = Dispatcher->ExecuteSignArtist(Command);
            if (!Result.bSuccess)
            {
                UE_LOG(LogTemp, Warning, TEXT("Audition sign failed: %s"), *Result.Message.ToString());
                return;
            }
        }
    }

    OnSignArtist.Broadcast();
}

void UAuditionWidget::HandlePassClicked()
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UCommandDispatcherSubsystem* Dispatcher = GameInstance->GetSubsystem<UCommandDispatcherSubsystem>())
        {
            FRejectArtistCommand Command;
            Command.ArtistId = AuditionData.ArtistData.ArtistId.IsEmpty()
                ? AuditionData.ArtistData.ArtistName
                : AuditionData.ArtistData.ArtistId;

            const FMusicCommandResult Result = Dispatcher->ExecuteRejectArtist(Command);
            if (!Result.bSuccess)
            {
                UE_LOG(LogTemp, Warning, TEXT("Audition pass failed: %s"), *Result.Message.ToString());
                return;
            }
        }
    }

    OnPass.Broadcast();
}

void UAuditionWidget::ConfigureSliderRanges()
{
    if (SliderSignUpBonus)
    {
        SliderSignUpBonus->SetMinValue(0.f);
        SliderSignUpBonus->SetMaxValue(100000.f);
        SliderSignUpBonus->SetStepSize(500.f);
    }

    if (SliderNumOfRecords)
    {
        SliderNumOfRecords->SetMinValue(1.f);
        SliderNumOfRecords->SetMaxValue(7.f);
        SliderNumOfRecords->SetStepSize(1.f);
    }

    if (SliderRoyaltyRate)
    {
        SliderRoyaltyRate->SetMinValue(0.05f);
        SliderRoyaltyRate->SetMaxValue(0.3f);
        SliderRoyaltyRate->SetStepSize(0.01f);
    }

    if (SliderContractYears)
    {
        SliderContractYears->SetMinValue(1.f);
        SliderContractYears->SetMaxValue(7.f);
        SliderContractYears->SetStepSize(1.f);
    }
}

void UAuditionWidget::RefreshStatMeters()
{
    auto ApplyScore = [this](const FName MeterName, float Score)
    {
        UMusicSegmentedMeterWidget* Meter = nullptr;
        if (WidgetTree)
        {
            Meter = Cast<UMusicSegmentedMeterWidget>(WidgetTree->FindWidget(MeterName));
        }

        if (Meter)
        {
            Meter->SetPercent(FMath::Clamp(Score / 100.f, 0.f, 1.f));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("AuditionWidget could not find segmented meter '%s'."), *MeterName.ToString());
        }
    };

    ApplyScore(FName(TEXT("PerformanceMeter")), AuditionData.ArtistData.PerformanceScore);
    ApplyScore(FName(TEXT("StagePresenceMeter")), AuditionData.ArtistData.StagePresence);
    ApplyScore(FName(TEXT("AudienceEngagementMeter")), AuditionData.ArtistData.AudienceEngagement);
    ApplyScore(FName(TEXT("VocalQualityMeter")), AuditionData.ArtistData.VocalQuality);
    ApplyScore(FName(TEXT("SongwritingQualityMeter")), AuditionData.ArtistData.SongwritingQuality);
}

void UAuditionWidget::RefreshPortrait()
{
    if (!ArtistPortraitImage)
    {
        return;
    }

    UTexture2D* PortraitTexture = DefaultPortraitTexture;
    if (!AuditionData.ArtistData.ImageAssetRef.IsEmpty())
    {
        if (UTexture2D* LoadedTexture = LoadObject<UTexture2D>(nullptr, *AuditionData.ArtistData.ImageAssetRef))
        {
            PortraitTexture = LoadedTexture;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Audition portrait could not load ImageAssetRef '%s' for artist '%s'."),
                *AuditionData.ArtistData.ImageAssetRef,
                *AuditionData.ArtistData.ArtistName);
        }
    }

    if (PortraitTexture)
    {
        FSlateBrush Brush;
        Brush.SetResourceObject(PortraitTexture);
        Brush.ImageSize = FVector2D(132.f, 132.f);
        Brush.SetUVRegion(FBox2f(FVector2f(0.16f, 0.03f), FVector2f(0.84f, 0.71f)));
        ArtistPortraitImage->SetBrush(Brush);
        ArtistPortraitImage->SetVisibility(ESlateVisibility::Visible);
    }
}

void UAuditionWidget::RefreshDealValueTexts()
{
    if (TextSignUpBonusValue)
    {
        TextSignUpBonusValue->SetText(FormatCurrency(AuditionData.DealData.SignUpBonus));
    }

    if (TextNumOfRecordsValue)
    {
        TextNumOfRecordsValue->SetText(FormatWholeNumber(static_cast<float>(AuditionData.DealData.NumOfRecords)));
    }

    if (TextRoyaltyRateValue)
    {
        TextRoyaltyRateValue->SetText(FormatRoyalty(AuditionData.DealData.RoyaltyRate));
    }

    if (TextContractYearsValue)
    {
        TextContractYearsValue->SetText(FormatYears(AuditionData.DealData.ContractYears));
    }
}

FText UAuditionWidget::FormatCurrency(float Value)
{
    FNumberFormattingOptions Options;
    Options.SetMaximumFractionalDigits(0);
    Options.SetMinimumFractionalDigits(0);
    Options.SetUseGrouping(true);
    return FText::Format(NSLOCTEXT("AuditionWidget", "CurrencyFormat", "${0}"), FText::AsNumber(FMath::RoundToInt(Value), &Options));
}

FText UAuditionWidget::FormatWholeNumber(float Value)
{
    return FText::AsNumber(FMath::RoundToInt(Value));
}

FText UAuditionWidget::FormatRoyalty(float Value)
{
    return FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f)));
}

FText UAuditionWidget::FormatYears(int32 Value)
{
    return FText::FromString(FString::Printf(TEXT("%d %s"), Value, Value == 1 ? TEXT("Year") : TEXT("Years")));
}
