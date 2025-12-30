#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AuditionTypes.h"
#include "ArtistActionAvailability.h"
#include "TimerManager.h"
#include "SignedArtistItemWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSignedArtistClicked, FString, ArtistId);

class UTexture2D;
class UMaterialInstanceDynamic;
class UArtistActionIconSet;

UCLASS()
class MUSICMANAGER_API USignedArtistItemWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /** Layer-1 widget: never spawns widgets, never calculates business rules, never displays text-heavy info. */
    void SetupItem(const FArtistData& InData, UTexture2D* PortraitTexture);

    void SetHovered(bool bHovered);

    void SetSelected(bool bSelected);

    FString GetArtistId() const { return LocalArtistData.ArtistName; }

    void UpdateVisualState();

    EArtistVisualState DetermineVisualState() const;

    UPROPERTY(BlueprintAssignable, Category="Events")
    FOnSignedArtistClicked OnArtistClicked;

private:
    void ApplySetupData();

    FArtistData LocalArtistData;
    FArtistData PendingArtistData;

    UTexture2D* PendingPortraitTexture = nullptr;

    bool bIsInitialized = false;
    bool bHasPendingSetupData = false;

protected:
    UFUNCTION()
    void HandleClicked();

    UFUNCTION()
    void HandleHovered();

    UFUNCTION()
    void HandleUnhovered();

    void HandleActionAvailabilityChanged(const FString& ArtistId, EArtistActionAvailability NewAvailability);
    void ApplyActionAvailability(EArtistActionAvailability NewAvailability, bool bTriggerAttention);
    void RefreshActionAvailabilityFromSubsystem(bool bTriggerAttention);
    const UTexture2D* ResolveActionIcon(EArtistActionAvailability Availability) const;
    void TriggerAttentionBoost();
    void ResetAttentionBoost();

    UPROPERTY(meta=(BindWidget))
    class UButton* ItemButton;

    UPROPERTY(meta=(BindWidget))
    class UImage* PortraitImage;

    UPROPERTY(meta=(BindWidget))
    class UImage* FrameImage;

    UPROPERTY(meta=(BindWidget))
    class UImage* ActionIconImage;

    UPROPERTY(Transient)
    UMaterialInstanceDynamic* FrameMID = nullptr;

    UPROPERTY(EditDefaultsOnly, Category="Appearance")
    UArtistActionIconSet* ActionIconSet = nullptr;

    UPROPERTY(EditDefaultsOnly, Category="Appearance")
    float AttentionBoostValue = 1.3f;

    UPROPERTY(EditDefaultsOnly, Category="Appearance")
    float AttentionBoostResetDelay = 2.7f;

    UPROPERTY(EditDefaultsOnly, Category="Appearance")
    FLinearColor RisingStateColor = FLinearColor(0.55f, 0.85f, 1.0f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category="Appearance")
    FLinearColor StableStateColor = FLinearColor(0.75f, 0.9f, 0.7f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category="Appearance")
    FLinearColor DecliningStateColor = FLinearColor(1.0f, 0.7f, 0.55f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category="Appearance")
    FLinearColor IdleStateColor = FLinearColor(0.6f, 0.6f, 0.65f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category="Appearance")
    float BaseRimIntensity = 0.25f;

    UPROPERTY(EditDefaultsOnly, Category="Appearance")
    float HoverRimBoost = 0.35f;

    UPROPERTY(EditDefaultsOnly, Category="Appearance")
    float SelectedRimBoost = 0.6f;

    FLinearColor CachedStateColor = FLinearColor::Transparent;
    float CachedRimIntensity = -1.0f;
    EArtistActionAvailability CachedAvailability = EArtistActionAvailability::None;

    FTimerHandle AttentionBoostTimerHandle;
    FDelegateHandle ActionAvailabilityHandle;

    bool bIsHovered = false;
    bool bIsSelected = false;
};
