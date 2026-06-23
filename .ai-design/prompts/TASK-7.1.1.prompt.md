# Goal
Implement backlog task `TASK-7.1.1` for story `ST-101` by updating `UGameTimeSubsystem` so it supports **weekly time advancement** and emits the domain events/delegates:

- `OnWeekAdvanced`
- `OnMonthClosed`
- `OnYearAdvanced`

The implementation should align with the architecture’s role of `UGameTimeSubsystem` as the simulation director/orchestrator, preserve deterministic progression, and avoid coupling UI logic into the subsystem.

# Scope
In scope:

- Add or extend in-game date/time state in `UGameTimeSubsystem` to support advancing by **one week at a time**
- Detect and emit:
  - weekly advancement every successful week tick
  - month close when a weekly advance crosses/closes a month boundary
  - year advance when a weekly advance crosses/closes a year boundary
- Ensure event payloads are useful and deterministic
- Keep implementation compatible with current codebase patterns where possible
- Add minimal tests or validation coverage if the project already has a test pattern
- Add logging for time advancement using a dedicated or existing log category if available

Out of scope unless required by existing code to compile:

- Full simulation phase execution across all future subsystems
- Fast-forward batching implementation beyond what is necessary to keep weekly advancement extensible
- UI widget rewrites
- Save/load schema expansion beyond any compile-required date snapshot adjustments
- Refactoring unrelated systems

If monthly-only logic already exists, wrap or preserve it so current callers do not break during the transition.

# Files to touch
Inspect the repo first and then update only the minimum necessary files. Likely targets include:

- `UGameTimeSubsystem` header/source
  - likely under something like:
    - `Source/MusicManager/.../GameTimeSubsystem.h`
    - `Source/MusicManager/.../GameTimeSubsystem.cpp`
- Any shared date/time structs used by the subsystem
- Any delegate/event definition files if events are centralized
- Existing tests for subsystem/time logic, or add a new focused test file if a test target exists
- Logging category definitions if needed

Before editing, locate:

- the actual `UGameTimeSubsystem` class
- current date representation
- any existing monthly advancement methods
- any existing delegates/events for time changes
- any save snapshot structs that serialize time state

Do not invent new top-level architecture if the repo already has a pattern.

# Implementation plan
1. **Discover current implementation**
   - Find `UGameTimeSubsystem` and inspect:
     - current public API
     - current date state
     - whether it already supports monthly advancement only
     - whether it already broadcasts any time-related delegates
   - Identify all callers of current advance-time methods.
   - Search for:
     - `GameTimeSubsystem`
     - `AdvanceMonth`
     - `AdvanceTime`
     - `OnMonth`
     - `OnYear`
     - `DECLARE_DYNAMIC_MULTICAST_DELEGATE`
     - `DECLARE_MULTICAST_DELEGATE`

2. **Define/extend deterministic time model**
   - Keep the existing date model if viable.
   - Add a clear weekly advancement API, preferably something like:
     - `AdvanceWeek()`
     - and optionally an internal helper for advancing multiple weeks later
   - Weekly advancement should:
     - capture previous date state
     - advance exactly 7 in-game days or the project’s canonical week representation
     - compute whether the move crossed into a new month and/or year
   - Prefer explicit boundary detection based on previous and new date values rather than implicit assumptions.

3. **Add event/delegate surface**
   - Expose three events on `UGameTimeSubsystem`:
     - `OnWeekAdvanced`
     - `OnMonthClosed`
     - `OnYearAdvanced`
   - Use the project’s existing delegate style:
     - if Blueprint-facing subsystem events are already used, prefer `BlueprintAssignable` dynamic multicast delegates
     - otherwise use native multicast delegates consistent with the codebase
   - Event payloads should include enough context for downstream systems. Prefer payloads such as:
     - previous date
     - new/current date
     - closed month/year values where relevant
   - Keep payloads serializable/simple UE structs if introducing new ones.

4. **Preserve monthly compatibility**
   - If the subsystem currently has monthly-only advancement, do not break existing callers.
   - Either:
     - keep the old monthly API and implement it in terms of repeated weekly advancement where safe, or
     - keep old events and trigger them from the new weekly logic when month boundaries are crossed
   - If exact old monthly semantics cannot be preserved, minimize API breakage and document behavior in code comments.

5. **Implement boundary detection**
   - On each weekly advance:
     - always emit `OnWeekAdvanced`
     - emit `OnMonthClosed` if the new date is in a different month than the previous date
     - emit `OnYearAdvanced` if the new date is in a different year than the previous date
   - Be careful with edge cases:
     - week crossing month boundary but not year boundary
     - week crossing year boundary
     - dates near end of month
     - leap year/date utility behavior if custom date logic exists
   - If the game uses a simplified calendar, follow the existing system rather than introducing real-world complexity.

6. **Keep orchestration-ready structure**
   - Even if full simulation phases are not implemented in this task, structure `AdvanceWeek()` so it can later host deterministic weekly phases in fixed order.
   - Example internal shape:
     1. validate subsystem state
     2. capture previous date
     3. advance date
     4. emit week event
     5. emit month close if crossed
     6. emit year event if crossed
   - If there is already a simulation tick method, route through it rather than duplicating logic.

7. **Add logging**
   - Use an existing time log category or add `LogMusicSimTime` if the project already follows the architecture naming.
   - Log at useful levels:
     - `Verbose/Log` for normal week advancement
     - `Log` for month close/year advance
   - Avoid noisy per-frame style logging.

8. **Update comments/API docs**
   - Add concise comments to public methods and delegates explaining:
     - weekly cadence is authoritative
     - month/year events are derived from weekly progression
     - subsystem is intended as simulation time authority

9. **Add focused validation/tests**
   - If automated tests exist, add coverage for:
     - advancing one week within same month => only `OnWeekAdvanced`
     - advancing one week across month boundary => `OnWeekAdvanced` + `OnMonthClosed`
     - advancing one week across year boundary => `OnWeekAdvanced` + `OnMonthClosed` if applicable + `OnYearAdvanced`
   - If no test framework exists, add a lightweight dev/test harness only if consistent with repo patterns; otherwise rely on compile + manual validation.

10. **Do not overreach**
   - Do not implement unrelated subsystems or speculative command bus work in this task.
   - Keep changes small, compile-safe, and aligned to the exact backlog item.

# Validation steps
1. **Codebase discovery**
   - Confirm actual file paths for `UGameTimeSubsystem`
   - Confirm whether delegates are native or Blueprint dynamic in this project

2. **Build**
   - From workspace root, run the most appropriate available command:
     - `dotnet build`
   - If there is a UE-specific build path already documented in `README.md`, use that as well

3. **Automated tests**
   - If a test project exists and is relevant:
     - `dotnet test`

4. **Manual behavior verification**
   - Start from a known in-game date and invoke weekly advancement
   - Verify:
     - one week advance updates date correctly
     - `OnWeekAdvanced` fires every time
     - `OnMonthClosed` fires only when crossing/closing a month
     - `OnYearAdvanced` fires only when crossing/closing a year
   - Test at least these date scenarios:
     - mid-month to mid-month
     - end-of-month crossing
     - end-of-year crossing

5. **Regression check**
   - Verify any existing monthly advancement entry points still compile and behave sensibly
   - Verify no UI or subsystem code is directly broken by renamed APIs or changed signatures

6. **Logging check**
   - Confirm time advancement logs appear and are readable without excessive spam

# Risks and follow-ups
- **Risk: existing date model is custom or incomplete**
  - Follow existing conventions instead of introducing a new calendar abstraction.
- **Risk: current monthly-only callers depend on old semantics**
  - Preserve compatibility shims where possible and avoid breaking public APIs unnecessarily.
- **Risk: delegate style mismatch**
  - Match the project’s existing event/delegate pattern rather than forcing Blueprint/native delegates arbitrarily.
- **Risk: month-close semantics ambiguity**
  - If “month closed” could mean either “crossed into new month” or “processed final monthly summary,” implement the event on boundary crossing for this task and leave summary orchestration for a follow-up if not already defined.

Follow-up recommendations after this task:
- Add fixed-order weekly simulation phase execution inside `UGameTimeSubsystem`
- Add multi-week fast-forward API that can suppress per-step UI rebuilds while still emitting summarized results
- Add save/load coverage for time snapshot if not already present
- Add explicit event payload structs if current delegates are too minimal for downstream systems