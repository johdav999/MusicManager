#include "RegionMapGeneratorTool.h"

#if WITH_EDITOR

#include "Blueprint/WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Misc/PackageName.h"
#include "Engine/DataTable.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"

// Include runtime references from game module
#include "UI/RegionMapButton.h"
#include "MarketRegion.h"

void URegionMapGeneratorTool::GenerateRegionButtons()
{
    const FString WidgetPath = TEXT("/Game/UI/RegionMap/BP_RegionMapWidget.BP_RegionMapWidget");
    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(
        StaticLoadObject(UWidgetBlueprint::StaticClass(), nullptr, *WidgetPath));

    if (!WidgetBP)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load RegionMapWidget at %s"), *WidgetPath);
        return;
    }

    UWidgetTree* WidgetTree = WidgetBP->WidgetTree;
    if (!WidgetTree)
    {
        UE_LOG(LogTemp, Error, TEXT("No WidgetTree in blueprint."));
        return;
    }

    UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
    if (!RootCanvas)
    {
        UE_LOG(LogTemp, Error, TEXT("Root widget is not a CanvasPanel."));
        return;
    }

    // Load regions from DataTable
    const FString DataTablePath = TEXT("/Game/Data/DT_Regions.DT_Regions");
    UDataTable* RegionTable = Cast<UDataTable>(
        StaticLoadObject(UDataTable::StaticClass(), nullptr, *DataTablePath));

    if (!RegionTable)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load Region DataTable at %s"), *DataTablePath);
        return;
    }

    TArray<FMarketRegion*> Regions;
    RegionTable->GetAllRows(TEXT("RegionGen"), Regions);

    int32 Added = 0;

    for (int32 Index = 0; Index < Regions.Num(); Index++)
    {
        const FMarketRegion* Region = Regions[Index];
        if (!Region) continue;

        FString WidgetName = FString::Printf(TEXT("RegionMapButton_%s"), *Region->RegionId);

        // Skip if existing
        if (WidgetTree->FindWidget(FName(*WidgetName)))
        {
            continue;
        }

        // Create and attach button
        URegionMapButton* Button = WidgetTree->ConstructWidget<URegionMapButton>(
            URegionMapButton::StaticClass(), FName(*WidgetName));

        RootCanvas->AddChild(Button);

        if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Button->Slot))
        {
            Slot->SetPosition(FVector2D(50.f * Index, 50.f));
            Slot->SetSize(FVector2D(150.f, 50.f));
        }

        Added++;
    }

    // Save updated blueprint
    WidgetBP->Modify();
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);
    WidgetBP->MarkPackageDirty();

    UPackage* Pkg = WidgetBP->GetPackage();
    FString OutPath = FPackageName::LongPackageNameToFilename(
        Pkg->GetName(), FPackageName::GetAssetPackageExtension());

    UPackage::SavePackage(Pkg, WidgetBP, RF_Public | RF_Standalone, *OutPath);

    UE_LOG(LogTemp, Log, TEXT("RegionMapGenerator: Added %d new region buttons."), Added);
}

#endif
