#include "UI/SignedArtistPanelWidget.h"

#include "Async/Async.h"
#include "Components/ScrollBox.h"
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

    for (const FArtistData& Data : SignedArtists)
    {
        USignedArtistItemWidget* Item = CreateWidget<USignedArtistItemWidget>(World, ItemClass);
        if (!IsValid(Item)) continue;

        Item->SetupItem(Data, nullptr);

        Item->OnArtistClicked.AddDynamic(this, &USignedArtistPanelWidget::HandleArtistItemClicked);

        ArtistScrollBox->AddChild(Item);
        SpawnedItems.Add(Item);
    }
}

void USignedArtistPanelWidget::HandleArtistItemClicked(FString ArtistId)
{
    if (!IsInGameThread()) return;

    OnArtistSelected.Broadcast(ArtistId);
}
