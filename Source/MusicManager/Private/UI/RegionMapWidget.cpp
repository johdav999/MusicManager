#include "UI/RegionMapWidget.h"

#include "Async/Async.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "MarketManagerSubsystem.h"

URegionMapWidget::URegionMapWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void URegionMapWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void URegionMapWidget::NativePreConstruct()
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<URegionMapWidget> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (URegionMapWidget* Strong = WeakThis.Get())
            {
                Strong->RebuildWidgetTree();
            }
        });
        return;
    }

    Super::NativePreConstruct();

    RebuildWidgetTree();
}

void URegionMapWidget::RebuildWidgetTree()
{
    if (!IsInGameThread())
    {
        const TWeakObjectPtr<URegionMapWidget> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]() {
            if (URegionMapWidget* Strong = WeakThis.Get())
            {
                Strong->RebuildWidgetTree();
            }
        });
        return;
    }

    if (WidgetTree && !RootCanvas)
    {
        RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
        WidgetTree->RootWidget = RootCanvas;
    }

    if (WidgetTree && WidgetTree->RootWidget == nullptr && RootCanvas)
    {
        WidgetTree->RootWidget = RootCanvas;
    }

    if (!RootCanvas)
    {
        return;
    }

    RootCanvas->ClearChildren();
    RegionButtons.Empty();

    const bool bIsRuntime = !IsDesignTime();
    TArray<FMarketRegion> Regions;

    if (bIsRuntime)
    {
        if (UGameInstance* GI = GetGameInstance())
        {
            if (UMarketManagerSubsystem* Market = GI->GetSubsystem<UMarketManagerSubsystem>())
            {
                Market->GetAllRegions(Regions);
            }
        }
    }
    else
    {
        const TArray<FString> MockRegionIds = {
            TEXT("Mock_NorthAmerica"),
            TEXT("Mock_Europe"),
            TEXT("Mock_Asia")
        };

        for (const FString& MockId : MockRegionIds)
        {
            FMarketRegion MockRegion;
            MockRegion.RegionId = MockId;
            MockRegion.DisplayName = MockId;
            Regions.Add(MockRegion);
        }
    }

    for (const FMarketRegion& Region : Regions)
    {
        URegionMapButton* NewButton = WidgetTree->ConstructWidget<URegionMapButton>(URegionMapButton::StaticClass());

        if (!NewButton)
        {
            continue;
        }

        NewButton->InitializeRegion(Region.RegionId);

        if (bIsRuntime)
        {
            NewButton->OnRegionClicked.AddDynamic(this, &URegionMapWidget::HandleButtonClicked);
        }

        RegionButtons.Add(Region.RegionId, NewButton);
        RootCanvas->AddChild(NewButton);
    }
}

void URegionMapWidget::HandleButtonClicked(const FString& RegionId)
{
    OnRegionSelected.Broadcast(RegionId);
}

void URegionMapWidget::RefreshRegions()
{
    RebuildWidgetTree();
}
