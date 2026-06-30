#pragma once

#include "Blueprint/UserWidget.h"
#include "MusicCommandResult.h"
#include "RecordManagerSubsystem.h"
#include "ReleasePlannerWidget.generated.h"

class UCommandDispatcherSubsystem;
class URecordManagerSubsystem;

UCLASS(BlueprintType, Blueprintable)
class MUSICMANAGER_API UReleasePlannerWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category="Release Planner")
    void RefreshPlanner();

    UFUNCTION(BlueprintCallable, Category="Release Planner")
    bool SelectRecord(const FString& RecordId);

    UFUNCTION(BlueprintCallable, Category="Release Planner")
    FMusicCommandResult ScheduleSelectedRelease(const FDateTime& ReleaseDate, const TArray<FString>& TargetRegionIds, const TArray<ERecordFormat>& Formats);

    UFUNCTION(BlueprintCallable, Category="Release Planner")
    const FReleasePlannerView& GetPlannerView() const { return CurrentView; }

    UFUNCTION(BlueprintCallable, Category="Release Planner")
    bool HasPlannableRecords() const { return CurrentView.bHasPlannableRecords; }

    UFUNCTION(BlueprintCallable, Category="Release Planner")
    bool CanScheduleSelectedRelease() const;

    UFUNCTION(BlueprintCallable, Category="Release Planner")
    FString GetLastError() const { return LastError; }

    UFUNCTION(BlueprintCallable, Category="Release Planner|Design")
    FString GetReferenceImagePath() const;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category="Release Planner")
    void OnPlannerViewRefreshed(const FReleasePlannerView& View);

    UFUNCTION(BlueprintImplementableEvent, Category="Release Planner")
    void OnPlannerCommandCompleted(const FMusicCommandResult& Result);

private:
    URecordManagerSubsystem* GetRecordSubsystem() const;
    UCommandDispatcherSubsystem* GetCommandDispatcher() const;

    UPROPERTY(VisibleAnywhere, Category="Release Planner")
    FReleasePlannerView CurrentView;

    UPROPERTY(VisibleAnywhere, Category="Release Planner")
    FString SelectedRecordId;

    UPROPERTY(VisibleAnywhere, Category="Release Planner")
    FString LastError;
};
