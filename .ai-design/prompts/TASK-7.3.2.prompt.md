# Goal
Implement backlog task **TASK-7.3.2** for **ST-103 — Command dispatcher for gameplay actions** by introducing or completing a **structured command result model** so gameplay commands return **success/failure state plus user-facing messages** in a consistent, typed way.

This task should ensure command execution no longer relies on ad hoc booleans, raw strings, or implicit failure handling. The result shape must be suitable for:
- command dispatcher logic
- subsystem validation
- UI feedback
- future event/news integration
- automated tests

Align the implementation with the architecture direction:
- **typed commands**
- **centralized validation**
- **widgets stay thin**
- **single-player but server-style validation**
- **stable, structured results with localized/user-facing messaging**

# Scope
Focus only on the result-contract portion of the command system for covered gameplay actions under ST-103.

In scope:
- Add or refine a shared **command result struct/type** for gameplay commands.
- Ensure the result includes at minimum:
  - success/failure flag
  - user-facing message
  - optional machine-readable error code / reason
  - optional payload fields if already idiomatic in the codebase
- Update command dispatcher execution paths to return this structured result consistently.
- Normalize existing covered commands to use the shared result type.
- Add tests for success and failure cases.
- Preserve compatibility with current architecture and code style.

Out of scope unless required by existing code coupling:
- Building a generic reflection-based command bus
- Large UI rewrites
- Full domain event pipeline work beyond what is needed to compile
- New gameplay features unrelated to command result handling
- Broad save/load changes

If the workspace already has partial command/result infrastructure, extend it rather than replacing it wholesale.

# Files to touch
Inspect the repo first and then modify the smallest correct set of files. Likely targets include:

- Command dispatcher subsystem/application layer files
  - e.g. `*CommandDispatcher*`, `*Commands*`, `*Application*`
- Shared gameplay/common types
  - e.g. `*CommandResult*`, `*ResultTypes*`, `*ErrorCodes*`, `*Types*`
- Covered command definitions
  - sign artist
  - start recording
  - schedule release
  - allocate marketing
  - plan tour
  - advance time
- Any UI-facing adapter/view-model code that consumes command responses
- Automated test files for command execution and validation behavior
- Potentially `README.md` or developer docs only if there is an existing section documenting command patterns

Do not touch generated, intermediate, or unrelated engine/build files.

# Implementation plan
1. **Discover current command architecture**
   - Search for:
     - command dispatcher subsystem
     - command structs
     - existing result/response types
     - direct subsystem mutation from UI
   - Identify the currently covered ST-103 actions and how they return outcomes today.
   - Determine whether the project is currently C++, C#, or mixed tooling despite the workspace stack hint. Follow the actual gameplay source language in the repo.

2. **Define a shared structured result contract**
   - Introduce or standardize a single reusable result type for command execution.
   - Preferred shape should include fields equivalent to:
     - `bSuccess` / `Success`
     - `UserMessage`
     - `ErrorCode` or `ErrorCodes`
     - optional `RemediationHint`
     - optional IDs/data created by successful execution if already needed
   - Keep the type lightweight and easy for UI/tests to consume.
   - If Unreal C++:
     - use `USTRUCT(BlueprintType)` if UI/Blueprint exposure is useful or already used
     - use `FText` for user-facing messages
     - use enum-based error codes where practical
   - If C#:
     - use a small immutable/record-like result type if idiomatic
     - keep user-facing message separate from machine-readable code

3. **Add machine-readable error codes**
   - Introduce a focused enum for command failures if one does not exist.
   - Include only codes justified by current covered commands, such as:
     - `None`
     - `InsufficientFunds`
     - `InvalidArtistState`
     - `SongLocked`
     - `ContractRequired`
     - `DateConflict`
     - `InvalidReference`
     - `ValidationFailed`
     - `UnsupportedAction`
   - Avoid overengineering; start with a minimal useful set.

4. **Provide helper factories/builders**
   - Add convenience constructors/helpers for:
     - success with message
     - failure with code and message
   - This reduces duplication and keeps messages consistent.
   - Example intent:
     - `Success("Recording started.")`
     - `Failure(ECommandErrorCode::InsufficientFunds, "Not enough funds to start this recording.")`

5. **Update dispatcher methods to return the shared result**
   - For each covered command path, ensure the dispatcher returns the structured result instead of:
     - raw bool
     - nullable object
     - thrown validation exceptions for normal gameplay failures
     - unstructured strings
   - Validation failures should return structured failure results, not crash or silently no-op.
   - Preserve existing domain event emission on success where already implemented.

6. **Normalize validation messaging**
   - Ensure each failure path produces a clear user-facing message.
   - Messages should be concise and actionable, for example:
     - “Not enough funds to sign this artist.”
     - “This artist is not eligible to record right now.”
     - “This release cannot be scheduled before recording is complete.”
   - Avoid leaking internal-only technical details into user messages.
   - Log technical context separately if needed.

7. **Update success responses**
   - Ensure successful commands also return meaningful messages, for example:
     - “Artist signed successfully.”
     - “Recording started.”
     - “Release scheduled.”
   - If the codebase already uses notifications/toasts, keep the result compatible with that flow.

8. **Adapt consumers**
   - Update any immediate callers/tests/UI adapters that expect old return types.
   - Keep widget logic thin:
     - dispatch command
     - inspect structured result
     - display `UserMessage`
     - react to `Success`
   - Do not move business logic into UI.

9. **Add or update automated tests**
   - Add tests covering at least:
     - a successful command returns success + non-empty user message
     - a validation failure returns failure + expected error code + non-empty user message
     - dispatcher behavior is consistent across at least 2–3 command types
   - Prefer focused unit tests around dispatcher/application logic.

10. **Keep implementation incremental**
   - If all ST-103 commands are not yet fully implemented, apply the structured result pattern to the commands that exist and leave clear TODOs for remaining covered actions.
   - Do not block on unrelated architecture cleanup.

# Validation steps
1. **Code inspection**
   - Confirm all covered command execution entry points return the shared structured result type.
   - Confirm no normal validation path relies solely on bool/string/exception.

2. **Build**
   - Run the appropriate build for the actual source language found in the repo.
   - If .NET tests/projects are the only available automation in this workspace, run:
     - `dotnet build`
   - If test projects exist, run:
     - `dotnet test`

3. **Automated tests**
   - Verify tests cover both success and failure result shapes.
   - Verify expected error codes/messages are asserted.

4. **Behavior verification**
   - For at least one command success path and one failure path, confirm:
     - success flag is correct
     - user-facing message is populated
     - machine-readable error code is correct
   - If UI adapters exist, confirm they can consume the result without additional parsing hacks.

5. **Regression check**
   - Ensure successful command execution still performs the underlying mutation and emits any existing domain events.
   - Ensure failure paths do not partially mutate state.

# Risks and follow-ups
- **Repo mismatch risk:** Workspace hints show `.NET`, while architecture targets Unreal/C++. Use the actual gameplay implementation language present in the repo and do not force Unreal-specific patterns if this repository currently models gameplay logic elsewhere.
- **Partial command infrastructure:** The repo may not yet contain all ST-103 commands. Apply the result pattern to existing commands and keep the design extensible.
- **UI coupling risk:** Some widgets or callers may currently assume direct mutation or primitive return values. Update only the necessary consumers to preserve behavior.
- **Localization risk:** If localization infrastructure is not yet established, still structure messages so they can be localized later.
- **Error taxonomy creep:** Keep error codes minimal and practical; avoid designing a huge enum for future hypothetical cases.
- **Follow-up recommended:** After this task, align all command handlers under the same result contract and add a shared notification/UI adapter so screens display command outcomes consistently.