#include "UI/ReleasePlannerWidget.h"

#include "CommandDispatcherSubsystem.h"
#include "Engine/GameInstance.h"

void UReleasePlannerWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshPlanner();
}

void UReleasePlannerWidget::RefreshPlanner()
{
    LastError.Reset();

    URecordManagerSubsystem* RecordSubsystem = GetRecordSubsystem();
    if (!RecordSubsystem)
    {
        CurrentView = FReleasePlannerView();
        LastError = TEXT("Release planner is unavailable because the record system is missing.");
        OnPlannerViewRefreshed(CurrentView);
        return;
    }

    FString Error;
    if (!RecordSubsystem->BuildReleasePlannerView(SelectedRecordId, CurrentView, Error))
    {
        LastError = Error.IsEmpty() ? TEXT("Release planner could not be refreshed.") : Error;
    }

    if (SelectedRecordId.IsEmpty() && !CurrentView.SelectedRecord.RecordId.IsEmpty())
    {
        SelectedRecordId = CurrentView.SelectedRecord.RecordId;
    }

    OnPlannerViewRefreshed(CurrentView);
}

bool UReleasePlannerWidget::SelectRecord(const FString& RecordId)
{
    if (RecordId.IsEmpty())
    {
        LastError = TEXT("Select a recorded release first.");
        return false;
    }

    SelectedRecordId = RecordId;
    RefreshPlanner();
    return CurrentView.SelectedRecord.RecordId == RecordId;
}

FMusicCommandResult UReleasePlannerWidget::ScheduleSelectedRelease(const FDateTime& ReleaseDate, const TArray<FString>& TargetRegionIds, const TArray<ERecordFormat>& Formats)
{
    UCommandDispatcherSubsystem* Dispatcher = GetCommandDispatcher();
    if (!Dispatcher)
    {
        FMusicCommandResult Result = FMusicCommandResult::Failure(
            EMusicCommandErrorCode::SubsystemUnavailable,
            FText::FromString(TEXT("Release scheduling is unavailable because the command dispatcher is missing.")));
        OnPlannerCommandCompleted(Result);
        return Result;
    }

    FScheduleReleaseCommand Command;
    Command.RecordId = CurrentView.SelectedRecord.RecordId;
    Command.LabelId = CurrentView.SelectedRecord.LabelId;
    Command.ReleaseDate = ReleaseDate;
    Command.TargetRegionIds = TargetRegionIds;
    Command.Formats = Formats;

    FMusicCommandResult Result = Dispatcher->ExecuteScheduleRelease(Command);
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

bool UReleasePlannerWidget::CanScheduleSelectedRelease() const
{
    return CurrentView.bHasPlannableRecords
        && !CurrentView.SelectedRecord.RecordId.IsEmpty()
        && CurrentView.AvailableRegions.Num() > 0
        && CurrentView.ValidFormats.Num() > 0;
}

FString UReleasePlannerWidget::GetReferenceImagePath() const
{
    return TEXT("docs/design/references/release_planner_reference.png");
}

URecordManagerSubsystem* UReleasePlannerWidget::GetRecordSubsystem() const
{
    return GetGameInstance() ? GetGameInstance()->GetSubsystem<URecordManagerSubsystem>() : nullptr;
}

UCommandDispatcherSubsystem* UReleasePlannerWidget::GetCommandDispatcher() const
{
    return GetGameInstance() ? GetGameInstance()->GetSubsystem<UCommandDispatcherSubsystem>() : nullptr;
}
