# Goal
Implement backlog task **TASK-7.4.1 — Core simulation subsystems emit typed events for major state changes** for story **ST-104 Domain event pipeline cleanup**.

The coding agent should introduce or clean up a **typed domain event pipeline** so that:
- core simulation/business subsystems emit **typed, serializable event payloads** for major state changes,
- event payloads use **stable IDs and dates** rather than UI-specific state,
- `UEventSubsystem` can consume those domain events and translate them into player-facing news,
- `UUIManagerSubsystem` can subscribe to those events to refresh screens without owning business logic.

This task should align with the architecture principle:
- **Subsystems own business state**
- **Commands mutate state**
- **Events notify UI**

# Scope
In scope:
- Add or refine a **shared typed event model** for major simulation state changes.
- Ensure relevant core subsystems emit typed events for major state changes already occurring in code.
- Ensure event payloads include:
  - stable entity IDs,
  - simulation date/week context where appropriate,
  - enough information for UI refresh and future save replay.
- Wire event publication so `UEventSubsystem` and `UUIManagerSubsystem` can subscribe without direct business-state ownership.
- Keep implementation lightweight and idiomatic for the current codebase.

Out of scope unless required to complete compilation:
- Full news generation feature depth.
- Full UI redesign.
- New gameplay systems not needed for event emission.
- Save replay persistence of events beyond making payloads serializable/practical.
- Generic reflection-heavy event bus frameworks.

If the codebase already has partial event/delegate infrastructure, prefer **extending and standardizing** it rather than replacing everything.

# Files to touch
Inspect the repo first and then update the smallest coherent set of files. Likely targets include:

- Core subsystem headers/cpps for simulation/business state changes, especially:
  - `UGameTimeSubsystem`
  - `UArtistManagerSubsystem`
  - `UProductionSubsystem` / `URecordManagerSubsystem`
  - `UChartManagerSubsystem`
  - `UFinanceManagerSubsystem`
  - `UTourManagerSubsystem`
  - `UEventSubsystem`
  - `UUIManagerSubsystem`
- Shared types area for new event payload structs, likely something like:
  - `Source/.../Public/Events/...`
  - `Source/.../Public/Types/...`
  - `Source/.../Public/Simulation/...`
- Any existing command dispatcher or orchestration layer that currently emits ad hoc UI notifications.
- Any existing news/event translation code that should consume typed domain events.
- Minimal tests if a test project exists.
- Build files only if needed for new source registration.

Do **not** touch generated, intermediate, or unrelated files.

# Implementation plan
1. **Survey existing event flow**
   - Find current uses of:
     - Unreal delegates,
     - ad hoc UI refresh calls,
     - stringly-typed event names,
     - direct `UUIManagerSubsystem` calls from simulation subsystems,
     - direct news creation from business logic.
   - Identify the current major state changes already represented in code, especially around:
     - week/month/year advancement,
     - artist contract/signing changes,
     - recording started/completed,
     - release scheduled/released,
     - chart updated,
     - finance balance changed,
     - tour/show completion,
     - news generated.

2. **Define a shared typed domain event set**
   - Create a small, focused set of `USTRUCT(BlueprintType)` payloads for major state changes.
   - Prefer payloads like:
     - `FGameWeekAdvancedEvent`
     - `FGameMonthClosedEvent`
     - `FArtistSignedEvent`
     - `FArtistContractExpiredEvent`
     - `FRecordingStartedEvent`
     - `FRecordingCompletedEvent`
     - `FRecordReleasedEvent`
     - `FChartUpdatedEvent`
     - `FShowCompletedEvent`
     - `FFinanceBalanceChangedEvent`
   - Each payload should be plain data only, with fields such as:
     - `ArtistId`
     - `RecordId`
     - `ContractId`
     - `TourId`
     - `EventId` if applicable
     - simulation date/week fields
     - summary values needed for UI/news refresh
   - Avoid embedding widget references, subsystem pointers, or transient UI state.

3. **Standardize event transport**
   - Use existing Unreal-native multicast delegates if present; otherwise add a lightweight event hub pattern appropriate to the codebase.
   - Prefer a centralized place for typed event dispatch/subscription if one already exists.
   - If no central event bus exists, introduce a minimal subsystem-level event publisher pattern that is easy for `UEventSubsystem` and `UUIManagerSubsystem` to subscribe to.
   - Keep it explicit and typed; do not introduce string event routing.

4. **Emit typed events from core simulation subsystems**
   - Update major subsystem mutation points so they emit typed events after successful state changes.
   - Focus on major state changes, not every internal calculation.
   - Ensure emission happens from business logic boundaries, e.g.:
     - after advancing week/month/year,
     - after contract creation/expiration,
     - after recording starts/completes,
     - after release state changes to released,
     - after chart results finalize,
     - after finance balance materially changes,
     - after show resolution completes.
   - Preserve determinism and avoid hidden side effects.

5. **Decouple UI refresh from business logic**
   - Remove or reduce direct UI mutation calls from simulation subsystems where found.
   - Update `UUIManagerSubsystem` to subscribe to typed events and trigger appropriate refresh/update methods.
   - UI should react to event payloads and query read-only projections as needed, not own business rules.

6. **Connect `UEventSubsystem` to domain events**
   - Update `UEventSubsystem` so it subscribes to the typed domain events.
   - Convert incoming domain events into player-facing news/notification items.
   - If deduplication exists or is easy to add, key it by stable identifiers plus event type/date.
   - Keep translation logic in `UEventSubsystem`, not in simulation subsystems.

7. **Make payloads replay/save friendly**
   - Ensure payload structs are serializable/practical Unreal data structs.
   - Use stable IDs and date values rather than raw object references.
   - If there is an existing date type, use it consistently.
   - Avoid storing non-deterministic or presentation-only data in payloads.

8. **Add logging and guardrails**
   - Add or use existing log categories to trace event emission and subscription flow in development.
   - Log warnings for malformed or incomplete event payloads only where necessary.
   - Do not spam logs for routine high-frequency operations beyond major state changes.

9. **Keep compatibility**
   - If existing code depends on older delegates/callbacks, preserve compatibility where practical by bridging old signals to the new typed events during transition.
   - Prefer incremental refactor over broad rewrites.

10. **Document with code comments where needed**
   - Add concise comments on:
     - what counts as a domain event,
     - why payloads use IDs instead of object refs,
     - which subsystem owns translation to news/UI.

# Validation steps
Run through these steps after implementation:

1. **Build/compile**
   - Run the most appropriate build command for the workspace:
     - `dotnet build`
   - If there are tests:
     - `dotnet test`

2. **Static code validation**
   - Confirm all new event payload structs compile cleanly.
   - Confirm delegate signatures and includes are correct.
   - Confirm no circular include issues were introduced.

3. **Behavior validation**
   - Verify major state changes emit typed events by tracing logs or breakpoints:
     - advancing time emits week/month/year events,
     - signing/contract lifecycle emits artist/contract events,
     - recording start/completion emits recording events,
     - release emits release event,
     - chart resolution emits chart update event,
     - finance updates emit balance change event,
     - tour/show resolution emits show/tour events if applicable.
   - Verify `UEventSubsystem` receives those events and can create/queue player-facing news items.
   - Verify `UUIManagerSubsystem` receives those events and triggers refresh behavior without direct business mutation.

4. **Architecture validation**
   - Confirm simulation subsystems no longer depend on selected artist/open modal/transient UI state.
   - Confirm event payloads contain stable IDs and date context.
   - Confirm no stringly-typed event routing was added.

5. **Regression check**
   - Ensure existing flows still work where applicable.
   - Ensure no duplicate event emission for a single state transition unless explicitly intended.
   - Ensure event emission order remains deterministic within a simulation tick.

# Risks and follow-ups
- **Repo mismatch risk:** Workspace hints `.NET`, but architecture is Unreal C++. First inspect the actual source layout before coding. If Unreal C++ sources are absent or partial, adapt the implementation to the nearest existing subsystem/event architecture and note the gap clearly.
- **Partial infrastructure risk:** There may already be multiple overlapping delegate/event patterns. Consolidate carefully to avoid breaking existing listeners.
- **Over-emission risk:** Emitting too many low-level events can create noisy UI/news behavior. Limit this task to major state changes.
- **Duplicate news risk:** If `UEventSubsystem` already generates news from direct subsystem calls, bridging to typed events may temporarily duplicate stories. Add dedupe or remove old direct paths.
- **Ordering risk:** Event ordering matters for deterministic simulation and UI summaries. Keep emission after state commit and in stable phase order.
- **Serialization risk:** Payloads should remain lightweight and ID-based; avoid UObject references that complicate save/replay.

Follow-ups after this task:
- Add automated tests for event emission and subscription behavior if a test harness exists.
- Expand event-to-news deduplication keys in `UEventSubsystem`.
- Standardize command results and domain events together if ST-103 is only partially implemented.
- Consider a dedicated shared event hub/subsystem if current subscriptions remain fragmented after cleanup.