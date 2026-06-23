# Goal
Implement backlog task **TASK-7.3.6 — Use typed structs for command payloads** for story **ST-103 Command dispatcher for gameplay actions**.

Refactor the command-dispatch path so gameplay actions use **strongly typed C++ payload structs** instead of loosely typed/stringly typed/maps/individual ad hoc parameters wherever command execution currently occurs. The result should align with the architecture direction:

- UI sends typed commands
- command dispatcher validates and executes
- subsystems mutate state
- results remain structured
- domain events continue to be emitted
- widgets do not directly mutate covered gameplay state

Because no explicit acceptance criteria were provided for this task, infer completion from ST-103 and the architecture notes, with emphasis on:
- typed command payload definitions for core actions
- dispatcher APIs consuming those payloads
- minimal breakage to existing callers
- lightweight implementation, not a generic reflection bus

# Scope
In scope:
- Introduce or consolidate **`USTRUCT` command payloads** for the currently covered gameplay actions in the command dispatcher layer.
- Update dispatcher method signatures to accept typed payload structs.
- Update call sites in UI/application code to construct and pass typed payloads.
- Preserve or improve structured command results.
- Keep validation centralized in the dispatcher/application layer.
- Add compatibility shims only if needed to avoid broad breakage during refactor.
- Add/adjust tests for command payload usage if test coverage exists nearby.

Target command payloads should cover the core ST-103 actions where present in codebase, such as:
- sign artist
- start recording
- schedule release
- allocate marketing
- plan tour
- advance time

Out of scope:
- building a generic command bus framework
- rewriting unrelated subsystem business logic
- changing domain event schemas unless required by the refactor
- broad UI redesign
- save/load schema changes unless command payloads are incorrectly persisted somewhere
- introducing Blueprint-heavy abstractions unless already used by current dispatcher APIs

# Files to touch
Inspect the repo first and then update the actual files you find. Likely candidates include:

- Command dispatcher subsystem/application layer files
  - e.g. `*CommandDispatcher*.*`
  - e.g. `*ApplicationLayer*.*`
- Shared command/result type headers
  - e.g. `*CommandTypes*.*`
  - e.g. `*Commands*.*`
  - e.g. `*CommandResult*.*`
- UI manager / widget integration files that dispatch gameplay actions
  - e.g. `*UIManagerSubsystem*.*`
  - e.g. relevant `UUserWidget` classes for artist/record/release/tour actions
- Any subsystem interfaces currently taking loose command parameters from dispatcher
- Existing tests covering command execution
- Potentially module build files only if new headers/source files must be registered

Prefer creating a dedicated shared header for typed command payloads if one does not already exist, for example:
- `Source/.../Commands/MusicCommandPayloads.h`
or
- `Source/.../Application/CommandPayloads.h`

If the project is not yet using UE C++ in the visible workspace, first locate the actual gameplay source tree under the Unreal solution and adapt accordingly.

# Implementation plan
1. **Discover the current command path**
   - Find the command dispatcher subsystem/class used for gameplay actions.
   - Identify all public execution methods and current payload style:
     - separate primitive parameters
     - `FName`/`FString` command IDs
     - maps/dictionaries/json-like payloads
     - direct widget-to-subsystem mutation bypasses
   - Map current covered actions to ST-103 core actions.

2. **Define typed command payload structs**
   - Create `USTRUCT(BlueprintType)` payloads if these commands are used by UI/Blueprint; otherwise plain `USTRUCT()` is fine.
   - Keep payloads lightweight, explicit, and serializable-friendly.
   - Use stable IDs in fields, not display names.
   - Prefer existing project ID types if already standardized; otherwise use the current canonical type in codebase.
   - Suggested shapes, adapted to existing types:

   ```cpp
   USTRUCT(BlueprintType)
   struct FSignArtistCommand
   {
       GENERATED_BODY()

       UPROPERTY(EditAnywhere, BlueprintReadWrite)
       FString ArtistId;

       UPROPERTY(EditAnywhere, BlueprintReadWrite)
       FString LabelId;

       UPROPERTY(EditAnywhere, BlueprintReadWrite)
       int32 Advance = 0;

       UPROPERTY(EditAnywhere, BlueprintReadWrite)
       float RoyaltyRate = 0.0f;

       UPROPERTY(EditAnywhere, BlueprintReadWrite)
       int32 AlbumCommitment = 0;
   };

   USTRUCT(BlueprintType)
   struct FStartRecordingCommand
   {
       GENERATED_BODY()

       UPROPERTY(EditAnywhere, BlueprintReadWrite)
       FString ArtistId;

       UPROPERTY(EditAnywhere, BlueprintReadWrite)
       TArray<FString> SongDefIds;

       UPROPERTY(EditAnywhere, BlueprintReadWrite)
       int32 StudioTier = 0;

       UPROPERTY(EditAnywhere, BlueprintReadWrite)
       FString ProducerId;

       UPROPERTY(EditAnywhere, BlueprintReadWrite)
       FString TargetType;
   };
   ```

   Also add typed structs for release scheduling, marketing allocation, tour planning, and time advancement if those commands exist.

3. **Standardize command result usage**
   - Reuse existing `FCommandResult` if present.
   - If needed, improve it without broad redesign:
     - `bSuccess`
     - user-facing message
     - optional error code enum/list
     - optional created/affected IDs
   - Do not over-expand scope into a full generic result hierarchy unless already present.

4. **Refactor dispatcher APIs to accept typed payloads**
   - Replace loose signatures like:
     - `ExecuteSignArtist(ArtistId, LabelId, Advance, RoyaltyRate, AlbumCommitment)`
     - `ExecuteCommand(FName CommandName, TMap<FString, FString> Params)`
   - With typed signatures like:
     - `ExecuteSignArtist(const FSignArtistCommand& Command)`
     - `ExecuteStartRecording(const FStartRecordingCommand& Command)`
   - Keep method names explicit and action-specific.
   - If there are many existing callers, add temporary overloads that construct the typed struct and forward internally, then mark them for cleanup.

5. **Move validation to typed payload handling**
   - Ensure dispatcher validation reads from the typed struct fields.
   - Preserve server-style validation:
     - existence checks
     - affordability/funds
     - eligibility/state checks
     - date conflicts
     - invalid references
   - Avoid pushing validation back into widgets.

6. **Update UI/application call sites**
   - Replace ad hoc parameter passing with explicit command construction.
   - Widgets/UI manager should gather user input, populate the typed struct, and call dispatcher.
   - Keep widgets thin; no business logic should be added during refactor.
   - If Blueprint-exposed, ensure payload structs and dispatcher methods remain callable.

7. **Preserve domain event emission**
   - Ensure successful command execution still emits the same typed domain events.
   - Do not regress UI/news refresh behavior.

8. **Add focused documentation/comments**
   - Add brief comments near command payload definitions describing intended use.
   - If there is a README or architecture note for command dispatch, update it only if a small targeted note is appropriate.

9. **Testing**
   - Add or update tests around dispatcher execution if test infrastructure exists.
   - At minimum, cover:
     - typed payload accepted and executed
     - invalid payload returns structured failure
     - old compatibility overloads forward correctly, if retained

10. **Keep the refactor narrow**
   - Do not convert unrelated event payloads or view models unless they are currently conflated with command payloads and block completion.
   - Prefer incremental cleanup over sweeping architecture changes.

# Validation steps
1. **Static code review**
   - Confirm each covered dispatcher action now has a dedicated typed payload struct.
   - Confirm dispatcher public APIs use typed payloads rather than loose primitive bundles/string maps.
   - Confirm payload fields use stable IDs or existing canonical identifier types.

2. **Build**
   - From workspace root, run:
     - `dotnet build`
   - If this repo includes the Unreal C++ project outside the visible .NET hints, also use the appropriate Unreal build path if available in repo docs.

3. **Tests**
   - Run:
     - `dotnet test`
   - If there are command-dispatch unit/integration tests, ensure they pass after signature changes.

4. **Manual code-path verification**
   - Verify UI or UI manager call sites now construct typed command structs for covered actions.
   - Verify no covered widget directly mutates subsystem state where dispatcher should be used.
   - Verify command results still return structured success/failure and user-facing messages.

5. **Behavior verification**
   - For each implemented command path present in codebase, confirm:
     - valid payload succeeds
     - invalid payload fails with structured result
     - success path still triggers downstream subsystem mutation
     - success path still emits expected domain events

6. **Regression check**
   - Search for old stringly typed command payload patterns and confirm they are removed or intentionally shimmed:
     - command maps
     - raw JSON-like payloads
     - long primitive parameter lists for dispatcher methods
   - If compatibility overloads remain, ensure they are thin wrappers only.

# Risks and follow-ups
- **Risk: Unreal source may not be in the visible workspace root.**
  - Follow-up: locate the actual `Source/` tree in the solution before implementing; do not assume the .NET hint reflects the gameplay code structure.

- **Risk: Existing UI may rely on Blueprint-callable dispatcher signatures.**
  - Follow-up: use `USTRUCT(BlueprintType)` and preserve `UFUNCTION` compatibility where needed.

- **Risk: Broad signature changes can create many compile breaks.**
  - Follow-up: add temporary forwarding overloads for old signatures if necessary, but keep new typed payload APIs as the primary path.

- **Risk: Inconsistent ID types across subsystems.**
  - Follow-up: use the project’s current canonical ID type rather than introducing a new one in this task.

- **Risk: Some commands may not yet exist in codebase despite architecture intent.**
  - Follow-up: only implement typed payloads for real existing command paths plus any minimal adjacent scaffolding needed for consistency.

- **Risk: Widgets may still bypass dispatcher for some actions.**
  - Follow-up: if discovered, convert only the covered ST-103 actions touched by this task and note remaining bypasses for future backlog work.

- **Recommended follow-up tasks**
  - remove temporary compatibility overloads after callers are migrated
  - add dedicated command validation tests per action
  - standardize command error codes across all dispatcher results
  - align command payload ID fields with ST-102 stable ID standard if any legacy name-based fields remain