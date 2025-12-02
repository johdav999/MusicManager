#include "ArtistManagerSubsystem.h"

#include "Engine/Engine.h"
#include "GameTimeSubsystem.h"
#include "MusicSaveGame.h"
#include "SongManagerSubsystem.h"
#include "Async/Async.h"
#include "HAL/PlatformProcess.h"

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
            TimeSubsystem->OnMonthAdvanced.AddDynamic(this, &UArtistManagerSubsystem::HandleMonthAdvanced);
            CurrentGameDate = TimeSubsystem->GetCurrentGameDate();
        }
    }
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
