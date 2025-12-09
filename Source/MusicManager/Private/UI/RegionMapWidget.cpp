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

    Super::NativeConstruct();

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

    if (!RootCanvas && WidgetTree)
    {
        RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
        WidgetTree->RootWidget = RootCanvas;
    }

    if (!RootCanvas)
    {
        return;
    }

    RootCanvas->ClearChildren();
    RegionButtons.Empty();

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UMarketManagerSubsystem* Market = GI->GetSubsystem<UMarketManagerSubsystem>())
        {
            TArray<FMarketRegion> Regions;
            Market->GetAllRegions(Regions);

            for (const FMarketRegion& Region : Regions)
            {
                URegionMapButton* NewButton = WidgetTree->ConstructWidget<URegionMapButton>(URegionMapButton::StaticClass());

                if (!NewButton)
                {
                    continue;
                }

                NewButton->InitializeRegion(Region.RegionId);

                RegionButtons.Add(Region.RegionId, NewButton);
                RootCanvas->AddChild(NewButton);
            }
        }
    }
}
