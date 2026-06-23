# Goal

Implement backlog task **TASK-7.1.5 — Keep deterministic ordering across subsystems** for story **ST-101 Weekly simulation director**.

The coding agent should make weekly simulation execution order explicit, stable, and repeatable across all participating subsystems, so that the same starting state always produces the same ordered phase execution and the same outcomes. This work should fit the existing architecture direction where `UGameTimeSubsystem` acts as the simulation director and coordinates subsystem-owned business logic.

Because this task has no standalone acceptance criteria, treat the story-level requirements and architecture notes as the source of truth. The implementation must support:

- fixed weekly phase ordering
- deterministic subsystem invocation within each phase
- no hidden ordering based on container iteration or registration timing
- compatibility with current transition state from monthly-only logic
- minimal UI coupling
- future support for fast-forward and save/load determinism testing

# Scope

Focus only on the deterministic ordering foundation for simulation orchestration. Do **not** expand into unrelated gameplay features.

In scope:

- Identify where weekly/monthly simulation advancement is currently orchestrated.
- Introduce or tighten a deterministic phase pipeline in `UGameTimeSubsystem` or the current simulation director equivalent.
- Ensure participating subsystems execute in a fixed, explicit order.
- Replace any non-deterministic iteration patterns in the orchestration path, especially:
  - iterating unordered maps/sets for subsystem execution
  - relying on subsystem discovery order
  - relying on delegate binding order for core simulation mutation
- Add lightweight logging/debug output showing phase and subsystem execution order in development builds.
- Preserve compatibility with existing monthly logic by wrapping or sequencing it under the deterministic weekly pipeline where needed.
- Keep implementation small and incremental.

Out of scope unless required to complete compilation:

- redesigning all subsystem internals
- changing business formulas
- broad save schema changes
- UI refactors
- command dispatcher work
- event/news redesign
- stable ID migration outside what is necessary for deterministic orchestration

# Files to touch

Start by inspecting these likely areas and update the prompt if actual names differ:

- `README.md` for project conventions if useful
- `Source/.../GameTimeSubsystem.*`
- any simulation director or time advancement service files
- subsystem headers/cpps for:
  - artist manager
  - production/record manager
  - release/catalog manager
  - market manager
  - chart manager
  - tour manager
  - finance manager
  - critic/event manager
- any shared simulation interfaces or utility headers
- any existing tests covering time advancement or simulation
- build files only if new source files must be included

If the codebase does not yet have a clean simulation director abstraction, prefer touching the smallest set of files necessary to establish one inside the current time subsystem.

# Implementation plan

1. **Discover the current orchestration path**
   - Find where advancing time/week/month currently happens.
   - Identify all subsystems that mutate simulation state during advancement.
   - Document current effective order in code comments or notes before changing behavior.
   - Look for non-deterministic patterns such as:
     - `TMap` / `TSet` iteration used to drive execution order
     - arrays populated from subsystem enumeration without sorting
     - mutation triggered indirectly through multicast delegates where order matters
     - mixed weekly and monthly mutation paths that can interleave unpredictably

2. **Define an explicit simulation phase model**
   - Add a small enum or equivalent for weekly phases, aligned with architecture guidance. Prefer something like:
     - TrendDrift
     - ArtistStateUpdate
     - ProductionProgress
     - ReleaseLaunchProcessing
     - MarketExposureUpdate
     - ChartCalculation
     - TourResolution
     - FinanceSettlement
     - CriticNewsGeneration
     - Notifications
   - If the existing code already has phases, normalize them rather than duplicating.
   - Keep phase names implementation-facing and stable.

3. **Create a deterministic execution contract**
   - Introduce a simple internal mechanism for ordered execution. Prefer one of:
     - explicit hardcoded ordered calls in `UGameTimeSubsystem`
     - or a small ordered registration structure with explicit phase + priority
   - Prefer explicit hardcoded ordering over dynamic registration if the codebase is still early-stage.
   - If using registration, require deterministic sorting by:
     1. phase
     2. numeric priority
     3. stable subsystem name/type as final tie-breaker
   - Do not rely on object pointer addresses, creation order, or delegate bind order.

4. **Refactor weekly advancement to use the ordered pipeline**
   - Route all weekly simulation mutation through one central method such as `AdvanceWeek()` / `RunWeeklySimulation()`.
   - Ensure each phase completes before the next begins.
   - Ensure monthly/yearly rollups happen at a deterministic point relative to weekly processing.
   - If monthly-only legacy logic exists, wrap it in a clearly named phase or post-week/month-close step rather than letting it run ad hoc.

5. **Isolate UI notifications from simulation mutation**
   - Make sure UI-facing broadcasts like `OnWeekAdvanced`, `OnMonthClosed`, and `OnYearAdvanced` occur after state mutation is complete for the relevant step.
   - Avoid using UI/event delegates as the mechanism that performs business-state mutation.
   - If existing delegates currently mutate state, move that mutation into explicit subsystem calls and leave delegates as notifications only.

6. **Eliminate non-deterministic subsystem iteration in the orchestration path**
   - Replace unordered iteration with:
     - explicit arrays in fixed order
     - sorted arrays using stable keys
   - If a subsystem internally exposes collections used during orchestration, only change them if they directly affect cross-subsystem ordering for this task.
   - Add comments where deterministic ordering is intentional and required for save/load reproducibility.

7. **Add observability for execution order**
   - Add a dedicated log category if one already exists for time/simulation, otherwise use the existing time log category.
   - In non-shipping builds, log:
     - week/date being processed
     - each phase start/end
     - each subsystem/action invoked in order
   - Keep logs concise and structured enough to compare runs.

8. **Add or update tests**
   - If there is an existing test project, add focused tests for deterministic ordering.
   - Good test targets:
     - weekly phases execute in the expected order
     - repeated runs from the same initial state produce the same ordered execution trace
     - monthly close happens at the same deterministic point
   - If automated tests are not yet present in the Unreal module, add at least a lightweight testable helper for phase ordering and cover it in the available test framework.
   - If no practical automated test path exists in this workspace, add a small deterministic trace helper and document manual verification steps clearly.

9. **Keep backward compatibility**
   - Do not break existing monthly summary behavior.
   - Preserve public events unless renaming is necessary; if changed, update all call sites.
   - Minimize API churn across subsystems.

10. **Document assumptions in code**
   - Add short comments near the orchestration code explaining:
     - why order is explicit
     - why unordered containers must not drive simulation sequencing
     - where future subsystems should be inserted

# Validation steps

1. **Build**
   - Run the most appropriate build command for the actual project setup.
   - From the provided workspace context, try:
     - `dotnet build`
   - If this is only a wrapper and the real code is Unreal C++, use the project’s normal compile path if available in the repo/tooling.

2. **Static verification**
   - Confirm there is exactly one central weekly simulation entry point.
   - Confirm phase order is explicit in code.
   - Confirm no core simulation sequencing depends on unordered container iteration or delegate subscription order.

3. **Runtime/manual verification**
   - Start from the same save or new campaign state twice.
   - Advance one week and compare logs:
     - phase order must match exactly
     - subsystem invocation order must match exactly
   - Advance multiple weeks and confirm ordering remains stable.
   - Verify monthly close still occurs and is emitted after the intended weekly processing point.
   - Verify `OnWeekAdvanced`, `OnMonthClosed`, and `OnYearAdvanced` still fire.

4. **Fast-forward sanity**
   - Advance several weeks in a batch if supported.
   - Confirm the same ordered phase pipeline is used for each processed week.
   - Confirm UI is not required to rebuild every intermediate step for simulation correctness.

5. **Regression checks**
   - Verify existing monthly-only systems still function through the new pipeline.
   - Verify no subsystem is skipped due to registration/order refactor.
   - Verify no duplicate execution occurs in a single week.

6. **If tests exist**
   - Run relevant tests, ideally all available simulation/time tests.
   - If the workspace supports it:
     - `dotnet test`

# Risks and follow-ups

- **Risk: hidden mutation through delegates**
  - Some subsystems may currently mutate state in response to time-advanced broadcasts. This can preserve accidental ordering bugs even after centralizing orchestration.
  - Follow-up: move all business mutation into explicit director-owned calls and keep delegates notification-only.

- **Risk: internal subsystem nondeterminism remains**
  - This task is about cross-subsystem ordering, but individual subsystems may still iterate unordered collections internally.
  - Follow-up: audit subsystem internals for deterministic iteration, especially charts, finance posting, event generation, and release processing.

- **Risk: monthly transition logic may run twice**
  - During migration from monthly-only logic, wrapping legacy code can accidentally duplicate updates.
  - Follow-up: add guards/tests around monthly rollup invocation counts.

- **Risk: Unreal subsystem discovery order**
  - If execution depends on `GetSubsystem` enumeration or registration timing, behavior may vary.
  - Follow-up: keep the simulation director’s ordered call list explicit and local.

- **Risk: insufficient automated test coverage**
  - The workspace context suggests `.NET` commands, but the architecture is Unreal C++. Test infrastructure may be incomplete.
  - Follow-up: add Unreal automation tests for simulation phase ordering and deterministic replay traces.

- **Recommended next tasks after this one**
  - add deterministic tie-breaking inside chart calculations
  - add deterministic trace snapshots for save/load reproducibility
  - audit stable ordering of event/news generation
  - formalize a simulation phase interface for future subsystem additions