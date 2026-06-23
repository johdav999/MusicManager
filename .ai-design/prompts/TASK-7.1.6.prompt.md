# Goal
Implement `TASK-7.1.6` for story `ST-101 — Weekly simulation director` by preserving a **monthly summary layer** for finance/news UX while the simulation advances on a **weekly cadence**.

The coding agent should:
- keep weekly simulation as the authoritative progression model,
- ensure monthly rollup/close behavior still exists as a first-class concept,
- expose monthly summary data/events in a way that finance and news UI can consume without reintroducing monthly-only simulation coupling,
- preserve determinism and compatibility with existing systems during the weekly-transition phase.

This task is specifically about the **monthly summary layer**, not the full weekly director implementation.

# Scope
In scope:
- Identify where current monthly-only finance/news behavior exists.
- Add or refactor a monthly close/summary mechanism that is triggered from weekly time advancement.
- Ensure `UGameTimeSubsystem` (or equivalent simulation director owner) can detect month boundaries during weekly advancement and emit/trigger monthly close behavior exactly once per closed month.
- Ensure finance-facing monthly aggregation remains available for UX/reporting.
- Ensure news/event-facing monthly summary hooks remain available for UX/reporting.
- Add lightweight data structures/events/APIs needed for monthly summaries.
- Preserve compatibility with existing monthly consumers where practical via wrappers/adapters/shims.

Out of scope:
- Full rewrite of finance subsystem.
- Full rewrite of event/news subsystem.
- New UI screens beyond minimal binding support.
- Save migration unless required by touched data structures.
- Broader command bus or stable ID work unless directly required for compilation.

# Files to touch
Inspect first, then modify only the minimum necessary set. Likely targets include:

- `README.md`  
  - only if there is a subsystem or architecture note that must be updated.

- Time/simulation orchestration files, likely one or more of:
  - `Source/.../GameTimeSubsystem.h`
  - `Source/.../GameTimeSubsystem.cpp`
  - `Source/.../SimulationDirector...`
  - any date/time utility structs used to detect week/month/year boundaries

- Finance subsystem files, likely one or more of:
  - `Source/.../FinanceManagerSubsystem.h`
  - `Source/.../FinanceManagerSubsystem.cpp`
  - finance summary/view-model structs

- Event/news subsystem files, likely one or more of:
  - `Source/.../EventSubsystem.h`
  - `Source/.../EventSubsystem.cpp`
  - news feed item generation helpers

- Shared model/event files, likely one or more of:
  - `Source/.../Types/...`
  - `Source/.../Events/...`
  - `Source/.../ViewModels/...`

- Tests if present:
  - `Source/.../Tests/...`
  - `.NET` test projects only if this repo actually uses them for gameplay logic helpers

Do not assume exact paths from the workspace hint. First discover the actual Unreal source layout and subsystem names.

# Implementation plan
1. **Discover current implementation**
   - Find the actual Unreal module source folders.
   - Locate `UGameTimeSubsystem` or equivalent time owner.
   - Locate any existing monthly advancement logic, monthly finance summaries, and monthly news generation.
   - Determine whether current code:
     - advances by month directly,
     - already emits `OnMonthClosed`,
     - stores monthly aggregates,
     - generates monthly reports/news inline.

2. **Map current monthly dependencies**
   - Identify all systems that currently depend on month transitions.
   - Classify each dependency as:
     - true simulation logic that should now run weekly,
     - monthly reporting/aggregation that should remain monthly,
     - UI-only summary behavior.
   - Preserve only the monthly reporting layer in this task.

3. **Define a monthly close contract**
   - Introduce or formalize a small monthly summary contract, ideally with:
     - a month identifier/date range,
     - summary payload or query hook,
     - a typed event/delegate such as `OnMonthClosed`.
   - If an event already exists, reuse it and make its semantics explicit:
     - fired once when the simulation crosses into a new month,
     - represents closure of the previous month,
     - safe for finance/news summary generation.

4. **Implement month-boundary detection from weekly advancement**
   - In weekly time advancement, detect whether the new week crosses a month boundary.
   - If one or more months are crossed, process monthly close in deterministic order for each closed month.
   - Ensure no duplicate monthly close firing.
   - Ensure year-close behavior still works if month close crosses December -> January.
   - Prefer explicit helper methods, e.g.:
     - `AdvanceWeek()`
     - `ProcessClosedMonths(OldDate, NewDate)`
     - `CloseMonth(FGameDate ClosedMonthDate)`

5. **Preserve finance monthly summary layer**
   - Add or adapt finance aggregation so monthly summaries are generated from weekly ledger/activity rather than requiring monthly-only simulation.
   - Preferred approach:
     - weekly finance posting remains authoritative,
     - monthly close computes or finalizes a monthly summary snapshot,
     - finance UI reads summary projections rather than raw ad hoc maps.
   - If a finance summary struct does not exist, add a minimal one, e.g.:
     - month/year,
     - income total,
     - expense total,
     - net result,
     - optional category breakdown if already easy to derive.
   - Keep implementation append-only and deterministic.

6. **Preserve news monthly summary layer**
   - Ensure the event/news system can still produce monthly summary-style content if that existed or is expected by UX.
   - Do not move core news generation back to monthly-only logic.
   - Instead:
     - weekly domain events remain the source of truth,
     - monthly close may generate a rollup/summary news item or trigger summary compilation.
   - Deduplicate by month key if needed.

7. **Add compatibility shims where needed**
   - If existing code expects a direct “monthly tick” method, wrap it behind month-close processing rather than deleting it outright.
   - Example pattern:
     - old monthly method becomes internal summary/finalization logic,
     - weekly advancement calls it only when a month closes.
   - Avoid breaking existing UI consumers if they already query monthly summaries.

8. **Keep widgets thin**
   - Do not put business calculations in UI code.
   - If UI needs monthly data, expose a read-only query/projection from the owning subsystem.

9. **Add logging**
   - Add or use existing UE log categories for time/finance/events.
   - Log month close processing in development-friendly form:
     - old date,
     - new date,
     - closed month,
     - summary generation success.

10. **Document assumptions in code**
   - Add concise comments where month-close semantics are subtle, especially around:
     - week crossing month boundaries,
     - multiple month closures during fast-forward,
     - ordering relative to weekly simulation phases.

# Validation steps
1. **Static inspection**
   - Confirm the project source layout and actual subsystem names before coding.
   - Confirm all new/changed APIs compile against existing includes and module dependencies.

2. **Build**
   - Use the repo’s real build path if available.
   - If the workspace supports it, run:
     - `dotnet build`
   - If Unreal C++ build commands are documented in the repo, prefer those.
   - Do not claim success without checking actual output.

3. **Behavior validation**
   - Verify weekly advancement still works.
   - Verify month close triggers exactly once when advancing across a month boundary.
   - Verify fast-forward across multiple weeks/months produces one monthly close per closed month in order.
   - Verify December -> January also preserves year advancement behavior.

4. **Finance validation**
   - Verify monthly finance summary data is still available after weekly progression.
   - Verify summary totals are derived consistently from weekly postings or existing ledger data.
   - Verify no duplicate monthly summary entries are created for the same month.

5. **News/event validation**
   - Verify monthly summary news/rollup generation still occurs if expected.
   - Verify no duplicate monthly summary news items for the same month.
   - Verify weekly news generation remains unaffected.

6. **Regression validation**
   - Check any existing monthly-only consumers still function through compatibility shims.
   - Check save/load assumptions are not silently broken by any new stored summary data.
   - If tests exist, run the relevant subset; otherwise add at least lightweight automated coverage if there is an established test pattern.

7. **Suggested test scenarios**
   - Advance one week within same month: no month close.
   - Advance one week crossing into next month: one month close.
   - Fast-forward 8–12 weeks crossing multiple months: multiple ordered month closes.
   - Cross year boundary: month close(s) plus year event.
   - Finance ledger with weekly entries: monthly summary totals match expected values.
   - Monthly summary news generation: one summary item per month.

# Risks and follow-ups
- **Risk: hidden monthly simulation coupling**
  - Existing finance/news code may still perform actual simulation mutations on monthly tick.
  - If found, isolate true simulation from summary generation rather than preserving incorrect coupling.

- **Risk: date boundary bugs**
  - Weekly advancement can skip over month boundaries in nontrivial ways.
  - Be careful with inclusive/exclusive date semantics and “closed month” identity.

- **Risk: duplicate summary generation during fast-forward**
  - If UI/event handlers also generate summaries, duplicates may appear.
  - Use stable month keys and centralize month-close generation.

- **Risk: save/load mismatch**
  - If monthly summaries are persisted, ensure they can be recomputed or versioned safely.
  - Prefer derived/rebuildable summaries unless persistence already exists.

- **Risk: over-touching unrelated systems**
  - Keep this task narrow: preserve the monthly summary layer, not a broad subsystem redesign.

Follow-ups after this task:
- Add explicit automated tests for weekly-to-monthly boundary handling.
- Standardize monthly summary view models for finance and news screens.
- Audit all remaining monthly-only logic and migrate true simulation work into weekly phases.
- Ensure `OnMonthClosed` payloads are stable and serializable enough for future save replay/debug tracing.