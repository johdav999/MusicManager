#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AuditionTypes.h"
#include "AuditionWidget.generated.h"

UCLASS(Blueprintable)
class UAuditionWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FAuditionEvent AuditionData;

    UFUNCTION(BlueprintCallable)
    void UpdateNegotiationValues();
};
