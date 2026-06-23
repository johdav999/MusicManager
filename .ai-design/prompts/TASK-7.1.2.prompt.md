# Goal
Implement backlog task **TASK-7.1.2 — Simulation phases execute in a fixed order each week** for story **ST-101 Weekly simulation director**.

The coding agent should update the weekly simulation orchestration so that advancing one in-game week always runs the same ordered set of simulation phases, with deterministic sequencing and clear extension points for subsystem participation.

This task is specifically about **fixed weekly phase ordering**, not the full weekly simulation feature set. However, the implementation should fit the architecture direction where `UGameTimeSubsystem` acts as the simulation director and coordinates subsystem work in a predictable pipeline.

# Scope
In scope:
- Add or formalize a **weekly simulation phase pipeline** in `UGameTimeSubsystem` or the existing time/simulation orchestration class.
- Ensure weekly advancement executes phases in a **single fixed order** every time.
- Make the phase order explicit in code via enum/constants/ordered array rather than implicit call order scattered across methods.
- Provide a clean mechanism for current systems to hook into the weekly phases without introducing hidden ordering.
- Emit/log enough information to verify the order in development.
- Preserve deterministic behavior for single-player local simulation.

Out of scope unless required by existing code structure:
- Full implementation of every subsystem phase listed in architecture.
- UI work beyond any existing event emissions already triggered by time advancement.
- Save/load changes.
- Fast-forward batching optimization beyond not breaking current behavior.
- Refactoring unrelated monthly logic except where needed to keep weekly advancement working.

Architecture constraints to follow:
- Keep orchestration in the **simulation/time subsystem**, not widgets.
- Prefer **typed phase definitions** over stringly-typed dispatch.
- Avoid per-frame simulation logic.
- Keep implementation compatible with future weekly phases such as:
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

If the current codebase is still monthly-first, wrap/adapt existing logic so the weekly pipeline exists now without forcing a full subsystem rewrite.

# Files to touch
Inspect first, then modify only what is necessary. Likely targets include:

- `README.md` only if there is an established developer-facing architecture note section that must stay current.
- The Unreal C++ module files containing:
  - `UGameTimeSubsystem`
  - any simulation director/time advancement implementation
  - related delegates/events for week/month/year advancement
  - any subsystem registration or orchestration helpers
- Relevant headers/cpps likely named similar to:
  - `GameTimeSubsystem.h`
  - `GameTimeSubsystem.cpp`
  - `*SimulationDirector*`
  - `*Time*Subsystem*`
- Existing tests if present, or add new tests in the project’s current test location.
- Logging category definitions if a dedicated time/simulation log category already exists or should be extended.

Before editing, search for:
- `UGameTimeSubsystem`
- `OnWeekAdvanced`
- `OnMonthClosed`
- `OnYearAdvanced`
- `AdvanceTime`
- `AdvanceWeek`
- monthly tick/update methods
- any direct subsystem calls made during time progression

# Implementation plan
1. **Discover current orchestration**
   - Find the current in-game time advancement path.
   - Identify where a week or month is advanced today.
   - Map all simulation side effects currently triggered by time progression.
   - Determine whether weekly advancement already exists but lacks explicit phase ordering, or whether monthly logic must be wrapped.

2. **Introduce an explicit weekly phase model**
   - Add a strongly typed enum for weekly phases, e.g. `EMusicSimulationPhase` or similar.
   - Define the canonical order in one place, preferably as:
     - enum declaration
     - ordered static array/list
     - helper for display/logging
   - Recommended initial phase list:
     1. TrendUpdate
     2. ArtistUpdate
     3. ProductionProgress
     4. ReleaseProcessing
     5. MarketUpdate
     6. ChartCalculation
     7. TourResolution
     8. FinanceSettlement
     9. EventNewsGeneration
     10. Notifications
   - If the current codebase cannot support all these immediately, still define the full ordered phase list and allow no-op/default handling for phases not yet implemented.

3. **Centralize weekly execution in one method**
   - Create or refactor to a single orchestration method such as:
     - `AdvanceOneWeek()`
     - `RunWeeklySimulation()`
     - `ExecuteWeeklyPhases()`
   - This method should:
     - advance/prepare the date as appropriate for current code conventions
     - iterate phases in fixed order
     - call phase handlers
     - emit existing week/month/year events at the correct points
   - Avoid scattered direct calls from multiple places that could reorder behavior.

4. **Implement phase dispatch cleanly**
   - Use one of these patterns:
     - `switch` on enum in a single executor method
     - ordered array of phase descriptors with bound handlers
   - Prefer readability and determinism over abstraction.
   - Example structure:
     - `for (Phase : OrderedPhases) { ExecutePhase(Phase); }`
   - Each phase handler should be isolated in a named method, even if currently a no-op or wrapper around existing monthly logic.

5. **Bridge existing systems safely**
   - If current systems are monthly-only:
     - wrap them in the appropriate weekly phase handler temporarily
     - only run monthly rollup logic when the date crosses a month boundary
   - If current systems already update during time advancement:
     - move those calls into the correct phase handler
   - Do not duplicate execution across old and new paths.

6. **Preserve and clarify event emission**
   - Ensure `OnWeekAdvanced` still fires once per completed week.
   - Ensure `OnMonthClosed` and `OnYearAdvanced` still fire when boundaries are crossed.
   - Keep event timing consistent and document in code comments whether they occur:
     - before phase execution
     - after all weekly phases complete
   - Preferred approach: weekly simulation phases complete first, then week/month/year events broadcast based on resulting date state, unless current code semantics clearly require otherwise.

7. **Add development logging/assertability**
   - Add logs under the time/simulation log category for:
     - week start/end
     - each phase execution in order
   - Keep logs lightweight but sufficient to verify deterministic ordering.
   - Example:
     - `Running weekly simulation for YYYY-WW`
     - `Executing phase: ProductionProgress`
   - If there is a debug/dev-only path, use it for more verbose traces.

8. **Add tests or verification hooks**
   - If automated tests exist:
     - add a test that advancing one week records the exact phase order
     - add a test that repeated weekly advancement uses the same order
     - add a test that month/year boundary events still fire correctly
   - If no test framework is available in the Unreal module:
     - add minimal testable helper methods and document manual verification steps in code comments or PR notes
   - Prefer a small deterministic unit/integration test over broad coverage.

9. **Keep code future-ready**
   - Make it easy for future subsystem stories to plug into the correct phase without changing ordering semantics.
   - Avoid exposing phase mutation to UI or arbitrary runtime registration unless such a mechanism already exists and is safe.
   - The order should be authoritative and owned by the simulation director.

10. **Document intent inline**
   - Add concise comments near the phase order definition explaining:
     - why order is fixed
     - that new simulation work must be assigned to a specific phase
     - that hidden side effects outside the pipeline should be avoided

# Validation steps
Run the most relevant validation available in the workspace after implementation.

1. **Build / compile**
   - Start with:
     - `dotnet build`
   - If the Unreal project has a more appropriate build path available in the repo, use that too, but at minimum run the provided candidate command.

2. **Tests**
   - If tests exist or are added:
     - `dotnet test`

3. **Code-level verification**
   - Confirm there is exactly one authoritative weekly phase order definition.
   - Confirm weekly advancement iterates that order every time.
   - Confirm no old time-advance path still performs duplicate simulation work outside the phase pipeline.

4. **Behavior verification**
   - Advance one week and verify logs show phases in the expected order.
   - Advance multiple weeks and verify the order is identical each week.
   - Advance across a month boundary and verify:
     - weekly phases still run once
     - `OnMonthClosed` still fires correctly
   - Advance across a year boundary and verify:
     - weekly phases still run once
     - `OnYearAdvanced` still fires correctly

5. **Regression checks**
   - Ensure existing monthly summary behavior is not broken.
   - Ensure no UI-triggered direct simulation mutation was introduced.
   - Ensure deterministic behavior is preserved: same starting state should produce same phase execution order and same outputs.

6. **Implementation summary**
   - In the final agent output, include:
     - files changed
     - the fixed phase order implemented
     - whether any phases are currently no-op wrappers
     - tests run and results
     - any assumptions made due to missing subsystem implementations

# Risks and follow-ups
- **Risk: current code may have hidden side effects during time advancement**
  - Mitigation: search for all time/date mutation and simulation calls before refactoring.
- **Risk: monthly-only logic may be tightly coupled**
  - Mitigation: wrap existing monthly behavior inside explicit weekly phase handlers and month-boundary checks rather than rewriting everything now.
- **Risk: duplicate execution after refactor**
  - Mitigation: remove or redirect old orchestration paths so only the new weekly pipeline drives simulation.
- **Risk: event timing changes could affect UI**
  - Mitigation: preserve existing delegate semantics where possible and note any intentional timing changes.
- **Risk: no existing automated test harness**
  - Mitigation: add deterministic helper methods and strong logging for manual verification.

Follow-ups after this task:
- Hook each simulation subsystem into its proper weekly phase as later stories are implemented.
- Add fast-forward batching so multiple weeks can process without rebuilding UI every step.
- Add richer observability/debug traces for per-phase timing and outputs.
- Migrate remaining monthly-only systems into weekly-compatible logic while preserving monthly summaries.