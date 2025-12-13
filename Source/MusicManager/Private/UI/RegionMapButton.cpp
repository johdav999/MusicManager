#include "UI/RegionMapButton.h"

void URegionMapButton::NativeConstruct()
{
    Super::NativeConstruct();

    if (RegionButton)
    {
        RegionButton->OnClicked.Clear();
        RegionButton->OnClicked.AddDynamic(this, &URegionMapButton::HandleClicked);
    }

    if (!RegionId.IsEmpty() && RegionText)
    {
        RegionText->SetText(FText::FromString(RegionId));
    }
}

void URegionMapButton::InitializeRegion(const FString& InRegionId)
{
    RegionId = InRegionId;

    if (RegionText)
    {
        RegionText->SetText(FText::FromString(RegionId));
    }
}

void URegionMapButton::HandleClicked()
{
    OnRegionClicked.Broadcast(RegionId);
}
