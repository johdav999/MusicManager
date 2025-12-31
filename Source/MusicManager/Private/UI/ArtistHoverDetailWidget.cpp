#include "UI/ArtistHoverDetailWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "RecordManagerSubsystem.h"

void UArtistHoverDetailWidget::SetupFromArtistData(const FArtistData& ArtistData)
{
    if (!IsInGameThread())
    {
        return;
    }

    if (ArtistNameText)
    {
        ArtistNameText->SetText(FText::FromString(ArtistData.ArtistName));
    }

    if (GenreText)
    {
        GenreText->SetText(FText::FromString(ArtistData.Genre));
    }

    const float PopularityScore = ComputePopularityScore(ArtistData);
    if (PopularityValueText)
    {
        PopularityValueText->SetText(FText::AsNumber(FMath::RoundToInt(PopularityScore * 100.0f)));
    }

    if (PopularityBar)
    {
        PopularityBar->SetPercent(PopularityScore);
    }

    if (PerformanceBar)
    {
        PerformanceBar->SetPercent(FMath::Clamp(ArtistData.PerformanceScore / 100.0f, 0.0f, 1.0f));
    }

    if (PerformanceValueText)
    {
        const float PerformancePercent = FMath::Clamp(ArtistData.PerformanceScore, 0.0f, 100.0f);
        PerformanceValueText->SetText(FText::AsNumber(FMath::RoundToInt(PerformancePercent)));
    }

    if (MomentumText)
    {
        const FString MomentumLabel = PopularityScore >= 0.75f ? TEXT("Rising")
            : PopularityScore >= 0.55f ? TEXT("Stable")
            : PopularityScore >= 0.35f ? TEXT("Declining")
            : TEXT("Idle");
        MomentumText->SetText(FText::FromString(MomentumLabel));
    }

    int32 RecordCount = 0;
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (URecordManagerSubsystem* RecordSubsystem = GameInstance->GetSubsystem<URecordManagerSubsystem>())
        {
            RecordCount = RecordSubsystem->GetRecordCountForArtist(ArtistData.ArtistName);
        }
    }

    if (RecordsCountText)
    {
        RecordsCountText->SetText(FText::AsNumber(RecordCount));
    }

    if (ContractStatusText)
    {
        const FString ContractStatus = ArtistData.ArtistName.IsEmpty() ? TEXT("Unknown") : TEXT("Signed");
        ContractStatusText->SetText(FText::FromString(ContractStatus));
    }
}

float UArtistHoverDetailWidget::ComputePopularityScore(const FArtistData& ArtistData) const
{
    const float CombinedScore = (ArtistData.PerformanceScore
        + ArtistData.StagePresence
        + ArtistData.AudienceEngagement
        + ArtistData.VocalQuality
        + ArtistData.SongwritingQuality) / 5.0f;

    return FMath::Clamp(CombinedScore / 100.0f, 0.0f, 1.0f);
}
