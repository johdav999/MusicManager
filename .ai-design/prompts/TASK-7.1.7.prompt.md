# Goal

Implement backlog task **TASK-7.1.7 — Avoid per-frame simulation logic** for story **ST-101 Weekly simulation director**.

The coding agent should refactor the project so that **core simulation/business logic no longer runs from per-frame `Tick`, widget refresh loops, or actor frame updates**. Simulation must instead advance through **explicit weekly/monthly progression entry points** owned by the simulation/time orchestration layer, consistent with the architecture direction:

- deterministic weekly cadence
- fixed simulation phase ordering
- event-driven UI refresh
- fast-forward support without per-frame UI rebuilds
- compatibility with any existing monthly-only logic during transition

There are no explicit acceptance criteria on the task itself, so use the story and architecture as the source of truth. The implementation should be **minimal, safe, and incremental**, avoiding broad rewrites unless required to remove frame-driven simulation behavior.

# Scope

In scope:

- Identify all places where simulation/business state changes are driven every frame, including:
  - `Tick`
  - timer callbacks used as pseudo-frame simulation
  - widget polling/refresh loops that mutate state
  - actor/component updates that advance game time or simulation state
- Move those mutations behind explicit time-advance methods, ideally in or coordinated by:
  - `UGameTimeSubsystem`
  - a simulation director/orchestration method
- Ensure simulation progression happens only when:
  - player advances time
  - fast-forward processes one or more weeks
  - explicit load/rebuild hooks recompute derived state without re-simulating
- Preserve or add deterministic phase ordering for weekly advancement.
- Ensure UI becomes event-driven/read-only where touched by this task.
- Add lightweight logging/observability around time advancement and removed frame-driven paths.

Out of scope unless required for compilation or to preserve behavior:

- Full command bus implementation
- Full event subsystem redesign
- Large UI redesigns
- Save schema expansion
- New gameplay systems beyond what is needed to stop per-frame simulation logic
- Rewriting unrelated Tick-based presentation/animation behavior that is not business simulation

# Files to touch

Start by inspecting and likely touching files related to time progression, simulation orchestration, and any frame-driven managers/widgets. Use the actual repo structure, but prioritize files matching these responsibilities:

- `UGameTimeSubsystem` implementation/header
- Any simulation director or orchestration class
- Subsystems that currently mutate business state from `Tick`
- Widgets or UI manager classes that currently:
  - poll simulation every frame
  - trigger simulation side effects during refresh
- Any actor/component classes used as simulation hosts
- Logging/category definitions if needed
- Automated tests covering time advancement, if present

Also inspect project-wide for these patterns before editing:

- `Tick(`
- `NativeTick(`
- `SetTimer(`
- `AdvanceTime`
- `AdvanceWeek`
- `AdvanceMonth`
- `OnWeekAdvanced`
- `OnMonthClosed`
- `OnYearAdvanced`
- direct state mutation inside widgets
- any monthly simulation entry points that need wrapping for weekly orchestration

Prefer the smallest set of files necessary. Do not touch generated, intermediate, or build output files.

# Implementation plan

1. **Audit current simulation entry points**
   - Search the codebase for all frame-driven update paths.
   - Classify each occurrence as:
     - business simulation/state mutation
     - UI refresh only
     - presentation/animation only
   - Focus only on business simulation and state mutation for this task.
   - Document in code comments or PR notes which frame-driven paths were removed or intentionally left alone.

2. **Establish a single authoritative simulation advancement path**
   - Ensure there is one explicit method for advancing simulation, ideally something like:
     - `AdvanceWeek()`
     - `AdvanceWeeks(int32 NumWeeks)`
     - `ProcessWeeklySimulation()`
   - If `UGameTimeSubsystem` already exists, make it the owner/coordinator.
   - Weekly advancement should execute deterministic ordered phases aligned with architecture guidance, even if some phases are currently stubs/wrappers:
     1. trend/market drift or equivalent
     2. artist updates
     3. production progress
     4. release processing
     5. market exposure
     6. chart processing
     7. tour resolution
     8. finance posting
     9. critic/news generation
     10. notifications/events
   - If the current codebase is not yet split this way, create a lightweight ordered pipeline that calls existing subsystem methods in a stable order.

3. **Remove per-frame business mutations**
   - For each offending `Tick`/frame callback:
     - remove direct simulation advancement
     - replace with explicit calls from the weekly advancement path
     - if needed, leave presentation-only behavior intact
   - Examples of what should no longer happen per frame:
     - advancing dates
     - incrementing recording progress
     - applying sales/exposure continuously
     - contract expiry checks
     - finance accrual
     - chart recalculation
     - event/news generation
   - If a class must still tick for presentation reasons, isolate simulation-free behavior and add comments clarifying that business logic is not allowed there.

4. **Wrap or migrate monthly-only logic**
   - If existing systems only support monthly processing, do not break them.
   - Introduce a compatibility layer so weekly advancement can:
     - process weekly-safe systems each week
     - trigger monthly rollups only when crossing month boundaries
   - Emit:
     - `OnWeekAdvanced` every week
     - `OnMonthClosed` when appropriate
     - `OnYearAdvanced` when appropriate
   - Preserve deterministic behavior across repeated runs.

5. **Make fast-forward batch-safe**
   - Ensure advancing multiple weeks does not rely on frame updates between weeks.
   - Add or update a method that processes N weeks in a loop.
   - Avoid forcing full UI rebuilds after each internal week when fast-forwarding.
   - If UI hooks exist, prefer:
     - summary notification after batch
     - or event emission per week without widget-side heavy rebuilds
   - Keep implementation modest if the UI architecture is still transitional.

6. **Stop widgets from driving simulation**
   - Inspect touched widgets/UI classes for any direct mutation of simulation state during:
     - `Tick`
     - `NativeConstruct`
     - refresh/bind methods
   - Convert them to:
     - read-only queries
     - event/delegate subscriptions
     - explicit button/command actions only
   - If full event-driven conversion is too large, at minimum remove hidden simulation side effects from UI refresh paths.

7. **Add logging and guardrails**
   - Add or use a dedicated log category such as `LogMusicSimTime` if available.
   - Log:
     - when a week advances
     - when a month/year boundary is crossed
     - when fast-forward batches begin/end
   - Add defensive comments/assertions where useful:
     - simulation methods should not be called from widget tick
     - business subsystems should not self-advance per frame

8. **Keep changes incremental and compile-safe**
   - Prefer adapting existing classes over introducing a large new framework.
   - Avoid speculative abstractions.
   - If you must add helper methods, keep them small and named clearly around weekly advancement/orchestration.

9. **Update or add tests if the repo supports them**
   - Add focused coverage for:
     - advancing one week changes state only once
     - advancing multiple weeks yields deterministic results
     - month/year events fire at correct boundaries
     - no dependency on per-frame ticking for simulation progression
   - If automated tests are not practical in the current project shape, add at least debug logging and a manual verification path.

# Validation steps

1. **Static code audit**
   - Search again for:
     - `Tick(`
     - `NativeTick(`
     - timer-driven simulation callbacks
   - Confirm no remaining per-frame business simulation logic exists in touched areas.
   - Confirm any remaining ticks are presentation-only.

2. **Build**
   - Run the most appropriate available build command from workspace root:
     - `dotnet build`
   - If there are tests/projects available:
     - `dotnet test`

3. **Behavior verification**
   - Verify simulation state changes only when explicit time advancement occurs.
   - Verify one explicit week advance:
     - updates date correctly
     - runs ordered simulation phases once
     - emits/logs week event
   - Verify month boundary:
     - monthly logic runs once
     - `OnMonthClosed` fires/logs once
   - Verify year boundary:
     - `OnYearAdvanced` fires/logs once

4. **Fast-forward verification**
   - Advance multiple weeks in one action.
   - Confirm:
     - no frame dependency
     - no repeated full UI rebuild requirement
     - deterministic final state versus repeated single-week advancement

5. **Regression checks**
   - Confirm existing monthly-only systems still function through wrappers/adapters.
   - Confirm UI still displays updated state after time advancement.
   - Confirm no hidden simulation occurs while idling on a screen.

6. **Determinism sanity check**
   - From the same starting state, run:
     - one 4-week fast-forward
     - four 1-week advances
   - Compare resulting key state where practical; it should match.

# Risks and follow-ups

- **Risk: hidden simulation in UI or actors**
  - Some business logic may be embedded in widgets, actor ticks, or blueprint-driven callbacks not obvious from C++ search.
  - Follow-up: perform a broader audit of blueprint/UI bindings if behavior still changes while idle.

- **Risk: monthly systems tightly coupled to frame updates**
  - Existing monthly logic may assume continuous updates.
  - Follow-up: extract explicit monthly rollup methods and call them only on boundary crossings.

- **Risk: event spam during fast-forward**
  - Emitting every event every week may cause noisy UI refreshes.
  - Follow-up: add batched summary notifications or UI suppression during multi-week processing.

- **Risk: partial architecture transition**
  - This task may expose the need for a fuller simulation director and command/event cleanup.
  - Follow-up stories likely include:
    - ST-101 full weekly simulation director hardening
    - ST-103 command dispatcher adoption
    - ST-104 domain event pipeline cleanup
    - ST-501 event-driven dashboard refresh

- **Risk: remaining non-deterministic ordering**
  - Even after removing per-frame logic, subsystem call order may still be implicit.
  - Follow-up: formalize a named weekly phase pipeline in `UGameTimeSubsystem` or a dedicated simulation director.

- **Risk: save/load assumptions**
  - If previous behavior relied on frame-driven recomputation after load, explicit rebuild hooks may be needed.
  - Follow-up: add post-load reconstruction methods that recompute derived state without advancing simulation.