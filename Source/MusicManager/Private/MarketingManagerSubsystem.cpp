#include "MarketingManagerSubsystem.h"

#include "FinanceManagerSubsystem.h"
#include "GameTimeSubsystem.h"
#include "MarketManagerSubsystem.h"
#include "MusicSaveGame.h"
#include "RecordManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/Guid.h"

void UMarketingManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    ChannelRules.Reset();
    ChannelRules.Add(EMarketingChannel::Radio, {EMarketingChannel::Radio, 1955, 2026, 0.080f, 1500.f});
    ChannelRules.Add(EMarketingChannel::Press, {EMarketingChannel::Press, 1955, 2026, 0.045f, 800.f});
    ChannelRules.Add(EMarketingChannel::Television, {EMarketingChannel::Television, 1955, 2010, 0.110f, 5000.f});
    ChannelRules.Add(EMarketingChannel::Posters, {EMarketingChannel::Posters, 1955, 2026, 0.030f, 500.f});
    ChannelRules.Add(EMarketingChannel::Social, {EMarketingChannel::Social, 2005, 2026, 0.095f, 1000.f});
    ChannelRules.Add(EMarketingChannel::Playlisting, {EMarketingChannel::Playlisting, 2010, 2026, 0.120f, 1200.f});
}

bool UMarketingManagerSubsystem::LaunchMarketingCampaign(const FLaunchMarketingCampaignCommand& Command, FString& OutCampaignId, FString& OutError)
{
    check(IsInGameThread());

    if (!ValidateLaunchCommand(Command, OutError))
    {
        return false;
    }

    UFinanceManagerSubsystem* FinanceSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFinanceManagerSubsystem>() : nullptr;
    if (!FinanceSubsystem)
    {
        OutError = TEXT("Finance subsystem is unavailable.");
        return false;
    }

    FMarketingCampaign Campaign;
    Campaign.CampaignId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
    Campaign.RecordId = Command.RecordId;
    Campaign.LabelId = Command.LabelId;
    Campaign.TargetRegionIds = Command.TargetRegionIds;
    Campaign.Channels = Command.Channels;
    Campaign.Budget = Command.Budget;
    Campaign.StartDate = Command.StartDate;
    Campaign.EndDate = Command.EndDate;
    Campaign.Status = EMarketingCampaignStatus::Active;

    FCashFlowEntry SpendEntry;
    SpendEntry.LabelId = Command.LabelId;
    SpendEntry.Type = ETransactionType::MarketingCost;
    SpendEntry.Amount = -Command.Budget;
    SpendEntry.Timestamp = Command.StartDate;
    SpendEntry.RefId = Campaign.CampaignId;
    FinanceSubsystem->RegisterTransaction(SpendEntry);

    Campaigns.Add(Campaign.CampaignId, Campaign);
    OutCampaignId = Campaign.CampaignId;

    return true;
}

void UMarketingManagerSubsystem::GetCampaignsForRecord(const FString& RecordId, TArray<FMarketingCampaign>& OutCampaigns) const
{
    OutCampaigns.Reset();
    for (const TPair<FString, FMarketingCampaign>& Pair : Campaigns)
    {
        if (Pair.Value.RecordId == RecordId)
        {
            OutCampaigns.Add(Pair.Value);
        }
    }
}

bool UMarketingManagerSubsystem::BuildMarketingPlannerView(const FString& RecordId, FMarketingPlannerView& OutView, FString& OutError) const
{
    OutView = FMarketingPlannerView();
    OutView.RecordId = RecordId;

    const URecordManagerSubsystem* RecordSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<URecordManagerSubsystem>() : nullptr;
    const UFinanceManagerSubsystem* FinanceSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFinanceManagerSubsystem>() : nullptr;
    const UGameTimeSubsystem* TimeSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGameTimeSubsystem>() : nullptr;

    if (!RecordSubsystem || !FinanceSubsystem || !TimeSubsystem)
    {
        OutError = TEXT("Required subsystems are unavailable.");
        return false;
    }

    FString LabelId;
    OutView.bRecordIsMarketable = RecordSubsystem->IsRecordMarketable(RecordId, LabelId);
    OutView.LabelId = LabelId;
    OutView.RecordDisplayName = RecordSubsystem->GetRecordDisplayName(RecordId);
    OutView.ArtistDisplayName = RecordSubsystem->GetArtistDisplayNameForRecord(RecordId);
    if (!OutView.bRecordIsMarketable)
    {
        OutView.ValidationWarnings.Add(TEXT("Record cannot be marketed in its current state."));
        return true;
    }

    FRecordData Record;
    RecordSubsystem->GetRecordById(RecordId, Record);
    OutView.TargetRegionIds = Record.TargetRegionIds;
    OutView.LabelCash = FinanceSubsystem->GetLabelBalance(LabelId);
    OutView.MaximumAffordableBudget = FMath::Max(0.f, OutView.LabelCash);

    const FDateTime CurrentDate = TimeSubsystem->GetCurrentGameDate();
    for (const TPair<EMarketingChannel, FMarketingChannelRule>& Pair : ChannelRules)
    {
        FMarketingChannelOption Option;
        Option.Channel = Pair.Key;
        Option.bAvailable = Pair.Value.IsActiveForDate(CurrentDate);
        Option.MinimumBudget = Pair.Value.MinimumBudget;
        Option.EstimatedExposurePerThousand = Pair.Value.ExposurePerThousand;
        if (!Option.bAvailable)
        {
            Option.UnavailableReason = TEXT("Channel is not available in the current era.");
        }
        OutView.ChannelOptions.Add(Option);
    }

    for (const TPair<FString, FMarketingCampaign>& Pair : Campaigns)
    {
        if (Pair.Value.RecordId == RecordId && Pair.Value.Status == EMarketingCampaignStatus::Active)
        {
            OutView.ActiveCampaigns.Add(Pair.Value);
            OutView.CampaignROISummaries.Add(CalculateROISummary(Pair.Value, OutView.RecordDisplayName, OutView.ArtistDisplayName));
        }
    }

    if (OutView.TargetRegionIds.Num() == 0)
    {
        OutView.ValidationWarnings.Add(TEXT("Schedule the release with target regions before launching marketing."));
    }
    if (OutView.LabelCash <= 0.f)
    {
        OutView.ValidationWarnings.Add(TEXT("The label has no available cash for marketing."));
    }

    OutView.ForecastSummary = BuildForecastSummary(OutView);

    return true;
}

bool UMarketingManagerSubsystem::BuildCampaignROISummary(const FString& CampaignId, FMarketingCampaignROISummary& OutSummary) const
{
    const FMarketingCampaign* Campaign = Campaigns.Find(CampaignId);
    if (!Campaign)
    {
        return false;
    }

    FString RecordDisplayName = Campaign->RecordId;
    FString ArtistDisplayName;
    if (const URecordManagerSubsystem* RecordSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<URecordManagerSubsystem>() : nullptr)
    {
        RecordDisplayName = RecordSubsystem->GetRecordDisplayName(Campaign->RecordId);
        ArtistDisplayName = RecordSubsystem->GetArtistDisplayNameForRecord(Campaign->RecordId);
    }

    OutSummary = CalculateROISummary(*Campaign, RecordDisplayName, ArtistDisplayName);
    return true;
}

void UMarketingManagerSubsystem::BuildMarketingDashboardSummary(FMarketingDashboardSummary& OutSummary) const
{
    OutSummary = FMarketingDashboardSummary();

    for (const TPair<FString, FMarketingCampaign>& Pair : Campaigns)
    {
        FMarketingCampaignROISummary Summary;
        if (!BuildCampaignROISummary(Pair.Key, Summary))
        {
            continue;
        }

        if (Pair.Value.Status == EMarketingCampaignStatus::Active || Pair.Value.Status == EMarketingCampaignStatus::Planned)
        {
            OutSummary.ActiveCampaigns.Add(Summary);
            OutSummary.TotalActiveSpend += Summary.Spend;
        }
        else if (Pair.Value.Status == EMarketingCampaignStatus::Completed)
        {
            OutSummary.CompletedCampaigns.Add(Summary);
        }

        OutSummary.TotalProjectedROI += Summary.EstimatedROI;
    }
}

FMarketingCampaignROISummary UMarketingManagerSubsystem::CalculateROISummary(
    const FMarketingCampaign& Campaign,
    const FString& RecordDisplayName,
    const FString& ArtistDisplayName,
    float AverageUnitRevenue)
{
    FMarketingCampaignROISummary Summary;
    Summary.CampaignId = Campaign.CampaignId;
    Summary.RecordId = Campaign.RecordId;
    Summary.RecordDisplayName = RecordDisplayName.IsEmpty() ? Campaign.RecordId : RecordDisplayName;
    Summary.ArtistDisplayName = ArtistDisplayName;
    Summary.Spend = Campaign.Budget;
    Summary.Status = Campaign.Status;

    for (const FMarketingExposureEntry& Entry : Campaign.GeneratedExposure)
    {
        if (FMath::IsFinite(Entry.Exposure) && Entry.Exposure > 0.f)
        {
            Summary.GeneratedExposure += Entry.Exposure;
        }
    }

    if (Summary.GeneratedExposure <= 0.f)
    {
        const float RegionCount = FMath::Max(1, Campaign.TargetRegionIds.Num());
        const float ChannelCount = FMath::Max(1, Campaign.Channels.Num());
        Summary.GeneratedExposure = FMath::Clamp((Campaign.Budget / 1000.f) * 0.05f * RegionCount * ChannelCount, 0.f, 10.f);
    }

    Summary.EstimatedUnitLift = FMath::Max(0, FMath::RoundToInt(Summary.GeneratedExposure * 1000.f));
    Summary.EstimatedGrossRevenue = Summary.EstimatedUnitLift * FMath::Max(0.f, AverageUnitRevenue);
    Summary.EstimatedROI = Summary.Spend > 0.f ? (Summary.EstimatedGrossRevenue - Summary.Spend) / Summary.Spend : 0.f;
    return Summary;
}

void UMarketingManagerSubsystem::HandleMonthAdvanced(const FDateTime& NewDate)
{
    check(IsInGameThread());

    UMarketManagerSubsystem* MarketSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMarketManagerSubsystem>() : nullptr;
    if (!MarketSubsystem)
    {
        return;
    }

    for (TPair<FString, FMarketingCampaign>& Pair : Campaigns)
    {
        FMarketingCampaign& Campaign = Pair.Value;
        if (Campaign.Status != EMarketingCampaignStatus::Active)
        {
            continue;
        }

        if (NewDate < Campaign.StartDate)
        {
            continue;
        }

        if (NewDate > Campaign.EndDate)
        {
            Campaign.Status = EMarketingCampaignStatus::Completed;
            continue;
        }

        const FString MonthKey = BuildCampaignMonthKey(Campaign.CampaignId, NewDate);
        if (AppliedCampaignMonthKeys.Contains(MonthKey))
        {
            continue;
        }

        for (const FString& RegionId : Campaign.TargetRegionIds)
        {
            for (EMarketingChannel Channel : Campaign.Channels)
            {
                const float Exposure = ResolveChannelExposure(Campaign, Channel);
                if (MarketSubsystem->AddRecordExposure(RegionId, Campaign.RecordId, Exposure))
                {
                    FMarketingExposureEntry Entry;
                    Entry.RegionId = RegionId;
                    Entry.Channel = Channel;
                    Entry.Exposure = Exposure;
                    Campaign.GeneratedExposure.Add(Entry);
                }
            }
        }

        AppliedCampaignMonthKeys.Add(MonthKey);
    }
}

bool UMarketingManagerSubsystem::ValidateLaunchCommand(const FLaunchMarketingCampaignCommand& Command, FString& OutError) const
{
    if (Command.RecordId.IsEmpty() || Command.LabelId.IsEmpty())
    {
        OutError = TEXT("Record and label are required for marketing.");
        return false;
    }

    const URecordManagerSubsystem* RecordSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<URecordManagerSubsystem>() : nullptr;
    const UMarketManagerSubsystem* MarketSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMarketManagerSubsystem>() : nullptr;
    const UFinanceManagerSubsystem* FinanceSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UFinanceManagerSubsystem>() : nullptr;
    if (!RecordSubsystem || !MarketSubsystem || !FinanceSubsystem)
    {
        OutError = TEXT("Required subsystems are unavailable.");
        return false;
    }

    FString OwningLabelId;
    if (!RecordSubsystem->IsRecordMarketable(Command.RecordId, OwningLabelId) || OwningLabelId != Command.LabelId)
    {
        OutError = TEXT("Record is not marketable by the selected label.");
        return false;
    }

    FRecordData Record;
    if (!RecordSubsystem->GetRecordById(Command.RecordId, Record))
    {
        OutError = TEXT("Record does not exist.");
        return false;
    }

    if (Command.Budget <= 0.f || !FMath::IsFinite(Command.Budget))
    {
        OutError = TEXT("Marketing budget must be positive.");
        return false;
    }

    if (FinanceSubsystem->GetLabelBalance(Command.LabelId) < Command.Budget)
    {
        OutError = TEXT("The label cannot afford this campaign.");
        return false;
    }

    if (Command.StartDate.GetTicks() <= 0 || Command.EndDate.GetTicks() <= 0 || Command.EndDate < Command.StartDate)
    {
        OutError = TEXT("Campaign dates are invalid.");
        return false;
    }

    if (Command.TargetRegionIds.Num() == 0)
    {
        OutError = TEXT("At least one marketing region is required.");
        return false;
    }

    TSet<FString> UniqueRegions;
    for (const FString& RegionId : Command.TargetRegionIds)
    {
        FMarketRegion Region;
        if (RegionId.IsEmpty() || !MarketSubsystem->GetRegion(RegionId, Region))
        {
            OutError = TEXT("One or more marketing regions are invalid.");
            return false;
        }
        if (UniqueRegions.Contains(RegionId))
        {
            OutError = TEXT("Duplicate marketing regions are not allowed.");
            return false;
        }
        if (!Record.TargetRegionIds.Contains(RegionId))
        {
            OutError = TEXT("Marketing regions must be part of the record release regions.");
            return false;
        }
        UniqueRegions.Add(RegionId);
    }

    if (Command.Channels.Num() == 0)
    {
        OutError = TEXT("At least one marketing channel is required.");
        return false;
    }

    TSet<EMarketingChannel> UniqueChannels;
    float MinimumBudget = 0.f;
    for (EMarketingChannel Channel : Command.Channels)
    {
        FMarketingChannelRule Rule;
        if (!IsChannelValidForDate(Channel, Command.StartDate, Rule))
        {
            OutError = TEXT("One or more marketing channels are not available for the campaign start date.");
            return false;
        }
        if (UniqueChannels.Contains(Channel))
        {
            OutError = TEXT("Duplicate marketing channels are not allowed.");
            return false;
        }
        UniqueChannels.Add(Channel);
        MinimumBudget += Rule.MinimumBudget;
    }

    if (Command.Budget < MinimumBudget)
    {
        OutError = TEXT("Marketing budget is below the selected channel minimum.");
        return false;
    }

    return true;
}

FMarketingCampaignROISummary UMarketingManagerSubsystem::BuildForecastSummary(const FMarketingPlannerView& View) const
{
    FMarketingCampaign ForecastCampaign;
    ForecastCampaign.CampaignId = TEXT("forecast");
    ForecastCampaign.RecordId = View.RecordId;
    ForecastCampaign.LabelId = View.LabelId;
    ForecastCampaign.TargetRegionIds = View.TargetRegionIds;
    ForecastCampaign.Budget = FMath::Min(FMath::Max(0.f, View.MaximumAffordableBudget), 10000.f);
    ForecastCampaign.Status = EMarketingCampaignStatus::Planned;

    for (const FMarketingChannelOption& Option : View.ChannelOptions)
    {
        if (Option.bAvailable && ForecastCampaign.Channels.Num() < 2)
        {
            ForecastCampaign.Channels.Add(Option.Channel);
        }
    }

    return CalculateROISummary(ForecastCampaign, View.RecordDisplayName, View.ArtistDisplayName);
}

float UMarketingManagerSubsystem::EstimateExposure(const FLaunchMarketingCampaignCommand& Command, EMarketingChannel Channel) const
{
    FMarketingChannelRule Rule;
    if (!IsChannelValidForDate(Channel, Command.StartDate, Rule))
    {
        return 0.f;
    }

    const float RegionCount = FMath::Max(1, Command.TargetRegionIds.Num());
    const float ChannelCount = FMath::Max(1, Command.Channels.Num());
    const float BudgetShare = Command.Budget / (RegionCount * ChannelCount);
    return FMath::Clamp((BudgetShare / 1000.f) * Rule.ExposurePerThousand, 0.f, 0.75f);
}

float UMarketingManagerSubsystem::ResolveChannelExposure(const FMarketingCampaign& Campaign, EMarketingChannel Channel) const
{
    FLaunchMarketingCampaignCommand Command;
    Command.RecordId = Campaign.RecordId;
    Command.LabelId = Campaign.LabelId;
    Command.TargetRegionIds = Campaign.TargetRegionIds;
    Command.Channels = Campaign.Channels;
    Command.Budget = Campaign.Budget;
    Command.StartDate = Campaign.StartDate;
    Command.EndDate = Campaign.EndDate;
    return EstimateExposure(Command, Channel);
}

FString UMarketingManagerSubsystem::BuildCampaignMonthKey(const FString& CampaignId, const FDateTime& Date) const
{
    return FString::Printf(TEXT("%s:%04d-%02d"), *CampaignId, Date.GetYear(), Date.GetMonth());
}

bool UMarketingManagerSubsystem::IsChannelValidForDate(EMarketingChannel Channel, const FDateTime& Date, FMarketingChannelRule& OutRule) const
{
    if (const FMarketingChannelRule* Rule = ChannelRules.Find(Channel))
    {
        OutRule = *Rule;
        return Rule->IsActiveForDate(Date);
    }

    return false;
}

void UMarketingManagerSubsystem::BuildSaveSnapshot(FMarketingSnapshot& OutSnapshot) const
{
    OutSnapshot.Campaigns.Reset();
    Campaigns.GenerateValueArray(OutSnapshot.Campaigns);
    OutSnapshot.AppliedCampaignMonthKeys = AppliedCampaignMonthKeys;
}

void UMarketingManagerSubsystem::ValidateSaveSnapshot(const FMarketingSnapshot& Snapshot, const TSet<FString>& KnownRecordIds, const TSet<FString>& KnownLabelIds, const TSet<FString>& KnownRegionIds, FMusicSaveValidationResult& Result) const
{
    TSet<FString> SeenCampaignIds;
    for (const FMarketingCampaign& Campaign : Snapshot.Campaigns)
    {
        if (Campaign.CampaignId.IsEmpty())
        {
            Result.AddError(TEXT("Marketing campaign has an empty campaign id."));
            continue;
        }
        if (SeenCampaignIds.Contains(Campaign.CampaignId))
        {
            Result.AddError(FString::Printf(TEXT("Duplicate marketing campaign id: %s."), *Campaign.CampaignId));
        }
        SeenCampaignIds.Add(Campaign.CampaignId);

        if (Campaign.RecordId.IsEmpty() || !KnownRecordIds.Contains(Campaign.RecordId))
        {
            Result.AddError(FString::Printf(TEXT("Marketing campaign %s references missing record %s."), *Campaign.CampaignId, *Campaign.RecordId));
        }
        if (Campaign.LabelId.IsEmpty() || !KnownLabelIds.Contains(Campaign.LabelId))
        {
            Result.AddError(FString::Printf(TEXT("Marketing campaign %s references missing label %s."), *Campaign.CampaignId, *Campaign.LabelId));
        }
        if (!FMath::IsFinite(Campaign.Budget) || Campaign.Budget <= 0.f)
        {
            Result.AddError(FString::Printf(TEXT("Marketing campaign %s has invalid budget."), *Campaign.CampaignId));
        }
        if (Campaign.StartDate.GetTicks() <= 0 || Campaign.EndDate.GetTicks() <= 0 || Campaign.EndDate < Campaign.StartDate)
        {
            Result.AddError(FString::Printf(TEXT("Marketing campaign %s has invalid dates."), *Campaign.CampaignId));
        }
        if (!StaticEnum<EMarketingCampaignStatus>()->IsValidEnumValue(static_cast<int64>(Campaign.Status)))
        {
            Result.AddError(FString::Printf(TEXT("Marketing campaign %s has invalid status."), *Campaign.CampaignId));
        }

        for (const FString& RegionId : Campaign.TargetRegionIds)
        {
            if (RegionId.IsEmpty() || !KnownRegionIds.Contains(RegionId))
            {
                Result.AddError(FString::Printf(TEXT("Marketing campaign %s references missing region %s."), *Campaign.CampaignId, *RegionId));
            }
        }

        for (EMarketingChannel Channel : Campaign.Channels)
        {
            if (!StaticEnum<EMarketingChannel>()->IsValidEnumValue(static_cast<int64>(Channel)))
            {
                Result.AddError(FString::Printf(TEXT("Marketing campaign %s contains invalid channel."), *Campaign.CampaignId));
            }
        }

        for (const FMarketingExposureEntry& Exposure : Campaign.GeneratedExposure)
        {
            if (Exposure.RegionId.IsEmpty() || !KnownRegionIds.Contains(Exposure.RegionId))
            {
                Result.AddError(FString::Printf(TEXT("Marketing campaign %s generated exposure for missing region %s."), *Campaign.CampaignId, *Exposure.RegionId));
            }
            if (!FMath::IsFinite(Exposure.Exposure) || Exposure.Exposure < 0.f)
            {
                Result.AddError(FString::Printf(TEXT("Marketing campaign %s has invalid generated exposure."), *Campaign.CampaignId));
            }
        }
    }

    for (const FString& Key : Snapshot.AppliedCampaignMonthKeys)
    {
        if (Key.IsEmpty())
        {
            Result.AddError(TEXT("Marketing applied month key set contains an empty key."));
        }
    }
}

void UMarketingManagerSubsystem::ApplySaveSnapshot(const FMarketingSnapshot& Snapshot)
{
    Campaigns.Reset();
    for (const FMarketingCampaign& Campaign : Snapshot.Campaigns)
    {
        Campaigns.Add(Campaign.CampaignId, Campaign);
    }
    AppliedCampaignMonthKeys = Snapshot.AppliedCampaignMonthKeys;
}
