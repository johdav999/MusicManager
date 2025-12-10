#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RegionMapGeneratorTool.generated.h"

/**
 * Editor-only utility that incrementally generates region map buttons without
 * overwriting existing widgets.
 */
UCLASS()
class MUSICMANAGER_API URegionMapGeneratorTool : public UObject
{
    GENERATED_BODY()

public:
#if WITH_EDITOR
    UFUNCTION(CallInEditor, Category = "RegionMap")
    void GenerateRegionButtons();
#endif
};

