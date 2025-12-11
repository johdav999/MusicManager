#include "RegionMapGeneratorTool.h"

#if WITH_EDITOR

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/DataTable.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "WidgetBlueprintEditorUtils.h"
#include "MarketRegion.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UI/RegionMapButton.h"
#include "WidgetBlueprint.h"

void URegionMapGeneratorTool::GenerateRegionButtons()
{
    check(IsInGameThread());

    const FString WidgetPath = TEXT("/Game/GUI/RegionMap/BP_RegionMapWidget.BP_RegionMapWidget");
    UObject* LoadedObject = StaticLoadObject(UWidgetBlueprint::StaticClass(), nullptr, *WidgetPath);
    UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(LoadedObject);

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
    const FString DataTablePath = TEXT("/Game/Data/RegionData.RegionData");
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

        const FString DesiredWidgetName = WidgetName;

        URegionMapButton* NewButton = WidgetTree->ConstructWidget<URegionMapButton>(
            URegionMapButton::StaticClass(),
            FName(*DesiredWidgetName)
        );

        RootCanvas->AddChild(NewButton);

        // Register with Blueprint system
        FWidgetBlueprintEditorUtils::CreateWidgetForBlueprint(WidgetBP, NewButton, NewButton->GetFName());

        const FGuid NewGuid = FGuid::NewGuid();
        WidgetBP->WidgetGuidMap.Add(NewButton->GetFName(), NewGuid);
        WidgetBP->WidgetVariableNameToGuidMap.Add(NewButton->GetFName(), NewGuid);

        NewButton->SetDesignerFlags(EWidgetDesignFlags::Designed);

        if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(NewButton->Slot))
        {
            const int32 Columns = 8;
            const float CellWidth = 100.f;
            const float CellHeight = 80.f;

            const int32 ColumnIndex = Index % Columns;
            const int32 RowIndex = Index / Columns;

            Slot->SetPosition(FVector2D(CellWidth * ColumnIndex, CellHeight * RowIndex));
            Slot->SetSize(FVector2D(150.f, 50.f));
        }

        Added++;
    }

    // Save updated blueprint
    if (Added > 0)
    {
        UPackage* Package = WidgetBP->GetPackage();

        WidgetBP->Modify();
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);
        FKismetEditorUtilities::CompileBlueprint(WidgetBP);
        WidgetBP->MarkPackageDirty();

        FString OutputFilename =
            FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        SaveArgs.Error = GError;
        SaveArgs.bWarnOfLongFilename = false;

        UPackage::SavePackage(
            Package,
            WidgetBP,
            *OutputFilename,
            SaveArgs
        );
    }
}

#endif
