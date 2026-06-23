# Goal
Implement backlog task **TASK-7.3.7 — “Validation should be server-style even in single-player”** for **ST-103 Command dispatcher for gameplay actions**.

The coding agent should harden the gameplay command path so that **all covered player actions are validated centrally and authoritatively in the command dispatcher/application layer**, even though the game is single-player and locally simulated.

This means:
- UI/widgets must not be trusted as the source of truth.
- Commands must be validated against current subsystem state at execution time.
- Invalid actions must fail gracefully with structured results.
- Covered subsystems should not rely on UI pre-filtering alone to enforce rules.
- Validation behavior should resemble a server-authoritative game flow, but entirely local.

Target outcome:
- A lightweight typed command dispatcher exists or is extended.
- Core commands perform precondition checks before mutating state.
- Results return structured success/failure, user-facing messages, and machine-readable error codes.
- Any direct widget-to-subsystem mutation for covered actions is removed or routed through dispatcher methods.
- Logging is added for rejected commands to aid debugging.

# Scope
Focus only on the “server-style validation” slice of **ST-103**, not the entire command system if it is not already present.

In scope:
- Identify existing gameplay actions already wired or partially wired for:
  - sign artist
  - start recording
  - schedule release
  - allocate marketing
  - plan tour
  - advance time
- Ensure each covered action validates in one authoritative place before state mutation.
- Introduce or standardize:
  - typed command payload structs
  - structured command result type
  - validation error enum/codes
- Add guardrails so invalid state transitions are rejected even if called from non-UI code.
- Update any existing UI call sites for covered actions to use dispatcher entry points instead of mutating subsystems directly.
- Add tests for validation failures and success paths where practical in the current workspace.

Out of scope:
- Building a generic reflection-based command bus.
- Full multiplayer/networking support.
- Large UI redesigns.
- New gameplay systems beyond what is needed to validate existing covered actions.
- Save/load migration work unless directly required by compilation.

# Files to touch
Inspect the repo first and adjust to actual names, but expect to touch files in these areas:

- **Command/application layer**
  - `Source/.../CommandDispatcherSubsystem.*`
  - or equivalent application/service layer files
  - any command/result shared header files

- **Gameplay subsystems owning business state**
  - artist manager subsystem
  - production/record manager subsystem
  - release/catalog subsystem
  - finance subsystem
  - tour subsystem
  - game time subsystem

- **UI integration points**
  - UI manager subsystem
  - widgets/viewmodels/presenters currently calling subsystem mutation methods directly

- **Shared types**
  - command payload structs
  - command result struct
  - error code enum
  - domain event payloads if needed

- **Tests**
  - existing unit/integration test project(s)
  - add focused tests around dispatcher validation behavior

- **Logging**
  - existing log category definitions or a command-dispatcher log category if appropriate

Do not invent broad new file trees if the project already has a pattern. Reuse existing architecture and naming.

# Implementation plan
1. **Survey the current implementation**
   - Find whether a command dispatcher already exists.
   - Identify current mutation paths for the covered actions.
   - Map where validation currently lives:
     - widget/UI only
     - subsystem only
     - nowhere
     - duplicated
   - Note any direct calls from UI into subsystem mutation methods that bypass validation.

2. **Define/standardize command contracts**
   - Introduce or refine typed command structs for covered actions.
   - Introduce a shared result type if missing, such as:
     - success flag
     - user-facing message
     - error code(s)
     - optional created/affected IDs
   - Add a validation error enum with practical values, e.g.:
     - `None`
     - `InvalidReference`
     - `InsufficientFunds`
     - `InvalidArtistState`
     - `ContractRequired`
     - `SongLocked`
     - `DateConflict`
     - `InvalidRecordState`
     - `InvalidFormatForEra`
     - `AvailabilityConflict`
   - Keep this lightweight and consistent with existing code style.

3. **Make dispatcher validation authoritative**
   - For each covered command, ensure the dispatcher:
     - resolves referenced entities by stable ID
     - checks existence and ownership/eligibility
     - checks state-machine preconditions
     - checks affordability where relevant
     - checks date/availability conflicts where relevant
     - only calls mutation logic after validation passes
   - Return structured failure instead of partial mutation or silent no-op.
   - Add clear user-facing messages suitable for UI display.

4. **Harden subsystem mutation boundaries**
   - If subsystems expose public mutation methods that can be called directly, do one of the following with minimal disruption:
     - make them internal/private/protected where feasible, or
     - rename/document them as internal execution methods intended only for dispatcher use, or
     - add defensive validation/assertion guards if visibility cannot be reduced safely
   - Goal: reduce accidental bypass of authoritative validation.
   - Do not over-refactor if the codebase is not ready; prefer pragmatic containment.

5. **Remove UI trust assumptions**
   - Update widgets/UI manager/presenters so they no longer directly mutate business state for covered actions.
   - UI may still pre-disable invalid buttons for UX, but dispatcher validation must remain the final authority.
   - Ensure UI handles structured failure results and surfaces messages instead of assuming success.

6. **Add logging/observability**
   - Log command execution attempts and validation failures in development-friendly form.
   - Include command type, relevant IDs, and failure code.
   - Avoid noisy per-frame logging.
   - Reuse existing log categories if present; otherwise add a focused one.

7. **Preserve event emission semantics**
   - Successful commands should still emit domain events used by UI/news systems.
   - Failed commands should not emit success events.
   - If there is an existing event pipeline, keep it intact.

8. **Add tests**
   - Add focused tests for dispatcher behavior, prioritizing:
     - invalid artist ID fails
     - insufficient funds fails
     - unsigned artist cannot start recording
     - locked/unavailable song cannot be reused
     - release cannot be scheduled before recording completion
     - invalid era format/channel fails
     - conflicting tour/recording dates fail
     - valid command succeeds and mutates state once
   - If automated tests are difficult in current UE/C++ setup, add the most realistic test coverage available in the workspace and document any gaps.

9. **Keep implementation incremental**
   - Prefer adapting existing code over introducing a large framework.
   - If all six listed commands are not yet implemented in the repo, apply the server-style validation pattern to the commands that do exist and leave clear TODOs for the remaining covered actions.

# Validation steps
1. **Build/test discovery**
   - Inspect solution/projects and determine the correct build path for this repo.
   - Start with the provided candidates:
     - `dotnet build`
     - `dotnet test`
   - If these are only for generated/build-rule artifacts and not the real gameplay code, document that and use the appropriate project/build target available in the repo.

2. **Static verification**
   - Confirm covered UI call sites now route through dispatcher methods rather than direct subsystem mutation.
   - Confirm dispatcher methods return structured results for both success and failure.
   - Confirm validation checks happen before mutation.

3. **Behavior verification**
   - Exercise at least these scenarios in tests or equivalent validation harness:
     - invalid IDs are rejected
     - insufficient funds are rejected
     - invalid state transitions are rejected
     - valid commands succeed
     - failed commands do not emit success events
     - successful commands do emit expected events

4. **Regression verification**
   - Ensure existing happy-path flows still compile and function.
   - Ensure no duplicate validation causes contradictory messages.
   - Ensure no partial state mutation occurs on failed commands.

5. **Document results**
   - In the final agent summary, list:
     - files changed
     - commands hardened
     - validation rules added
     - tests added/run
     - any commands still pending because the underlying feature is not yet present in the repo

# Risks and follow-ups
- **Repo mismatch risk:** Workspace hints show `.NET`, while architecture implies Unreal/C++. The agent must inspect the actual repo structure before coding and adapt to what exists.
- **Partial implementation risk:** Some covered commands may not exist yet. Apply the pattern to existing commands and avoid fabricating large unused systems.
- **Bypass risk:** If subsystem mutation methods remain public, future code may still bypass dispatcher validation. Minimize this where feasible and note follow-up work.
- **Validation duplication risk:** UI may still perform prechecks for UX. Keep dispatcher as final authority and avoid divergent rule implementations.
- **Testing risk:** Automated test infrastructure may be limited. Add the strongest feasible tests and clearly document any manual verification gaps.
- **Follow-up recommendation:** After this task, consider a small cleanup story to standardize all gameplay mutations behind dispatcher-only entry points and to formalize shared command/result/error types across the codebase.