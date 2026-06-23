# Goal
Implement backlog task **TASK-7.3.5** for **ST-103 Command dispatcher for gameplay actions** by ensuring the command-dispatcher implementation stays **lightweight and explicit**, with **no generic reflection-based bus**.

The coding agent should:
- preserve the architectural intent of a typed command/application layer,
- avoid introducing dynamic registration, reflection, string-based dispatch, or overly abstract bus infrastructure,
- favor a small, maintainable Unreal/C++ implementation that directly supports the currently required gameplay actions.

This task is specifically about **keeping the implementation simple** while still meeting the story goals:
- typed commands,
- structured results,
- centralized validation/state mutation,
- domain event emission,
- UI no longer directly mutating covered simulation state.

# Scope
In scope:
- Review the current implementation for ST-103-related work.
- Refactor or implement the command dispatcher as a **concrete, typed subsystem/application service**.
- Support only the currently identified core actions where present in codebase:
  - sign artist,
  - start recording,
  - schedule release,
  - allocate marketing,
  - plan tour,
  - advance time.
- Use explicit methods such as `ExecuteSignArtist`, `ExecuteStartRecording`, etc.
- Use typed payload structs and a shared structured result type.
- Keep event emission explicit and compatible with existing subsystem/event patterns.
- Update call sites so covered UI flows stop mutating simulation state directly.

Out of scope:
- Building a reusable generic command bus framework.
- Reflection-based dispatch using `UClass`, `UStruct`, `ProcessEvent`, type-erasure, or string-to-handler maps.
- Plugin-style handler registration.
- A full CQRS/event-sourcing architecture.
- Expanding beyond the currently needed commands unless required by existing compile paths.
- Broad UI redesign unrelated to replacing direct mutations with dispatcher calls.

Implementation constraints:
- Prefer minimal code and low-risk changes.
- Match existing project conventions and naming where possible.
- If the codebase is partial/incomplete, implement the smallest coherent slice that compiles cleanly.
- Do not invent unnecessary abstractions.

# Files to touch
Inspect first, then touch only the minimum necessary set. Likely candidates include:

- `README.md` only if there is an existing architecture/dev note section that should mention the lightweight dispatcher approach.
- Unreal gameplay subsystem headers/cpps related to:
  - `UCommandDispatcherSubsystem` or equivalent new file(s),
  - `UUIManagerSubsystem`,
  - `UGameTimeSubsystem`,
  - artist/record/release/finance/tour-related subsystems currently mutated directly by UI.
- Command/result/event type definition files if a shared location already exists.
- Relevant widget/controller/view-model bridge files that currently call business subsystems directly.

If creating new files, prefer a focused pair such as:
- `Source/.../Subsystems/CommandDispatcherSubsystem.h`
- `Source/.../Subsystems/CommandDispatcherSubsystem.cpp`

Potential supporting files:
- `Source/.../Types/CommandTypes.h`
- `Source/.../Types/CommandResult.h`

Do not create a large folder tree or framework unless the repository already has a clear home for these types.

# Implementation plan
1. **Inspect the existing codebase**
   - Find current ST-103-related implementation or partial dispatcher work.
   - Identify where widgets/UI currently mutate gameplay state directly.
   - Identify existing event/delegate patterns and result/error patterns.
   - Confirm actual module/file layout before adding files.

2. **Design the lightweight shape**
   - Implement or refactor to a concrete `UGameInstanceSubsystem` command dispatcher.
   - Expose explicit typed methods, for example:
     - `ExecuteSignArtist(const FSignArtistCommand&)`
     - `ExecuteStartRecording(const FStartRecordingCommand&)`
     - `ExecuteScheduleRelease(const FScheduleReleaseCommand&)`
     - `ExecuteAllocateMarketing(const FAllocateMarketingCommand&)`
     - `ExecutePlanTour(const FPlanTourCommand&)`
     - `ExecuteAdvanceTime(const FAdvanceTimeCommand&)`
   - Use plain typed structs for payloads.
   - Use a shared `FCommandResult` or equivalent with:
     - success flag,
     - user-facing message,
     - optional error code(s),
     - optional created/affected IDs if project patterns support it.

3. **Explicitly avoid generic bus behavior**
   - Do not add:
     - handler registries,
     - reflection-based routing,
     - string command names for execution,
     - templated generic dispatch frameworks unless already trivially present and simpler than replacing.
   - If any generic bus already exists in partial form, simplify it to explicit methods where feasible.
   - Keep command handling code readable and discoverable.

4. **Implement validation and orchestration**
   - Each execute method should:
     - validate input,
     - check preconditions against owning subsystems,
     - call the appropriate subsystem(s),
     - emit domain events through the existing event mechanism,
     - return structured result.
   - Validation should remain “server-style” even though this is single-player.

5. **Wire covered UI flows through the dispatcher**
   - Replace direct UI-to-subsystem mutations for covered actions with dispatcher calls.
   - Keep widgets thin.
   - Preserve existing UI behavior/messages where possible.
   - If some actions are not yet fully implemented in subsystems, add TODO-safe stubs only if necessary to keep compile integrity and make unsupported actions fail clearly.

6. **Keep event emission explicit**
   - Reuse existing typed delegates/events if present.
   - If adding events, keep them typed and minimal.
   - Do not introduce a second generic event framework as part of this task.

7. **Minimize surface area**
   - Only add the commands actually needed by current code paths or story scope.
   - Avoid speculative abstractions for future commands.
   - Prefer straightforward subsystem lookups and direct method calls over indirection.

8. **Document intent in code**
   - Add brief comments where useful to clarify:
     - this dispatcher is intentionally explicit/lightweight,
     - generic reflection-based dispatch is intentionally not used,
     - new commands should be added as explicit methods unless requirements change.

# Validation steps
1. **Build/compile validation**
   - Run the most appropriate available build command from workspace context:
     - `dotnet build`
   - If there are tests and they are relevant/runnable:
     - `dotnet test`

2. **Static code validation**
   - Confirm no new reflection-based or string-based dispatch path was introduced.
   - Confirm command payloads are typed structs, not loosely typed maps/JSON/string payloads.
   - Confirm dispatcher methods are explicit and named per action.

3. **Behavior validation**
   - Verify covered UI flows now route through the dispatcher instead of directly mutating business subsystems.
   - Verify each implemented command returns structured success/failure results.
   - Verify validation failures produce user-facing messages.
   - Verify successful execution emits the expected domain event(s) through the existing event pipeline.

4. **Architecture validation**
   - Confirm subsystem ownership remains intact:
     - dispatcher validates/orchestrates,
     - business subsystems own state mutation,
     - UI remains thin.
   - Confirm no unnecessary framework or abstraction layer was added.

5. **Regression check**
   - Ensure existing direct subsystem APIs still work where needed internally.
   - Ensure no unrelated systems were refactored beyond what was necessary for this task.

# Risks and follow-ups
- **Risk: repository may not actually contain the Unreal source files implied by the architecture.**
  - If so, implement the smallest compatible structure in the existing code layout and note assumptions clearly in code/comments.

- **Risk: existing partial dispatcher may already use generic patterns.**
  - Simplify carefully to avoid breaking call sites; prefer incremental refactor over wholesale rewrite.

- **Risk: some commands may not yet have complete subsystem support.**
  - In that case, implement only the commands that can be wired correctly now, and leave explicit TODOs for unsupported actions rather than inventing placeholder frameworks.

- **Risk: UI call sites may be numerous.**
  - Prioritize the covered actions currently reachable in code and avoid broad churn.

Follow-ups after this task:
- Add/complete command coverage for remaining gameplay actions as explicit methods.
- Standardize command error codes if not already unified.
- Add focused tests around dispatcher validation and event emission.
- Align with ST-104 event cleanup without introducing a generic bus.