#include "CommandDispatcherSubsystem.h"
#include "Misc/AutomationTest.h"
#include "UIManagerSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicCommandResultConstructorsTest, "MusicManager.Commands.ResultConstructors", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicCommandResultConstructorsTest::RunTest(const FString& Parameters)
{
    const FMusicCommandResult Success = FMusicCommandResult::Success(FText::FromString(TEXT("Done")), { TEXT("entity_a") });
    TestTrue(TEXT("Success result should be successful."), Success.bSuccess);
    TestEqual(TEXT("Success result error code should be None."), Success.ErrorCode, EMusicCommandErrorCode::None);
    TestEqual(TEXT("Success result should carry affected ids."), Success.AffectedEntityIds.Num(), 1);

    const FMusicCommandResult Failure = FMusicCommandResult::Failure(
        EMusicCommandErrorCode::InvalidReference,
        FText::FromString(TEXT("Invalid")),
        FText::FromString(TEXT("Pick another entity")));
    TestFalse(TEXT("Failure result should not be successful."), Failure.bSuccess);
    TestEqual(TEXT("Failure should preserve error code."), Failure.ErrorCode, EMusicCommandErrorCode::InvalidReference);
    TestFalse(TEXT("Failure should preserve remediation hint."), Failure.RemediationHint.IsEmpty());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicCommandDispatcherInvalidCommandTest, "MusicManager.Commands.InvalidCommandFailureAndDomainEvent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicCommandDispatcherInvalidCommandTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    UCommandDispatcherSubsystem* Dispatcher = NewObject<UCommandDispatcherSubsystem>(GameInstance);
    TestNotNull(TEXT("Dispatcher should be created."), Dispatcher);

    int32 DomainEvents = 0;
    FMusicCommandDomainEvent LastEvent;
    Dispatcher->OnCommandDomainEventNative.AddLambda([&DomainEvents, &LastEvent](const FMusicCommandDomainEvent& Event)
    {
        ++DomainEvents;
        LastEvent = Event;
    });

    FAdvanceTimeCommand Command;
    Command.WeeksToAdvance = 0;
    const FMusicCommandResult Result = Dispatcher->ExecuteAdvanceTime(Command);

    TestFalse(TEXT("Invalid advance command should fail cleanly."), Result.bSuccess);
    TestEqual(TEXT("Invalid week count should be reported as validation failure."), Result.ErrorCode, EMusicCommandErrorCode::ValidationFailed);
    TestEqual(TEXT("Exactly one domain event should be emitted."), DomainEvents, 1);
    TestEqual(TEXT("Domain event should preserve command type."), LastEvent.CommandType, EMusicCommandType::AdvanceTime);
    TestTrue(TEXT("Domain event id should be valid."), LastEvent.EventId.IsValid());
    TestFalse(TEXT("Domain event result should preserve failure."), LastEvent.Result.bSuccess);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMusicCommandNotificationPayloadTest, "MusicManager.Commands.UINotificationPayload", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMusicCommandNotificationPayloadTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    UUIManagerSubsystem* UIManager = NewObject<UUIManagerSubsystem>(GameInstance);
    TestNotNull(TEXT("UI manager should be created."), UIManager);

    const FMusicCommandResult Result = FMusicCommandResult::Failure(
        EMusicCommandErrorCode::ValidationFailed,
        FText::FromString(TEXT("Validation failed")));

    UIManager->HandleCommandExecuted(Result);

    TArray<FCommandNotification> Notifications;
    UIManager->GetPendingCommandNotifications(Notifications);

    TestEqual(TEXT("One notification should be buffered."), Notifications.Num(), 1);
    if (Notifications.Num() > 0)
    {
        TestEqual(TEXT("Validation failures should be warnings."), Notifications[0].Severity, ECommandNotificationSeverity::Warning);
        TestEqual(TEXT("Notification should preserve error code."), Notifications[0].Result.ErrorCode, EMusicCommandErrorCode::ValidationFailed);
        TestTrue(TEXT("Notification id should be valid."), Notifications[0].NotificationId.IsValid());
    }

    UIManager->ClearPendingCommandNotifications();
    UIManager->GetPendingCommandNotifications(Notifications);
    TestEqual(TEXT("Notifications should clear."), Notifications.Num(), 0);

    return true;
}

#endif
