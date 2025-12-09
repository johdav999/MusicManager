#include "UI/RegionMapButton.h"

#include "Blueprint/WidgetTree.h"

URegionMapButton::URegionMapButton(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void URegionMapButton::NativeConstruct()
{
    ensure(IsInGameThread());

    Super::NativeConstruct();

    EnsureDefaultWidgets();

    if (!RegionId.IsEmpty() && RegionText)
    {
        RegionText->SetText(FText::FromString(RegionId));
    }
}

void URegionMapButton::InitializeRegion(const FString& InRegionId)
{
    RegionId = InRegionId;

    EnsureDefaultWidgets();

    if (RegionText)
    {
        RegionText->SetText(FText::FromString(RegionId));
    }
}

void URegionMapButton::EnsureDefaultWidgets()
{
    if (!WidgetTree)
    {
        return;
    }

    if (!RegionButton)
    {
        RegionButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RegionButton"));
        WidgetTree->RootWidget = RegionButton;
    }

    if (!RegionText)
    {
        RegionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RegionText"));

        if (RegionButton)
        {
            RegionButton->AddChild(RegionText);
        }
    }
    else if (RegionButton && RegionText->GetParent() != RegionButton)
    {
        RegionButton->AddChild(RegionText);
    }
}
