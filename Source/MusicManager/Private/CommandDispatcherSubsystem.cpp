#include "CommandDispatcherSubsystem.h"

#include "ArtistManagerSubsystem.h"
#include "EventSubsystem.h"
#include "FinanceManagerSubsystem.h"
#include "GameTimeSubsystem.h"
#include "MarketingManagerSubsystem.h"
#include "PlayerLabelSubsystem.h"
#include "RecordManagerSubsystem.h"
#include "Song.h"
#include "SongManagerSubsystem.h"

DEFINE_LOG_CATEGORY(LogMusicCommands);

namespace
{
    FMusicNewsEvent BuildArtistSignedNewsEvent(
        const FSignArtistCommand& Command,
        const FArtistContract& Contract,
        const FString& LabelDisplayName,
        const FDateTime& Timestamp)
    {
        const FString ArtistName = Contract.ArtistData.ArtistName.IsEmpty() ? Contract.ArtistId : Contract.ArtistData.ArtistName;
        const FString Genre = Contract.ArtistData.Genre.IsEmpty() ? TEXT("Music") : Contract.ArtistData.Genre;
        const FString LabelName = LabelDisplayName.IsEmpty() ? TEXT("Player Label") : LabelDisplayName;

        FMusicNewsEvent Event;
        Event.NewsId = FGuid::NewGuid();
        Event.Timestamp = Timestamp;
        Event.NewsType = EMusicNewsType::ArtistSigned;
        Event.SourceName = LabelName;
        Event.SubjectName = ArtistName;
        Event.Headline = FString::Printf(TEXT("%s signs %s"), *LabelName, *ArtistName);
        Event.BodyText = FString::Printf(
            TEXT("%s has signed %s to a %d-year %s deal covering %d record(s), with a %.0f%% royalty and a $%.0f signing bonus."),
            *LabelName,
            *ArtistName,
            Command.ContractYears,
            Command.bExclusive ? TEXT("exclusive") : TEXT("non-exclusive"),
            Command.RecordCommitment,
            Command.RoyaltyRate * 100.f,
            Command.SignUpBonus);
        Event.Tags = { TEXT("Artist"), TEXT("Contract"), TEXT("Signing"), Genre };
        Event.Metadata.Add(TEXT("ArtistId"), Contract.ArtistId);
        Event.Metadata.Add(TEXT("ArtistName"), ArtistName);
        Event.Metadata.Add(TEXT("Genre"), Genre);
        Event.Metadata.Add(TEXT("LabelId"), Command.LabelId);
        Event.Metadata.Add(TEXT("LabelName"), LabelName);
        Event.Metadata.Add(TEXT("ContractYears"), FString::FromInt(Command.ContractYears));
        Event.Metadata.Add(TEXT("RecordCommitment"), FString::FromInt(Command.RecordCommitment));
        Event.Metadata.Add(TEXT("RoyaltyRate"), FString::SanitizeFloat(Command.RoyaltyRate));
        Event.Metadata.Add(TEXT("SignUpBonus"), FString::SanitizeFloat(Command.SignUpBonus));
        return Event;
    }

    FMusicNewsEvent BuildRecordingStartedNewsEvent(
        const FStartRecordingCommand& Command,
        const FRecordingProjection& Projection,
        const FString& ArtistDisplayName,
        const FString& RecordTitle)
    {
        const FString DisplayTitle = RecordTitle.IsEmpty() ? TEXT("Untitled Record") : RecordTitle;
        const TCHAR* TypeName = Projection.RecordType == ERecordType::Single ? TEXT("Single")
            : Projection.RecordType == ERecordType::EP ? TEXT("EP")
            : TEXT("LP");

        FMusicNewsEvent Event;
        Event.NewsId = FGuid::NewGuid();
        Event.Timestamp = Projection.StartDate;
        Event.NewsType = EMusicNewsType::RecordingSession;
        Event.SourceName = ArtistDisplayName.IsEmpty() ? Command.ArtistId : ArtistDisplayName;
        Event.SubjectName = DisplayTitle;
        Event.Headline = FString::Printf(TEXT("%s enters the studio"), *Event.SourceName);
        Event.BodyText = FString::Printf(
            TEXT("%s started recording %s, a %s with %d track(s). Estimated cost is $%.0f and completion is expected on %s."),
            *Event.SourceName,
            *DisplayTitle,
            TypeName,
            Projection.SongCount,
            Projection.EstimatedRecordingCost,
            *Projection.EstimatedCompletionDate.ToString(TEXT("%Y-%m-%d")));
        Event.Tags = { TEXT("Recording"), TEXT("Studio"), TypeName };
        Event.Metadata.Add(TEXT("ArtistId"), Command.ArtistId);
        Event.Metadata.Add(TEXT("RecordTitle"), DisplayTitle);
        Event.Metadata.Add(TEXT("RecordType"), TypeName);
        Event.Metadata.Add(TEXT("SongCount"), FString::FromInt(Projection.SongCount));
        Event.Metadata.Add(TEXT("RecordingCost"), FString::SanitizeFloat(Projection.EstimatedRecordingCost));
        Event.Metadata.Add(TEXT("CompletionDate"), Projection.EstimatedCompletionDate.ToString(TEXT("%Y-%m-%d")));
        return Event;
    }
}

void UCommandDispatcherSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogMusicCommands, Log, TEXT("Command dispatcher initialized."));
}

FMusicCommandResult UCommandDispatcherSubsystem::ExecuteSignArtist(const FSignArtistCommand& Command)
{
    FMusicCommandResult FailureResult;
    if (!RequireGameInstance(FailureResult))
    {
        return FinishCommand(EMusicCommandType::SignArtist, FailureResult);
    }

    UArtistManagerSubsystem* ArtistSubsystem = GetGameInstance()->GetSubsystem<UArtistManagerSubsystem>();
    UGameTimeSubsystem* TimeSubsystem = GetGameInstance()->GetSubsystem<UGameTimeSubsystem>();
    if (!ArtistSubsystem || !TimeSubsystem)
    {
        return FinishCommand(EMusicCommandType::SignArtist, FMusicCommandResult::Failure(
            EMusicCommandErrorCode::SubsystemUnavailable,
            FText::FromString(TEXT("Artist signing is unavailable because required systems are missing."))));
    }

    const FString PlayerLabelId = GetPlayerLabelId();
    if (Command.LabelId.IsEmpty() || Command.LabelId != PlayerLabelId)
    {
        return FinishCommand(EMusicCommandType::SignArtist, FMusicCommandResult::Failure(
            EMusicCommandErrorCode::InvalidReference,
            FText::FromString(TEXT("Artist can only be signed to the player label."))));
    }

    if (Command.ArtistId.IsEmpty())
    {
        return FinishCommand(EMusicCommandType::SignArtist, FMusicCommandResult::Failure(
            EMusicCommandErrorCode::InvalidReference,
            FText::FromString(TEXT("Select an artist before signing."))));
    }

    if (Command.ContractYears < 1 || Command.ContractYears > 10
        || Command.RecordCommitment < 1 || Command.RecordCommitment > 20
        || Command.RoyaltyRate < 0.01f || Command.RoyaltyRate > 0.5f || !FMath::IsFinite(Command.RoyaltyRate)
        || Command.SignUpBonus < 0.f || !FMath::IsFinite(Command.SignUpBonus))
    {
        return FinishCommand(EMusicCommandType::SignArtist, FMusicCommandResult::Failure(
            EMusicCommandErrorCode::ValidationFailed,
            FText::FromString(TEXT("Contract terms are outside allowed ranges."))));
    }

    TArray<FArtistData> UnsignedArtists;
    ArtistSubsystem->GetUnsignedArtists(UnsignedArtists);
    const bool bArtistIsUnsigned = UnsignedArtists.ContainsByPredicate([&Command](const FArtistData& Artist)
    {
        return Artist.ArtistId == Command.ArtistId || Artist.ArtistName == Command.ArtistId;
    });

    if (!bArtistIsUnsigned)
    {
        return FinishCommand(EMusicCommandType::SignArtist, FMusicCommandResult::Failure(
            EMusicCommandErrorCode::InvalidState,
            FText::FromString(TEXT("Artist is not available to sign."))));
    }

    FArtistDealTerms Deal;
    Deal.ArtistId = Command.ArtistId;
    Deal.ContractYears = Command.ContractYears;
    Deal.NumRecords = Command.RecordCommitment;
    Deal.RoyaltyRate = Command.RoyaltyRate;
    Deal.SignUpBonus = Command.SignUpBonus;
    Deal.bExclusive = Command.bExclusive;
    Deal.ProposedStartDate = TimeSubsystem->GetCurrentGameDate();

    FArtistContract NewContract;
    FString Error;
    if (!ArtistSubsystem->SignArtistById(Command.ArtistId, Deal, NewContract, Error))
    {
        return FinishCommand(EMusicCommandType::SignArtist, FMusicCommandResult::Failure(
            EMusicCommandErrorCode::ValidationFailed,
            FText::FromString(Error.IsEmpty() ? TEXT("Artist could not be signed.") : Error)));
    }

    if (UEventSubsystem* EventSubsystem = GetGameInstance()->GetSubsystem<UEventSubsystem>())
    {
        FString LabelDisplayName = TEXT("Player Label");
        if (const UPlayerLabelSubsystem* LabelSubsystem = GetGameInstance()->GetSubsystem<UPlayerLabelSubsystem>())
        {
            LabelDisplayName = LabelSubsystem->GetPlayerLabelName();
        }

        const FMusicNewsEvent SignedEvent = BuildArtistSignedNewsEvent(
            Command,
            NewContract,
            LabelDisplayName,
            TimeSubsystem->GetCurrentGameDate());
        const FString DeduplicationKey = FString::Printf(
            TEXT("ArtistSigned:%s:%s"),
            *NewContract.ArtistId,
            *NewContract.StartDate.ToString(TEXT("%Y%m%d")));
        EventSubsystem->PublishNewsEvent(SignedEvent, DeduplicationKey);
    }
    else
    {
        UE_LOG(LogMusicCommands, Warning, TEXT("Artist signed but EventSubsystem is unavailable; signed-artist news was not published. ArtistId='%s'."),
            *NewContract.ArtistId);
    }

    FMusicCommandResult Result = FMusicCommandResult::Success(
        FText::FromString(TEXT("Artist signed.")),
        { NewContract.ArtistId });
    Result.CreatedEntityId = NewContract.ArtistId;
    return FinishCommand(EMusicCommandType::SignArtist, Result);
}

FMusicCommandResult UCommandDispatcherSubsystem::ExecuteRejectArtist(const FRejectArtistCommand& Command)
{
    FMusicCommandResult FailureResult;
    if (!RequireGameInstance(FailureResult))
    {
        return FinishCommand(EMusicCommandType::RejectArtist, FailureResult);
    }

    UArtistManagerSubsystem* ArtistSubsystem = GetGameInstance()->GetSubsystem<UArtistManagerSubsystem>();
    if (!ArtistSubsystem)
    {
        return FinishCommand(EMusicCommandType::RejectArtist, FMusicCommandResult::Failure(
            EMusicCommandErrorCode::SubsystemUnavailable,
            FText::FromString(TEXT("Artist rejection is unavailable because the artist system is missing."))));
    }

    if (Command.ArtistId.IsEmpty())
    {
        return FinishCommand(EMusicCommandType::RejectArtist, FMusicCommandResult::Failure(
            EMusicCommandErrorCode::InvalidReference,
            FText::FromString(TEXT("Select an artist before passing."))));
    }

    if (ArtistSubsystem->GetContractByArtistId(Command.ArtistId))
    {
        return FinishCommand(EMusicCommandType::RejectArtist, FMusicCommandResult::Failure(
            EMusicCommandErrorCode::InvalidState,
            FText::FromString(TEXT("Signed artists cannot be rejected from auditions."))));
    }

    TArray<FArtistData> UnsignedArtists;
    ArtistSubsystem->GetUnsignedArtists(UnsignedArtists);
    const bool bArtistIsUnsigned = UnsignedArtists.ContainsByPredicate([&Command](const FArtistData& Artist)
    {
        return Artist.ArtistId == Command.ArtistId || Artist.ArtistName == Command.ArtistId;
    });

    if (!bArtistIsUnsigned)
    {
        return FinishCommand(EMusicCommandType::RejectArtist, FMusicCommandResult::Failure(
            EMusicCommandErrorCode::InvalidReference,
            FText::FromString(TEXT("Artist is not in the unsigned audition pool."))));
    }

    FString Error;
    if (!ArtistSubsystem->RejectArtistById(Command.ArtistId, Error))
    {
        return FinishCommand(EMusicCommandType::RejectArtist, FMusicCommandResult::Failure(
            EMusicCommandErrorCode::ValidationFailed,
            FText::FromString(Error.IsEmpty() ? TEXT("Artist could not be passed.") : Error)));
    }

    return FinishCommand(EMusicCommandType::RejectArtist, FMusicCommandResult::Success(
        FText::FromString(TEXT("Artist passed.")),
        { Command.ArtistId }));
}

FMusicCommandResult UCommandDispatcherSubsystem::ExecuteStartRecording(const FStartRecordingCommand& Command)
{
    FMusicCommandResult FailureResult;
    if (!RequireGameInstance(FailureResult))
    {
        return FinishCommand(EMusicCommandType::StartRecording, FailureResult);
    }

    URecordManagerSubsystem* RecordSubsystem = GetGameInstance()->GetSubsystem<URecordManagerSubsystem>();
    UFinanceManagerSubsystem* FinanceSubsystem = GetGameInstance()->GetSubsystem<UFinanceManagerSubsystem>();
    UGameTimeSubsystem* TimeSubsystem = GetGameInstance()->GetSubsystem<UGameTimeSubsystem>();
    UArtistManagerSubsystem* ArtistSubsystem = GetGameInstance()->GetSubsystem<UArtistManagerSubsystem>();
    if (!RecordSubsystem || !FinanceSubsystem || !TimeSubsystem)
    {
        return FinishCommand(EMusicCommandType::StartRecording, FMusicCommandResult::Failure(
            EMusicCommandErrorCode::SubsystemUnavailable,
            FText::FromString(TEXT("Recording is unavailable because required systems are missing."))));
    }

    FRecordRecordingIntent Intent;
    Intent.ArtistId = Command.ArtistId;
    Intent.AlbumName = Command.RecordTitle;
    Intent.RecordType = Command.RecordType;
    Intent.bIsSingle = Command.bIsSingle;
    Intent.bIsLP = Command.bIsLP;
    Intent.SongIds = Command.SongIds;
    Intent.RequestedFormats = Command.RequestedFormats;
    if (Command.DesiredReleaseDate.GetTicks() > 0)
    {
        Intent.DesiredReleaseDate = Command.DesiredReleaseDate;
    }

    FRecordingProjection Projection;
    FString ProjectionError;
    if (!RecordSubsystem->BuildRecordingProjection(Intent, Projection, ProjectionError))
    {
        EMusicCommandErrorCode Code = EMusicCommandErrorCode::ValidationFailed;
        if (ProjectionError.Contains(TEXT("signed"), ESearchCase::IgnoreCase))
        {
            Code = EMusicCommandErrorCode::ArtistNotSigned;
        }
        else if (ProjectionError.Contains(TEXT("locked"), ESearchCase::IgnoreCase))
        {
            Code = EMusicCommandErrorCode::SongLocked;
        }
        else if (ProjectionError.Contains(TEXT("Invalid"), ESearchCase::IgnoreCase))
        {
            Code = EMusicCommandErrorCode::InvalidReference;
        }

        return FinishCommand(EMusicCommandType::StartRecording, FMusicCommandResult::Failure(
            Code,
            FText::FromString(ProjectionError.IsEmpty() ? TEXT("Recording could not be projected.") : ProjectionError)));
    }

    const FString LabelId = GetPlayerLabelId();
    if (FinanceSubsystem->GetLabelBalance(LabelId) < Projection.EstimatedRecordingCost)
    {
        return FinishCommand(EMusicCommandType::StartRecording, FMusicCommandResult::Failure(
            EMusicCommandErrorCode::InsufficientFunds,
            FText::FromString(TEXT("Not enough cash to start this recording."))));
    }

    FString Error;
    if (!RecordSubsystem->SubmitRecordingIntent(Intent, Error))
    {
        EMusicCommandErrorCode Code = EMusicCommandErrorCode::ValidationFailed;
        if (Error.Contains(TEXT("signed"), ESearchCase::IgnoreCase))
        {
            Code = EMusicCommandErrorCode::ArtistNotSigned;
        }
        else if (Error.Contains(TEXT("locked"), ESearchCase::IgnoreCase))
        {
            Code = EMusicCommandErrorCode::SongLocked;
        }
        else if (Error.Contains(TEXT("Invalid"), ESearchCase::IgnoreCase))
        {
            Code = EMusicCommandErrorCode::InvalidReference;
        }

        return FinishCommand(EMusicCommandType::StartRecording, FMusicCommandResult::Failure(
            Code,
            FText::FromString(Error.IsEmpty() ? TEXT("Recording could not be started.") : Error)));
    }

    FCashFlowEntry CostEntry;
    CostEntry.LabelId = LabelId;
    CostEntry.Type = ETransactionType::RecordingCost;
    CostEntry.Amount = -Projection.EstimatedRecordingCost;
    CostEntry.Timestamp = TimeSubsystem->GetCurrentGameDate();
    CostEntry.RefId = FString::Printf(TEXT("Recording:%s:%s"), *Command.ArtistId, *Command.RecordTitle);
    FinanceSubsystem->RegisterTransaction(CostEntry);

    if (UEventSubsystem* EventSubsystem = GetGameInstance()->GetSubsystem<UEventSubsystem>())
    {
        FString ArtistDisplayName = Command.ArtistId;
        if (ArtistSubsystem)
        {
            if (const FArtistContract* Contract = ArtistSubsystem->GetContractByArtistId(Command.ArtistId))
            {
                ArtistDisplayName = Contract->ArtistData.ArtistName.IsEmpty() ? Contract->ArtistId : Contract->ArtistData.ArtistName;
            }
        }

        const FMusicNewsEvent Event = BuildRecordingStartedNewsEvent(Command, Projection, ArtistDisplayName, Command.RecordTitle);
        const FString Key = FString::Printf(
            TEXT("RecordingStarted:%s:%s:%s"),
            *Command.ArtistId,
            *Command.RecordTitle,
            *Projection.StartDate.ToString(TEXT("%Y%m%d")));
        EventSubsystem->PublishNewsEvent(Event, Key);
    }

    FMusicCommandResult Result = FMusicCommandResult::Success(
        FText::FromString(TEXT("Recording started.")),
        Command.SongIds);
    Result.AffectedEntityIds.Add(Command.ArtistId);
    Result.Quantity = Projection.EstimatedDurationDays;
    Result.ResultDate = Projection.EstimatedCompletionDate;
    return FinishCommand(EMusicCommandType::StartRecording, Result);
}

FMusicCommandResult UCommandDispatcherSubsystem::ExecuteScheduleRelease(const FScheduleReleaseCommand& Command)
{
    FMusicCommandResult FailureResult;
    if (!RequireGameInstance(FailureResult))
    {
        return FinishCommand(EMusicCommandType::ScheduleRelease, FailureResult);
    }

    URecordManagerSubsystem* RecordSubsystem = GetGameInstance()->GetSubsystem<URecordManagerSubsystem>();
    if (!RecordSubsystem)
    {
        return FinishCommand(EMusicCommandType::ScheduleRelease, FMusicCommandResult::Failure(
            EMusicCommandErrorCode::SubsystemUnavailable,
            FText::FromString(TEXT("Release scheduling is unavailable because the record system is missing."))));
    }

    FString Error;
    if (!RecordSubsystem->ScheduleRelease(Command, Error))
    {
        const EMusicCommandErrorCode Code = Error.Contains(TEXT("date"), ESearchCase::IgnoreCase)
            ? EMusicCommandErrorCode::DateConflict
            : EMusicCommandErrorCode::ValidationFailed;
        return FinishCommand(EMusicCommandType::ScheduleRelease, FMusicCommandResult::Failure(
            Code,
            FText::FromString(Error.IsEmpty() ? TEXT("Release could not be scheduled.") : Error)));
    }

    FMusicCommandResult Result = FMusicCommandResult::Success(
        FText::FromString(TEXT("Release scheduled.")),
        { Command.RecordId });
    Result.ResultDate = Command.ReleaseDate;

    if (UEventSubsystem* EventSubsystem = GetGameInstance()->GetSubsystem<UEventSubsystem>())
    {
        const FString RecordName = RecordSubsystem->GetRecordDisplayName(Command.RecordId);
        const FString ArtistName = RecordSubsystem->GetArtistDisplayNameForRecord(Command.RecordId);
        const FMusicNewsEvent Event = BuildReleaseScheduledNewsEvent(Command, RecordName, ArtistName);
        EventSubsystem->PublishNewsEvent(Event, FString::Printf(TEXT("ReleaseScheduled:%s:%s"), *Command.RecordId, *Command.ReleaseDate.ToString(TEXT("%Y%m%d"))));
    }

    return FinishCommand(EMusicCommandType::ScheduleRelease, Result);
}

FMusicCommandResult UCommandDispatcherSubsystem::ExecuteLaunchMarketingCampaign(const FLaunchMarketingCampaignCommand& Command)
{
    FMusicCommandResult FailureResult;
    if (!RequireGameInstance(FailureResult))
    {
        return FinishCommand(EMusicCommandType::LaunchMarketingCampaign, FailureResult);
    }

    UMarketingManagerSubsystem* MarketingSubsystem = GetGameInstance()->GetSubsystem<UMarketingManagerSubsystem>();
    if (!MarketingSubsystem)
    {
        return FinishCommand(EMusicCommandType::LaunchMarketingCampaign, FMusicCommandResult::Failure(
            EMusicCommandErrorCode::SubsystemUnavailable,
            FText::FromString(TEXT("Marketing is unavailable because the marketing system is missing."))));
    }

    FString CampaignId;
    FString Error;
    if (!MarketingSubsystem->LaunchMarketingCampaign(Command, CampaignId, Error))
    {
        EMusicCommandErrorCode Code = EMusicCommandErrorCode::ValidationFailed;
        if (Error.Contains(TEXT("afford"), ESearchCase::IgnoreCase))
        {
            Code = EMusicCommandErrorCode::InsufficientFunds;
        }
        else if (Error.Contains(TEXT("date"), ESearchCase::IgnoreCase))
        {
            Code = EMusicCommandErrorCode::DateConflict;
        }

        return FinishCommand(EMusicCommandType::LaunchMarketingCampaign, FMusicCommandResult::Failure(
            Code,
            FText::FromString(Error.IsEmpty() ? TEXT("Marketing campaign could not be launched.") : Error)));
    }

    FMusicCommandResult Result = FMusicCommandResult::Success(
        FText::FromString(TEXT("Marketing campaign launched.")),
        { Command.RecordId, CampaignId });
    Result.CreatedEntityId = CampaignId;

    if (UEventSubsystem* EventSubsystem = GetGameInstance()->GetSubsystem<UEventSubsystem>())
    {
        FString RecordName = Command.RecordId;
        FString ArtistName;
        if (const URecordManagerSubsystem* RecordSubsystem = GetGameInstance()->GetSubsystem<URecordManagerSubsystem>())
        {
            RecordName = RecordSubsystem->GetRecordDisplayName(Command.RecordId);
            ArtistName = RecordSubsystem->GetArtistDisplayNameForRecord(Command.RecordId);
        }

        const FMusicNewsEvent Event = BuildMarketingLaunchNewsEvent(Command, CampaignId, RecordName, ArtistName);
        EventSubsystem->PublishNewsEvent(Event, FString::Printf(TEXT("MarketingLaunch:%s"), *CampaignId));
    }

    return FinishCommand(EMusicCommandType::LaunchMarketingCampaign, Result);
}

FMusicCommandResult UCommandDispatcherSubsystem::ExecuteAdvanceTime(const FAdvanceTimeCommand& Command)
{
    FMusicCommandResult FailureResult;
    if (!RequireGameInstance(FailureResult))
    {
        return FinishCommand(EMusicCommandType::AdvanceTime, FailureResult);
    }

    if (Command.WeeksToAdvance <= 0)
    {
        return FinishCommand(EMusicCommandType::AdvanceTime, FMusicCommandResult::Failure(
            EMusicCommandErrorCode::ValidationFailed,
            FText::FromString(TEXT("Weeks to advance must be positive."))));
    }

    UGameTimeSubsystem* TimeSubsystem = GetGameInstance()->GetSubsystem<UGameTimeSubsystem>();
    if (!TimeSubsystem)
    {
        return FinishCommand(EMusicCommandType::AdvanceTime, FMusicCommandResult::Failure(
            EMusicCommandErrorCode::SubsystemUnavailable,
            FText::FromString(TEXT("Time advancement is unavailable because the time system is missing."))));
    }

    const int32 WeeksAdvanced = TimeSubsystem->AdvanceWeeks(Command.WeeksToAdvance);
    if (WeeksAdvanced <= 0)
    {
        FMusicCommandResult Result = FMusicCommandResult::Failure(
            EMusicCommandErrorCode::InvalidState,
            FText::FromString(TEXT("The simulation cannot advance further.")));
        Result.ResultDate = TimeSubsystem->GetCurrentGameDate();
        return FinishCommand(EMusicCommandType::AdvanceTime, Result);
    }

    FMusicCommandResult Result = FMusicCommandResult::Success(FText::FromString(TEXT("Time advanced.")));
    Result.Quantity = WeeksAdvanced;
    Result.ResultDate = TimeSubsystem->GetCurrentGameDate();
    return FinishCommand(EMusicCommandType::AdvanceTime, Result);
}

FMusicNewsEvent UCommandDispatcherSubsystem::BuildReleaseScheduledNewsEvent(const FScheduleReleaseCommand& Command, const FString& RecordDisplayName, const FString& ArtistDisplayName)
{
    const FString RecordName = RecordDisplayName.IsEmpty() ? Command.RecordId : RecordDisplayName;
    const FString ArtistName = ArtistDisplayName.IsEmpty() ? TEXT("Player Label") : ArtistDisplayName;

    FMusicNewsEvent Event;
    Event.NewsId = FGuid::NewGuid();
    Event.Timestamp = Command.ReleaseDate;
    Event.NewsType = EMusicNewsType::RecordRelease;
    Event.SourceName = ArtistName;
    Event.SubjectName = RecordName;
    Event.Headline = FString::Printf(TEXT("%s scheduled for release"), *RecordName);
    Event.BodyText = FString::Printf(
        TEXT("%s is set to release %s on %s across %d region(s)."),
        *ArtistName,
        *RecordName,
        *Command.ReleaseDate.ToString(TEXT("%Y-%m-%d")),
        Command.TargetRegionIds.Num());
    Event.Tags = { TEXT("Release"), TEXT("Planning") };
    Event.Metadata.Add(TEXT("RecordId"), Command.RecordId);
    Event.Metadata.Add(TEXT("ArtistName"), ArtistDisplayName);
    Event.Metadata.Add(TEXT("RegionCount"), FString::FromInt(Command.TargetRegionIds.Num()));
    return Event;
}

FMusicNewsEvent UCommandDispatcherSubsystem::BuildMarketingLaunchNewsEvent(const FLaunchMarketingCampaignCommand& Command, const FString& CampaignId, const FString& RecordDisplayName, const FString& ArtistDisplayName)
{
    const FString RecordName = RecordDisplayName.IsEmpty() ? Command.RecordId : RecordDisplayName;
    const FString ArtistName = ArtistDisplayName.IsEmpty() ? TEXT("Marketing Department") : ArtistDisplayName;
    const FString BudgetText = FString::Printf(TEXT("$%.0f"), Command.Budget);

    FMusicNewsEvent Event;
    Event.NewsId = FGuid::NewGuid();
    Event.Timestamp = Command.StartDate;
    Event.NewsType = EMusicNewsType::MarketingPush;
    Event.SourceName = ArtistName;
    Event.SubjectName = RecordName;
    Event.Headline = FString::Printf(TEXT("Campaign launched for %s"), *RecordName);
    Event.BodyText = FString::Printf(
        TEXT("A %s marketing campaign for %s is now active across %d region(s) and %d channel(s)."),
        *BudgetText,
        *RecordName,
        Command.TargetRegionIds.Num(),
        Command.Channels.Num());
    Event.Tags = { TEXT("Marketing"), TEXT("Campaign") };
    Event.Metadata.Add(TEXT("RecordId"), Command.RecordId);
    Event.Metadata.Add(TEXT("CampaignId"), CampaignId);
    Event.Metadata.Add(TEXT("Budget"), FString::SanitizeFloat(Command.Budget));
    return Event;
}

FMusicCommandResult UCommandDispatcherSubsystem::FinishCommand(EMusicCommandType CommandType, const FMusicCommandResult& Result)
{
    if (Result.bSuccess)
    {
        UE_LOG(LogMusicCommands, Log, TEXT("Command succeeded: %s"), *Result.Message.ToString());
    }
    else
    {
        UE_LOG(LogMusicCommands, Warning, TEXT("Command failed [%d]: %s"), static_cast<int32>(Result.ErrorCode), *Result.Message.ToString());
    }

    const FMusicCommandDomainEvent DomainEvent = BuildDomainEvent(CommandType, Result);
    OnCommandExecuted.Broadcast(Result);
    OnCommandDomainEvent.Broadcast(DomainEvent);
    OnCommandDomainEventNative.Broadcast(DomainEvent);
    return Result;
}

FMusicCommandDomainEvent UCommandDispatcherSubsystem::BuildDomainEvent(EMusicCommandType CommandType, const FMusicCommandResult& Result) const
{
    FMusicCommandDomainEvent Event;
    Event.EventId = FGuid::NewGuid();
    Event.CommandType = CommandType;
    Event.Result = Result;
    Event.AffectedEntityIds = Result.AffectedEntityIds;
    Event.CreatedEntityId = Result.CreatedEntityId;
    Event.Timestamp = Result.ResultDate.GetTicks() > 0 ? Result.ResultDate : FDateTime::UtcNow();
    return Event;
}

bool UCommandDispatcherSubsystem::RequireGameInstance(FMusicCommandResult& OutFailure) const
{
    if (!GetGameInstance())
    {
        OutFailure = FMusicCommandResult::Failure(
            EMusicCommandErrorCode::SubsystemUnavailable,
            FText::FromString(TEXT("Command failed because the game instance is unavailable.")));
        return false;
    }

    return true;
}

FString UCommandDispatcherSubsystem::GetPlayerLabelId() const
{
    if (const UGameInstance* GameInstance = GetGameInstance())
    {
        if (const UPlayerLabelSubsystem* LabelSubsystem = GameInstance->GetSubsystem<UPlayerLabelSubsystem>())
        {
            return LabelSubsystem->GetPlayerLabelId();
        }
    }

    return TEXT("label_player");
}
