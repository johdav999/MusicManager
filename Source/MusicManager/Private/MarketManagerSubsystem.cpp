#include "MarketManagerSubsystem.h"
#include "GameTimeSubsystem.h"
#include "Engine/GameInstance.h"

void UMarketManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    BindToTimeSubsystem();
}

void UMarketManagerSubsystem::Deinitialize()
{
    if (UGameTimeSubsystem* TS = TimeSubsystem.Get())
    {
        TS->OnMonthAdvanced.RemoveAll(this);
    }
    TimeSubsystem.Reset();

    Super::Deinitialize();
}

void UMarketManagerSubsystem::BindToTimeSubsystem()
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGameTimeSubsystem* TS = GI->GetSubsystem<UGameTimeSubsystem>())
        {
            TimeSubsystem = TS;
            TS->OnMonthAdvanced.AddDynamic(this, &UMarketManagerSubsystem::HandleMonthAdvanced);
        }
    }
}

void UMarketManagerSubsystem::HandleMonthAdvanced(const FDateTime& NewDate)
{
    // Simulation placeholder
}

void UMarketManagerSubsystem::RegisterRecordRelease(const FString& RecordId, const FString& LabelId)
{
    ActiveRecords.Add(RecordId);
}

void UMarketManagerSubsystem::ApplyRadioExposure(const FString& RegionId, const FString& ArtistId, float Intensity)
{
    if (FMarketRegion* Region = Regions.Find(RegionId))
    {
        Region->RecentArtistExposure.FindOrAdd(ArtistId) += Intensity;
    }
}

void UMarketManagerSubsystem::SimulateMonthlyRecordSales(const FDateTime& PeriodStart, const FDateTime& PeriodEnd)
{
    // Sales simulation placeholder
    // UFinanceManagerSubsystem* Finance = GetGameInstance()->GetSubsystem<UFinanceManagerSubsystem>();
    // Finance->RegisterRecordSalesRevenue(LabelId, RecordId, Revenue, PeriodEnd);
}

void UMarketManagerSubsystem::GetLastSalesForRecord(const FString& RecordId, TArray<FRecordSalesSnapshot>& OutSales) const
{
    for (const FRecordSalesSnapshot& Entry : SalesHistory)
    {
        if (Entry.RecordId == RecordId)
        {
            OutSales.Add(Entry);
        }
    }
}
