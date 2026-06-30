#pragma once

#include "Blueprint/UserWidget.h"
#include "MarketingManagerSubsystem.h"
#include "MusicCommandResult.h"
#include "RecordManagerSubsystem.h"
#include "MarketingPlannerWidget.generated.h"

class UCommandDispatcherSubsystem;
class UMarketingManagerSubsystem;
class URecordManagerSubsystem;

UCLASS(BlueprintType, Blueprintable)
class MUSICMANAGER_API UMarketingPlannerWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category="Marketing Planner")
    void RefreshPlanner();

    UFUNCTION(BlueprintCallable, Category="Marketing Planner")
    bool SelectRecord(const FString& RecordId);

    UFUNCTION(BlueprintCallable, Category="Marketing Planner")
    void GetMarketableRecords(TArray<FRecordData>& OutRecords) const;

    UFUNCTION(BlueprintCallable, Category="Marketing Planner")
    FMusicCommandResult LaunchCampaign(float Budget, const FDateTime& StartDate, const FDateTime& EndDate, const TArray<FString>& TargetRegionIds, const TArray<EMarketingChannel>& Channels);

    UFUNCTION(BlueprintCallable, Category="Marketing Planner")
    const FMarketingPlannerView& GetPlannerView() const { return CurrentView; }

    UFUNCTION(BlueprintCallable, Category="Marketing Planner")
    bool CanLaunchCampaign() const;

    UFUNCTION(BlueprintCallable, Category="Marketing Planner")
    FString GetLastError() const { return LastError; }

    UFUNCTION(BlueprintCallable, Category="Marketing Planner|Design")
    FString GetReferenceImagePath() const;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category="Marketing Planner")
    void OnPlannerViewRefreshed(const FMarketingPlannerView& View);

    UFUNCTION(BlueprintImplementableEvent, Category="Marketing Planner")
    void OnPlannerCommandCompleted(const FMusicCommandResult& Result);

private:
    UMarketingManagerSubsystem* GetMarketingSubsystem() const;
    URecordManagerSubsystem* GetRecordSubsystem() const;
    UCommandDispatcherSubsystem* GetCommandDispatcher() const;

    UPROPERTY(VisibleAnywhere, Category="Marketing Planner")
    FMarketingPlannerView CurrentView;

    UPROPERTY(VisibleAnywhere, Category="Marketing Planner")
    FString SelectedRecordId;

    UPROPERTY(VisibleAnywhere, Category="Marketing Planner")
    FString LastError;
};
