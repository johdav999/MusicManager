#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AuditionTypes.h"
#include "SignedArtistItemWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSignedArtistClicked, FString, ArtistId);

class UTexture2D;

UCLASS()
class MUSICMANAGER_API USignedArtistItemWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    void SetupItem(const FArtistData& InData, UTexture2D* PortraitTexture);

    void SetHovered(bool bHovered);

    void SetSelected(bool bSelected);

    FString GetArtistId() const { return LocalArtistData.ArtistName; }

    void UpdateVisualState();

    UPROPERTY(BlueprintAssignable, Category="Events")
    FOnSignedArtistClicked OnArtistClicked;

private:
    FArtistData LocalArtistData;

protected:
    UFUNCTION()
    void HandleClicked();

    UFUNCTION()
    void HandleHovered();

    UFUNCTION()
    void HandleUnhovered();

    UPROPERTY(meta=(BindWidget))
    class UButton* ItemButton;

    UPROPERTY(meta=(BindWidgetOptional))
    class UBorder* BackgroundBorder;

    UPROPERTY(meta=(BindWidget))
    class UImage* PortraitImage;

    UPROPERTY(meta=(BindWidget))
    class UTextBlock* ArtistNameText;

    UPROPERTY(meta=(BindWidget))
    class UTextBlock* ArtistGenreText;

    UPROPERTY(EditDefaultsOnly, Category="Appearance")
    FLinearColor NormalColor = FLinearColor::White;

    UPROPERTY(EditDefaultsOnly, Category="Appearance")
    FLinearColor HoveredColor = FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category="Appearance")
    FLinearColor SelectedColor = FLinearColor(0.75f, 0.85f, 1.0f, 1.0f);

    bool bIsHovered = false;
    bool bIsSelected = false;
};
