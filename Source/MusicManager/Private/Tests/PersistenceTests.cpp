#include "MusicSaveGame.h"
#include "Misc/AutomationTest.h"
#include "RecordManagerSubsystem.h"
#include "SongManagerSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicSaveMigrationFromLegacyTest, "MusicManager.Persistence.MigrateLegacySave", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicSaveMigrationFromLegacyTest::RunTest(const FString& Parameters)
{
    UMusicSaveGame* SaveGame = NewObject<UMusicSaveGame>();
    TestNotNull(TEXT("Save game should be created."), SaveGame);

    SaveGame->SaveVersion = 1;
    SaveGame->CampaignMeta.SlotName = TEXT("LegacySlot");
    SaveGame->SavedGameDate = FDateTime(1984, 6, 1);
    SaveGame->PlayerMoney = 250000;
    SaveGame->PlayerLabelSnapshot.LabelId.Empty();
    SaveGame->PlayerLabelSnapshot.DisplayName.Empty();

    FArtistContract Contract;
    Contract.ArtistId = TEXT("artist_legacy");
    Contract.bContractActive = true;
    Contract.StartDate = FDateTime(1984, 1, 1);
    Contract.EndDate = FDateTime(1986, 1, 1);
    SaveGame->SavedContracts.Add(Contract);

    const FMusicSaveValidationResult MigrationResult = SaveGame->MigrateToCurrentVersion();

    TestTrue(TEXT("Legacy save should migrate."), MigrationResult.bIsValid);
    TestEqual(TEXT("Save should migrate to current version."), SaveGame->SaveVersion, UMusicSaveGame::CurrentSaveVersion);
    TestEqual(TEXT("Legacy date should migrate to time snapshot."), SaveGame->TimeSnapshot.CurrentGameDate, FDateTime(1984, 6, 1));
    TestEqual(TEXT("Legacy contracts should migrate to active contract snapshot."), SaveGame->ArtistSnapshot.ActiveContracts.Num(), 1);
    TestTrue(TEXT("Legacy player money should migrate to a finance account."), SaveGame->FinanceSnapshot.LabelAccounts.Contains(TEXT("label_player")));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicSaveUnsupportedFutureVersionTest, "MusicManager.Persistence.UnsupportedFutureVersion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicSaveUnsupportedFutureVersionTest::RunTest(const FString& Parameters)
{
    UMusicSaveGame* SaveGame = NewObject<UMusicSaveGame>();
    SaveGame->SaveVersion = UMusicSaveGame::CurrentSaveVersion + 1;

    const FMusicSaveValidationResult MigrationResult = SaveGame->MigrateToCurrentVersion();
    TestFalse(TEXT("Future save versions should not migrate."), MigrationResult.bIsValid);
    TestTrue(TEXT("Migration should explain the failure."), MigrationResult.Errors.Num() > 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicSaveSlotDescriptorTest, "MusicManager.Persistence.SlotDescriptor", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicSaveSlotDescriptorTest::RunTest(const FString& Parameters)
{
    UMusicSaveGame* SaveGame = NewObject<UMusicSaveGame>();
    SaveGame->InitializeNewSave(TEXT("CampaignA"));
    SaveGame->PlayerLabelSnapshot.DisplayName = TEXT("Gold Needle Records");
    SaveGame->TimeSnapshot.CurrentGameDate = FDateTime(1991, 9, 2);
    SaveGame->CampaignMeta.ThumbnailAsset = FSoftObjectPath(TEXT("/Game/GUI/SaveThumbs/GoldNeedle"));

    const FSaveSlotDescriptor Descriptor = SaveGame->BuildSlotDescriptor(TEXT("CampaignA"), false, false);

    TestEqual(TEXT("Descriptor should use requested slot name."), Descriptor.SlotName, FString(TEXT("CampaignA")));
    TestEqual(TEXT("Descriptor should expose player label name."), Descriptor.PlayerLabelName, FString(TEXT("Gold Needle Records")));
    TestEqual(TEXT("Descriptor should expose in-game date."), Descriptor.InGameDate, FDateTime(1991, 9, 2));
    TestEqual(TEXT("Descriptor should expose current save version."), Descriptor.SaveVersion, UMusicSaveGame::CurrentSaveVersion);
    TestEqual(TEXT("Descriptor should expose thumbnail path."), Descriptor.ThumbnailAsset.ToString(), FString(TEXT("/Game/GUI/SaveThumbs/GoldNeedle")));
    TestTrue(TEXT("Fresh descriptor should be loadable."), Descriptor.bIsLoadable);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicFutureSnapshotValidationTest, "MusicManager.Persistence.FutureSnapshotValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicFutureSnapshotValidationTest::RunTest(const FString& Parameters)
{
    UMusicSaveGame* SaveGame = NewObject<UMusicSaveGame>();
    SaveGame->InitializeNewSave(TEXT("FutureInvalid"));

    FFutureCriticReviewSaveRecord Review;
    Review.ReviewId = TEXT("review_001");
    Review.RecordId = TEXT("record_001");
    Review.PublicationId = TEXT("press_001");
    Review.Score = 140.f;
    Review.PublishedAt = FDateTime(1980, 1, 1);
    SaveGame->FutureSystemsSnapshot.CriticReviews.Add(Review);

    const FMusicSaveValidationResult Result = SaveGame->ValidateFutureSystemsSnapshot();
    TestFalse(TEXT("Invalid future snapshot should fail validation."), Result.bIsValid);
    TestTrue(TEXT("Invalid future snapshot should report errors."), Result.Errors.Num() > 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicSongDuplicateValidationTest, "MusicManager.Persistence.DuplicateSongValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicSongDuplicateValidationTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    USongManagerSubsystem* SongManager = NewObject<USongManagerSubsystem>(GameInstance);
    FMusicSaveValidationResult Result;

    FSongSaveRecord First;
    First.SongId = TEXT("song_duplicate");
    First.ArtistId = TEXT("artist_001");
    First.Data.SongName = TEXT("First");

    FSongSaveRecord Second = First;
    Second.Data.SongName = TEXT("Second");

    SongManager->ValidateSaveRecords({ First, Second }, Result);

    TestFalse(TEXT("Duplicate song ids should fail validation."), Result.bIsValid);
    TestTrue(TEXT("Duplicate song validation should report an error."), Result.Errors.Num() > 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicRecordBrokenSongReferenceValidationTest, "MusicManager.Persistence.RecordBrokenSongReference", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicRecordBrokenSongReferenceValidationTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    URecordManagerSubsystem* RecordManager = NewObject<URecordManagerSubsystem>(GameInstance);
    FMusicSaveValidationResult Result;

    FRecordData Record;
    Record.RecordId = TEXT("record_001");
    Record.ArtistId = TEXT("artist_001");
    Record.LabelId = TEXT("label_player");
    Record.AlbumName = TEXT("Missing Song Record");
    Record.bIsSingle = true;
    Record.SongIds = { TEXT("song_missing") };
    Record.DateRecorded = FDateTime(1982, 5, 1);
    Record.ReleaseDate = FDateTime(1982, 6, 1);
    Record.Formats = { ERecordFormat::Vinyl };
    Record.TargetRegionIds = { TEXT("US") };

    FRecordManagerSnapshot Snapshot;
    Snapshot.Records.Add(Record);
    Snapshot.RecordStates.Add(Record.RecordId, ERecordLifecycleState::Released);

    const TSet<FString> KnownArtists = { TEXT("artist_001") };
    const TSet<FString> KnownSongs;
    const TSet<FString> KnownLabels = { TEXT("label_player") };

    RecordManager->ValidateSaveSnapshot(Snapshot, KnownArtists, KnownSongs, KnownLabels, Result);

    TestFalse(TEXT("Record with missing song reference should fail validation."), Result.bIsValid);
    TestTrue(TEXT("Missing song reference should report an error."), Result.Errors.Num() > 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicActiveRecordingBrokenSongReferenceValidationTest, "MusicManager.Persistence.ActiveRecordingBrokenSongReference", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicActiveRecordingBrokenSongReferenceValidationTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    URecordManagerSubsystem* RecordManager = NewObject<URecordManagerSubsystem>(GameInstance);
    FMusicSaveValidationResult Result;

    FActiveRecordingSession Session;
    Session.RecordingId = TEXT("recording_001");
    Session.Intent.ArtistId = TEXT("artist_001");
    Session.Intent.RecordType = ERecordType::EP;
    Session.Intent.SongIds = { TEXT("song_missing") };
    Session.StartDate = FDateTime(1982, 5, 1);
    Session.CompletionDate = FDateTime(1982, 6, 1);
    Session.RecordingCost = 10000.f;

    FRecordManagerSnapshot Snapshot;
    Snapshot.ActiveRecordingSessions.Add(Session);

    const TSet<FString> KnownArtists = { TEXT("artist_001") };
    const TSet<FString> KnownSongs;
    const TSet<FString> KnownLabels = { TEXT("label_player") };

    RecordManager->ValidateSaveSnapshot(Snapshot, KnownArtists, KnownSongs, KnownLabels, Result);

    TestFalse(TEXT("Active recording with missing song reference should fail validation."), Result.bIsValid);
    TestTrue(TEXT("Missing active recording song reference should report an error."), Result.Errors.Num() > 0);

    return true;
}

#endif
