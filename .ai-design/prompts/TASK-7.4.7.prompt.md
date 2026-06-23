# Goal
Implement backlog task **TASK-7.4.7 — Keep event payloads serializable where practical** for **ST-104 Domain event pipeline cleanup**.

The coding agent should update the domain event pipeline so that event payloads are consistently shaped for safe serialization, replay, save/load compatibility, and UI/news consumption, while preserving the architecture principle that **simulation emits typed domain events** and **presentation reacts without owning business logic**.

This task is specifically about making event payloads serialization-friendly where practical, not redesigning the full event system from scratch.

# Scope
In scope:

- Audit existing domain event payload structs/classes used by simulation, UI, and event/news systems.
- Refactor payloads so they prefer:
  - stable IDs over raw object pointers/references
  - value types over transient engine/runtime-only state
  - explicit dates/timestamps where relevant
  - serializable Unreal-friendly fields (`USTRUCT`, `UPROPERTY`) where appropriate
- Remove or reduce non-serializable members from event payloads where practical, such as:
  - direct `UObject*` / subsystem pointers
  - widget references
  - lambdas/delegates captured inside payloads
  - raw asset paths from untrusted runtime state
  - presentation-only transient state
- Ensure payloads remain sufficient for:
  - UI refresh decisions
  - news generation
  - possible save replay / debug trace usage
- Keep event payloads aligned with ST-104 notes:
  - include stable IDs and dates sufficient for UI refresh and save replay
  - keep simulation decoupled from presentation
  - keep payloads serializable where practical

Out of scope unless required by compilation:

- Full replacement of the event bus architecture
- Large UI rewrites
- Save-game schema expansion beyond what is needed to support serializable event payloads
- Adding a generic reflection-heavy event framework
- Reworking unrelated command or subsystem logic

# Files to touch
Prioritize the smallest set of files that define and consume domain event payloads. Likely areas include:

- Event payload definition headers/cpp files
- `UEventSubsystem` files that translate domain events into news
- `UUIManagerSubsystem` files that subscribe to and react to events
- Any shared event bus / dispatcher / delegate definitions
- Any simulation subsystem files emitting events with non-serializable payloads

Based on the repo, first inspect and then touch only the relevant files, likely under source folders for:

- simulation/domain events
- event/news subsystem
- UI manager subsystem
- shared model/struct definitions

If there is no centralized event payload file yet, create one in the most appropriate shared gameplay/domain location and migrate payload definitions there.

# Implementation plan
1. **Discover the current event pipeline**
   - Search for:
     - `Event`
     - `On...`
     - `Broadcast`
     - `DECLARE_`
     - `UEventSubsystem`
     - `UUIManagerSubsystem`
     - domain event structs
   - Identify all typed event payloads currently emitted by simulation subsystems.
   - Build a quick inventory of payload fields and classify them as:
     - serializable-safe
     - questionable
     - non-serializable/transient

2. **Define a serialization-friendly payload standard**
   Apply these rules unless a strong local reason prevents it:
   - Use `USTRUCT(BlueprintType)` for shared payload structs when appropriate.
   - Mark fields with `UPROPERTY()` if they should participate in Unreal serialization/reflection.
   - Prefer stable identifiers:
     - `FGuid`, `FName`, `FString`, or project-standard ID types
   - Include explicit simulation date/week fields where relevant.
   - Prefer plain structs, enums, numbers, booleans, text, names, arrays, and maps of serializable value types.
   - Avoid embedding:
     - `UObject*`
     - subsystem references
     - widget references
     - actor/component references
     - raw delegates/callbacks
   - If a consumer needs richer data, it should resolve it from subsystem state using IDs after receiving the event.

3. **Refactor existing payloads**
   For each event payload found:
   - Replace object references with stable IDs.
   - Replace transient display-only fields with durable equivalents where possible.
   - Keep only the minimum fields needed for:
     - UI invalidation/refresh routing
     - news generation
     - replay/debug/save compatibility
   - If a payload currently includes both object refs and IDs, remove the refs unless clearly required and safe.
   - If a payload lacks date/time context for replay or UI refresh, add it.

4. **Preserve consumer behavior**
   Update event consumers so they resolve runtime objects/data from IDs instead of expecting direct references.
   - `UEventSubsystem` should generate player-facing news from serializable payload data plus subsystem lookups.
   - `UUIManagerSubsystem` should react by refreshing projections/view models using IDs, not by storing business objects from the event.
   - Keep widgets thin and avoid pushing presentation state back into simulation payloads.

5. **Add lightweight compatibility helpers if needed**
   If the codebase currently relies on richer payloads:
   - Add helper functions to resolve IDs into current subsystem projections.
   - Add adapter methods rather than bloating payloads again.
   - Keep compatibility shims small and clearly marked for future cleanup.

6. **Document intent in code**
   Add concise comments near shared event payload definitions stating:
   - payloads should remain serialization-friendly
   - use stable IDs and value types
   - consumers should resolve rich state from owning subsystems

7. **Do not over-engineer**
   The goal is practical serializability, not perfect persistence of every event type.
   If some event cannot reasonably be made fully serializable without major churn:
   - keep the payload as simple as possible
   - document the limitation in code or TODO
   - avoid blocking the task on a full redesign

# Validation steps
1. **Build / compile**
   - Run the most appropriate build command for the workspace.
   - Start with:
     - `dotnet build`
   - If there are tests:
     - `dotnet test`

2. **Static validation by inspection**
   Confirm that shared event payloads:
   - no longer contain direct UI/widget references
   - no longer contain subsystem pointers
   - prefer stable IDs over object references
   - include date/time context where relevant
   - use Unreal-serializable struct patterns where applicable

3. **Consumer validation**
   Verify that:
   - event emitters still compile and broadcast correctly
   - `UEventSubsystem` can still generate news from payloads
   - `UUIManagerSubsystem` can still react to events by refreshing via IDs/lookups
   - no consumer depends on removed transient fields without replacement

4. **Regression checks**
   If there are existing tests, update/add focused coverage for:
   - event payload construction
   - event-to-news translation
   - event consumer lookup by stable ID
   - serialization round-trip of representative payload structs, if test infrastructure supports it

5. **Manual code review checklist**
   Ensure the final implementation satisfies ST-104 intent:
   - typed events remain in place
   - simulation stays decoupled from presentation
   - payloads are serializable where practical
   - payloads include stable IDs and dates sufficient for UI refresh and save replay

# Risks and follow-ups
- **Risk: hidden consumer coupling**
  - Some UI or news code may implicitly rely on direct object references in payloads.
  - Mitigation: update consumers to resolve from IDs and keep compatibility helpers minimal.

- **Risk: over-pruning payloads**
  - Removing too much data may force expensive or awkward lookups.
  - Mitigation: keep enough durable context in payloads for efficient refresh and news generation.

- **Risk: Unreal serialization mismatch**
  - Some structs may not currently be `USTRUCT`/`UPROPERTY` compatible.
  - Mitigation: only convert shared payloads that benefit from serialization/reflection and keep changes localized.

- **Risk: inconsistent ID types**
  - The codebase may mix names, strings, and GUIDs.
  - Mitigation: follow existing project conventions where already established, but prefer stable ID fields consistently within event payloads.

Follow-up suggestions after this task, only if natural and low-cost:
- Add a small shared guideline doc or header comment for domain event payload design.
- Add tests for representative event payload serialization/replay.
- Consider a dedicated serializable event record type for debug traces or save replay if the project later needs full event persistence.