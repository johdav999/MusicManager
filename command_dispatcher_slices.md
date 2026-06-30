# Command Dispatcher Implementation Slices

This document breaks the command dispatcher/application layer into production-ready vertical slices. Each completed implementation slice must update `current.md`.

## Overall Goal

Introduce a typed command dispatcher so UI and gameplay callers do not directly mutate subsystem state. Commands validate inputs, execute through owning subsystems, emit domain events, and return structured success/failure results.

## Global Implementation Rules

- Read `AGENTS.md` and `docs/architecture.md` before implementation.
- Keep widgets thin. Widgets gather input and dispatch commands.
- Commands must use stable IDs, not display names.
- Commands must return structured results with player-facing messages and error codes.
- Do not add mock commands, placeholder handlers, or unused scaffolding.
- Every command added must be wired to at least one real gameplay path or documented as internal support for an immediately implemented slice.
- Update `current.md` after each completed slice.

## Slice 1 - Command Result And Error Model

### Prompt

Create the shared command result and command error model used by all player action commands.

### User-Facing Outcome

Gameplay actions can fail cleanly with understandable messages instead of silent no-ops or log-only warnings.

### Implementation Scope

- Add `FMusicCommandResult` or equivalent.
- Include:
  - `bSuccess`
  - primary error code
  - user-facing `FText` message
  - optional remediation hint
  - affected entity ids
- Add `EMusicCommandErrorCode` with initial values:
  - `None`
  - `InvalidReference`
  - `InvalidState`
  - `InsufficientFunds`
  - `DateConflict`
  - `SongLocked`
  - `ArtistNotSigned`
  - `ValidationFailed`
  - `SubsystemUnavailable`
- Add helper constructors for success/failure.

### Acceptance Criteria

- Result type is Blueprint-visible where useful.
- Error codes are stable enum values.
- Existing command-like paths can start returning the result type without breaking compile.
- `current.md` is updated.

### Primary Files

- New command result header/source under `Source/MusicManager/Public` and `Private`
- `Source/MusicManager/MusicManager.Build.cs` if needed
- `current.md`

## Slice 2 - Command Dispatcher Subsystem Skeleton With Real Routing

### Prompt

Create `UCommandDispatcherSubsystem` as the application layer entry point for validated player actions.

### User-Facing Outcome

There is one authoritative service for executing player commands.

### Implementation Scope

- Add `UCommandDispatcherSubsystem : UGameInstanceSubsystem`.
- Add dependency lookup helpers for existing subsystems.
- Add logging category `LogMusicCommands`.
- Add no placeholder commands. Only include command methods implemented in following slices or immediately wired in this slice.
- Add a minimal internal validation helper for required subsystem availability.

### Acceptance Criteria

- Subsystem compiles and initializes.
- No command mutates state without validation.
- No unused fake handlers are added.
- `current.md` is updated.

### Primary Files

- `Source/MusicManager/Public/CommandDispatcherSubsystem.h`
- `Source/MusicManager/Private/CommandDispatcherSubsystem.cpp`
- `Source/MusicManager/MusicManager.Build.cs`
- `current.md`

## Slice 3 - Sign Artist Command

### Prompt

Move artist signing behind a typed sign artist command.

### User-Facing Outcome

Signing an artist validates the artist, deal terms, label state, and funds before changing the roster.

### Implementation Scope

- Add `FSignArtistCommand`.
- Include:
  - artist id
  - label id
  - sign-up bonus/advance
  - royalty rate
  - record commitment
  - contract years
- Implement `ExecuteSignArtist`.
- Validate:
  - artist exists and is unsigned/eligible
  - label id is the player label
  - contract terms are within allowed ranges
  - player label has enough cash if advances are charged now
- Execute through `UArtistManagerSubsystem`.
- Register advance/expense in finance if the existing finance model supports it.
- Emit existing artist signed delegates and any new command-level result.

### Acceptance Criteria

- Signing through the command produces the same or better behavior than direct signing.
- Invalid artist id fails without roster mutation.
- Invalid terms fail without roster mutation.
- Insufficient funds fail if advance payment is enforced.
- `current.md` is updated.

### Primary Files

- `CommandDispatcherSubsystem`
- `ArtistManagerSubsystem`
- `FinanceManagerSubsystem`
- `FArtistDealTerms`
- `current.md`

## Slice 4 - Reject Artist Command

### Prompt

Move unsigned artist rejection/pass behavior behind a typed command.

### User-Facing Outcome

Passing on an artist is validated and consistently updates the discovery pool.

### Implementation Scope

- Add `FRejectArtistCommand`.
- Implement `ExecuteRejectArtist`.
- Validate:
  - artist id exists in unsigned pool
  - artist is not currently signed
- Execute through `UArtistManagerSubsystem`.
- Preserve existing audition/pass behavior.

### Acceptance Criteria

- Invalid artist id fails cleanly.
- Rejected artist is removed or rotated according to existing production behavior.
- Existing UI pass flow can call the command.
- `current.md` is updated.

### Primary Files

- `CommandDispatcherSubsystem`
- `ArtistManagerSubsystem`
- `AuditionWidget` / `UIManagerSubsystem` if wired here
- `current.md`

## Slice 5 - Start Recording Command

### Prompt

Move recording submission behind a typed start recording command that wraps `URecordManagerSubsystem::SubmitRecordingIntent`.

### User-Facing Outcome

Starting a recording gives reliable validation messages and never silently fails.

### Implementation Scope

- Add `FStartRecordingCommand`.
- Include:
  - artist id
  - record title
  - release type
  - selected song ids
  - requested formats
  - desired release date if still supported
- Implement `ExecuteStartRecording`.
- Validate:
  - artist signed and eligible
  - song ids exist, belong to artist, and are recordable
  - single/LP rules
  - formats valid for era
  - player label/funds if recording costs are implemented
- Call `SubmitRecordingIntent`.
- Convert string errors from the current record subsystem into structured command results.

### Acceptance Criteria

- Recording GUI can dispatch this command.
- All current recording validation failures return structured results.
- Successful command creates/starts a record exactly once.
- `current.md` is updated.

### Primary Files

- `CommandDispatcherSubsystem`
- `RecordManagerSubsystem`
- `RecordWidget`
- `current.md`

## Slice 6 - Advance Time Command

### Prompt

Move time advancement behind a typed command so UI fast-forward controls use validated simulation advancement.

### User-Facing Outcome

Advancing time is a controlled action that can be validated, batched, and summarized.

### Implementation Scope

- Add `FAdvanceTimeCommand`.
- Include number of weeks or a named advance mode.
- Implement `ExecuteAdvanceTime`.
- Validate:
  - positive week count
  - simulation has not ended
  - no blocking modal/action state if such state exists
- Call `UGameTimeSubsystem::AdvanceWeeks`.
- Return number of weeks actually advanced and final date.

### Acceptance Criteria

- Invalid week count fails cleanly.
- Simulation end is respected.
- Batch UI suppression still works.
- `current.md` is updated.

### Primary Files

- `CommandDispatcherSubsystem`
- `GameTimeSubsystem`
- `UIManagerSubsystem` if command routing is changed
- `current.md`

## Slice 7 - UI Command Panel Integration

### Prompt

Replace stringly typed command-panel routing with dispatcher-backed command invocations for currently supported commands.

### User-Facing Outcome

Command panel actions execute real validated commands or open real screens; unsupported commands are not presented as active actions.

### Implementation Scope

- Audit existing command panel command names.
- Keep only commands backed by real behavior enabled.
- Route supported actions through `UCommandDispatcherSubsystem` or screen router as appropriate.
- Surface command failure messages to a real UI feedback path or log clearly until a production notification widget exists.
- Remove direct first-contract fallback behavior if it bypasses selection/validation.

### Acceptance Criteria

- `Contracts` and `Studio` no longer rely on brittle direct logic where a command/result path is more appropriate.
- Unsupported command icons are disabled/hidden rather than clickable no-ops.
- No mock command responses are introduced.
- `current.md` is updated.

### Primary Files

- `UIManagerSubsystem`
- `CommandPanelWidget`
- `CommandItemWidget`
- `CommandDispatcherSubsystem`
- `current.md`

## Slice 8 - Audition Widget Command Integration

### Prompt

Wire audition sign/pass actions through the command dispatcher.

### User-Facing Outcome

Audition decisions use the same validation and feedback path as other player actions.

### Implementation Scope

- Update audition sign action to build `FSignArtistCommand`.
- Update pass action to build `FRejectArtistCommand`.
- Show or route command result.
- Ensure the audition widget does not directly mutate artist state.

### Acceptance Criteria

- Sign/pass still work from the audition GUI.
- Failed sign/pass attempts leave state unchanged.
- The UI closes/updates only after command success.
- `current.md` is updated.

### Primary Files

- `AuditionWidget`
- `Layout`
- `UIManagerSubsystem`
- `CommandDispatcherSubsystem`
- `current.md`

## Slice 9 - Domain Event Emission

### Prompt

Add typed domain event emission for command results that need UI/news/system reactions.

### User-Facing Outcome

UI and news systems can respond consistently to command outcomes like artist signed, recording started, and time advanced.

### Implementation Scope

- Define minimal command-domain events currently needed by implemented commands.
- Do not duplicate existing subsystem delegates unless needed for clean application-layer orchestration.
- Bridge command success to existing news/UI delegates where appropriate.
- Keep event payloads typed and stable-id based.

### Acceptance Criteria

- Artist signed and recording started can be observed through typed events or existing clean delegates.
- Event payloads do not use display names as keys.
- No duplicate news/cards are generated from a single command.
- `current.md` is updated.

### Primary Files

- `CommandDispatcherSubsystem`
- `EventSubsystem`
- `UIManagerSubsystem`
- `current.md`

## Slice 10 - Command Regression Coverage

### Prompt

Add automated coverage for implemented commands and validation failures.

### User-Facing Outcome

Core player actions are protected from regressions.

### Implementation Scope

- Add Unreal automation tests or the repo's existing test pattern.
- Cover:
  - successful sign artist
  - invalid artist sign failure
  - invalid recording command failure
  - successful recording command
  - invalid advance-time failure
  - successful advance-time command
- Build state through real subsystem APIs.

### Acceptance Criteria

- Tests compile.
- Tests verify no state mutation on failed commands.
- `current.md` is updated with command test status.

### Primary Files

- `Source/MusicManager/Private/Tests/...`
- `CommandDispatcherSubsystem`
- `current.md`

## Slice 11 - Command Failure UI Notifications

### Prompt

Add a production UI notification surface for command failures and important command outcomes so player actions no longer rely primarily on logs.

### User-Facing Outcome

When a command fails validation, the UI can show a clear, player-facing message with any remediation hint.

### Implementation Scope

- Add a UI-facing notification data structure for command results.
- Add a Blueprint-assignable notification delegate in the UI orchestration layer.
- Route command failures from `UCommandDispatcherSubsystem` into the UI notification surface.
- Buffer notifications until a UI widget can consume them.
- Keep logging as diagnostics only, not the primary player feedback path.

### Acceptance Criteria

- Failed commands emit a UI notification with message and error code.
- Successful important commands can emit non-error notifications.
- Widgets can bind to a real delegate instead of scraping logs.
- No mock notification widgets or placeholder UI screens are added.
- `current.md` is updated.

### Primary Files

- `CommandDispatcherSubsystem`
- `UIManagerSubsystem`
- `MusicCommandResult`
- `current.md`

## Slice 12 - Rich Command Domain Events

### Prompt

Add richer typed command-domain events for downstream UI, news, analytics, and future automation consumers.

### User-Facing Outcome

The game can react consistently to command outcomes such as artist signing, recording start, release scheduling, marketing launch, and time advancement.

### Implementation Scope

- Add a stable command type enum.
- Add a typed command-domain event payload with:
  - command event id
  - command type
  - success/failure result
  - affected stable entity ids
  - created entity id where applicable
  - event timestamp
- Emit this event once per command execution.
- Ensure event payloads use stable ids, not display names.
- Avoid duplicate news/cards from a single command.

### Acceptance Criteria

- Every implemented command emits one command-domain event.
- Events are Blueprint-visible.
- Events are usable by UI/news/analytics subscribers.
- Existing subsystem delegates still work.
- `current.md` is updated.

### Primary Files

- `CommandDispatcherSubsystem`
- `MusicCommandResult`
- `UIManagerSubsystem`
- `current.md`

## Slice 13 - Command Regression Tests

### Prompt

Add automated regression coverage for command result behavior, invalid command failures, command-domain event emission, and UI notification routing.

### User-Facing Outcome

Core command behavior is protected from regressions as more systems move behind the dispatcher.

### Implementation Scope

- Add Unreal automation tests for:
  - command result helper constructors
  - invalid command failure without mutation
  - command-domain event emission
  - UI notification payload creation
- Prefer real command/result/subsystem APIs.
- Keep tests deterministic and free of fake gameplay data.

### Acceptance Criteria

- Tests compile in the Unreal automation framework.
- Tests verify structured failure output.
- Tests verify event/notification payloads are populated with stable fields.
- `current.md` is updated with command test status.

### Primary Files

- `Source/MusicManager/Private/Tests/...`
- `CommandDispatcherSubsystem`
- `UIManagerSubsystem`
- `MusicCommandResult`
- `current.md`

## Suggested Execution Order

1. Slice 1 - Command result and error model
2. Slice 2 - Dispatcher subsystem skeleton with real routing
3. Slice 3 - Sign artist command
4. Slice 4 - Reject artist command
5. Slice 5 - Start recording command
6. Slice 6 - Advance time command
7. Slice 7 - UI command panel integration
8. Slice 8 - Audition widget command integration
9. Slice 9 - Domain event emission
10. Slice 10 - Command regression coverage
11. Slice 11 - Command failure UI notifications
12. Slice 12 - Rich command domain events
13. Slice 13 - Command regression tests

## Definition Of Done

- Core implemented player actions go through typed commands.
- Commands validate before mutating state.
- Commands return structured results.
- Command failures are visible through a UI notification surface.
- Command outcomes emit typed domain events.
- Widgets no longer directly mutate core simulation state for covered actions.
- Unsupported command panel actions are disabled/hidden.
- Command regression tests compile.
- `current.md` accurately documents dispatcher coverage.
