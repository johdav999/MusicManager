#include "RegionMapGeneratorTool.h"

#if WITH_EDITOR

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


#endif
void URegionMapGeneratorTool::GenerateRegionButtons()
{
    check(IsInGameThread());

    const FString WidgetPath =
        TEXT("/Game/GUI/RegionMap/BP_RegionMapWidget.BP_RegionMapWidget");

    UWidgetBlueprint* WidgetBP =
        LoadObject<UWidgetBlueprint>(nullptr, *WidgetPath);

    if (!WidgetBP || !WidgetBP->WidgetTree)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid WidgetBlueprint"));
        return;
    }

    UWidgetTree* WidgetTree = WidgetBP->WidgetTree;
    WidgetTree->Modify();

    UCanvasPanel* RootCanvas =
        Cast<UCanvasPanel>(WidgetTree->RootWidget);

    if (!RootCanvas)
    {
        UE_LOG(LogTemp, Error, TEXT("Root widget is not CanvasPanel"));
        return;
    }

    // Load region data
    const FString DataTablePath =
        TEXT("/Game/Data/RegionData.RegionData");

    UDataTable* RegionTable =
        LoadObject<UDataTable>(nullptr, *DataTablePath);

    if (!RegionTable)
    {
        UE_LOG(LogTemp, Error, TEXT("Region DataTable not found"));
        return;
    }

    TArray<FMarketRegion*> Regions;
    RegionTable->GetAllRows(TEXT("RegionMapGen"), Regions);

    int32 AddedCount = 0;

    for (int32 Index = 0; Index < Regions.Num(); ++Index)
    {
        const FMarketRegion* Region = Regions[Index];
        if (!Region) continue;

        const FString WidgetName =
            FString::Printf(TEXT("RegionMapButton_%s"), *Region->RegionId);

        if (WidgetTree->FindWidget(FName(*WidgetName)))
        {
            continue;
        }

        // 1️⃣ Construct widget instance
        URegionMapButton* NewButton =
            WidgetTree->ConstructWidget<URegionMapButton>(
                URegionMapButton::StaticClass(),
                FName(*WidgetName));

        if (!NewButton)
        {
            continue;
        }

        NewButton->Modify();
        NewButton->InitializeRegion(Region->RegionId);

        // 2️⃣ Register widget with UMG designer (THIS CREATES THE GUID)
        FWidgetBlueprintEditorUtils::AddWidget(
            WidgetBP,
            NewButton,
            RootCanvas
        );

        // 3️⃣ Layout
        if (UCanvasPanelSlot* Slot =
            Cast<UCanvasPanelSlot>(NewButton->Slot))
        {
            const int32 Columns = 8;
            const float CellWidth = 100.f;
            const float CellHeight = 80.f;

            const int32 ColumnIndex = Index % Columns;
            const int32 RowIndex = Index / Columns;

            Slot->SetPosition(
                FVector2D(CellWidth * ColumnIndex,
                    CellHeight * RowIndex));
            Slot->SetSize(FVector2D(150.f, 50.f));
        }

        ++AddedCount;
    }

    if (AddedCount == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("No new region buttons added"));
        return;
    }

    // Finalize Blueprint
    WidgetBP->Modify();
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBP);
    FKismetEditorUtilities::CompileBlueprint(WidgetBP);
    WidgetBP->MarkPackageDirty();

    // Save
    UPackage* Package = WidgetBP->GetPackage();
    const FString Filename =
        FPackageName::LongPackageNameToFilename(
            Package->GetName(),
            FPackageName::GetAssetPackageExtension());

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.Error = GError;

    UPackage::SavePackage(
        Package,
        WidgetBP,
        *Filename,
        SaveArgs
    );

    UE_LOG(LogTemp, Log,
        TEXT("RegionMapGenerator: added %d buttons"),
        AddedCount);
}
