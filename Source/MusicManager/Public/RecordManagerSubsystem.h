#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RecordManagerSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FRecordData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString RecordId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ArtistId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString AlbumName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsSingle = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsLP = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> SongIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDateTime DateRecorded;
};

UCLASS()
class MUSICMANAGER_API URecordManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION()
    FString CreateRecord(const FRecordData& Data);

    UFUNCTION(BlueprintCallable)
    bool GetRecordById(const FString& RecordId, FRecordData& OutData) const;

private:
    UPROPERTY()
    TMap<FString, FRecordData> Records;
};
