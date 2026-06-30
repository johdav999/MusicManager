#include "CommandDispatcherSubsystem.h"
#include "MarketingManagerSubsystem.h"
#include "Misc/AutomationTest.h"
#include "MusicSaveGame.h"
#include "RecordManagerSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
FRecordData BuildValidReleaseMarketingRecord()
{
    FRecordData Record;
    Record.RecordId = TEXT("record_gold_001");
    Record.ArtistId = TEXT("artist_gold_001");
    Record.LabelId = TEXT("label_player");
    Record.AlbumName = TEXT("Gold Needle Nights");
    Record.bIsLP = true;
    Record.SongIds = { TEXT("song_a"), TEXT("song_b") };
    Record.DateRecorded = FDateTime(1985, 3, 1);
    Record.ReleaseDate = FDateTime(1985, 5, 1);
    Record.Formats = { ERecordFormat::Vinyl, ERecordFormat::Cassette };
    Record.TargetRegionIds = { TEXT("US") };
    return Record;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicReleaseSnapshotValidationTest, "MusicManager.ReleaseMarketing.ReleaseSnapshotValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicReleaseSnapshotValidationTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    URecordManagerSubsystem* RecordManager = NewObject<URecordManagerSubsystem>(GameInstance);

    FRecordManagerSnapshot Snapshot;
    Snapshot.Records.Add(BuildValidReleaseMarketingRecord());
    Snapshot.RecordStates.Add(TEXT("record_gold_001"), ERecordLifecycleState::Scheduled);

    FMusicSaveValidationResult ValidResult;
    RecordManager->ValidateSaveSnapshot(
        Snapshot,
        { TEXT("artist_gold_001") },
        { TEXT("song_a"), TEXT("song_b") },
        { TEXT("label_player") },
        ValidResult);
    TestTrue(TEXT("Valid release snapshot should pass validation."), ValidResult.bIsValid);

    Snapshot.Records[0].SongIds.Add(TEXT("song_missing"));
    FMusicSaveValidationResult InvalidResult;
    RecordManager->ValidateSaveSnapshot(
        Snapshot,
        { TEXT("artist_gold_001") },
        { TEXT("song_a"), TEXT("song_b") },
        { TEXT("label_player") },
        InvalidResult);
    TestFalse(TEXT("Release snapshot with missing song should fail validation."), InvalidResult.bIsValid);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicMarketingSnapshotValidationTest, "MusicManager.ReleaseMarketing.MarketingSnapshotValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicMarketingSnapshotValidationTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    UMarketingManagerSubsystem* MarketingManager = NewObject<UMarketingManagerSubsystem>(GameInstance);

    FMarketingCampaign Campaign;
    Campaign.CampaignId = TEXT("campaign_001");
    Campaign.RecordId = TEXT("record_gold_001");
    Campaign.LabelId = TEXT("label_player");
    Campaign.TargetRegionIds = { TEXT("US") };
    Campaign.Channels = { EMarketingChannel::Radio };
    Campaign.Budget = 5000.f;
    Campaign.StartDate = FDateTime(1985, 4, 1);
    Campaign.EndDate = FDateTime(1985, 5, 1);
    Campaign.Status = EMarketingCampaignStatus::Active;

    FMarketingSnapshot Snapshot;
    Snapshot.Campaigns.Add(Campaign);

    FMusicSaveValidationResult ValidResult;
    MarketingManager->ValidateSaveSnapshot(
        Snapshot,
        { TEXT("record_gold_001") },
        { TEXT("label_player") },
        { TEXT("US") },
        ValidResult);
    TestTrue(TEXT("Valid marketing snapshot should pass validation."), ValidResult.bIsValid);

    Snapshot.Campaigns[0].TargetRegionIds = { TEXT("NOPE") };
    FMusicSaveValidationResult InvalidResult;
    MarketingManager->ValidateSaveSnapshot(
        Snapshot,
        { TEXT("record_gold_001") },
        { TEXT("label_player") },
        { TEXT("US") },
        InvalidResult);
    TestFalse(TEXT("Marketing snapshot with missing region should fail validation."), InvalidResult.bIsValid);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicMarketingROITest, "MusicManager.ReleaseMarketing.CampaignROISummary", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicMarketingROITest::RunTest(const FString& Parameters)
{
    FMarketingCampaign Campaign;
    Campaign.CampaignId = TEXT("campaign_roi");
    Campaign.RecordId = TEXT("record_gold_001");
    Campaign.Budget = 1000.f;
    Campaign.TargetRegionIds = { TEXT("US") };
    Campaign.Channels = { EMarketingChannel::Radio };
    Campaign.Status = EMarketingCampaignStatus::Completed;

    FMarketingExposureEntry Exposure;
    Exposure.RegionId = TEXT("US");
    Exposure.Channel = EMarketingChannel::Radio;
    Exposure.Exposure = 2.5f;
    Campaign.GeneratedExposure.Add(Exposure);

    const FMarketingCampaignROISummary Summary = UMarketingManagerSubsystem::CalculateROISummary(Campaign, TEXT("Gold Needle Nights"), TEXT("The Needles"), 10.f);

    TestEqual(TEXT("ROI summary should preserve campaign id."), Summary.CampaignId, FString(TEXT("campaign_roi")));
    TestEqual(TEXT("ROI summary should preserve readable record name."), Summary.RecordDisplayName, FString(TEXT("Gold Needle Nights")));
    TestTrue(TEXT("ROI summary should estimate unit lift from exposure."), Summary.EstimatedUnitLift > 0);
    TestTrue(TEXT("ROI summary should estimate gross revenue."), Summary.EstimatedGrossRevenue > 0.f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicReleaseDashboardSummaryTest, "MusicManager.ReleaseMarketing.ReleaseDashboardSummary", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicReleaseDashboardSummaryTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    URecordManagerSubsystem* RecordManager = NewObject<URecordManagerSubsystem>(GameInstance);

    FRecordManagerSnapshot Snapshot;
    Snapshot.Records.Add(BuildValidReleaseMarketingRecord());
    Snapshot.RecordStates.Add(TEXT("record_gold_001"), ERecordLifecycleState::Scheduled);
    RecordManager->ApplySaveSnapshot(Snapshot);

    FReleaseDashboardSummary Summary;
    RecordManager->BuildReleaseDashboardSummary(Summary);

    TestEqual(TEXT("Scheduled release should appear in dashboard summary."), Summary.ScheduledReleases.Num(), 1);
    if (Summary.ScheduledReleases.Num() > 0)
    {
        TestEqual(TEXT("Dashboard should expose readable record name."), Summary.ScheduledReleases[0].RecordDisplayName, FString(TEXT("Gold Needle Nights")));
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicReleaseMarketingNewsPayloadTest, "MusicManager.ReleaseMarketing.ReadableNewsPayloads", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicReleaseMarketingNewsPayloadTest::RunTest(const FString& Parameters)
{
    FScheduleReleaseCommand ReleaseCommand;
    ReleaseCommand.RecordId = TEXT("record_gold_001");
    ReleaseCommand.LabelId = TEXT("label_player");
    ReleaseCommand.ReleaseDate = FDateTime(1985, 5, 1);
    ReleaseCommand.TargetRegionIds = { TEXT("US"), TEXT("UK") };
    ReleaseCommand.Formats = { ERecordFormat::Vinyl };

    const FMusicNewsEvent ReleaseEvent = UCommandDispatcherSubsystem::BuildReleaseScheduledNewsEvent(ReleaseCommand, TEXT("Gold Needle Nights"), TEXT("The Needles"));
    TestTrue(TEXT("Release headline should use readable record name."), ReleaseEvent.Headline.Contains(TEXT("Gold Needle Nights")));
    TestTrue(TEXT("Release body should use readable artist name."), ReleaseEvent.BodyText.Contains(TEXT("The Needles")));
    TestEqual(TEXT("Release metadata should preserve record id."), ReleaseEvent.Metadata.FindRef(TEXT("RecordId")), FString(TEXT("record_gold_001")));

    FLaunchMarketingCampaignCommand MarketingCommand;
    MarketingCommand.RecordId = TEXT("record_gold_001");
    MarketingCommand.LabelId = TEXT("label_player");
    MarketingCommand.Budget = 7500.f;
    MarketingCommand.StartDate = FDateTime(1985, 4, 1);
    MarketingCommand.EndDate = FDateTime(1985, 5, 1);
    MarketingCommand.TargetRegionIds = { TEXT("US") };
    MarketingCommand.Channels = { EMarketingChannel::Radio, EMarketingChannel::Press };

    const FMusicNewsEvent MarketingEvent = UCommandDispatcherSubsystem::BuildMarketingLaunchNewsEvent(MarketingCommand, TEXT("campaign_001"), TEXT("Gold Needle Nights"), TEXT("The Needles"));
    TestTrue(TEXT("Marketing headline should use readable record name."), MarketingEvent.Headline.Contains(TEXT("Gold Needle Nights")));
    TestTrue(TEXT("Marketing body should include budget."), MarketingEvent.BodyText.Contains(TEXT("$7500")));
    TestEqual(TEXT("Marketing metadata should preserve campaign id."), MarketingEvent.Metadata.FindRef(TEXT("CampaignId")), FString(TEXT("campaign_001")));

    return true;
}

#endif
