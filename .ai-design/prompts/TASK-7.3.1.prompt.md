# Goal
Implement backlog task **TASK-7.3.1 / ST-103 — Command dispatcher for gameplay actions** by adding a lightweight **application-layer command dispatcher subsystem** for core gameplay actions in the Unreal project.

The dispatcher must centralize validation and state mutation for these covered actions:

- sign artist
- start recording
- schedule release
- allocate marketing
- plan tour
- advance time

The implementation should align with the architecture direction:

- use a **typed command pattern**, not a generic reflection bus
- keep **widgets/UI thin**
- ensure **commands return structured success/failure results**
- ensure **command execution emits domain events**
- remove or deprecate any direct widget-to-subsystem mutation paths for the covered actions where currently present

This task is foundational. Favor a clean, minimal, extensible implementation over broad feature completeness.

# Scope
In scope:

- Add a new `UGameInstanceSubsystem` command dispatcher/application layer
- Define typed command payload structs for the covered actions
- Define a shared command result type with:
  - success flag
  - user-facing message
  - error code(s)
  - optional created/affected IDs where useful
- Route command execution into existing owning subsystems rather than duplicating business logic
- Add validation/precondition checks in the dispatcher and/or owning subsystem boundaries
- Emit typed domain events for successful command execution
- Update existing UI-facing call sites for covered actions to use the dispatcher instead of mutating business state directly
- Add logging for command execution and failures
- Add focused automated tests if the workspace already has a test pattern; otherwise add at least compile-safe validation hooks and clear TODOs

Out of scope unless required to make this compile:

- building a fully generic command bus
- broad UI redesign
- implementing all downstream simulation systems in full if some do not yet exist
- save/load changes beyond what is needed for compile compatibility
- deep event/news pipeline refactors beyond emitting the new events
- adding new gameplay depth beyond command orchestration

Implementation guidance for missing systems:

- If one or more target subsystems/actions do not yet exist in usable form, still add the command type and dispatcher entry point, but return a structured failure such as `NotImplemented` rather than inventing large placeholder systems.
- Prefer incremental compatibility shims over risky rewrites.

# Files to touch
Inspect the repo first and adjust to actual structure, but expect to touch files in areas like:

- **Subsystems / application layer**
  - add `UCommandDispatcherSubsystem` header/cpp
- **Shared gameplay types**
  - add command structs
  - add command result/error enums
  - add event payload structs if not already present
- **Existing manager subsystems**
  - artist manager
  - production/record manager
  - release/catalog manager
  - finance manager
  - tour manager
  - game time subsystem
- **UI orchestration / widgets / controllers**
  - any current direct mutation call sites for:
    - sign artist
    - start recording
    - schedule release
    - allocate marketing
    - plan tour
    - advance time
- **Event/news integration**
  - event subsystem or existing event delegate definitions
- **Logging**
  - add or extend a log category for command dispatching, or use an existing gameplay/UI category if that is the project convention
- **Tests**
  - any existing unit/integration/spec test project or Unreal automation test location

If the codebase already has naming conventions or folders for:
- `Commands`
- `Application`
- `Subsystems`
- `Events`
- `ViewModels`
follow those conventions instead of inventing new ones.

# Implementation plan
1. **Survey the current codebase**
   - Find the actual Unreal module structure and naming conventions.
   - Identify:
     - current subsystem classes
     - current UI command/action entry points
     - any existing event/delegate infrastructure
     - any existing result/error types
   - Map each covered action to its current mutation path.

2. **Design the minimal command API**
   Create typed structs for:
   - `FSignArtistCommand`
   - `FStartRecordingCommand`
   - `FScheduleReleaseCommand`
   - `FAllocateMarketingCommand`
   - `FPlanTourCommand`
   - `FAdvanceTimeCommand`

   Create a shared result type, e.g.:
   - `ECommandErrorCode`
   - `FCommandResult`

   Recommended fields:
   - `bool bSuccess`
   - `FText UserMessage`
   - `TArray<ECommandErrorCode>` or a primary code plus optional details
   - optional affected IDs such as `ArtistId`, `RecordId`, `ContractId`, `TourId`
   - optional `bNotImplemented` only if needed, otherwise use an error code

   Keep IDs aligned with the project’s current stable ID approach.

3. **Add the dispatcher subsystem**
   Implement a `UGameInstanceSubsystem`, e.g. `UCommandDispatcherSubsystem`, with explicit methods:
   - `ExecuteSignArtist(const FSignArtistCommand&)`
   - `ExecuteStartRecording(const FStartRecordingCommand&)`
   - `ExecuteScheduleRelease(const FScheduleReleaseCommand&)`
   - `ExecuteAllocateMarketing(const FAllocateMarketingCommand&)`
   - `ExecutePlanTour(const FPlanTourCommand&)`
   - `ExecuteAdvanceTime(const FAdvanceTimeCommand&)`

   Responsibilities:
   - validate command payload shape
   - resolve required subsystems
   - perform cross-subsystem precondition checks
   - call owning subsystem methods
   - emit success events
   - return structured results
   - log execution/failure

   Do **not** move all business logic into the dispatcher. It is an orchestration/application layer, not the owner of domain state.

4. **Define event emission for successful commands**
   Add typed events/delegates for covered actions if they do not already exist, such as:
   - `FArtistSignedEvent`
   - `FRecordingStartedEvent`
   - `FReleaseScheduledEvent`
   - `FMarketingAllocatedEvent`
   - `FTourPlannedEvent`
   - `FTimeAdvancedEvent` or rely on existing time events if already present

   Prefer existing event infrastructure if available.
   Event payloads should include stable IDs and relevant dates/amounts.

5. **Wire dispatcher to owning subsystems**
   For each action:
   - **Sign artist**
     - validate artist exists and is signable
     - validate affordability if advance/cost applies
     - call artist/contract/finance subsystem methods
     - emit signed event
   - **Start recording**
     - validate artist eligibility, song availability, funds
     - call production/record subsystem
     - emit recording started event
   - **Schedule release**
     - validate record state and release date/formats/regions
     - call release/record subsystem
     - emit release scheduled event
   - **Allocate marketing**
     - validate record/release exists, budget non-negative, funds/channels valid
     - call market/finance/release subsystem as appropriate
     - emit marketing allocated event
   - **Plan tour**
     - validate artist availability, date conflicts, affordability
     - call tour subsystem
     - emit tour planned event
   - **Advance time**
     - validate week count or cadence input
     - call `UGameTimeSubsystem`
     - preserve deterministic advancement behavior
     - avoid forcing UI rebuilds from inside dispatcher

   If subsystem APIs are too UI-shaped or mutation-heavy, add small internal methods to make them callable cleanly from the dispatcher.

6. **Refactor covered UI call sites**
   Replace direct state mutation for covered actions with dispatcher calls.
   This may include:
   - widgets
   - UI manager subsystem
   - player controller helpers
   - blueprint-callable wrappers

   Preferred pattern:
   - UI gathers input
   - UI calls dispatcher
   - UI receives `FCommandResult`
   - UI shows success/failure message
   - UI refreshes from events/read models, not direct mutation assumptions

   If full UI migration is too broad, at minimum:
   - update all obvious covered paths
   - add deprecation comments/TODOs on any remaining direct mutation path
   - ensure no primary covered flow bypasses the dispatcher

7. **Add logging and defensive error handling**
   Add a dedicated log category if appropriate, e.g. `LogMusicCommands`.
   Log:
   - command start
   - validation failure
   - subsystem missing/unavailable
   - successful execution
   - not-yet-implemented branches

   Use player-facing `FText` messages for result objects and developer-facing logs for diagnostics.

8. **Keep Blueprint/UI usability in mind**
   If the project exposes subsystem methods to Blueprints:
   - mark dispatcher methods appropriately
   - ensure command/result structs are `USTRUCT(BlueprintType)` if needed
   - keep API ergonomic for UMG integration

   But do not overcomplicate the design for Blueprint reflection if the project is currently C++-only.

9. **Add tests or verification coverage**
   If there is an existing automated test pattern:
   - add focused tests for dispatcher validation and success/failure routing
   - verify structured result behavior
   - verify events fire on success
   - verify invalid commands do not mutate state

   Candidate tests:
   - sign artist fails with insufficient funds
   - start recording fails for invalid artist state
   - schedule release fails before recording completion
   - allocate marketing rejects negative budget
   - advance time succeeds and triggers expected time event
   - successful command emits corresponding domain event

   If no test harness exists, add lightweight validation helpers and document follow-up test debt clearly.

10. **Document assumptions in code comments**
   Since acceptance criteria are minimal, leave concise comments where behavior is intentionally provisional, especially for:
   - not-yet-implemented subsystem integrations
   - temporary compatibility shims
   - remaining direct mutation paths to migrate later

# Validation steps
1. **Build/compile validation**
   - Run the most appropriate build command available in the workspace.
   - Start with:
     - `dotnet build`
   - If there are test projects:
     - `dotnet test`
   - Also ensure Unreal C++ code compiles according to the project’s normal build flow if available in the repo/tooling.

2. **Static code validation**
   Confirm:
   - new subsystem compiles
   - command/result/event structs are visible where needed
   - includes and module dependencies are correct
   - no circular dependencies were introduced

3. **Behavior validation for each covered command**
   Verify each dispatcher method:
   - returns structured success/failure
   - does not crash on missing/invalid IDs
   - logs failures
   - emits event on success
   - routes to owning subsystem rather than mutating unrelated state directly

4. **UI path validation**
   For each covered action, inspect and verify the primary UI flow now uses the dispatcher:
   - sign artist
   - start recording
   - schedule release
   - allocate marketing
   - plan tour
   - advance time

   Ensure widgets are no longer the direct owners of business mutations for these flows.

5. **Regression validation**
   Check that existing gameplay flows still function at a basic level:
   - artist signing still updates roster/contract/finance state
   - recording still starts and locks/associates songs as expected
   - release scheduling still updates record lifecycle
   - marketing allocation still updates spend/exposure inputs
   - tour planning still creates/plans a tour
   - time advancement still triggers simulation progression

6. **Event validation**
   Confirm successful commands produce domain events consumable by UI/news systems.
   If there is already an event subsystem, ensure integration is compatible and non-duplicative.

7. **Failure-path validation**
   Manually or via tests verify representative failures:
   - invalid artist ID
   - insufficient funds
   - invalid record state
   - negative marketing budget
   - date conflict for tour
   - invalid advance-time input

# Risks and follow-ups
- **Repo mismatch risk**
  - Workspace hints mention `.NET`, but the architecture is Unreal/C++.
  - First confirm actual source layout before implementing.
  - If Unreal source is missing or partial, implement only where the real code exists and document blockers.

- **Subsystem maturity risk**
  - Some target systems may not yet exist or may be incomplete.
  - Do not fabricate large systems just to satisfy dispatcher coverage.
  - Return structured `NotImplemented`/equivalent failures where necessary.

- **UI coupling risk**
  - Existing widgets may be tightly coupled to subsystem internals.
  - Use compatibility wrappers where needed, but keep the dispatcher as the new primary path.

- **Event duplication risk**
  - If subsystems already emit events, avoid double-emitting semantically identical events from both dispatcher and subsystem unless the project explicitly wants both.
  - Prefer one authoritative event source.

- **Validation ownership ambiguity**
  - Keep cross-subsystem orchestration checks in the dispatcher.
  - Keep domain invariants in owning subsystems too, so rules are not bypassable.

- **Follow-up recommendations**
  - Next story should likely tighten the domain event pipeline (`ST-104`) once command dispatch is in place.
  - Add broader automation coverage for command execution and event emission.
  - Migrate any remaining direct mutation paths not covered in this task.
  - Consider read-only view/projection structs for UI if current widgets still query too many subsystems directly.