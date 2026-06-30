#pragma once

#include "CoreMinimal.h"
#include "MusicCommandResult.generated.h"

UENUM(BlueprintType)
enum class EMusicCommandErrorCode : uint8
{
    None,
    InvalidReference,
    InvalidState,
    InsufficientFunds,
    DateConflict,
    SongLocked,
    ArtistNotSigned,
    ValidationFailed,
    SubsystemUnavailable
};

USTRUCT(BlueprintType)
struct FMusicCommandResult
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
    bool bSuccess = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
    EMusicCommandErrorCode ErrorCode = EMusicCommandErrorCode::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
    FText Message;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
    FText RemediationHint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
    TArray<FString> AffectedEntityIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
    FString CreatedEntityId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
    int32 Quantity = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Command")
    FDateTime ResultDate;

    static FMusicCommandResult Success(const FText& InMessage, const TArray<FString>& InAffectedEntityIds = TArray<FString>());
    static FMusicCommandResult Failure(EMusicCommandErrorCode InErrorCode, const FText& InMessage, const FText& InRemediationHint = FText());
};
