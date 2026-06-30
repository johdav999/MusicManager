#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FArtistContract.h"
#include "ActiveContractItemWidget.generated.h"

class UButton;
class UImage;
class UProgressBar;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActiveContractItemSelected, FString, ArtistId);

UCLASS()
class MUSICMANAGER_API UActiveContractItemWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category="Contracts")
    void SetupContractItem(const FArtistContract& InContract);

    UFUNCTION(BlueprintCallable, Category="Contracts")
    void SetSelected(bool bInSelected);

    UFUNCTION(BlueprintCallable, Category="Contracts")
    FString GetArtistId() const { return ContractData.ArtistId; }

    UPROPERTY(BlueprintAssignable, Category="Contracts")
    FOnActiveContractItemSelected OnContractSelected;

protected:
    UPROPERTY(meta=(BindWidget))
    UButton* ContractButton = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UImage* ArtistPortraitImage = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* ArtistNameText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* GenreText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* TermText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* RecordsText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* StatusText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UTextBlock* EconomicsText = nullptr;

    UPROPERTY(meta=(BindWidgetOptional))
    UProgressBar* TermProgressBar = nullptr;

private:
    UFUNCTION()
    void HandleClicked();

    void ApplyVisualState();
    void SetTextSafe(UTextBlock* TextBlock, const FString& Value) const;
    void ApplyPortrait();

    UPROPERTY()
    FArtistContract ContractData;

    bool bSelected = false;
};
