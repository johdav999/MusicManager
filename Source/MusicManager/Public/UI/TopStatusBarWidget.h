#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MusicCommandResult.h"
#include "TopStatusBarWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UCommandDispatcherSubsystem;
class UFinanceManagerSubsystem;
class UGameTimeSubsystem;
class UHorizontalBox;
class UImage;
class UPlayerLabelSubsystem;
class UTextBlock;
class UUIManagerSubsystem;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTopStatusMenuRequested);

USTRUCT(BlueprintType)
struct FTopStatusBarViewModel
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DateText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText LabelNameText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText CashText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText ReputationText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bTimeRunning = false;
};

/**
 * Premium top HUD status strip for date, label identity, finances, reputation, and time controls.
 */
UCLASS(BlueprintType, Blueprintable)
class MUSICMANAGER_API UTopStatusBarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UTopStatusBarWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category="Top Status")
    void RefreshFromSubsystems();

    UFUNCTION(BlueprintCallable, Category="Top Status")
    void ApplyViewModel(const FTopStatusBarViewModel& ViewModel);

    UPROPERTY(BlueprintAssignable, Category="Top Status")
    FOnTopStatusMenuRequested OnMenuRequested;

protected:
    UPROPERTY(meta=(BindWidget))
    UCanvasPanel* RootCanvas;

    UPROPERTY(meta=(BindWidget))
    UBorder* StatusBarRoot;

    UPROPERTY(meta=(BindWidget))
    UImage* BackgroundImage;

    UPROPERTY(meta=(BindWidget))
    UImage* BrandIconImage;

    UPROPERTY(meta=(BindWidget))
    UImage* DateIconImage;

    UPROPERTY(meta=(BindWidget))
    UImage* LabelIconImage;

    UPROPERTY(meta=(BindWidget))
    UImage* CashIconImage;

    UPROPERTY(meta=(BindWidget))
    UImage* ReputationIconImage;

    UPROPERTY(meta=(BindWidget))
    UTextBlock* BrandText;

    UPROPERTY(meta=(BindWidget))
    UTextBlock* DateText;

    UPROPERTY(meta=(BindWidget))
    UTextBlock* LabelNameText;

    UPROPERTY(meta=(BindWidget))
    UTextBlock* CashText;

    UPROPERTY(meta=(BindWidget))
    UTextBlock* ReputationText;

    UPROPERTY(meta=(BindWidget))
    UButton* PauseButton;

    UPROPERTY(meta=(BindWidget))
    UImage* PauseIconImage;

    UPROPERTY(meta=(BindWidget))
    UButton* PlayButton;

    UPROPERTY(meta=(BindWidget))
    UImage* PlayIconImage;

    UPROPERTY(meta=(BindWidget))
    UButton* FastForwardButton;

    UPROPERTY(meta=(BindWidget))
    UImage* FastForwardIconImage;

    UPROPERTY(meta=(BindWidget))
    UButton* MenuButton;

    UPROPERTY(meta=(BindWidget))
    UImage* MenuIconImage;

private:
    void BindButtonEvents();
    void UnbindButtonEvents();

    UFUNCTION()
    void HandlePauseClicked();

    UFUNCTION()
    void HandlePlayClicked();

    UFUNCTION()
    void HandleFastForwardClicked();

    UFUNCTION()
    void HandleMenuClicked();

    UFUNCTION()
    void HandleWeekAdvanced(const FDateTime& PreviousDate, const FDateTime& NewDate);

    UFUNCTION()
    void HandleMonthAdvanced(const FDateTime& NewDate);

    UFUNCTION()
    void HandleTimeBatchAdvanced(int32 WeeksAdvanced, const FDateTime& NewDate);

    UFUNCTION()
    void HandleCurrentLabelChanged(const FString& NewLabelId);

    UFUNCTION()
    void HandleCommandExecuted(const FMusicCommandResult& Result);

    FTopStatusBarViewModel BuildViewModel() const;
    FString ResolveLabelId() const;

    UGameTimeSubsystem* GetTimeSubsystem() const;
    UFinanceManagerSubsystem* GetFinanceSubsystem() const;
    UPlayerLabelSubsystem* GetPlayerLabelSubsystem() const;
    UUIManagerSubsystem* GetUIManagerSubsystem() const;
    UCommandDispatcherSubsystem* GetCommandDispatcherSubsystem() const;

    static FText FormatMonthYear(const FDateTime& Date);
    static FText FormatCurrency(float Value);
    static FText FormatReputation(float Reputation);

    UPROPERTY()
    TWeakObjectPtr<UGameTimeSubsystem> CachedTimeSubsystem;

    UPROPERTY()
    TWeakObjectPtr<UFinanceManagerSubsystem> CachedFinanceSubsystem;

    UPROPERTY()
    TWeakObjectPtr<UPlayerLabelSubsystem> CachedPlayerLabelSubsystem;

    UPROPERTY()
    TWeakObjectPtr<UUIManagerSubsystem> CachedUIManagerSubsystem;

    UPROPERTY()
    TWeakObjectPtr<UCommandDispatcherSubsystem> CachedCommandDispatcherSubsystem;
};
