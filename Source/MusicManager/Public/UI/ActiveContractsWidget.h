#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FArtistContract.h"
#include "ActiveContractsWidget.generated.h"

class UActiveContractItemWidget;
class UButton;
class UProgressBar;
class UScrollBox;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActiveContractsCloseRequested);

UCLASS()
class MUSICMANAGER_API UActiveContractsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category="Contracts")
    void RefreshContracts();

    UFUNCTION(BlueprintCallable, Category="Contracts")
    void ShowForArtist(const FString& ArtistId);

    UFUNCTION(BlueprintCallable, Category="Contracts")
    void SelectArtist(const FString& ArtistId);

    UPROPERTY(BlueprintAssignable, Category="Contracts")
    FOnActiveContractsCloseRequested OnCloseRequested;

protected:
    UPROPERTY(meta=(BindWidget))
    UButton* CloseButton = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UButton* ViewArtistButton = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UButton* CloseActionButton = nullptr;

    UPROPERTY(meta=(BindWidget))
    UScrollBox* ContractScrollBox = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* EmptyStateText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* HeaderCountText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* DetailArtistNameText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* DetailGenreText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* DetailStatusText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* DetailPeriodText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* DetailTermsText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* DetailRoyaltyText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* DetailBonusText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* DetailUpkeepText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* DetailRevenueText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* DetailCostText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* DetailLastRoyaltyText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* DetailRecordsText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* DetailMomentumText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* DetailProgressText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UProgressBar* DetailTermProgressBar = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UProgressBar* DetailProductionProgressBar = nullptr;

    UPROPERTY(EditAnywhere, Category="Contracts")
    TSubclassOf<UActiveContractItemWidget> ItemClass;

private:
    UFUNCTION()
    void HandleCloseClicked();

    UFUNCTION()
    void HandleViewArtistClicked();

    UFUNCTION()
    void HandleContractSelected(FString ArtistId);

    UFUNCTION()
    void HandleArtistSigned(const FArtistContract& SignedContract);

    UFUNCTION()
    void HandleContractExpired(const FArtistContract& ExpiredContract);

    UFUNCTION()
    void HandleContractsUpdated(const TArray<FArtistContract>& UpdatedContracts);

    void RebuildList(const TArray<FArtistContract>& Contracts);
    void UpdateDetailPanel(const FArtistContract* Contract);
    void UpdateSelectionVisuals();
    void SetTextSafe(UTextBlock* TextBlock, const FString& Value) const;
    void BindSubsystemEvents();
    void UnbindSubsystemEvents();

    const FArtistContract* FindContractByArtistId(const FString& ArtistId) const;
    class UArtistManagerSubsystem* GetArtistSubsystem() const;

    UPROPERTY()
    TArray<FArtistContract> CachedContracts;

    UPROPERTY()
    TArray<TObjectPtr<UActiveContractItemWidget>> SpawnedItems;

    FString SelectedArtistId;
};
