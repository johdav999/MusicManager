#include "UI/ChartsWidget.h"

#include "Engine/GameInstance.h"

void UChartsWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshCharts();
}

void UChartsWidget::RefreshCharts()
{
    LastError.Reset();

    UChartManagerSubsystem* ChartSubsystem = GetChartSubsystem();
    if (!ChartSubsystem)
    {
        CurrentChartView = FChartListView();
        DashboardView = FChartDashboardView();
        LastError = TEXT("Charts are unavailable because the chart system is missing.");
        OnChartsRefreshed(CurrentChartView, DashboardView);
        return;
    }

    if (!ChartSubsystem->BuildChartListView(SelectedChartId, CurrentChartView))
    {
        LastError = TEXT("Charts could not be refreshed.");
    }

    if (SelectedChartId.IsEmpty() && !CurrentChartView.SelectedChartId.IsEmpty())
    {
        SelectedChartId = CurrentChartView.SelectedChartId;
    }

    if (!ChartSubsystem->BuildChartDashboardView(DashboardView))
    {
        LastError = TEXT("Chart dashboard could not be refreshed.");
    }

    OnChartsRefreshed(CurrentChartView, DashboardView);
}

bool UChartsWidget::SelectChart(const FString& ChartId)
{
    if (ChartId.IsEmpty())
    {
        LastError = TEXT("Select a chart first.");
        return false;
    }

    SelectedChartId = ChartId;
    RefreshCharts();
    return CurrentChartView.SelectedChartId == ChartId;
}

bool UChartsWidget::SelectRecordHistory(const FString& RecordId)
{
    if (RecordId.IsEmpty())
    {
        LastError = TEXT("Select a chart entry first.");
        return false;
    }

    UChartManagerSubsystem* ChartSubsystem = GetChartSubsystem();
    if (!ChartSubsystem)
    {
        RecordHistoryView = FRecordChartHistoryView();
        LastError = TEXT("Record chart history is unavailable because the chart system is missing.");
        OnRecordHistoryRefreshed(RecordHistoryView);
        return false;
    }

    const bool bBuilt = ChartSubsystem->BuildRecordChartHistoryView(RecordId, RecordHistoryView);
    if (!bBuilt)
    {
        LastError = TEXT("Record chart history could not be refreshed.");
    }

    OnRecordHistoryRefreshed(RecordHistoryView);
    return bBuilt && RecordHistoryView.RecordId == RecordId;
}

FString UChartsWidget::GetReferenceImagePath() const
{
    return TEXT("docs/design/references/charts_reference.png");
}

UChartManagerSubsystem* UChartsWidget::GetChartSubsystem() const
{
    return GetGameInstance() ? GetGameInstance()->GetSubsystem<UChartManagerSubsystem>() : nullptr;
}
