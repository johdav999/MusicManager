#include "MusicCommandResult.h"

FMusicCommandResult FMusicCommandResult::Success(const FText& InMessage, const TArray<FString>& InAffectedEntityIds)
{
    FMusicCommandResult Result;
    Result.bSuccess = true;
    Result.ErrorCode = EMusicCommandErrorCode::None;
    Result.Message = InMessage;
    Result.AffectedEntityIds = InAffectedEntityIds;
    return Result;
}

FMusicCommandResult FMusicCommandResult::Failure(EMusicCommandErrorCode InErrorCode, const FText& InMessage, const FText& InRemediationHint)
{
    FMusicCommandResult Result;
    Result.bSuccess = false;
    Result.ErrorCode = InErrorCode;
    Result.Message = InMessage;
    Result.RemediationHint = InRemediationHint;
    return Result;
}
