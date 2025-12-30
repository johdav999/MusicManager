#include "ArtistManagerSubsystem.h"

#include "Engine/Engine.h"
#include "GameTimeSubsystem.h"
#include "MusicSaveGame.h"
#include "RecordManagerSubsystem.h"
#include "SongManagerSubsystem.h"
#include "Song.h"
#include "Async/Async.h"
#include "HAL/PlatformProcess.h"
#include "UIManagerSubsystem.h"

UArtistManagerSubsystem::UArtistManagerSubsystem()
{
    static ConstructorHelpers::FObjectFinder<UDataTable> ArtistDataObj(
        TEXT("/Game/Data/ArtistData.ArtistData")
    );

    if (ArtistDataObj.Succeeded())
    {
        ArtistDataTable = ArtistDataObj.Object;
    }
    else
    {
        ArtistDataTable = nullptr;
        UE_LOG(LogTemp, Error, TEXT("Could not load ArtistData datatable at /Game/Data/ArtistData.ArtistData"));
    }
}

void UArtistManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    ActiveContracts.Reset();
    ExpiredContracts.Reset();
    ArtistActionAvailability.Reset();
    //static ConstructorHelpers::FObjectFinder<UDataTable> ArtistDataObj(
    //    TEXT("/Game/Data/ArtistData.ArtistData")
    //);

    //if (ArtistDataObj.Succeeded())
    //{
    //    ArtistDataTable = ArtistDataObj.Object;
    //}
    //else
    //{
    //    ArtistDataTable = nullptr;
    //    UE_LOG(LogTemp, Error, TEXT("Could not load ArtistData datatable at /Game/Data/ArtistData.ArtistData"));
    //}
    //LoadArtistsFromDataTable();

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UGameTimeSubsystem* TimeSubsystem = GameInstance->GetSubsystem<UGameTimeSubsystem>())
        {
            CurrentGameDate = TimeSubsystem->GetCurrentGameDate();
        }

        if (URecordManagerSubsystem* RecordSubsystem = GameInstance->GetSubsystem<URecordManagerSubsystem>())
        {
            RecordCreatedHandle = RecordSubsystem->OnArtistRecordCreated.AddUObject(this, &UArtistManagerSubsystem::HandleArtistRecordCreated);
        }
    }
}

void UArtistManagerSubsystem::Deinitialize()
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (URecordManagerSubsystem* RecordSubsystem = GameInstance->GetSubsystem<URecordManagerSubsystem>())
        {
            if (RecordCreatedHandle.IsValid())
            {
                RecordSubsystem->OnArtistRecordCreated.Remove(RecordCreatedHandle);
            }
        }
    }

    ArtistActionAvailability.Reset();
    Super::Deinitialize();
}

void UArtistManagerSubsystem::GetUnsignedArtists(TArray<FArtistData>& OutArtists) const
{
    OutArtists = UnsignedArtists;
}

bool UArtistManagerSubsystem::GetNextUnsignedArtist(FArtistData& OutArtist) const
{
    if (UnsignedArtists.Num() == 0)
    {
        return false;
    }

    OutArtist = UnsignedArtists[0];
    return true;
}

void UArtistManagerSubsystem::RotateUnsignedArtist()
{
    if (UnsignedArtists.Num() > 1)
    {
        const FArtistData Temp = UnsignedArtists[0];
        UnsignedArtists.RemoveAt(0);
        UnsignedArtists.Add(Temp);
    }
}

USong* UArtistManagerSubsystem::CreateSongForArtist(const FString& ArtistId, const FSongData& Data)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<UArtistManagerSubsystem> WeakThis(this);
        TWeakObjectPtr<USong> CreatedSong;
        FEvent* SyncEvent = FPlatformProcess::GetSynchEventFromPool(true);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, ArtistId, Data, &CreatedSong, SyncEvent]()
        {
            if (UArtistManagerSubsystem* StrongThis = WeakThis.Get())
            {
                CreatedSong = StrongThis->CreateSongForArtist(ArtistId, Data);
            }
            SyncEvent->Trigger();
        });

        SyncEvent->Wait();
        FPlatformProcess::ReturnSynchEventToPool(SyncEvent);
        return CreatedSong.Get();
    }

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (USongManagerSubsystem* SongManager = GameInstance->GetSubsystem<USongManagerSubsystem>())
        {
            USong* Song = SongManager->CreateSong(ArtistId, Data);
            if (Song)
            {
                RegisterSongToArtist(ArtistId, Song->SongId);
            }
            return Song;
        }
    }
    return nullptr;
}

void UArtistManagerSubsystem::GetSongsForArtist(const FString& ArtistId, TArray<USong*>& OutSongs) const
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<const UArtistManagerSubsystem> WeakThis(this);
        FEvent* SyncEvent = FPlatformProcess::GetSynchEventFromPool(true);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, ArtistId, &OutSongs, SyncEvent]()
        {
            if (const UArtistManagerSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->GetSongsForArtist(ArtistId, OutSongs);
            }
            SyncEvent->Trigger();
        });
        SyncEvent->Wait();
        FPlatformProcess::ReturnSynchEventToPool(SyncEvent);
        return;
    }

    if (const UGameInstance* GameInstance = GetGameInstance())
    {
        if (const USongManagerSubsystem* SongManager = GameInstance->GetSubsystem<USongManagerSubsystem>())
        {
            SongManager->GetSongsForArtist(ArtistId, OutSongs);
        }
    }
}

void UArtistManagerSubsystem::RegisterSongToArtist(const FString& ArtistId, const FString& SongId)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<UArtistManagerSubsystem> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, ArtistId, SongId]()
        {
            if (UArtistManagerSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->RegisterSongToArtist(ArtistId, SongId);
            }
        });
        return;
    }

    for (const auto& Pair : ArtistToSongs)
    {
        const FString& ExistingArtistId = Pair.Key;
        const FArtistSongList& SongList = Pair.Value;

        if (ExistingArtistId != ArtistId &&
            SongList.SongIds.Contains(SongId))
        {
            UE_LOG(LogTemp, Warning, TEXT("RegisterSongToArtist: Song %s is already assigned to Artist %s. Cannot assign to Artist %s."),
                   *SongId, *ExistingArtistId, *ArtistId);

            return;
        }
    }

    ArtistToSongs.FindOrAdd(ArtistId).SongIds.AddUnique(SongId);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (USongManagerSubsystem* SongManager = GameInstance->GetSubsystem<USongManagerSubsystem>())
        {           
            if (USong* Song = SongManager->GetSongById(SongId))
            {
                Song->ArtistId = ArtistId;
            }
        }
    }

    UpdateArtistActionAvailability(ArtistId);
}

bool UArtistManagerSubsystem::IsArtistReadyToRecord(const FString& ArtistId) const
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<const UArtistManagerSubsystem> WeakThis(this);
        bool bReady = false;
        FEvent* SyncEvent = FPlatformProcess::GetSynchEventFromPool(true);

        AsyncTask(ENamedThreads::GameThread, [WeakThis, ArtistId, &bReady, SyncEvent]()
        {
            if (const UArtistManagerSubsystem* StrongThis = WeakThis.Get())
            {
                bReady = StrongThis->IsArtistReadyToRecord(ArtistId);
            }
            SyncEvent->Trigger();
        });

        SyncEvent->Wait();
        FPlatformProcess::ReturnSynchEventToPool(SyncEvent);
        return bReady;
    }

    if (const UGameInstance* GameInstance = GetGameInstance())
    {
        if (const USongManagerSubsystem* SongManager = GameInstance->GetSubsystem<USongManagerSubsystem>())
        {
            TArray<USong*> EligibleSongs;
            SongManager->GetEligibleSongsForRecording(ArtistId, EligibleSongs);
            return EligibleSongs.Num() > 0;
        }
    }

    return false;
}

EArtistActionAvailability UArtistManagerSubsystem::GetArtistActionAvailability(const FString& ArtistId) const
{
    return EvaluateArtistActionAvailability(ArtistId);
}

void UArtistManagerSubsystem::RefreshArtistActionAvailability(const FString& ArtistId)
{
    UpdateArtistActionAvailability(ArtistId);
}

void UArtistManagerSubsystem::SetSelectedArtist(const FString& ArtistId)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<UArtistManagerSubsystem> WeakThis(this);
        const FString CopyId = ArtistId;

        AsyncTask(ENamedThreads::GameThread, [WeakThis, CopyId]()
        {
            if (UArtistManagerSubsystem* Strong = WeakThis.Get())
            {
                Strong->SetSelectedArtist(CopyId);
            }
        });
        return;
    }

    SelectedArtistId = ArtistId;
    UE_LOG(LogTemp, Log, TEXT("Selected Artist Updated To: %s"), *ArtistId);

    if (UGameInstance* GameInstance = GetGameInstance())
    {
        if (UUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<UUIManagerSubsystem>())
        {
            UIManager->SetCurrentLabelId(ArtistId);
        }
    }
}

FString UArtistManagerSubsystem::GetSelectedArtist() const
{
    return SelectedArtistId;
}

EArtistVisualState UArtistManagerSubsystem::GetArtistVisualState(const FString& ArtistId) const
{
    const FArtistContract* Contract = GetContractByArtistId(ArtistId);
    if (!Contract)
    {
        Contract = FindContractByArtistName(ArtistId);
    }

    const FArtistData* ArtistData = nullptr;
    if (Contract)
    {
        ArtistData = &Contract->ArtistData;
    }
    else
    {
        for (const FArtistData& Artist : UnsignedArtists)
        {
            if (Artist.ArtistId == ArtistId || Artist.ArtistName == ArtistId)
            {
                ArtistData = &Artist;
                break;
            }
        }
    }

    if (!ArtistData)
    {
        return EArtistVisualState::Idle;
    }

    const float CombinedScore = (ArtistData->PerformanceScore
        + ArtistData->StagePresence
        + ArtistData->AudienceEngagement
        + ArtistData->VocalQuality
        + ArtistData->SongwritingQuality) / 5.0f;

    if (CombinedScore >= 80.0f)
    {
        return EArtistVisualState::Rising;
    }

    if (CombinedScore >= 55.0f)
    {
        return EArtistVisualState::Stable;
    }

    if (CombinedScore >= 30.0f)
    {
        return EArtistVisualState::Declining;
    }

    return EArtistVisualState::Idle;
}

void UArtistManagerSubsystem::LoadArtistsFromDataTable()
{
    ensure(IsInGameThread());

    UnsignedArtists.Empty();

    if (!ArtistDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("ArtistManagerSubsystem: No ArtistDataTable assigned."));
        return;
    }

    static const FString ContextString(TEXT("Artist Data Table"));

    TArray<FName> RowNames = ArtistDataTable->GetRowNames();
    for (const FName& RowName : RowNames)
    {
        if (FArtistData* Row = ArtistDataTable->FindRow<FArtistData>(RowName, ContextString))
        {
            UnsignedArtists.Add(*Row);

            if (UGameInstance* GameInstance = GetGameInstance())
            {
                if (USongManagerSubsystem* SongManager = GameInstance->GetSubsystem<USongManagerSubsystem>())
                {
                    TArray<USong*> AllSongs;
                    SongManager->GetAllSongs(AllSongs);

                    if (AllSongs.Num() > 0 && AllSongs[0])
                    {
                        RegisterSongToArtist(Row->ArtistName, AllSongs[0]->SongId);
                    }
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Loaded %d unsigned artists from DataTable."), UnsignedArtists.Num());
}

void UArtistManagerSubsystem::SignArtist(const FArtistDealTerms& Deal)
{
    if (UnsignedArtists.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("SignArtist: No unsigned artists available."));
        return;
    }

    const FArtistData ArtistInfo = UnsignedArtists[0];

    FArtistContract NewContract;
    NewContract.ArtistId = ArtistInfo.ArtistName;
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

    UnsignedArtists.RemoveAt(0);

    // Select the newly signed artist as the active label context.
    SetSelectedArtist(NewContract.ArtistId);

    OnArtistSigned.Broadcast(NewContract);
    OnArtistListChanged.Broadcast();

    UpdateArtistActionAvailability(NewContract.ArtistId);
}

void UArtistManagerSubsystem::RejectArtist(const FString& ArtistId)
{
    OnArtistRejected.Broadcast(ArtistId);
}

void UArtistManagerSubsystem::AdvanceMonth()
{
    check(IsInGameThread());

    ApplyMonthlyMomentum();

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

    if (ActiveContracts.Num() > 0)
    {
        SetSelectedArtist(ActiveContracts[0].ArtistId);
    }
    else
    {
        SetSelectedArtist(FString());
    }

    RefreshAllArtistActionAvailability();
}

void UArtistManagerSubsystem::GetArtistMarketModifiers(const FString& ArtistId, const FString& MarketId, const FMarketDemandSnapshot& MarketDemand, FArtistMarketModifiers& OutModifiers) const
{
    OutModifiers = FArtistMarketModifiers();

    const FArtistContract* Contract = GetContractByArtistId(ArtistId);
    if (!Contract)
    {
        return;
    }

    const FArtistData& ArtistData = Contract->ArtistData;

    // Popularity is anchored to artist quality but gently scaled by reachable audience in the market.
    const float AudienceComposite = ArtistData.PerformanceScore + ArtistData.StagePresence + ArtistData.AudienceEngagement;
    const float CreativeComposite = ArtistData.VocalQuality + ArtistData.SongwritingQuality;
    const float BasePopularity = FMath::Clamp((AudienceComposite + CreativeComposite) / 500.f, 0.25f, 2.5f);
    OutModifiers.PopularityMultiplier = BasePopularity * FMath::Clamp(0.75f + MarketDemand.TotalReach * 0.05f, 0.5f, 1.5f);

    if (const float* Momentum = ArtistMomentum.Find(ArtistId))
    {
        OutModifiers.MomentumMultiplier = *Momentum;
    }
    else
    {
        OutModifiers.MomentumMultiplier = FMath::Clamp(ArtistData.PerformanceScore / 75.f, 0.5f, 1.5f);
    }

    if (const float* Reputation = ArtistReputation.Find(ArtistId))
    {
        OutModifiers.ReputationMultiplier = *Reputation;
    }

    // Genre fit leans on the market snapshot, rewarding trends when momentum is already high.
    const float* MarketAffinity = MarketDemand.GenreDemand.Find(ArtistData.Genre);
    const float DemandFit = MarketAffinity ? *MarketAffinity : 0.25f;
    OutModifiers.GenreFitMultiplier = FMath::Lerp(0.65f, 1.35f, DemandFit);

    const float TrendBoost = MarketAffinity ? *MarketAffinity : 0.f;
    OutModifiers.MomentumMultiplier *= (1.0f + TrendBoost * 0.2f);

    // Cannibalization reduces conversion when multiple releases are competing in the same month.
    const int32* Concurrent = ConcurrentReleasesCache.Find(ArtistId);
    if (Concurrent && *Concurrent > 1)
    {
        const float Suppression = FMath::Clamp(0.12f * static_cast<float>(*Concurrent - 1), 0.f, 0.6f);
        OutModifiers.GenreFitMultiplier *= (1.0f - Suppression);
    }

    // MarketId preserved for future local reputation calculations.
    (void)MarketId;
}

void UArtistManagerSubsystem::ApplyMonthlyMomentum()
{
    for (const FArtistContract& Contract : ActiveContracts)
    {
        float& MomentumValue = ArtistMomentum.FindOrAdd(Contract.ArtistId);
        MomentumValue = FMath::Clamp(MomentumValue * 0.97f + (Contract.PerformanceMomentum / 120.f), 0.5f, 2.0f);

        float& ReputationValue = ArtistReputation.FindOrAdd(Contract.ArtistId);
        const float ContractAging = FMath::Clamp(static_cast<float>(Contract.MonthsActive) / 60.f, 0.f, 0.5f);
        ReputationValue = FMath::Clamp(ReputationValue * 0.99f + 0.6f + ContractAging, 0.5f, 2.5f);
    }
}

void UArtistManagerSubsystem::ApplyRadioExposureMomentum(const TMap<FString, float>& ArtistMomentumBoosts)
{
    for (const auto& Pair : ArtistMomentumBoosts)
    {
        if (!GetContractByArtistId(Pair.Key))
        {
            continue; // Ignore artists we do not manage.
        }

        float& MomentumValue = ArtistMomentum.FindOrAdd(Pair.Key, 1.0f);
        MomentumValue = FMath::Clamp(MomentumValue * Pair.Value, 0.5f, 2.5f);
    }
}

void UArtistManagerSubsystem::SetConcurrentReleaseCount(const FString& ArtistId, int32 ConcurrentReleases)
{
    ConcurrentReleasesCache.FindOrAdd(ArtistId) = ConcurrentReleases;
}

void UArtistManagerSubsystem::ClearConcurrentReleaseCache()
{
    ConcurrentReleasesCache.Empty();
}

void UArtistManagerSubsystem::RefreshAllArtistActionAvailability()
{
    for (const FArtistContract& Contract : ActiveContracts)
    {
        UpdateArtistActionAvailability(Contract.ArtistId);
    }
}

EArtistActionAvailability UArtistManagerSubsystem::EvaluateArtistActionAvailability(const FString& ArtistId) const
{
    if (IsArtistReadyToRecord(ArtistId))
    {
        return EArtistActionAvailability::RecordReady;
    }

    return EArtistActionAvailability::None;
}
static FString ArtistActionAvailabilityToString(EArtistActionAvailability Value)
{
    const UEnum* Enum = StaticEnum<EArtistActionAvailability>();
    return Enum
        ? Enum->GetNameStringByValue(static_cast<int64>(Value))
        : TEXT("Invalid");
}

void UArtistManagerSubsystem::UpdateArtistActionAvailability(const FString& ArtistId)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<UArtistManagerSubsystem> WeakThis(this);
        const FString ArtistIdCopy = ArtistId;
        AsyncTask(ENamedThreads::GameThread, [WeakThis, ArtistIdCopy]()
        {
            if (UArtistManagerSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->UpdateArtistActionAvailability(ArtistIdCopy);
            }
        });
        return;
    }

    const EArtistActionAvailability NewAvailability = EvaluateArtistActionAvailability(ArtistId);
    const EArtistActionAvailability* PreviousAvailability = ArtistActionAvailability.Find(ArtistId);
    UE_LOG(
        LogTemp,
        Log,
        TEXT("Artist [%s] action availability: Previous=%s → New=%s"),
        *ArtistId,
        PreviousAvailability
        ? *ArtistActionAvailabilityToString(*PreviousAvailability)
        : TEXT("None"),
        *ArtistActionAvailabilityToString(NewAvailability)
    );

    if (PreviousAvailability && *PreviousAvailability == NewAvailability)
    {
        return;
    }

    ArtistActionAvailability.FindOrAdd(ArtistId) = NewAvailability;

    if (!PreviousAvailability && NewAvailability == EArtistActionAvailability::None)
    {
        return;
    }

    OnArtistActionAvailabilityChanged.Broadcast(ArtistId, NewAvailability);
}

void UArtistManagerSubsystem::HandleArtistRecordCreated(FString ArtistId)
{
    UpdateArtistActionAvailability(ArtistId);
}
