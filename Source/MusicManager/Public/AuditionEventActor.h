#pragma once

#include "AuditionTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AuditionEventActor.generated.h"

class UArtistManagerSubsystem;
class USongManagerSubsystem;
class UMusicPlayerComponent;
class USong;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNegotiationUpdated);

UCLASS()
class AAuditionEventActor : public AActor
{
    GENERATED_BODY()

public:
    AAuditionEventActor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audition")
    FAuditionEvent AuditionData;

    UPROPERTY(BlueprintAssignable, Category = "Audition")
    FOnNegotiationUpdated OnNegotiationUpdated;

    UFUNCTION(BlueprintCallable, Category = "Audition")
    void StartAudition();

    UFUNCTION(BlueprintCallable, Category = "Audition")
    void FinalizeDeal(bool bAcceptDeal);

protected:
    virtual void BeginPlay() override;

    void BeginPerformanceScoring(const FArtistData& Artist, USong* Song);
    void FinalizePerformance();
    void FinalizePerformanceResults();

private:
    UPROPERTY()
    TObjectPtr<UMusicPlayerComponent> MusicPlayer;

    FString CurrentArtistId;
    TWeakObjectPtr<USong> CurrentSong;

    UFUNCTION()
    void HandlePerformanceFinished();
};
