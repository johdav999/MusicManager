#include "UI/SignedArtistPanelWidget.h"

#include "Async/Async.h"
#include "Components/ScrollBox.h"
#include "Engine/Texture2D.h"
#include "UObject/SoftObjectPath.h"
#include "UI/SignedArtistItemWidget.h"

void USignedArtistPanelWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void USignedArtistPanelWidget::NativeDestruct()
{
    for (TWeakObjectPtr<USignedArtistItemWidget>& ItemPtr : SpawnedItems)
    {
        if (USignedArtistItemWidget* Item = ItemPtr.Get())
        {
            Item->OnArtistClicked.RemoveDynamic(this, &USignedArtistPanelWidget::HandleArtistItemClicked);
        }
    }
    SpawnedItems.Reset();

    Super::NativeDestruct();
}

void USignedArtistPanelWidget::PopulateArtistList(const TArray<FArtistData>& SignedArtists)
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<USignedArtistPanelWidget> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis, SignedArtists]()
        {
            if (USignedArtistPanelWidget* Strong = WeakThis.Get())
            {
                Strong->PopulateArtistList(SignedArtists);
            }
        });
        return;
    }

    if (!ArtistScrollBox || !ItemClass)
    {
        return;
    }

    ArtistScrollBox->ClearChildren();
    SpawnedItems.Reset();

    UWorld* World = GetWorld();
    if (!World) return;

    const FString PreviousSelection = SelectedArtistId;
    bool bSelectionStillValid = false;

    for (const FArtistData& Data : SignedArtists)
    {
        USignedArtistItemWidget* Item = CreateWidget<USignedArtistItemWidget>(World, ItemClass);
        if (!IsValid(Item)) continue;

        UTexture2D* LoadedTexture = nullptr;

        if (!Data.ImageAssetRef.IsEmpty())
        {
            FSoftObjectPath SoftPath(Data.ImageAssetRef);
            UObject* RawObj = SoftPath.TryLoad();
            LoadedTexture = Cast<UTexture2D>(RawObj);
        }

        Item->SetupItem(Data, LoadedTexture);

        Item->OnArtistClicked.AddDynamic(this, &USignedArtistPanelWidget::HandleArtistItemClicked);

        ArtistScrollBox->AddChild(Item);
        SpawnedItems.Add(Item);

        if (!bSelectionStillValid && PreviousSelection == Data.ArtistName)
        {
            bSelectionStillValid = true;
        }
    }

    if (SignedArtists.Num() > 0)
    {
        if (bSelectionStillValid)
        {
            SelectedArtistId = PreviousSelection;
        }
        else
        {
            SelectedArtistId = SignedArtists.Last().ArtistName;
        }
    }
    else
    {
        SelectedArtistId.Reset();
    }

    UpdateSelectionVisuals();
}

void USignedArtistPanelWidget::HandleArtistItemClicked(FString ArtistId)
{
    if (!IsInGameThread()) return;

    SelectedArtistId = ArtistId;
    UpdateSelectionVisuals();

    OnArtistSelected.Broadcast(ArtistId);
}

void USignedArtistPanelWidget::UpdateSelectionVisuals()
{
    for (TWeakObjectPtr<USignedArtistItemWidget>& ItemPtr : SpawnedItems)
    {
        if (USignedArtistItemWidget* Item = ItemPtr.Get())
        {
            const bool bShouldSelect = Item->GetArtistId() == SelectedArtistId;
            Item->SetSelected(bShouldSelect);
        }
    }
}
