# Goal
Implement backlog task **TASK-7.1.4** for **ST-101 — Weekly simulation director** by ensuring all **existing monthly-only simulation logic is either migrated to weekly-aware execution or wrapped behind compatibility adapters** so current systems continue to function during the transition to weekly simulation.

The coding agent should:
- preserve current gameplay behavior where full weekly refactors are not yet complete,
- avoid breaking existing monthly consumers,
- keep simulation ordering deterministic,
- support a weekly tick pipeline with monthly rollup behavior,
- introduce compatibility seams rather than forcing a full rewrite in one pass.

This task is specifically about **transition safety and backward compatibility**, not about fully redesigning every subsystem.

# Scope
In scope:
- Inspect current time/simulation orchestration and identify monthly-only entry points.
- Add or refine a weekly advancement path in the simulation director / time subsystem.
- Wrap legacy monthly logic so it still executes correctly during weekly progression.
- Ensure monthly systems run:
  - either on month boundary only,
  - or through a compatibility adapter that accumulates weekly state and triggers equivalent monthly processing.
- Preserve existing events and behavior expected by current systems.
- Add clear TODOs / extension points for future subsystem-native weekly migration.
- Add tests for weekly advancement and month-boundary compatibility behavior where test infrastructure exists.

Out of scope:
- Full rebalance of formulas for weekly simulation.
- Deep refactors of unrelated subsystems.
- UI redesign.
- Save migration unless directly required by touched code.
- Converting every monthly subsystem to true weekly-native logic if a safe wrapper is sufficient for now.

Implementation constraints:
- Follow the architecture direction: `UGameInstanceSubsystem` ownership, deterministic ordered phases, thin UI.
- Prefer minimal-risk compatibility shims over broad rewrites.
- Do not remove monthly summary behavior.
- Do not introduce per-frame simulation logic.
- Keep code paths explicit and readable; avoid hidden magic.

# Files to touch
Touch the smallest set of files necessary after inspection. Likely candidates include:

- `README.md` only if there is an architecture/dev note section that must be updated for the transition behavior.
- Time/simulation orchestration files, likely something like:
  - `UGameTimeSubsystem` implementation/header
  - simulation director/orchestration classes
- Any existing monthly-only manager/subsystem files that are directly invoked by time advancement.
- Event/delegate definitions if weekly/monthly boundary signaling needs cleanup.
- Test files covering time advancement / simulation cadence.

Before editing, search for:
- `Month`
- `Monthly`
- `AdvanceMonth`
- `OnMonthClosed`
- `TickMonth`
- `SimulateMonth`
- `AdvanceTime`
- `EndOfMonth`
- finance/news/chart rollup entry points
- any direct date increment logic

If the repo is sparse or code is not where expected, first locate the actual implementation files and adapt the plan accordingly.

# Implementation plan
1. **Inspect current simulation/time flow**
   - Find the authoritative time advancement path.
   - Identify whether `UGameTimeSubsystem` already exists and whether weekly events are partially implemented.
   - Map all current monthly-only calls and note which systems depend on them.

2. **Classify monthly-only logic**
   For each monthly-only path, classify it as one of:
   - **Boundary-only logic**: should run once when a month closes, unchanged.
   - **Rollup logic**: can accumulate weekly changes and publish monthly summary at boundary.
   - **Truly cadence-sensitive logic**: needs a temporary weekly wrapper or proportional execution strategy.
   Document this in code comments where helpful.

3. **Introduce/clean up weekly advancement orchestration**
   - Ensure there is a single deterministic weekly advancement entry point.
   - Weekly advancement should:
     - advance date by one week,
     - execute weekly phases in fixed order,
     - detect month/year boundary crossings,
     - emit `OnWeekAdvanced`,
     - invoke month-close compatibility processing when appropriate,
     - emit `OnMonthClosed` and `OnYearAdvanced` as appropriate.
   - If fast-forward exists, ensure it loops through weekly advancement without forcing full UI rebuild behavior in core logic.

4. **Add a monthly compatibility layer**
   Implement a simple compatibility mechanism for legacy monthly systems. Preferred order:
   - If a subsystem’s logic is inherently monthly, keep it as-is and invoke it only when a month boundary is crossed.
   - If a subsystem currently expects “advance one month” semantics but is now called from weekly flow, add a wrapper/adaptor such as:
     - `ProcessLegacyMonthlySystemsIfNeeded(...)`
     - `RunMonthlyCompatibilityPass(...)`
     - or subsystem-local `HandleMonthClosed(...)`
   - Avoid duplicating business logic across weekly and monthly paths.

5. **Preserve deterministic ordering**
   - Make ordering explicit in the weekly pipeline.
   - Ensure month-close compatibility processing happens in a stable, documented place in the sequence.
   - If multiple subsystems have monthly handlers, call them in a fixed order.

6. **Avoid behavior regressions**
   - Do not change public behavior more than necessary.
   - Keep existing monthly summaries/reports functional.
   - If a subsystem is not yet weekly-native, prefer “weekly progression + monthly legacy execution” over partial broken weekly behavior.

7. **Add targeted tests**
   Add or update tests for the most important transition guarantees:
   - advancing one week emits weekly event once,
   - crossing a month boundary triggers monthly compatibility logic once,
   - non-boundary weeks do not trigger monthly-only handlers,
   - crossing year boundary also emits year event correctly,
   - repeated weekly fast-forward preserves deterministic boundary counts.
   If automated tests are not available in the current project shape, add lightweight validation hooks and document manual verification steps in code comments or notes.

8. **Document follow-up seams**
   - Add concise TODOs where a subsystem is still running through compatibility mode.
   - Mark places intended for future native weekly migration.
   - Keep comments practical and implementation-focused.

Implementation guidance:
- Prefer small helper methods over one large monolithic advance function.
- Prefer names that make transition intent obvious, e.g.:
  - `AdvanceOneWeek`
  - `HandleCalendarBoundaryEvents`
  - `RunLegacyMonthlyCompatibility`
  - `DidCrossMonthBoundary`
- If there is no existing abstraction for cadence, do not over-engineer one unless clearly needed by the codebase.

# Validation steps
Run the most relevant local validation available in the workspace.

1. Build/test discovery:
   - Inspect solution/projects to determine actual buildable targets.
   - Try:
     - `dotnet build`
     - `dotnet test`
   - If this is primarily Unreal C++ and .NET commands are only incidental, still use available test/build commands where possible and report limitations clearly.

2. Functional validation in code/tests:
   - Verify weekly advancement path compiles.
   - Verify monthly compatibility handlers are invoked only on month boundary.
   - Verify no duplicate month-close execution when multiple weekly ticks remain inside the same month.
   - Verify year-close behavior still works when month boundary also crosses year boundary.
   - Verify existing monthly consumers still receive expected callbacks/events.

3. Manual validation checklist if runtime execution is not available:
   - Start from a date in the middle of a month and advance one week: only weekly processing should run.
   - Start from a date near month end and advance one week across the boundary: weekly + monthly compatibility should run.
   - Advance several weeks across multiple months: monthly compatibility count should equal number of crossed month boundaries.
   - Confirm deterministic ordering remains stable across repeated runs from same starting state.

4. In the final work summary, include:
   - files changed,
   - monthly-only systems found,
   - which were wrapped vs left unchanged on month boundary,
   - any remaining systems that still need native weekly migration.

# Risks and follow-ups
Risks:
- Existing monthly logic may implicitly assume a full month of state change happened before execution.
- Boundary detection can be error-prone if current date math is inconsistent.
- Some systems may have hidden side effects tied to direct month advancement calls.
- Fast-forward may accidentally trigger duplicate monthly processing if implemented through mixed old/new paths.
- If tests are limited, regressions may only surface through runtime simulation behavior.

Follow-ups to note for future tasks:
- Migrate compatibility-wrapped subsystems to true weekly-native logic one by one.
- Add explicit simulation phase definitions if current orchestration is still ad hoc.
- Add stronger automated tests around date progression and deterministic simulation ordering.
- Review finance/news/chart systems for whether they should remain monthly summary consumers or become weekly-native with monthly rollups.
- If save/load stores cadence-sensitive transient state, verify compatibility with weekly progression in a later task.