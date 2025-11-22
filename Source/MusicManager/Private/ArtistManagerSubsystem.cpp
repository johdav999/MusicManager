#include "ArtistManagerSubsystem.h"

#include "Engine/Engine.h"
#include "GameTimeSubsystem.h"
#include "MusicSaveGame.h"

void UArtistManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    ActiveContracts.Reset();
    ExpiredContracts.Reset();

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>())
        {
            TimeSubsystem->OnMonthAdvanced.AddDynamic(this, &UArtistManagerSubsystem::HandleMonthAdvanced);
            CurrentGameDate = TimeSubsystem->GetCurrentGameDate();
        }
    }
}

void UArtistManagerSubsystem::SignArtist(const FArtistDealTerms& Deal, const FArtistData& ArtistInfo)
{
    FArtistContract NewContract;
    NewContract.ArtistId = Deal.ArtistId;
    NewContract.ArtistData = ArtistInfo;
    NewContract.Terms = Deal;

    const FDateTime EffectiveStartDate = Deal.ProposedStartDate.GetTicks() > 0 ? Deal.ProposedStartDate : CurrentGameDate;
    NewContract.StartDate = EffectiveStartDate;

    const int32 ContractMonths = CalculateContractDurationMonths(Deal);
    const double DaysPerMonth = 30.0;
    NewContract.EndDate = EffectiveStartDate + FTimespan::FromDays(ContractMonths * DaysPerMonth);

    NewContract.bContractActive = true;
    NewContract.RecordsDelivered = 0;
    NewContract.LifetimeRevenue = 0.f;
    NewContract.LifetimeCost = Deal.SignUpBonus;
    NewContract.LastRoyaltyPayment = 0.f;
    NewContract.CumulativeRoyaltyPaid = 0.f;
    NewContract.MonthlyUpkeepCost = 2000.f + ArtistInfo.PerformanceScore * 25.f;
    NewContract.PerformanceMomentum = ArtistInfo.PerformanceScore;
    NewContract.ProductionProgress = 0.f;
    NewContract.MonthsActive = 0;

    ActiveContracts.Add(NewContract);

    OnArtistSigned.Broadcast(NewContract);
    OnArtistListChanged.Broadcast();
}

void UArtistManagerSubsystem::RejectArtist(const FString& ArtistId)
{
    OnArtistRejected.Broadcast(ArtistId);
}

void UArtistManagerSubsystem::AdvanceMonth()
{
    check(IsInGameThread());

    TArray<FString> ContractsToExpire;
    for (FArtistContract& Contract : ActiveContracts)
    {
        ProcessMonthlyContractFinancials(Contract);

        const bool bReachedEndDate = CurrentGameDate >= Contract.EndDate;
        const bool bReachedDuration = Contract.MonthsActive >= CalculateContractDurationMonths(Contract.Terms);
        if (bReachedEndDate || bReachedDuration)
        {
            ContractsToExpire.Add(Contract.ArtistId);
        }
    }

    for (const FString& ArtistId : ContractsToExpire)
    {
        ExpireContract(ArtistId);
    }

    OnMonthlyFinancialUpdate.Broadcast(ActiveContracts);
}

void UArtistManagerSubsystem::HandleMonthAdvanced(const FDateTime& NewDate)
{
    check(IsInGameThread());

    CurrentGameDate = NewDate;
    AdvanceMonth();
}

void UArtistManagerSubsystem::ProcessMonthlyContractFinancials(FArtistContract& Contract)
{
    Contract.MonthsActive++;

    const float AudienceComposite = Contract.ArtistData.AudienceEngagement + Contract.ArtistData.StagePresence + Contract.ArtistData.PerformanceScore;
    const float CreativeComposite = Contract.ArtistData.VocalQuality + Contract.ArtistData.SongwritingQuality;
    const float PopularityFactor = FMath::Clamp((AudienceComposite + CreativeComposite) / 500.f, 0.1f, 2.5f);

    Contract.PerformanceMomentum = FMath::Clamp(Contract.PerformanceMomentum * 0.85f + Contract.ArtistData.PerformanceScore * 0.15f, 0.f, 100.f);

    const float MomentumMultiplier = 1.f + (Contract.PerformanceMomentum / 200.f);
    const float MonthlyGrossRevenue = (12000.f + Contract.MonthsActive * 400.f) * PopularityFactor * MomentumMultiplier;

    const float RoyaltyPayment = MonthlyGrossRevenue * (Contract.Terms.RoyaltyRate / 100.f);
    Contract.LastRoyaltyPayment = RoyaltyPayment;
    Contract.CumulativeRoyaltyPaid += RoyaltyPayment;

    const float UpkeepCost = Contract.MonthlyUpkeepCost;

    Contract.LifetimeRevenue += MonthlyGrossRevenue;
    Contract.LifetimeCost += RoyaltyPayment + UpkeepCost;

    const int32 ContractMonths = FMath::Max(CalculateContractDurationMonths(Contract.Terms), 1);
    const float RecordsPerMonth = Contract.Terms.NumRecords > 0 ? static_cast<float>(Contract.Terms.NumRecords) / static_cast<float>(ContractMonths) : 0.f;
    Contract.ProductionProgress += RecordsPerMonth;

    if (Contract.ProductionProgress >= 1.f && Contract.Terms.NumRecords > Contract.RecordsDelivered)
    {
        const int32 CompletedRecords = FMath::Clamp(static_cast<int32>(Contract.ProductionProgress), 0, Contract.Terms.NumRecords - Contract.RecordsDelivered);
        Contract.RecordsDelivered += CompletedRecords;
        Contract.ProductionProgress -= CompletedRecords;
    }
}

void UArtistManagerSubsystem::ExpireContract(const FString& ArtistId)
{
    const int32 ContractIndex = ActiveContracts.IndexOfByPredicate([&ArtistId](const FArtistContract& Contract)
    {
        return Contract.ArtistId == ArtistId;
    });

    if (ContractIndex != INDEX_NONE)
    {
        FArtistContract Contract = ActiveContracts[ContractIndex];
        Contract.bContractActive = false;
        Contract.EndDate = CurrentGameDate;

        ActiveContracts.RemoveAt(ContractIndex);
        ExpiredContracts.Add(Contract);

        OnContractExpired.Broadcast(Contract);
        OnArtistListChanged.Broadcast();
    }
}

const FArtistContract* UArtistManagerSubsystem::GetContractByArtistId(const FString& ArtistId) const
{
    return ActiveContracts.FindByPredicate([&ArtistId](const FArtistContract& Contract)
    {
        return Contract.ArtistId == ArtistId;
    });
}

void UArtistManagerSubsystem::GetSignedArtistData(TArray<FArtistData>& OutArtistData) const
{
    OutArtistData.Reset();

    for (const FArtistContract& Contract : ActiveContracts)
    {
        OutArtistData.Add(Contract.ArtistData);
    }
}

const FArtistContract* UArtistManagerSubsystem::FindContractByArtistName(const FString& ArtistName) const
{
    return ActiveContracts.FindByPredicate([&ArtistName](const FArtistContract& Contract)
    {
        return Contract.ArtistData.ArtistName == ArtistName;
    });
}

int32 UArtistManagerSubsystem::CalculateContractDurationMonths(const FArtistDealTerms& Deal) const
{
    return FMath::Max(Deal.ContractYears * 12, 0);
}

void UArtistManagerSubsystem::SaveState(UMusicSaveGame* SaveObject)
{
    ensure(IsInGameThread());

    if (!SaveObject)
    {
        return;
    }

    SaveObject->SavedContracts = ActiveContracts;
}

void UArtistManagerSubsystem::LoadState(const UMusicSaveGame* SaveObject)
{
    ensure(IsInGameThread());

    if (!SaveObject)
    {
        return;
    }

    ActiveContracts = SaveObject->SavedContracts;
    ExpiredContracts.Reset();
    OnMonthlyFinancialUpdate.Broadcast(ActiveContracts);
    OnArtistListChanged.Broadcast();
}
