#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AuditionTypes.h"
#include "SignedArtistItemWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSignedArtistClicked, FString, ArtistId);

UCLASS()
class MUSICMANAGER_API USignedArtistItemWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    void SetupItem(const FArtistData& InData, UTexture2D* PortraitTexture);

    UPROPERTY(BlueprintAssignable, Category="Events")
    FOnSignedArtistClicked OnArtistClicked;

private:
    FArtistData LocalArtistData;

protected:
    UFUNCTION()
    void HandleClicked();

    UPROPERTY(meta=(BindWidget))
    class UButton* ItemButton;

    UPROPERTY(meta=(BindWidget))
    class UImage* PortraitImage;

    UPROPERTY(meta=(BindWidget))
    class UTextBlock* ArtistNameText;

    UPROPERTY(meta=(BindWidget))
    class UTextBlock* ArtistGenreText;
};
