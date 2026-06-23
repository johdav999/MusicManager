# Goal
Implement backlog task **TASK-ST-104 — Domain event pipeline cleanup** for the Unreal-based MusicManager project.

The coding agent should refactor and standardize the event flow so that:

- **core simulation/business subsystems emit typed domain events**
- **UI state remains outside simulation**
- **`UEventSubsystem` translates domain events into player-facing news/feed items**
- **`UUIManagerSubsystem` reacts to domain events for refresh/routing without taking on business logic**
- **event payloads carry stable IDs and dates and are serializable-friendly where practical**

This task supports the architecture principle:

- subsystems own business state
- commands mutate state
- events notify UI
- widgets stay thin

There are no explicit acceptance criteria beyond the story/backlog details, so use the story acceptance criteria as the implementation target.

# Scope
Focus only on the **domain event pipeline cleanup** needed for ST-104.

In scope:

- Audit current event/delegate usage related to simulation, UI refresh, and news generation.
- Introduce or normalize **typed domain event payload structs** for major state changes already present in the codebase.
- Ensure event payloads use **stable IDs** rather than display names wherever possible.
- Add or refine a central event publishing/subscription path, likely through existing subsystems or a lightweight event bus pattern already compatible with the codebase.
- Update `UEventSubsystem` so it consumes domain events and produces **player-facing news items** with deduplication support.
- Update `UUIManagerSubsystem` so it subscribes to domain events and triggers UI refresh/update behavior without embedding business rules.
- Remove or reduce direct coupling where simulation subsystems push UI-specific state or messages.
- Keep payloads serializable-friendly: plain structs, IDs, dates, primitive fields, arrays of IDs.

Out of scope unless required to complete compilation/integration:

- Large UI redesigns
- New gameplay systems unrelated to events
- Full save/load replay system implementation
- Broad command dispatcher refactors beyond what is necessary to hook event emission
- Reworking every subsystem in the project if only a subset currently exists

If the current codebase is partial or inconsistent, implement the smallest coherent event pipeline that can be extended later.

# Files to touch
Inspect first, then update the most relevant files. Likely targets include:

- `README.md` if architecture notes or developer usage need a brief update
- Core subsystem headers/cpps under the game module, especially files resembling:
  - `*GameTimeSubsystem*`
  - `*EventSubsystem*`
  - `*UIManagerSubsystem*`
  - `*ArtistManagerSubsystem*`
  - `*ProductionSubsystem*` / `*RecordManagerSubsystem*`
  - `*FinanceManagerSubsystem*`
  - `*ChartManagerSubsystem*`
- Any shared types files suitable for:
  - domain event enums
  - event payload structs
  - news item structs
  - event keys / dedupe helpers
- Any existing command dispatcher files if command execution is currently where events should be emitted
- Any existing widget/view-model integration files only if needed to replace direct business mutation or direct subsystem polling

Prefer creating or consolidating shared event definitions into a small number of files, for example:

- `DomainEvents.h/.cpp`
- `MusicEventTypes.h/.cpp`
- `EventSubsystem.h/.cpp`

Do not invent file names blindly if the project already has an established pattern; align with existing naming and module organization.

# Implementation plan
1. **Audit the current event flow**
   - Identify:
     - where simulation state changes happen
     - where delegates/events are already emitted
     - where UI is directly updated from business logic
     - where news/feed items are currently created
   - Document the current major event sources and consumers in code comments or a short internal note if helpful.
   - Look for anti-patterns:
     - simulation subsystem storing selected artist/current tab/open modal
     - UI widgets directly mutating subsystem state
     - business subsystems emitting UI strings instead of typed payloads
     - news generation embedded inside unrelated subsystems

2. **Define a typed domain event model**
   - Create a compact, explicit set of event payload structs for major state changes already present in code.
   - At minimum support events that are likely already relevant to M2:
     - week advanced
     - month closed
     - artist signed / contract changed if present
     - recording started/completed if present
     - record released if present
     - chart updated if present
     - finance balance changed if present
     - news generated
   - Use Unreal-friendly types:
     - `USTRUCT(BlueprintType)` only if needed by current consumers
     - otherwise plain `USTRUCT()`
   - Include fields such as:
     - stable entity IDs (`FString`, `FName`, `FGuid`, or existing project ID type)
     - simulation date/week fields
     - optional summary values needed for UI refresh
   - Avoid display-name-only payloads.
   - Avoid embedding widget references, UObject UI references, or transient presentation state.

3. **Add a lightweight central event dispatch path**
   - If the project already has an event subsystem, extend it rather than replacing it.
   - If not, introduce a minimal centralized mechanism that supports:
     - publishing typed domain events
     - subscribing from `UEventSubsystem`
     - subscribing from `UUIManagerSubsystem`
   - Keep it simple and native to Unreal:
     - multicast delegates per event type, or
     - a small event hub subsystem with typed broadcast methods
   - Do not build a reflection-heavy generic bus unless the codebase already uses one.

4. **Refactor simulation/business subsystems to emit domain events**
   - Update existing subsystems so major state changes emit typed events through the central path.
   - Emit events at the point of successful state mutation.
   - Ensure event emission is deterministic and not duplicated by both command layer and subsystem unless there is a clear ownership rule.
   - Prefer one owner per event:
     - command validates
     - owning subsystem mutates
     - owning subsystem or dispatcher emits event consistently
   - If current code emits raw strings or UI notifications directly, replace those with typed domain events and let downstream systems translate them.

5. **Implement `UEventSubsystem` translation to player-facing news**
   - Subscribe `UEventSubsystem` to relevant domain events.
   - Convert domain events into news/feed items using stable references:
     - event/news item ID or dedupe key
     - related artist/record/contract/tour IDs
     - date
     - category/type
     - localized/display text if current architecture supports it
   - Add deduplication by event key so repeated broadcasts do not create duplicate news.
   - Keep news generation logic here rather than in simulation subsystems.
   - If a news model already exists, adapt it instead of replacing it.

6. **Update `UUIManagerSubsystem` to react to events, not own business logic**
   - Subscribe `UUIManagerSubsystem` to relevant domain events and/or news events.
   - Trigger screen/view-model refreshes, notifications, or routing updates based on event payloads.
   - Keep UI manager behavior limited to presentation concerns:
     - invalidate cached projections
     - request refresh of dashboard/roster/charts/news
     - show notifications/toasts if such a system exists
   - Do not move business calculations into UI manager.
   - Do not let UI manager become the source of truth for simulation state.

7. **Separate UI selection/transient state from simulation if leakage is found**
   - If this task reveals simulation subsystems storing selected artist or similar transient UI state, remove or deprecate that coupling where feasible.
   - Move such state to `UUIManagerSubsystem` or an existing selection context object/subsystem.
   - Add compatibility shims only if necessary to avoid broad breakage.

8. **Make payloads serializable-friendly**
   - Ensure event payload structs are plain-data oriented and can reasonably be logged, persisted, or replayed later.
   - Include dates and IDs sufficient for:
     - UI refresh
     - news generation
     - future save replay/debug tracing
   - Avoid raw asset paths or direct UObject references in payloads unless already required and safe.

9. **Add logging and guardrails**
   - Use existing log categories or add a focused one if needed for event flow.
   - Log event publication and dedupe decisions at verbose/debug level where useful.
   - Warn on malformed payloads or unresolved IDs when translating to news/UI.
   - Keep logs lightweight in normal runtime.

10. **Keep changes incremental and compile-safe**
   - Prefer adapting existing code paths over large rewrites.
   - If some systems are not yet implemented, stub only the minimum needed interfaces and leave clear TODOs.
   - Preserve backward compatibility where practical.

# Validation steps
1. **Build the solution**
   - From workspace root:
     - `dotnet build`
   - If tests exist and are relevant:
     - `dotnet test`

2. **Static code review checks**
   - Confirm major simulation/business subsystems no longer emit UI-specific strings or directly manipulate UI state where touched.
   - Confirm event payloads use stable IDs, not display names as primary references.
   - Confirm `UEventSubsystem` is the place where domain events become player-facing news.
   - Confirm `UUIManagerSubsystem` subscribes to events and performs presentation refresh behavior only.

3. **Functional validation in code paths**
   - Verify at least these flows emit typed events end-to-end if the systems exist:
     - time advance -> week/month event -> UI refresh/news as applicable
     - recording/release/chart/finance event -> event subsystem translation -> UI manager reaction
   - Verify no duplicate news items are created for the same event key.
   - Verify event payloads include enough context for consumers:
     - IDs
     - date/week
     - event type/category

4. **Regression checks**
   - Existing UI flows should still function after refactor.
   - Existing command or subsystem operations should still complete without requiring widgets to own business logic.
   - If selection state was moved, verify current artist/detail flows still resolve through UI-owned context.

5. **Code quality checks**
   - No unnecessary generic framework added.
   - New types are named clearly and grouped logically.
   - Comments explain ownership:
     - who emits events
     - who translates to news
     - who updates UI

6. **If feasible, add or update tests**
   - Unit/integration coverage for:
     - event emission on a representative state change
     - news deduplication by event key
     - UI manager subscription behavior or projection invalidation
   - If no test harness exists, at least add deterministic helper methods that are testable later.

# Risks and follow-ups
- **Risk: current codebase may not yet have all named subsystems**
  - Mitigation: implement the event pipeline around the subsystems that actually exist, with extension points for later stories.

- **Risk: event ownership may be ambiguous between command dispatcher and subsystem**
  - Mitigation: choose one consistent owner per event and document it in code comments.

- **Risk: UI currently depends on direct subsystem mutation or polling**
  - Mitigation: preserve compatibility where needed, but route touched flows through event-driven refresh.

- **Risk: duplicate or noisy events**
  - Mitigation: add dedupe keys and avoid double-broadcasting from nested calls.

- **Risk: payloads may still rely on unstable names**
  - Mitigation: use existing stable ID types wherever available; if mixed references remain, normalize touched paths and leave TODOs for remaining migration.

Follow-ups after this task:
- Expand event coverage across all simulation subsystems as they mature.
- Add save/load replay or event trace support if needed for debugging and deterministic validation.
- Introduce read-only view models/projections for dashboard, charts, and news screens if not already present.
- Align future stories like ST-501/ST-504 with this event pipeline so UI refresh remains event-driven.