# Goal
Implement backlog task **TASK-7.1.3** for story **ST-101 — Weekly simulation director** so that **fast-forward can process multiple in-game weeks without rebuilding the UI on every intermediate step**.

The implementation should preserve the architecture direction:
- `UGameTimeSubsystem` remains the orchestration point for weekly advancement
- simulation stays deterministic
- widgets remain thin
- UI updates become **batched/summarized** during fast-forward instead of forcing per-week redraws

Because this task is specifically about fast-forward behavior, focus on introducing a **batched advancement mode** and **UI notification suppression/aggregation** rather than broad simulation refactors.

# Scope
In scope:
- Add a fast-forward/multi-week advancement path to the simulation time flow
- Prevent per-step UI rebuild/refresh during batched advancement
- Emit a final summarized update after the batch completes
- Preserve existing single-week advancement behavior
- Keep deterministic weekly phase execution for every simulated week in the batch
- Add lightweight observability/logging around batched advancement
- Add/adjust tests for multi-week advancement behavior if test coverage exists or can be added affordably

Out of scope:
- Large command bus refactors unless required to expose fast-forward cleanly
- Rebuilding all UI screens
- New gameplay systems beyond what is needed for batched advancement
- Full monthly migration work outside what this task directly touches
- Save/load changes unless a new transient batching flag must explicitly be excluded from persistence

Assume no explicit acceptance criteria beyond the story-level requirement:
- “Fast-forward can process multiple weeks without rebuilding UI every step.”

# Files to touch
Inspect the repo first and then update the most relevant files. Likely candidates include:

- `README.md` if there is developer-facing documentation for simulation/time controls
- `Source/.../GameTimeSubsystem.*`
- `Source/.../UIManagerSubsystem.*`
- `Source/.../StatusWidget.*`
- `Source/.../Dashboard*`
- `Source/.../EventSubsystem.*`
- `Source/.../CommandDispatcherSubsystem.*` if time advancement is routed through commands
- Any existing simulation event/delegate definitions
- Any existing view-model/projection classes used by dashboard/status UI
- Relevant automated test files under `Source/.../Tests/*` or equivalent

Before coding:
1. Locate the actual Unreal C++ source tree under `Source/`
2. Find the current implementation of:
   - weekly advancement
   - fast-forward / advance-time actions
   - UI refresh triggers
   - any `OnWeekAdvanced`, `OnMonthClosed`, `OnYearAdvanced` delegates
3. Prefer modifying existing systems over introducing parallel ones

# Implementation plan
1. **Discover current time advancement and UI refresh coupling**
   - Find how a single week is currently advanced
   - Identify where UI rebuilds are triggered:
     - direct widget refresh calls
     - subsystem delegates
     - event bus notifications
     - dashboard/status polling hooks
   - Identify whether fast-forward already exists and whether it loops single-week advancement naively

2. **Introduce an explicit batched advancement mode in `UGameTimeSubsystem`**
   - Add a public API for advancing multiple weeks, e.g. conceptually:
     - `AdvanceWeeks(int32 WeekCount)`
     - or `FastForwardWeeks(int32 WeekCount)`
   - Internally, execute the same deterministic weekly simulation phases for each week in order
   - Do **not** skip simulation work; only suppress/aggregate presentation churn
   - Track whether the subsystem is currently in a batched advancement session with a transient flag, e.g.:
     - `bIsBatchAdvancing`
   - Optionally track a small summary struct for the batch, e.g.:
     - weeks processed
     - start/end date
     - whether any month/year boundaries were crossed
     - count of major events generated if cheaply available

3. **Separate simulation progression from presentation notifications**
   - Keep core simulation delegates/events available for correctness, but prevent UI from doing full redraws on every intermediate week during batch mode
   - Preferred approach:
     - add a batching-aware notification path such that intermediate week events are either:
       - marked as suppressed for UI, or
       - routed through an aggregation layer
   - At the end of the batch, emit one final summary-style notification/event for UI refresh
   - If existing code directly calls widget rebuilds from time advancement, replace that with subsystem/event-driven refresh requests

4. **Add UI refresh suppression/aggregation**
   - In `UUIManagerSubsystem` and/or relevant widgets, add support for:
     - begin batched update
     - end batched update
     - deferred refresh once at completion
   - Preferred pattern:
     - `BeginSimulationBatchUpdate()`
     - `EndSimulationBatchUpdate(const FSimBatchSummary&)`
   - During batch mode:
     - ignore or queue repeated refresh requests
     - mark affected screens/view-models dirty
   - On batch completion:
     - rebuild/update once using latest state
   - Avoid introducing widget-owned business logic

5. **Preserve monthly/yearly summary semantics**
   - If the batch crosses month/year boundaries, still run the underlying monthly/yearly simulation logic at the correct times
   - UI should not fully redraw at each boundary during fast-forward unless there is already a hard-stop rule
   - If there is an existing summary/news system, prefer accumulating summary data and surfacing it once after the batch

6. **Handle interruption points only if already supported**
   - The architecture mentions possible interruption on major events, but this task does not require implementing interruption unless the codebase already has a hook
   - If interruption support already exists, ensure the batch mode can stop cleanly and still flush deferred UI refresh
   - Otherwise, keep the implementation simple: process requested weeks and refresh once at the end

7. **Add logging/observability**
   - Use or add a dedicated log category such as `LogMusicSimTime`
   - Log:
     - batch start
     - requested week count
     - actual processed week count
     - start/end in-game date
     - elapsed real time if easy to measure
   - In development builds, optionally log how many UI refreshes were suppressed/deferred

8. **Add tests**
   - Add or update automated tests where practical:
     - single-week advancement still emits expected signals
     - multi-week advancement advances exact number of weeks
     - simulation state after `N` batched weeks matches `N` repeated single-week advances
     - UI refresh callback/rebuild trigger fires once for a batch instead of once per week
   - If no UI tests exist, add a subsystem-level test using a mock/delegate counter rather than full widget tests

9. **Keep implementation minimal and architecture-aligned**
   - Do not let widgets directly inspect batching internals unless unavoidable
   - Prefer subsystem delegates / view-model invalidation flags
   - Keep any batching state transient and not serialized

Suggested implementation shape:
- `UGameTimeSubsystem`
  - add batch advancement API
  - add transient `bIsBatchAdvancing`
  - add summary/deferred notification support
- `UUIManagerSubsystem`
  - add deferred refresh handling
  - coalesce repeated refresh requests during batch
- widgets/view-models
  - respond to one final refresh after batch completion
  - avoid per-week full rebuilds during fast-forward

If there is already a command path for time advancement:
- wire the fast-forward action through that path
- keep validation simple: reject non-positive week counts

# Validation steps
1. **Code inspection**
   - Confirm there is a dedicated multi-week advancement path rather than a UI-driven loop that rebuilds every iteration
   - Confirm UI refreshes are deferred/coalesced during batch mode

2. **Build**
   - Run:
     - `dotnet build`
   - If there are Unreal-specific project/build instructions in the repo, use those as appropriate after discovery

3. **Tests**
   - Run:
     - `dotnet test`
   - If no relevant tests exist, add targeted automated coverage and run the smallest relevant suite

4. **Manual behavior verification**
   - Start from a campaign/state where weekly advancement updates visible UI
   - Advance **1 week**
     - verify normal behavior remains unchanged
   - Fast-forward **multiple weeks** (e.g. 4, 8, or 12)
     - verify simulation date advances correctly
     - verify underlying weekly systems still resolve each week
     - verify dashboard/status UI does **not** visibly rebuild every intermediate week
     - verify UI updates once at the end with final state
   - If month/year boundaries are crossed:
     - verify resulting summaries/state are correct
     - verify no excessive intermediate redraws occur

5. **Determinism check**
   - Compare:
     - state after advancing 8 weeks one-by-one
     - state after advancing 8 weeks via batch fast-forward
   - They should match for core simulation outputs

6. **Logging verification**
   - Confirm batch start/end logs appear
   - Confirm no warning/error spam is introduced during fast-forward

# Risks and follow-ups
- **Risk: hidden UI coupling**
  - Existing widgets may be directly bound to per-week delegates and still rebuild despite subsystem batching
  - Mitigation: search for all listeners to time advancement delegates and route them through UI manager or dirty-flag refresh logic

- **Risk: simulation and UI notifications are currently inseparable**
  - If the same delegate drives both business logic and presentation, suppressing it may break gameplay
  - Mitigation: preserve simulation correctness and only suppress/coalesce presentation-layer reactions

- **Risk: monthly/yearly events may assume immediate UI refresh**
  - Batched advancement across boundaries could expose assumptions in finance/news screens
  - Mitigation: mark those screens dirty and refresh from final authoritative state after batch completion

- **Risk: fast-forward implemented outside `UGameTimeSubsystem`**
  - A UI loop may currently call single-week advancement repeatedly
  - Mitigation: move batching ownership into the time subsystem so orchestration is centralized

- **Risk: no existing automated test harness for Unreal subsystems**
  - Mitigation: add lightweight delegate-count/state-equivalence tests if feasible; otherwise document manual verification clearly in code comments/PR notes

Follow-ups after this task:
- Add optional interruption rules for major events during fast-forward
- Add richer batch summary payloads for dashboard/news UX
- Audit all management screens to ensure event-driven refreshes instead of full rebuilds
- Route advance-time actions through the command dispatcher if not already done