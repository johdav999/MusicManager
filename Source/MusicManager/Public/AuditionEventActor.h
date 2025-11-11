#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AuditionTypes.h"
#include "AuditionWidget.h"
#include "Delegates/DelegateCombinations.h"
#include "AuditionEventActor.generated.h"

UCLASS()
class AAuditionEventActor : public AActor
{
    GENERATED_BODY()

public:
    AAuditionEventActor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audition")
    FAuditionEvent AuditionData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audition")
    TSubclassOf<UAuditionWidget> WidgetClass;

    UPROPERTY(BlueprintAssignable, Category="Audition")
    FSimpleMulticastDelegate OnNegotiationUpdated;

    UFUNCTION(BlueprintCallable, Category="Audition")
    void StartAudition();

    UFUNCTION(BlueprintCallable, Category="Audition")
    void FinalizeDeal(bool bAcceptDeal);

private:
    UPROPERTY()
    UAuditionWidget* ActiveWidget;
};
