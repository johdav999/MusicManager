#include "UI/RegionMapWidget.h"

void URegionMapWidget::NativeConstruct()
{
    Super::NativeConstruct();

    RegionButtons.Empty();

    BindRegionButtons();
}

void URegionMapWidget::RegisterRegionButton(
    const FString& RegionId,
    URegionMapButton* Button)
{
    if (!Button)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("RegionMapWidget: Missing button for region %s"), *RegionId);
        return;
    }

    Button->InitializeRegion(RegionId);
    Button->OnRegionClicked.RemoveDynamic(this, &URegionMapWidget::HandleButtonClicked);
    Button->OnRegionClicked.AddDynamic(this, &URegionMapWidget::HandleButtonClicked);
    RegionButtons.Add(RegionId, Button);
}

void URegionMapWidget::BindRegionButtons()
{
    RegisterRegionButton(TEXT("AL"), RegionMapButton_AL);
    RegisterRegionButton(TEXT("AK"), RegionMapButton_AK);
    RegisterRegionButton(TEXT("AZ"), RegionMapButton_AZ);
    RegisterRegionButton(TEXT("AR"), RegionMapButton_AR);
    RegisterRegionButton(TEXT("CA"), RegionMapButton_CA);
    RegisterRegionButton(TEXT("CO"), RegionMapButton_CO);
    RegisterRegionButton(TEXT("CT"), RegionMapButton_CT);
    RegisterRegionButton(TEXT("DE"), RegionMapButton_DE);
    RegisterRegionButton(TEXT("FL"), RegionMapButton_FL);
}

void URegionMapWidget::HandleButtonClicked(const FString& RegionId)
{
    OnRegionSelected.Broadcast(RegionId);
}

void URegionMapWidget::RefreshRegions()
{
    RegionButtons.Empty();
    BindRegionButtons();
}
