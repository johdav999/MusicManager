# Goal
Implement backlog task `TASK-ST-103` / story `ST-103 — Command dispatcher for gameplay actions` by adding a lightweight typed command-dispatch layer for core gameplay actions in the Unreal project, so UI-triggered actions no longer directly mutate simulation/business state for covered flows.

The implementation must align with the provided architecture:
- use a central command/application layer
- keep widgets/UI thin
- validate commands in a server-style manner even in single-player
- return structured success/failure results with user-facing messages
- emit domain events after successful execution so UI/news systems can react

Covered actions for this task:
- sign artist
- start recording
- schedule release
- allocate marketing
- plan tour
- advance time

No reflection-based or overly generic bus is needed. Prefer explicit typed C++ structs and explicit dispatcher methods.

# Scope
Implement the minimum vertical slice needed to satisfy `ST-103` cleanly and safely in the current codebase.

In scope:
- Add a `UCommandDispatcherSubsystem` (or equivalent application-layer subsystem if naming conventions in repo differ).
- Define typed command payload structs for the six core actions.
- Define a shared structured command result type with:
  - success flag
  - user-facing message
  - machine-readable error code(s)
  - optional created/affected IDs where practical
- Route covered actions through dispatcher validation + execution.
- Ensure successful command execution emits domain events through the project’s existing event mechanism, or add a minimal typed event emission path if missing.
- Refactor existing UI/business entry points for covered actions so they call the dispatcher instead of mutating subsystems directly.
- Add logging for command execution and validation failures.

Out of scope unless required by existing code structure:
- building a generic command registry
- reflection-driven dispatch
- broad UI redesign
- implementing every downstream subsystem in full if some actions are not yet fully supported
- full news generation logic beyond emitting events/hooks
- save/load changes beyond any compile-required wiring
- adding new gameplay features beyond command orchestration

If some covered actions are not fully implemented in downstream systems yet, still add dispatcher methods and return a structured “not implemented / unavailable” failure result rather than bypassing the pattern.

# Files to touch
Inspect the repo first and update this list to actual files before editing. Prefer modifying existing systems over introducing parallel patterns.

Likely files/folders to touch:
- `Source/.../` gameplay subsystem headers/cpps
- `Source/.../UI/...` widgets or UI manager classes that currently invoke business logic directly
- `Source/.../...Subsystem.h/.cpp` for:
  - game time
  - artist/contracts
  - production/recording
  - release/marketing
  - tour
  - event/news/UI manager integration
- module build file if new subsystem/classes require registration/includes

Expected new files:
- `Source/.../Commands/CommandTypes.h`
- `Source/.../Commands/CommandResult.h`
- `Source/.../Commands/CommandDispatcherSubsystem.h`
- `Source/.../Commands/CommandDispatcherSubsystem.cpp`

Possible additional new files if event types are missing:
- `Source/.../Events/CommandEvents.h`

Before coding, identify and document the actual equivalents for:
- current UI manager subsystem
- current game time subsystem
- current artist/contract subsystem
- current recording/release subsystem
- current finance subsystem
- current tour subsystem
- current event/news subsystem

# Implementation plan
1. **Survey current architecture**
   - Find existing Unreal module source folders; ignore the `.NET` workspace hint if the actual gameplay code is UE C++.
   - Identify current subsystem names and where UI currently mutates gameplay state directly.
   - Search for likely patterns:
     - `UGameInstanceSubsystem`
     - `Subsystem`
     - `SignArtist`
     - `Record`
     - `Release`
     - `Marketing`
     - `Tour`
     - `AdvanceTime`
     - widget button handlers calling subsystem mutators
   - Determine whether there is already an event bus, delegates, or UI manager subscription model.

2. **Add shared command result model**
   - Create a lightweight result type, e.g.:
     - `ECommandErrorCode`
     - `FCommandResult`
   - Include:
     - `bool bSuccess`
     - `FText UserMessage`
     - `TArray<FName>` or enum-backed error codes
     - optional `PrimaryEntityId` / `CreatedEntityId`
   - Add helper factories like:
     - `Success(...)`
     - `Failure(...)`
   - Keep it Blueprint-friendly only if existing UI requires it; otherwise native C++ is fine.

3. **Define typed command payload structs**
   - Add explicit `USTRUCT`s for:
     - `FSignArtistCommand`
     - `FStartRecordingCommand`
     - `FScheduleReleaseCommand`
     - `FAllocateMarketingCommand`
     - `FPlanTourCommand`
     - `FAdvanceTimeCommand`
   - Use stable IDs, not display names.
   - Keep fields minimal and aligned with architecture/backlog.
   - Include only data needed for validation/execution.

4. **Implement `UCommandDispatcherSubsystem`**
   - Make it a `UGameInstanceSubsystem`.
   - Add explicit methods:
     - `ExecuteSignArtist(const FSignArtistCommand&)`
     - `ExecuteStartRecording(const FStartRecordingCommand&)`
     - `ExecuteScheduleRelease(const FScheduleReleaseCommand&)`
     - `ExecuteAllocateMarketing(const FAllocateMarketingCommand&)`
     - `ExecutePlanTour(const FPlanTourCommand&)`
     - `ExecuteAdvanceTime(const FAdvanceTimeCommand&)`
   - Dispatcher responsibilities:
     - validate command payload shape
     - resolve required subsystems
     - check preconditions
     - call owning subsystem(s)
     - emit domain events on success
     - return structured result
   - Add a dedicated log category if one does not already exist, e.g. `LogMusicCommands`.

5. **Validation rules**
   Implement server-style validation in dispatcher even for local play. At minimum:
   - **Sign artist**
     - artist exists
     - artist is available/unsigned/not already under active contract
     - player can afford advance if finance exists
   - **Start recording**
     - artist exists and is eligible
     - songs/record inputs valid
     - no invalid lock/conflict state
     - funds sufficient if cost applies
   - **Schedule release**
     - record exists
     - record is in a releasable state
     - date/formats/regions valid
   - **Allocate marketing**
     - target release/record exists
     - amount non-negative
     - funds sufficient
   - **Plan tour**
     - artist exists and available
     - dates/venues valid if supported
     - no obvious conflicts
   - **Advance time**
     - weeks > 0 or valid advancement request
   - If downstream systems already validate too, keep those checks; dispatcher validation should be additive, not a replacement.

6. **Integrate with existing subsystems**
   - Do not move all business logic into dispatcher.
   - Dispatcher should orchestrate and centralize entry points while subsystems remain owners of business state.
   - Reuse existing subsystem methods where possible.
   - If existing subsystem methods are too UI-coupled, add thin internal methods that accept typed IDs/parameters and return deterministic results.

7. **Emit domain events**
   - For each successful command, emit or forward a typed event/hook consistent with current architecture.
   - If a full event bus already exists, use it.
   - If not, add minimal typed delegates/events sufficient for this story.
   - Events should include stable IDs and dates where practical.
   - Suggested events/hooks:
     - artist signed
     - recording started
     - release scheduled / released-planned
     - marketing allocated
     - tour planned
     - time advanced
   - Ensure UI/news systems can subscribe without direct state mutation coupling.

8. **Refactor UI entry points**
   - Find widgets/UI manager methods for covered actions.
   - Replace direct subsystem mutation calls with dispatcher calls.
   - Preserve current UX by surfacing `FCommandResult.UserMessage` on success/failure.
   - If widgets currently call multiple subsystems directly, collapse that into one dispatcher call.
   - Keep UI thin: gather input, call dispatcher, display result, refresh via events.

9. **Backward compatibility / partial implementation handling**
   - If some covered actions are not yet fully wired in the project:
     - still expose dispatcher methods
     - return a structured failure like “Action not available yet”
     - log clearly
   - Do not leave any covered UI path bypassing dispatcher if that path is active.

10. **Code quality**
   - Keep implementation lightweight and explicit.
   - Avoid introducing templates, reflection, or generic buses unless already established in repo.
   - Follow existing naming/style/macros.
   - Add comments only where behavior is non-obvious.

11. **Document assumptions in code or task notes**
   - If subsystem ownership differs from architecture doc, adapt to repo reality and note it in final summary.
   - If some actions map to existing methods with different names, preserve existing domain naming where sensible.

# Validation steps
1. **Static/code validation**
   - Build the solution/project using the appropriate command for the actual repo setup.
   - If UE C++ project files are present, ensure headers/cpps compile cleanly.
   - If only `.sln`/workspace build commands are available here, run the best available compile validation and report limitations.

2. **Search-based validation**
   - Confirm covered UI paths no longer directly mutate simulation state for implemented actions.
   - Search for direct calls from widgets/UI classes into business mutators for:
     - sign artist
     - start recording
     - schedule release
     - allocate marketing
     - plan tour
     - advance time
   - Replace or document any remaining intentional exceptions.

3. **Behavior validation**
   For each covered action, verify:
   - dispatcher method is callable
   - invalid input returns structured failure
   - valid input returns structured success
   - success path triggers downstream subsystem mutation
   - success path emits event/hook
   - UI can consume/display result message

4. **Logging validation**
   - Ensure validation failures log useful warnings.
   - Ensure successful command execution logs action + key IDs at verbose/log level.

5. **Regression validation**
   - Existing flows still work after refactor.
   - Time advancement still functions through dispatcher.
   - No widget compile errors from signature changes.
   - No circular subsystem dependencies introduced.

6. **If tests exist**
   - Add or update focused tests for dispatcher validation/execution.
   - Prefer small unit/integration tests around:
     - invalid sign artist
     - successful sign artist
     - invalid recording start
     - invalid advance time
   - Run available test command(s), e.g. `dotnet test` only if relevant to actual test projects in workspace.

# Risks and follow-ups
- **Repo mismatch risk:** workspace hints mention `.NET`, but task architecture is Unreal C++. First confirm actual source layout before implementing.
- **Subsystem naming drift:** actual code may not match architecture names exactly; adapt to existing ownership rather than forcing a parallel architecture.
- **Partial downstream support:** some actions may not yet have complete subsystem implementations. Use structured failure results instead of bypassing dispatcher.
- **UI coupling risk:** widgets may currently depend on direct mutation side effects. After refactor, ensure refresh happens via events or explicit UI manager updates.
- **Event pipeline incompleteness:** if domain events are not yet standardized, implement the smallest typed event hook needed now and avoid overengineering before `ST-104`.
- **Duplicate validation risk:** dispatcher and subsystem validation may overlap. Prefer simple dispatcher prechecks plus authoritative subsystem checks.
- **Blueprint exposure risk:** if current UI is Blueprint-driven, command/result structs and dispatcher methods may need `BlueprintType`/`BlueprintCallable`.
- **Follow-up for ST-104:** after this task, domain event routing should be cleaned up and standardized so UI/news consumption is fully decoupled.
- **Follow-up for ST-502/ST-503:** artist command panels and planner screens should be fully routed through dispatcher once those screens are implemented/refined.