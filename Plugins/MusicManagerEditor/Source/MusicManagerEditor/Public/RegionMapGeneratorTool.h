#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RegionMapGeneratorTool.generated.h"

UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class MUSICMANAGEREDITOR_API URegionMapGeneratorTool : public UObject
{
    GENERATED_BODY()

public:

    UFUNCTION(CallInEditor, Category="Region Map")
    void GenerateRegionButtons();
};
