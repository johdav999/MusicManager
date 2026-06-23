# Goal
Implement backlog task `TASK-ST-101` / story `ST-101 — Weekly simulation director` in the existing `MusicManager` codebase by introducing or formalizing a deterministic weekly simulation director centered on `UGameTimeSubsystem`.

The implementation must satisfy the story acceptance criteria from the backlog, even though the task header says no explicit acceptance criteria were provided. Specifically:

- `UGameTimeSubsystem` supports weekly advancement and emits `OnWeekAdvanced`, `OnMonthClosed`, and `OnYearAdvanced`.
- Simulation phases execute in a fixed deterministic order each week.
- Fast-forward can process multiple weeks without rebuilding UI every step.
- Existing monthly-only logic is migrated or wrapped so current systems still function during transition.

Use the provided architecture as the source of truth:
- Unreal Engine 5
- C++ core systems
- `UGameInstanceSubsystem`-based orchestration
- deterministic, data-driven simulation
- widgets remain thin
- simulation state is owned by subsystems
- preserve monthly summary UX while moving runtime cadence to weekly

Produce production-quality code, not a prototype.

# Scope
In scope for this task:

- Audit the current time/simulation flow and identify where monthly-only advancement exists.
- Refactor or extend `UGameTimeSubsystem` into a weekly simulation director without breaking current behavior.
- Add a deterministic weekly phase pipeline with explicit ordered phases.
- Add/bind week, month, and year advancement events/delegates.
- Add support for advancing one week and multiple weeks (fast-forward/batch mode).
- Ensure batch advancement can suppress per-step UI rebuild behavior and instead emit summarized/final notifications where appropriate.
- Wrap or adapt existing monthly systems so they still run correctly during the transition.
- Preserve compatibility as much as possible with current callers.
- Add logging/observability around weekly advancement and phase execution.
- Add or update automated tests if the project already has a test pattern; otherwise add lightweight validation hooks and clear manual verification steps.

Out of scope unless required to keep the build green:

- Full command dispatcher implementation from `ST-103`
- Full domain event pipeline cleanup from `ST-104`
- New UI screens
- Deep save/load schema changes beyond what is necessary to keep time state coherent
- Rewriting unrelated simulation subsystems
- Large-scale data model migrations unrelated to weekly time progression

Implementation constraints:

- Do not introduce per-frame simulation logic.
- Do not let widgets directly own simulation progression logic.
- Keep deterministic ordering across subsystems.
- Prefer compatibility shims over broad breakage.
- If architecture in code differs from the target architecture, move incrementally and document TODOs.

# Files to touch
Start by discovering the actual implementation, then update the most relevant files. Expect to touch files in areas like these, but verify exact paths/names first:

- Time/simulation subsystem files:
  - `Source/.../GameTimeSubsystem.h`
  - `Source/.../GameTimeSubsystem.cpp`
  - any existing date/time structs or helpers
- Any current monthly simulation orchestration files:
  - manager/subsystem classes currently called by time advancement
  - old monthly tick entry points
- Event/delegate definitions if centralized elsewhere
- UI manager or status/dashboard integration points only if needed to support batched fast-forward refresh behavior
- Save/load time snapshot files only if current time serialization must be updated for week-aware progression
- Test files if present:
  - unit/integration tests for time advancement
  - subsystem tests
- Logging category definitions if not already present

Before editing, inspect:
- `README.md`
- solution/project layout under the `.sln`
- all references to:
  - `UGameTimeSubsystem`
  - `AdvanceTime`
  - `AdvanceMonth`
  - `OnMonthClosed`
  - `OnYearAdvanced`
  - any monthly tick/update methods
  - any UI refresh calls triggered directly from time advancement

If Unreal source files are not present in the visible workspace and this repository is only partial, still prepare the implementation in the correct project structure if possible, but first confirm what code actually exists.

# Implementation plan
1. **Audit the current time flow**
   - Locate `UGameTimeSubsystem` and all callers.
   - Identify:
     - current date representation
     - current monthly advancement logic
     - existing delegates/events
     - subsystems invoked during time progression
     - any direct UI refreshes or widget rebuilds triggered per advancement step
   - Map current behavior against `ST-101` acceptance criteria.

2. **Define the weekly simulation model**
   - Introduce or formalize a week-based advancement API in `UGameTimeSubsystem`, such as:
     - `AdvanceOneWeek()`
     - `AdvanceWeeks(int32 NumWeeks, bool bBatchMode)`
     - or equivalent existing naming consistent with the codebase
   - Preserve or adapt existing monthly advancement APIs so old callers still work, ideally by routing them through weekly advancement.
   - Ensure the date model can determine:
     - current week boundary
     - month rollover
     - year rollover
   - If the project already has a custom game date struct, extend it rather than replacing it.

3. **Add explicit deterministic weekly phases**
   - Implement a fixed ordered phase pipeline inside `UGameTimeSubsystem`.
   - Use architecture-recommended order unless the existing codebase requires a compatible variant:
     1. trend drift
     2. artist condition/personality updates
     3. production progress
     4. release launch processing
     5. market exposure updates
     6. chart calculation
     7. tour show resolution
     8. finance posting
     9. critic/news generation
     10. UI notifications
   - If some subsystems do not exist yet, keep the phase slots explicit and no-op safely.
   - Avoid hidden side effects and unordered iteration where determinism matters.
   - Add clear internal helper methods like `RunWeeklySimulationPhase(...)` or dedicated phase functions.

4. **Emit advancement events**
   - Ensure `UGameTimeSubsystem` exposes and broadcasts:
     - `OnWeekAdvanced`
     - `OnMonthClosed`
     - `OnYearAdvanced`
   - Event payloads should include enough context for listeners to react deterministically, ideally current/previous date or week info if the codebase supports it.
   - Preserve existing event signatures where possible; add overloads/new delegates only when necessary.

5. **Bridge monthly-only logic**
   - Find systems that currently assume monthly ticks.
   - Wrap them so they still function during the transition, for example:
     - run monthly rollup logic only when a month boundary is crossed during weekly advancement
     - keep existing monthly summary generation on `OnMonthClosed`
     - adapt old `AdvanceMonth()` to call weekly advancement repeatedly until month close, if appropriate
   - Do not duplicate business logic in both weekly and monthly paths if avoidable.
   - Add comments/TODOs where a subsystem still needs a future native weekly implementation.

6. **Implement fast-forward batching**
   - Add support for processing multiple weeks in one call.
   - In batch mode:
     - run simulation for each week in deterministic order
     - avoid triggering expensive UI rebuilds every step
     - emit either:
       - per-week simulation events plus a batched UI refresh signal, or
       - existing per-week events but with a suppression/aggregation mechanism for UI listeners
   - Prefer minimal invasive changes:
     - a scoped batch flag in `UGameTimeSubsystem`
     - or a lightweight notification suppression mechanism
   - Do not skip simulation phases during fast-forward; only suppress presentation churn.

7. **Add observability**
   - Use or add a dedicated log category such as `LogMusicSimTime`.
   - Log:
     - start/end of weekly advancement
     - number of weeks processed
     - month/year rollovers
     - phase execution order
     - batch mode usage
   - In non-shipping/dev builds, include enough detail to verify deterministic sequencing.

8. **Keep compatibility with existing callers**
   - Update existing callers to use the new weekly API where appropriate.
   - If there is an existing “advance month” button or command path, keep it working by routing through the new system.
   - Avoid breaking signatures unless necessary; if changed, update all call sites.

9. **Testing**
   - Add tests if the repo has a test framework.
   - At minimum validate:
     - one week advancement updates date and emits `OnWeekAdvanced`
     - crossing a month emits `OnMonthClosed`
     - crossing a year emits `OnYearAdvanced`
     - phases execute in fixed order
     - fast-forward of N weeks processes all weeks and does not require per-step UI rebuild behavior
     - old monthly path still functions through compatibility wrapping
   - If automated tests are not practical in the current repo, add deterministic debug hooks and document manual verification.

10. **Document assumptions**
   - If the codebase lacks some target subsystems from the architecture, implement extension points and note follow-ups.
   - Keep comments concise and focused on why compatibility shims exist.

# Validation steps
1. **Code discovery**
   - Search the repo for:
     - `UGameTimeSubsystem`
     - `AdvanceMonth`
     - `AdvanceTime`
     - `OnMonthClosed`
     - `OnYearAdvanced`
     - direct UI refresh calls from time progression
   - Confirm actual source layout and adjust implementation accordingly.

2. **Build**
   - Run the most appropriate build command available for the workspace.
   - Since the workspace hint says `.net`, first inspect whether Unreal C++ sources are actually present.
   - If only solution-level validation is available, run:
     - `dotnet build`
   - If tests exist:
     - `dotnet test`

3. **Functional verification**
   - Verify advancing one week:
     - date advances correctly
     - weekly phases run in the expected order
     - `OnWeekAdvanced` fires exactly once
   - Verify month rollover:
     - `OnMonthClosed` fires when expected
     - monthly summary/legacy monthly logic still runs
   - Verify year rollover:
     - `OnYearAdvanced` fires when expected
   - Verify fast-forward:
     - advancing multiple weeks processes all intermediate weeks
     - simulation state matches repeated single-week advancement
     - UI-facing refresh behavior is batched/suppressed appropriately
   - Verify compatibility:
     - any existing monthly advancement entry point still works
     - existing systems depending on monthly logic still receive expected updates

4. **Determinism check**
   - Run the same multi-week advancement twice from the same starting state and confirm identical outputs/log ordering/state transitions.
   - If practical, compare logs or trace summaries.

5. **Regression check**
   - Confirm no per-frame simulation hooks were introduced.
   - Confirm widgets are not directly mutating simulation state as part of this change.
   - Confirm no obvious save/load time state regressions if time snapshot code exists.

# Risks and follow-ups
- **Workspace mismatch risk:** The provided workspace hints at `.NET`, while the architecture requires Unreal Engine C++. First verify whether the Unreal gameplay source is actually present. If not, implement only what the repository supports and clearly note the gap.
- **Legacy monthly coupling:** Existing systems may be tightly coupled to monthly advancement. Use compatibility wrappers now, but plan follow-up stories to convert each subsystem to native weekly processing.
- **UI coupling risk:** Current time advancement may directly trigger widget rebuilds. Minimize churn in this task; if suppression requires broader UI refactoring, add a follow-up under `EP-5`.
- **Event signature drift:** Adding richer event payloads may break listeners. Prefer additive changes or compatibility delegates.
- **Date model limitations:** If the current date representation is too coarse for weekly boundaries, extend it carefully rather than replacing it wholesale.
- **Partial subsystem availability:** Some architecture phases may not yet exist. Keep explicit no-op phase slots so the director remains stable and future stories can plug in cleanly.
- **Testing limitations:** If there is no existing Unreal automation test setup in the repo, add lightweight validation hooks/logging now and recommend follow-up automated coverage.

Recommended follow-ups after this task:
- `ST-103` command dispatcher integration for time advancement
- `ST-104` event pipeline cleanup so UI refresh batching is cleaner
- native weekly updates for artist/market/chart/finance subsystems where still monthly-wrapped
- save/load determinism tests once `ST-402`/`ST-403` are implemented