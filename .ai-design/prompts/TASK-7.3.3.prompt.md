# Goal
Implement backlog task **TASK-7.3.3** for **ST-103 — Command dispatcher for gameplay actions** by refactoring covered UI actions so that **widgets no longer directly mutate simulation/business subsystem state**.

The coding agent should:

- identify the currently covered gameplay actions already routed or intended to be routed through the command dispatcher
- remove direct widget-to-subsystem mutation for those actions
- ensure widgets gather input, call the command dispatcher/application layer, and react only to structured results/events
- preserve existing player-facing behavior as much as possible
- keep the implementation lightweight and aligned with the architecture:
  - **subsystems own business state**
  - **widgets are thin**
  - **commands mutate state**
  - **events/results notify UI**

Because this task is specifically about eliminating direct mutation from widgets for **covered actions**, do **not** broaden scope into a full generic command bus rewrite unless required by the existing code structure.

# Scope
In scope:

- Audit UI widgets, UI manager code, and related presentation-layer classes for covered gameplay actions under ST-103, especially:
  - sign artist
  - start recording
  - schedule release
  - allocate marketing
  - plan tour
  - advance time
- Replace direct calls from widgets to business/simulation subsystem mutators with command-dispatcher calls.
- Introduce or complete typed command payloads and structured command results where missing for the covered actions.
- Keep or add user-facing success/failure messaging from command execution.
- Update UI flow so widgets:
  - collect input
  - dispatch command
  - display result
  - refresh from events/read models rather than assuming mutation succeeded
- Add minimal compatibility shims only if needed to avoid breaking unrelated screens during refactor.

Out of scope unless required to complete this task safely:

- building a fully generic reflection-based command bus
- redesigning unrelated UI screens
- large-scale event pipeline cleanup beyond what is needed for covered actions
- save/load changes
- broad simulation logic changes unrelated to command routing
- adding new gameplay features beyond existing covered actions

# Files to touch
Touch the smallest set of files necessary, but expect to inspect and possibly modify files in these areas:

- **Command/application layer**
  - command dispatcher subsystem/class
  - command structs for covered actions
  - command result/result-code types
- **UI orchestration**
  - `UUIManagerSubsystem` or equivalent UI routing/orchestration classes
  - selection/context helpers if they are currently used to infer mutation targets
- **Widgets / presentation classes**
  - artist roster/detail/command widgets
  - recording planner widgets
  - release planner widgets
  - finance/marketing widgets
  - tour planner widgets
  - status/dashboard/time advance widgets
- **Subsystem interfaces**
  - only where needed to move mutation behind dispatcher-owned execution paths
- **Events / notifications**
  - typed event emission or existing delegate hooks used by UI refresh
- **Tests**
  - existing unit/integration tests around command dispatch or UI orchestration
  - add focused tests for “widget no longer mutates subsystem directly” behavior where practical

If the repo structure is unclear, first locate:
- command dispatcher implementation
- widget handlers for button clicks / confirm actions
- subsystem public mutator methods currently called by widgets

# Implementation plan
1. **Discover current command coverage**
   - Find the existing command dispatcher/application layer.
   - Enumerate which ST-103 actions are already implemented or partially implemented.
   - Map each covered action to:
     - command struct/type
     - dispatcher entry point
     - owning subsystem mutator
     - UI widgets/screens invoking it

2. **Audit direct widget mutations**
   - Search widget and UI-layer code for direct calls into subsystem mutators for covered actions.
   - Look for patterns such as:
     - widget obtaining subsystem via `GetGameInstance()->GetSubsystem<...>()`
     - widget directly calling methods like sign/start/schedule/allocate/plan/advance
     - widget mutating stateful objects or arrays owned by subsystems
   - Build a short internal list of each direct mutation site before editing.

3. **Define the target interaction pattern**
   - For each covered action, the desired flow should be:
     1. widget gathers input
     2. widget calls UI manager or dispatcher directly, depending on current architecture
     3. dispatcher validates and executes
     4. dispatcher returns structured result
     5. subsystem emits domain event / existing notification
     6. UI refreshes from read models or event-driven updates
   - Prefer routing through `UUIManagerSubsystem` if that is already the established presentation boundary; otherwise let widgets call the dispatcher directly only if that is the current intended pattern.

4. **Refactor covered widget handlers**
   - Replace direct subsystem mutation calls with dispatcher calls.
   - Preserve existing UX:
     - button enable/disable behavior
     - validation messages
     - confirmation dialogs
     - success/failure toasts or labels
   - Ensure widgets no longer assume success by mutating local UI state optimistically unless that pattern already exists and is backed by command results.

5. **Normalize command payload/result usage**
   - For each covered action, ensure there is a typed command payload.
   - Ensure dispatcher returns a structured result with at least:
     - success flag
     - user-facing message
     - optional error code(s)
   - If multiple ad hoc result types exist, unify only as much as needed for consistency in the touched flows.

6. **Keep subsystem mutation centralized**
   - Ensure business state changes happen inside the dispatcher-owned execution path and/or owning subsystem methods invoked by the dispatcher.
   - Do not leave alternate direct widget mutation paths for the same covered action.
   - If necessary, reduce visibility of subsystem mutators or add comments/TODOs marking them as non-UI entry points.

7. **Wire UI refresh to results/events**
   - After command execution, widgets should:
     - show result feedback
     - request refreshed projections/read models if needed
     - rely on existing events/delegates for broader screen updates
   - Avoid embedding business recalculation in widgets.

8. **Add focused safeguards**
   - Add lightweight assertions, comments, or helper wrappers to discourage future direct widget mutation.
   - If practical, centralize widget command invocation through a small helper in the UI manager.

9. **Testing**
   - Add or update tests to cover:
     - covered action succeeds through dispatcher path
     - invalid input returns structured failure
     - widget-facing handler no longer calls subsystem mutator directly
   - If UI-level automated tests are impractical, add lower-level tests around UI manager/dispatcher integration.

10. **Document assumptions in code comments**
   - Where architecture is still transitional, leave concise comments indicating:
     - widgets must not mutate simulation state directly
     - covered actions must route through dispatcher
     - remaining non-covered actions can be migrated later

# Validation steps
1. **Inspect and build**
   - Restore/build the solution after changes.
   - Use the workspace-provided commands where applicable:
     - `dotnet build`
     - `dotnet test`
   - If the Unreal/C++ code is not buildable through the provided .NET commands alone, still run the available commands and note any workspace limitations.

2. **Static verification**
   - Search the touched widget/UI files to confirm covered actions no longer directly call subsystem mutators.
   - Verify direct calls were replaced by command dispatcher or UI-manager-mediated command execution.

3. **Behavior verification for each covered action found in repo**
   - For each implemented covered action:
     - trigger from UI handler path
     - confirm command payload is created
     - confirm dispatcher executes
     - confirm structured result is returned
     - confirm UI feedback still appears
   - Verify failure cases still surface meaningful messages.

4. **Regression checks**
   - Confirm screens still open and selection/context flows still work.
   - Confirm no duplicate execution occurs from both old and new paths.
   - Confirm event-driven refresh still updates relevant widgets after successful commands.

5. **Code quality checks**
   - Ensure no new business logic was added to widgets.
   - Ensure subsystem ownership boundaries remain clearer after the refactor, not blurrier.

6. **Report**
   - Summarize:
     - which covered actions were found and migrated
     - which files were changed
     - any covered actions not present in the current codebase
     - any remaining direct mutation sites intentionally left because they are outside this task’s covered scope

# Risks and follow-ups
- **Repo/stack mismatch risk:** Workspace hints show `.NET`, while the architecture describes Unreal Engine C++/UMG. The agent should inspect the actual repo structure first and adapt terminology/implementation to the real codebase without forcing Unreal-specific patterns where they do not exist.
- **Partial architecture risk:** Some covered actions may not yet have full dispatcher support. In that case, implement the minimum dispatcher path needed for the touched UI flows rather than overengineering.
- **Hidden coupling risk:** Widgets may currently depend on immediate side effects from direct subsystem mutation. Refactoring may require small follow-up UI refresh fixes.
- **Event gap risk:** Some flows may rely on direct widget refresh instead of domain events. If events are missing, use structured command results plus targeted refresh calls as a transitional step.
- **Scope creep risk:** Do not migrate every UI action in the project. Limit changes to covered ST-103 actions found in the codebase.
- **Follow-up recommendation:** After this task, create a separate cleanup task to:
  - migrate remaining non-covered direct widget mutations
  - tighten subsystem mutator visibility
  - standardize command result/error codes
  - expand automated tests around UI-manager/dispatcher integration