#include "UI/RegionMapGeneratorTool.h"

#if WITH_EDITOR
#include "Blueprint/WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/DataTable.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/PackageName.h"
#include "UI/RegionMapButton.h"
#include "MarketRegion.h"
#include "UObject/Package.h"

void URegionMapGeneratorTool::GenerateRegionButtons()
{
    const FString WidgetPath = TEXT("/Game/UI/RegionMap/BP_RegionMapWidget.BP_RegionMapWidget");
    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(StaticLoadObject(UWidgetBlueprint::StaticClass(), nullptr, *WidgetPath));
    if (!WidgetBP)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load Region Map Widget Blueprint at %s"), *WidgetPath);
        return;
    }

    UWidgetTree* WidgetTree = WidgetBP->WidgetTree;
    if (!WidgetTree)
    {
        UE_LOG(LogTemp, Error, TEXT("Widget tree missing on %s"), *WidgetBP->GetName());
        return;
    }

    UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
    if (!RootCanvas)
    {
        UE_LOG(LogTemp, Error, TEXT("Root widget is not a CanvasPanel for %s"), *WidgetBP->GetName());
        return;
    }

    const FString DataTablePath = TEXT("/Game/Data/DT_Regions.DT_Regions");
    UDataTable* RegionTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *DataTablePath));
    if (!RegionTable)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load Region DataTable at %s"), *DataTablePath);
        return;
    }

    TArray<FMarketRegion*> Regions;
    RegionTable->GetAllRows<FMarketRegion>(TEXT("RegionMapGenerator"), Regions);

    int32 NewButtonCount = 0;
    for (int32 Index = 0; Index < Regions.Num(); ++Index)
    {
        const FMarketRegion* Region = Regions[Index];
        if (!Region)
        {
            continue;
        }

        const FString DesiredWidgetName = FString::Printf(TEXT("RegionMapButton_%s"), *Region->RegionId);
        if (WidgetTree->FindWidget(FName(*DesiredWidgetName)))
        {
            continue;
        }

        URegionMapButton* NewButton = WidgetTree->ConstructWidget<URegionMapButton>(URegionMapButton::StaticClass(), FName(*DesiredWidgetName));
        if (!NewButton)
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to construct region map button for %s"), *Region->RegionId);
            continue;
        }

        RootCanvas->AddChild(NewButton);

        if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(NewButton->Slot))
        {
            Slot->SetPosition(FVector2D(50.f * Index, 50.f));
            Slot->SetSize(FVector2D(120.f, 40.f));
        }

        ++NewButtonCount;
    }

    WidgetBP->Modify();
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);
    WidgetBP->MarkPackageDirty();

    if (UPackage* Package = WidgetBP->GetPackage())
    {
        const FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        UPackage::SavePackage(Package, WidgetBP, EObjectFlags::RF_Standalone | EObjectFlags::RF_Public, *PackageFileName);
    }

    UE_LOG(LogTemp, Log, TEXT("Region Map generation complete. Added %d new buttons."), NewButtonCount);
}

#endif // WITH_EDITOR

