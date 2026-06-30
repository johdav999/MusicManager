#include "MusicSaveGame.h"

namespace
{
template <typename RecordType, typename IdGetterType>
void ValidateUniqueRequiredIds(const TArray<RecordType>& Records, IdGetterType IdGetter, const TCHAR* AreaName, FMusicSaveValidationResult& Result)
{
    TSet<FString> SeenIds;
    for (const RecordType& Record : Records)
    {
        const FString Id = IdGetter(Record);
        if (Id.IsEmpty())
        {
            Result.AddError(FString::Printf(TEXT("%s snapshot contains an empty id."), AreaName));
            continue;
        }
        if (SeenIds.Contains(Id))
        {
            Result.AddError(FString::Printf(TEXT("%s snapshot contains duplicate id %s."), AreaName, *Id));
        }
        SeenIds.Add(Id);
    }
}
} // namespace

void UMusicSaveGame::InitializeNewSave(const FString& SlotName)
{
    SaveVersion = CurrentSaveVersion;
    CampaignMeta.SlotName = SlotName;
    const FDateTime Now = FDateTime::UtcNow();
    if (CampaignMeta.CreatedAt.GetTicks() <= 0)
    {
        CampaignMeta.CreatedAt = Now;
    }
    CampaignMeta.LastSavedAt = Now;
}

FMusicSaveValidationResult UMusicSaveGame::ValidateTopLevel() const
{
    FMusicSaveValidationResult Result;

    if (SaveVersion < MinimumSupportedSaveVersion || SaveVersion > CurrentSaveVersion)
    {
        Result.AddError(FString::Printf(
            TEXT("Unsupported save version %d. Supported range is %d-%d."),
            SaveVersion,
            MinimumSupportedSaveVersion,
            CurrentSaveVersion));
    }

    if (PlayerLabelSnapshot.LabelId.IsEmpty())
    {
        Result.AddError(TEXT("Player label id is empty."));
    }

    if (TimeSnapshot.CurrentGameDate.GetTicks() <= 0)
    {
        Result.AddError(TEXT("Current game date is unset."));
    }

    if (CampaignMeta.SlotName.IsEmpty())
    {
        Result.AddWarning(TEXT("Campaign metadata slot name is empty."));
    }

    return Result;
}

FMusicSaveValidationResult UMusicSaveGame::MigrateToCurrentVersion()
{
    FMusicSaveValidationResult Result;

    if (SaveVersion > CurrentSaveVersion)
    {
        Result.AddError(FString::Printf(TEXT("Cannot migrate future save version %d. Current version is %d."), SaveVersion, CurrentSaveVersion));
        return Result;
    }

    if (SaveVersion < MinimumSupportedSaveVersion)
    {
        Result.AddError(FString::Printf(TEXT("Cannot migrate unsupported save version %d. Minimum supported version is %d."), SaveVersion, MinimumSupportedSaveVersion));
        return Result;
    }

    if (SaveVersion == CurrentSaveVersion)
    {
        return Result;
    }

    if (SaveVersion == 1)
    {
        if (SavedGameDate.GetTicks() > 0)
        {
            TimeSnapshot.CurrentGameDate = SavedGameDate;
        }

        if (PlayerLabelSnapshot.LabelId.IsEmpty())
        {
            PlayerLabelSnapshot.LabelId = TEXT("label_player");
        }
        if (PlayerLabelSnapshot.DisplayName.IsEmpty())
        {
            PlayerLabelSnapshot.DisplayName = TEXT("Player Label");
        }
        if (PlayerLabelSnapshot.FoundedDate.GetTicks() <= 0)
        {
            PlayerLabelSnapshot.FoundedDate = FDateTime(1955, 1, 1);
        }

        if (ArtistSnapshot.ActiveContracts.IsEmpty() && !SavedContracts.IsEmpty())
        {
            ArtistSnapshot.ActiveContracts = SavedContracts;
        }

        if (PlayerMoney != 0 && !FinanceSnapshot.LabelAccounts.Contains(PlayerLabelSnapshot.LabelId))
        {
            FLabelAccount Account;
            Account.LabelId = PlayerLabelSnapshot.LabelId;
            Account.CurrentBalance = static_cast<float>(PlayerMoney);

            FCashFlowEntry OpeningBalance;
            OpeningBalance.LabelId = Account.LabelId;
            OpeningBalance.Type = ETransactionType::GenericIncome;
            OpeningBalance.Amount = Account.CurrentBalance;
            OpeningBalance.Timestamp = TimeSnapshot.CurrentGameDate.GetTicks() > 0 ? TimeSnapshot.CurrentGameDate : FDateTime(1955, 1, 1);
            OpeningBalance.RefId = TEXT("legacy_migration_opening_balance");
            Account.Ledger.Add(OpeningBalance);

            FinanceSnapshot.LabelAccounts.Add(Account.LabelId, Account);
        }

        Result.AddWarning(TEXT("Migrated legacy v1 save fields into versioned snapshots."));
        SaveVersion = 2;
    }

    if (SaveVersion == 2)
    {
        if (CampaignMeta.CreatedAt.GetTicks() <= 0)
        {
            CampaignMeta.CreatedAt = FDateTime::UtcNow();
        }
        if (CampaignMeta.LastSavedAt.GetTicks() <= 0)
        {
            CampaignMeta.LastSavedAt = CampaignMeta.CreatedAt;
        }

        Result.AddWarning(TEXT("Migrated save metadata from v2 to v3."));
        SaveVersion = 3;
    }

    if (SaveVersion == 3)
    {
        Result.AddWarning(TEXT("Migrated save from v3 to v4 with release, marketing, chart, and news snapshot defaults."));
        SaveVersion = 4;
    }

    if (SaveVersion == 4)
    {
        Result.AddWarning(TEXT("Migrated save from v4 to v5 with reserved future-system and UI-state snapshots."));
        SaveVersion = 5;
    }

    if (SaveVersion != CurrentSaveVersion)
    {
        Result.AddError(FString::Printf(TEXT("Save migration stopped at version %d instead of current version %d."), SaveVersion, CurrentSaveVersion));
    }

    return Result;
}

FMusicSaveValidationResult UMusicSaveGame::ValidateFutureSystemsSnapshot() const
{
    FMusicSaveValidationResult Result;

    ValidateUniqueRequiredIds(FutureSystemsSnapshot.Tours, [](const FFutureTourSaveRecord& Record) { return Record.TourId; }, TEXT("Tour"), Result);
    for (const FFutureTourSaveRecord& Tour : FutureSystemsSnapshot.Tours)
    {
        if (Tour.ArtistId.IsEmpty())
        {
            Result.AddError(FString::Printf(TEXT("Tour %s has no artist id."), *Tour.TourId));
        }
        if (Tour.Status.IsEmpty())
        {
            Result.AddError(FString::Printf(TEXT("Tour %s has no status."), *Tour.TourId));
        }
        if (Tour.StartDate.GetTicks() <= 0 || Tour.EndDate.GetTicks() <= 0 || Tour.EndDate < Tour.StartDate)
        {
            Result.AddError(FString::Printf(TEXT("Tour %s has invalid dates."), *Tour.TourId));
        }
    }

    ValidateUniqueRequiredIds(FutureSystemsSnapshot.Awards, [](const FFutureAwardSaveRecord& Record) { return Record.AwardSeasonId + TEXT(":") + Record.AwardId; }, TEXT("Award"), Result);
    for (const FFutureAwardSaveRecord& Award : FutureSystemsSnapshot.Awards)
    {
        if (Award.AwardSeasonId.IsEmpty() || Award.AwardId.IsEmpty())
        {
            Result.AddError(TEXT("Award snapshot contains an empty season or award id."));
        }
        if (Award.Year < 1955 || Award.Year > 2026)
        {
            Result.AddError(FString::Printf(TEXT("Award %s has invalid year %d."), *Award.AwardId, Award.Year));
        }
    }

    ValidateUniqueRequiredIds(FutureSystemsSnapshot.CriticReviews, [](const FFutureCriticReviewSaveRecord& Record) { return Record.ReviewId; }, TEXT("Critic review"), Result);
    for (const FFutureCriticReviewSaveRecord& Review : FutureSystemsSnapshot.CriticReviews)
    {
        if (Review.RecordId.IsEmpty() || Review.PublicationId.IsEmpty())
        {
            Result.AddError(FString::Printf(TEXT("Critic review %s has missing record or publication id."), *Review.ReviewId));
        }
        if (!FMath::IsFinite(Review.Score) || Review.Score < 0.f || Review.Score > 100.f)
        {
            Result.AddError(FString::Printf(TEXT("Critic review %s has invalid score."), *Review.ReviewId));
        }
        if (Review.PublishedAt.GetTicks() <= 0)
        {
            Result.AddError(FString::Printf(TEXT("Critic review %s has unset published date."), *Review.ReviewId));
        }
    }

    ValidateUniqueRequiredIds(FutureSystemsSnapshot.RivalLabels, [](const FFutureRivalLabelSaveRecord& Record) { return Record.LabelId; }, TEXT("Rival label"), Result);
    for (const FFutureRivalLabelSaveRecord& Rival : FutureSystemsSnapshot.RivalLabels)
    {
        if (Rival.DisplayName.IsEmpty())
        {
            Result.AddError(FString::Printf(TEXT("Rival label %s has no display name."), *Rival.LabelId));
        }
        if (!FMath::IsFinite(Rival.Cash) || !FMath::IsFinite(Rival.Reputation) || Rival.Reputation < 0.f)
        {
            Result.AddError(FString::Printf(TEXT("Rival label %s has invalid numeric state."), *Rival.LabelId));
        }
    }

    ValidateUniqueRequiredIds(FutureSystemsSnapshot.Staff, [](const FFutureStaffSaveRecord& Record) { return Record.StaffId; }, TEXT("Staff"), Result);
    for (const FFutureStaffSaveRecord& Staff : FutureSystemsSnapshot.Staff)
    {
        if (Staff.RoleId.IsEmpty() || Staff.Level <= 0 || Staff.HiredAt.GetTicks() <= 0)
        {
            Result.AddError(FString::Printf(TEXT("Staff member %s has invalid role, level, or hire date."), *Staff.StaffId));
        }
    }

    ValidateUniqueRequiredIds(FutureSystemsSnapshot.Unlocks, [](const FFutureUnlockSaveRecord& Record) { return Record.UnlockId; }, TEXT("Unlock"), Result);
    for (const FFutureUnlockSaveRecord& Unlock : FutureSystemsSnapshot.Unlocks)
    {
        if (Unlock.UnlockedAt.GetTicks() <= 0)
        {
            Result.AddError(FString::Printf(TEXT("Unlock %s has unset unlock date."), *Unlock.UnlockId));
        }
    }

    TSet<FString> SeenSettingKeys;
    for (const TPair<FString, FString>& Pair : FutureSystemsSnapshot.SettingsUIState.UserSettings)
    {
        if (Pair.Key.IsEmpty())
        {
            Result.AddError(TEXT("Settings/UI snapshot contains an empty setting key."));
        }
        if (SeenSettingKeys.Contains(Pair.Key))
        {
            Result.AddError(FString::Printf(TEXT("Settings/UI snapshot contains duplicate setting key %s."), *Pair.Key));
        }
        SeenSettingKeys.Add(Pair.Key);
    }

    return Result;
}

FSaveSlotDescriptor UMusicSaveGame::BuildSlotDescriptor(const FString& RequestedSlotName, bool bIsAutosave, bool bIsBackup, const FString& ParentSlotName) const
{
    FSaveSlotDescriptor Descriptor;
    Descriptor.SlotName = RequestedSlotName.IsEmpty() ? CampaignMeta.SlotName : RequestedSlotName;
    Descriptor.DisplayName = FText::FromString(Descriptor.SlotName);
    Descriptor.PlayerLabelName = PlayerLabelSnapshot.DisplayName;
    Descriptor.InGameDate = TimeSnapshot.CurrentGameDate;
    Descriptor.CreatedAt = CampaignMeta.CreatedAt;
    Descriptor.LastSavedAt = CampaignMeta.LastSavedAt;
    Descriptor.ThumbnailAsset = CampaignMeta.ThumbnailAsset;
    Descriptor.SaveVersion = SaveVersion;
    Descriptor.bIsAutosave = bIsAutosave;
    Descriptor.bIsBackup = bIsBackup;
    Descriptor.ParentSlotName = ParentSlotName;

    const FMusicSaveValidationResult Validation = ValidateTopLevel();
    Descriptor.bIsLoadable = Validation.bIsValid;
    Descriptor.ValidationMessages.Append(Validation.Errors);
    Descriptor.ValidationMessages.Append(Validation.Warnings);

    return Descriptor;
}
