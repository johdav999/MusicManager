# Goal
Implement **TASK-7.3.4** for **ST-103 — Command dispatcher for gameplay actions** by ensuring that **command execution emits typed domain events** that can be consumed by **UI** and **news/event systems**.

The implementation should fit the existing architecture direction:

- Unreal Engine gameplay architecture
- typed command dispatcher/application layer
- subsystems own business state
- widgets remain thin
- commands mutate state
- events notify UI/news
- no reflection-heavy generic bus

Deliver a lightweight but extensible event emission path so that when covered commands execute successfully or fail in meaningful ways, the system can publish domain events with stable IDs and enough context for downstream consumers.

# Scope
Focus only on the backlog slice:

- Add or extend the command dispatcher so covered gameplay commands emit domain events.
- Ensure emitted events are typed, serializable-friendly `USTRUCT` payloads where practical.
- Ensure UI/news-facing systems have a clear subscription/consumption path.
- Keep implementation lightweight and local to current code patterns.

Covered command areas from ST-103:

- sign artist
- start recording
- schedule release
- allocate marketing
- plan tour
- advance time

Expected behavior:

- command executes
- validation occurs centrally
- result is returned as structured success/failure
- on successful mutation, one or more domain events are emitted
- UI/news systems can subscribe without directly owning business logic

Out of scope unless required by existing code coupling:

- full generic event bus framework
- broad UI refactors beyond wiring subscriptions
- save/replay of domain events
- implementing all missing commands from scratch if some do not yet exist
- ST-104 full event pipeline cleanup beyond what is necessary to support this task

# Files to touch
Inspect the repo first and adjust to actual names, but prioritize files/classes matching these responsibilities:

- Command dispatcher subsystem/application layer
  - likely something like `UCommandDispatcherSubsystem`
  - command result structs
  - command payload structs
- Domain event definitions
  - add new typed event payload structs if not already present
  - add event delegate declarations / event publishing API
- UI manager subsystem
  - likely `UUIManagerSubsystem`
  - subscribe to command/domain events
  - trigger lightweight refresh/notification hooks
- Event/news subsystem
  - likely `UEventSubsystem`
  - subscribe to domain events and translate to player-facing news items
- Relevant owning subsystems only if needed to expose IDs/dates/context for event payloads
  - artist/contracts
  - production/recording
  - release
  - marketing/market
  - tour
  - time
- Tests
  - add or extend automated tests around command execution and event emission
- Logging categories if missing for observability

If the project is not yet using UE C++ in this workspace, still implement against the actual codebase conventions you find. Do not invent parallel architecture if an equivalent pattern already exists.

# Implementation plan
1. **Discover current command and event architecture**
   - Search for:
     - command dispatcher classes
     - command result structs
     - existing delegates/events
     - UI manager subscriptions
     - event/news subsystem hooks
   - Identify whether there is already:
     - a central dispatcher
     - per-command methods
     - a shared event source
     - existing typed event payloads
   - Reuse existing patterns first.

2. **Define/standardize domain event payloads for command outcomes**
   Add typed payload structs for the command-driven state changes that UI/news care about. Prefer stable IDs and simulation dates, not display-name-only payloads.

   Minimum event set to support this task, using actual project naming conventions:

   - `FArtistSignedEvent`
   - `FRecordingStartedEvent`
   - `FReleaseScheduledEvent` or `FRecordReleasedEvent` if scheduling is not yet distinct
   - `FMarketingAllocatedEvent`
   - `FTourPlannedEvent`
   - `FGameWeekAdvancedEvent` or reuse existing week-advance event if already present

   Each payload should include only the fields needed for downstream refresh/news generation, e.g.:

   - entity IDs (`ArtistId`, `RecordId`, `TourId`, `LabelId`)
   - relevant date/week
   - command-relevant summary values (budget amount, regions count, etc.)
   - optional event key/category if the current event subsystem benefits from it

   Keep payloads `USTRUCT(BlueprintType)` if that matches project style and helps UMG integration.

3. **Add a lightweight domain event publishing surface**
   In the command dispatcher or a dedicated event source owned by a subsystem, expose typed multicast delegates or equivalent publish methods.

   Preferred lightweight pattern:
   - one dispatcher-owned event hub or subsystem-owned delegates
   - strongly typed per-event delegates
   - no stringly typed routing for business events

   Example shape only; adapt to repo style:
   - `OnArtistSigned`
   - `OnRecordingStarted`
   - `OnReleaseScheduled`
   - `OnMarketingAllocated`
   - `OnTourPlanned`
   - `OnCommandExecuted` only if already useful, but do not replace typed events with a generic blob

4. **Emit events from successful command execution paths**
   Update each covered command execution method so that:
   - validation happens first
   - state mutation happens through owning subsystem
   - structured result is returned
   - typed domain event is emitted only after successful mutation
   - failed validation returns a failure result and does not emit success events

   For each command:
   - **Sign artist**
     - emit after contract creation / finance posting succeeds
   - **Start recording**
     - emit after recording intent/state and song locks are committed
   - **Schedule release**
     - emit after release state/date/regions/formats are committed
   - **Allocate marketing**
     - emit after finance/marketing state is committed
   - **Plan tour**
     - emit after tour entity/show plan is committed
   - **Advance time**
     - if command dispatcher wraps time advancement, ensure existing week/month/year events remain available and are surfaced consistently to UI/news consumers

5. **Wire UI manager consumption**
   Update `UUIManagerSubsystem` or equivalent to subscribe to the new typed events.

   Keep UI behavior thin:
   - trigger targeted refreshes or invalidate relevant view models
   - enqueue notifications/toasts if that pattern exists
   - avoid business logic in UI manager

   Examples:
   - artist signed → refresh roster/dashboard/artist detail if selected
   - recording started → refresh production panels
   - release scheduled → refresh release planner/dashboard
   - marketing allocated → refresh finance/release detail
   - tour planned → refresh tour planner/dashboard
   - week advanced → batch summary refresh

6. **Wire news/event subsystem consumption**
   Update `UEventSubsystem` or equivalent to subscribe to the same domain events and translate them into player-facing news items where appropriate.

   Keep this translation layer separate from command execution:
   - command emits domain event
   - event subsystem decides whether/how to create news
   - dedupe if existing news keys are supported

   Not every event must become headline news, but the consumption path must exist.

7. **Preserve structured command results**
   Ensure command methods still return structured results with:
   - success flag
   - user-facing message
   - error code(s) if present

   Do not replace result handling with event-only behavior. Events complement results; they do not replace them.

8. **Add logging/observability**
   Add debug logs around command execution and event emission using existing categories or introduce appropriate ones if missing.

   Log at least:
   - command start/end
   - validation failure reason
   - emitted event type and key IDs

9. **Add automated tests**
   Add or extend tests to verify:
   - successful command emits expected event exactly once
   - failed command does not emit success event
   - event payload contains stable IDs and expected values
   - UI/news subscribers can receive the event without direct state mutation coupling

   Prefer focused unit/integration tests around dispatcher behavior rather than broad UI tests.

10. **Document assumptions in code comments**
   Where architecture is still evolving, leave concise comments/TODOs noting:
   - this is command-driven domain event emission for ST-103
   - broader event pipeline cleanup belongs to ST-104
   - payloads should remain stable-ID based

# Validation steps
1. **Codebase inspection**
   - Confirm actual command dispatcher and subsystem names.
   - Confirm whether event delegates already exist and reuse them where possible.

2. **Build**
   - Run:
     - `dotnet build`
   - If there are test projects:
     - `dotnet test`

3. **Behavior verification**
   For each implemented command path:
   - execute a valid command
   - assert structured success result
   - assert corresponding domain event fired
   - assert payload contains expected IDs/date/value fields

4. **Negative-path verification**
   - execute invalid command inputs
   - assert failure result
   - assert no success domain event emitted

5. **Consumer verification**
   - verify `UUIManagerSubsystem` or equivalent receives/subscribes to emitted events
   - verify `UEventSubsystem` or equivalent receives/subscribes to emitted events
   - verify no direct widget/business-state mutation is introduced

6. **Regression check**
   - ensure existing command flows still work
   - ensure time advancement events are not duplicated unexpectedly
   - ensure event emission happens after successful state mutation, not before

7. **Logging sanity**
   - confirm logs are informative but not noisy
   - confirm event emission logs include stable IDs rather than display names only

# Risks and follow-ups
- **Repo mismatch risk:** Workspace hints show `.NET`, while architecture targets Unreal C++. Inspect actual project contents before implementing. Follow the real codebase conventions if this repo is a tooling/support project rather than the UE game module.
- **Partial command coverage:** Some commands may not yet exist. If so, wire event emission only for existing covered commands and leave clear TODOs for missing command implementations.
- **Duplicate event sources:** Time advancement may already emit events from `UGameTimeSubsystem`. Reuse existing authoritative events instead of emitting duplicate week/month events from multiple places.
- **UI coupling risk:** Do not let UI manager start performing business calculations in response to events; it should only refresh projections/navigation/notifications.
- **News spam risk:** Not every domain event should become a visible news item. Keep translation/deduping in the event/news subsystem.
- **Ordering risk:** Emit events only after state mutation is committed, especially for commands that touch multiple subsystems like signing or recording.
- **Follow-up for ST-104:** A later cleanup should centralize domain event definitions, standardize event replay/serialization expectations, and fully separate simulation events from presentation concerns.