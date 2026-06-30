#include "PlayerLabelSubsystem.h"

void UPlayerLabelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (LabelState.LabelId.IsEmpty())
    {
        LabelState.LabelId = TEXT("label_player");
    }

    if (LabelState.DisplayName.IsEmpty())
    {
        LabelState.DisplayName = TEXT("Player Label");
    }

    if (LabelState.FoundedDate.GetTicks() <= 0)
    {
        LabelState.FoundedDate = FDateTime(1955, 1, 1);
    }
}

void UPlayerLabelSubsystem::BuildSaveSnapshot(FPlayerLabelSnapshot& OutSnapshot) const
{
    OutSnapshot = LabelState;
}

void UPlayerLabelSubsystem::ApplySaveSnapshot(const FPlayerLabelSnapshot& Snapshot)
{
    LabelState = Snapshot;
}

void UPlayerLabelSubsystem::ValidateSaveSnapshot(const FPlayerLabelSnapshot& Snapshot, FMusicSaveValidationResult& Result) const
{
    if (Snapshot.LabelId.IsEmpty())
    {
        Result.AddError(TEXT("Player label snapshot has an empty label id."));
    }

    if (Snapshot.DisplayName.IsEmpty())
    {
        Result.AddError(TEXT("Player label snapshot has an empty display name."));
    }

    if (Snapshot.FoundedDate.GetTicks() <= 0)
    {
        Result.AddError(TEXT("Player label founded date is unset."));
    }

    if (!FMath::IsFinite(Snapshot.Reputation) || Snapshot.Reputation < 0.f)
    {
        Result.AddError(TEXT("Player label reputation is invalid."));
    }
}
