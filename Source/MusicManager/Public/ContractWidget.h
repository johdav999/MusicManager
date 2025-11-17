#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FArtistContract.h"
#include "ContractWidget.generated.h"

class UTextBlock;

/**
 * UUserWidget that displays the details of an artist contract in a read-only format.
 */
UCLASS()
class MUSICMANAGER_API UContractWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Populate the widget with the provided contract data. */
    UFUNCTION(BlueprintCallable, Category = "Contract")
    void SetContractData(const FArtistContract& Contract);

protected:
    /** Copy of the contract currently displayed by the widget. */
    UPROPERTY()
    FArtistContract ContractData;

    // Bound widget references for each displayed field
    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextArtistName = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextContractYears = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextNumRecords = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextRoyaltyRate = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextSignUpBonus = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextStartDate = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextEndDate = nullptr;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* TextRecordsDelivered = nullptr;

    //UPROPERTY(meta = (BindWidget))
    //UTextBlock* TextContractActive = nullptr;

    //UPROPERTY(meta = (BindWidget))
    //UTextBlock* TextLifetimeRevenue = nullptr;

    //UPROPERTY(meta = (BindWidget))
    //UTextBlock* TextLifetimeCost = nullptr;

    //UPROPERTY(meta = (BindWidget))
    //UTextBlock* TextLastRoyaltyPayment = nullptr;

    //UPROPERTY(meta = (BindWidget))
    //UTextBlock* TextMonthlyUpkeepCost = nullptr;

    //UPROPERTY(meta = (BindWidget))
    //UTextBlock* TextCumulativeRoyaltyPaid = nullptr;

    //UPROPERTY(meta = (BindWidget))
    //UTextBlock* TextMonthsActive = nullptr;

    //UPROPERTY(meta = (BindWidget))
    //UTextBlock* TextPerformanceMomentum = nullptr;

    //UPROPERTY(meta = (BindWidget))
    //UTextBlock* TextProductionProgress = nullptr;
};
