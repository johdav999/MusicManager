#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AuditionTypes.h"
#include "SignedArtistPanelWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArtistFromPanelSelected, FString, ArtistId);

UCLASS()
class MUSICMANAGER_API USignedArtistPanelWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(BlueprintAssignable)
    FOnArtistFromPanelSelected OnArtistSelected;

    void PopulateArtistList(const TArray<FArtistData>& SignedArtists);

protected:
    UFUNCTION()
    void HandleArtistItemClicked(FString ArtistId);

    UPROPERTY(meta=(BindWidget))
    class UScrollBox* ArtistScrollBox;

    UPROPERTY(EditAnywhere, Category="UI")
    TSubclassOf<class USignedArtistItemWidget> ItemClass;

    TArray<TWeakObjectPtr<USignedArtistItemWidget>> SpawnedItems;
};
