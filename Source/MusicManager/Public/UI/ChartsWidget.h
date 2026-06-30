#pragma once

#include "Blueprint/UserWidget.h"
#include "ChartManagerSubsystem.h"
#include "ChartsWidget.generated.h"

class UChartManagerSubsystem;

UCLASS(BlueprintType, Blueprintable)
class MUSICMANAGER_API UChartsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category="Charts")
    void RefreshCharts();

    UFUNCTION(BlueprintCallable, Category="Charts")
    bool SelectChart(const FString& ChartId);

    UFUNCTION(BlueprintCallable, Category="Charts")
    bool SelectRecordHistory(const FString& RecordId);

    UFUNCTION(BlueprintCallable, Category="Charts")
    const FChartListView& GetChartListView() const { return CurrentChartView; }

    UFUNCTION(BlueprintCallable, Category="Charts")
    const FChartDashboardView& GetDashboardView() const { return DashboardView; }

    UFUNCTION(BlueprintCallable, Category="Charts")
    const FRecordChartHistoryView& GetRecordHistoryView() const { return RecordHistoryView; }

    UFUNCTION(BlueprintCallable, Category="Charts")
    FString GetSelectedChartId() const { return SelectedChartId; }

    UFUNCTION(BlueprintCallable, Category="Charts")
    FString GetLastError() const { return LastError; }

    UFUNCTION(BlueprintCallable, Category="Charts|Design")
    FString GetReferenceImagePath() const;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category="Charts")
    void OnChartsRefreshed(const FChartListView& ChartView, const FChartDashboardView& Dashboard);

    UFUNCTION(BlueprintImplementableEvent, Category="Charts")
    void OnRecordHistoryRefreshed(const FRecordChartHistoryView& HistoryView);

private:
    UChartManagerSubsystem* GetChartSubsystem() const;

    UPROPERTY(VisibleAnywhere, Category="Charts")
    FChartListView CurrentChartView;

    UPROPERTY(VisibleAnywhere, Category="Charts")
    FChartDashboardView DashboardView;

    UPROPERTY(VisibleAnywhere, Category="Charts")
    FRecordChartHistoryView RecordHistoryView;

    UPROPERTY(VisibleAnywhere, Category="Charts")
    FString SelectedChartId;

    UPROPERTY(VisibleAnywhere, Category="Charts")
    FString LastError;
};
