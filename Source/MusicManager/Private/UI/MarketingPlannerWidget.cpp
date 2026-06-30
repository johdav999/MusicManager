#include "UI/MarketingPlannerWidget.h"

#include "CommandDispatcherSubsystem.h"
#include "Engine/GameInstance.h"

void UMarketingPlannerWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshPlanner();
}

void UMarketingPlannerWidget::RefreshPlanner()
{
    LastError.Reset();

    UMarketingManagerSubsystem* MarketingSubsystem = GetMarketingSubsystem();
    if (!MarketingSubsystem)
    {
        CurrentView = FMarketingPlannerView();
        LastError = TEXT("Marketing planner is unavailable because the marketing system is missing.");
        OnPlannerViewRefreshed(CurrentView);
        return;
    }

    if (SelectedRecordId.IsEmpty())
    {
        TArray<FRecordData> Records;
        GetMarketableRecords(Records);
        if (Records.Num() > 0)
        {
            SelectedRecordId = Records[0].RecordId;
        }
    }

    FString Error;
    if (!MarketingSubsystem->BuildMarketingPlannerView(SelectedRecordId, CurrentView, Error))
    {
        LastError = Error.IsEmpty() ? TEXT("Marketing planner could not be refreshed.") : Error;
    }

    OnPlannerViewRefreshed(CurrentView);
}

bool UMarketingPlannerWidget::SelectRecord(const FString& RecordId)
{
    if (RecordId.IsEmpty())
    {
        LastError = TEXT("Select a marketable record first.");
        return false;
    }

    SelectedRecordId = RecordId;
    RefreshPlanner();
    return CurrentView.RecordId == RecordId;
}

void UMarketingPlannerWidget::GetMarketableRecords(TArray<FRecordData>& OutRecords) const
{
    OutRecords.Reset();

    const URecordManagerSubsystem* RecordSubsystem = GetRecordSubsystem();
    if (!RecordSubsystem)
    {
        return;
    }

    TArray<FRecordData> ReleasedRecords;
    RecordSubsystem->GetReleasedOrCatalogRecords(ReleasedRecords);
    for (const FRecordData& Record : ReleasedRecords)
    {
        FString LabelId;
        if (RecordSubsystem->IsRecordMarketable(Record.RecordId, LabelId))
        {
            OutRecords.Add(Record);
        }
    }

    TArray<FRecordData> ScheduledRecords;
    RecordSubsystem->GetScheduledReleases(ScheduledRecords);
    for (const FRecordData& Record : ScheduledRecords)
    {
        FString LabelId;
        if (RecordSubsystem->IsRecordMarketable(Record.RecordId, LabelId))
        {
            OutRecords.Add(Record);
        }
    }
}

FMusicCommandResult UMarketingPlannerWidget::LaunchCampaign(float Budget, const FDateTime& StartDate, const FDateTime& EndDate, const TArray<FString>& TargetRegionIds, const TArray<EMarketingChannel>& Channels)
{
    UCommandDispatcherSubsystem* Dispatcher = GetCommandDispatcher();
    if (!Dispatcher)
    {
        FMusicCommandResult Result = FMusicCommandResult::Failure(
            EMusicCommandErrorCode::SubsystemUnavailable,
            FText::FromString(TEXT("Marketing launch is unavailable because the command dispatcher is missing.")));
        OnPlannerCommandCompleted(Result);
        return Result;
    }

    FLaunchMarketingCampaignCommand Command;
    Command.RecordId = CurrentView.RecordId;
    Command.LabelId = CurrentView.LabelId;
    Command.Budget = Budget;
    Command.StartDate = StartDate;
    Command.EndDate = EndDate;
    Command.TargetRegionIds = TargetRegionIds;
    Command.Channels = Channels;

    FMusicCommandResult Result = Dispatcher->ExecuteLaunchMarketingCampaign(Command);
    if (Result.bSuccess)
    {
        RefreshPlanner();
    }
    else
    {
        LastError = Result.Message.ToString();
    }

    OnPlannerCommandCompleted(Result);
    return Result;
}

bool UMarketingPlannerWidget::CanLaunchCampaign() const
{
    return CurrentView.bRecordIsMarketable
        && !CurrentView.RecordId.IsEmpty()
        && !CurrentView.LabelId.IsEmpty()
        && CurrentView.TargetRegionIds.Num() > 0
        && CurrentView.MaximumAffordableBudget > 0.f
        && CurrentView.ChannelOptions.ContainsByPredicate([](const FMarketingChannelOption& Option)
        {
            return Option.bAvailable;
        });
}

FString UMarketingPlannerWidget::GetReferenceImagePath() const
{
    return TEXT("docs/design/references/marketing_planner_reference.png");
}

UMarketingManagerSubsystem* UMarketingPlannerWidget::GetMarketingSubsystem() const
{
    return GetGameInstance() ? GetGameInstance()->GetSubsystem<UMarketingManagerSubsystem>() : nullptr;
}

URecordManagerSubsystem* UMarketingPlannerWidget::GetRecordSubsystem() const
{
    return GetGameInstance() ? GetGameInstance()->GetSubsystem<URecordManagerSubsystem>() : nullptr;
}

UCommandDispatcherSubsystem* UMarketingPlannerWidget::GetCommandDispatcher() const
{
    return GetGameInstance() ? GetGameInstance()->GetSubsystem<UCommandDispatcherSubsystem>() : nullptr;
}
