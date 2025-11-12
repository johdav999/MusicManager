#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AuditionTypes.h"
#include "AuditionWidget.h"
#include "AuditionEventActor.generated.h"

// Declare a dynamic multicast delegate that Blueprints can bind to
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNegotiationUpdated);

UCLASS()
class AAuditionEventActor : public AActor
{
    GENERATED_BODY()

public:
    AAuditionEventActor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audition")
    FAuditionEvent AuditionData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audition")
    TSubclassOf<UAuditionWidget> WidgetClass;

    // Blueprint event version of the delegate
    UPROPERTY(BlueprintAssignable, Category = "Audition")
    FOnNegotiationUpdated OnNegotiationUpdated;

    UFUNCTION(BlueprintCallable, Category = "Audition")
    void StartAudition();

    UFUNCTION(BlueprintCallable, Category = "Audition")
    void FinalizeDeal(bool bAcceptDeal);

private:
    UPROPERTY()
    UAuditionWidget* ActiveWidget;

    UFUNCTION()
    void HandleSignArtist();

    UFUNCTION()
    void HandlePassOnArtist();
};
