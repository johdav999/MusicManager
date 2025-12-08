#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "MarketRegion.h"
#include "MarketManagerSubsystem.generated.h"

UCLASS()
class MUSICMANAGER_API UMarketManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /** Assign in BP or defaults: DataTable with FMarketRegion rows */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Market")
    UDataTable* RegionDataTable;

    /** Loaded regions keyed by RegionId */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Market")
    TMap<FString, FMarketRegion> LoadedRegions;

    /** Load all rows from the DataTable */
    UFUNCTION()
    void LoadRegions();

    /** Lookup region by RegionId */
    UFUNCTION(BlueprintCallable)
    bool GetRegion(const FString& RegionId, FMarketRegion& OutRegion) const;
};
