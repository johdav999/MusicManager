# Goal

Implement backlog task **TASK-7.4.3** for **ST-104 Domain event pipeline cleanup** so that **`UUIManagerSubsystem` subscribes to typed domain events and refreshes/reroutes UI without owning or duplicating business logic**.

The coding agent should:

- preserve the layered architecture:
  - **simulation subsystems own state and emit domain events**
  - **`UEventSubsystem` converts domain events into player-facing news**
  - **`UUIManagerSubsystem` listens to events and updates presentation state/screens**
- ensure event payloads are **typed, stable-ID based, and date-aware**
- avoid putting selected entity state or business rules back into simulation subsystems
- keep the implementation incremental and compatible with the current codebase

Because no explicit task-level acceptance criteria were provided, use the story acceptance criteria for **ST-104** as the definition of done.

# Scope

In scope:

- Identify the current event flow between simulation subsystems, event/news handling, and UI.
- Add or standardize **typed domain event payloads** for major state changes already present or directly needed by current UI flows.
- Ensure payloads include:
  - stable IDs
  - relevant dates/time markers
  - enough context for UI refresh and news generation
- Update **`UEventSubsystem`** to consume domain events and produce player-facing news items without embedding simulation logic.
- Update **`UUIManagerSubsystem`** to:
  - subscribe to domain events
  - trigger screen/view refreshes, notifications, routing, or selection-safe updates
  - not compute business outcomes
- Keep transient UI state in UI layer only.
- Add lightweight logging/observability around event subscription and handling.

Out of scope unless required to compile/fix regressions:

- large-scale redesign of all subsystems
- introducing a fully generic reflection-based event bus
- rewriting unrelated widgets/screens
- save migration work beyond making event payloads serializable-friendly
- adding new gameplay systems not needed for this task

# Files to touch

Touch the minimum set needed after inspecting the repo. Prioritize files that already represent these responsibilities.

Likely targets:

- `UUIManagerSubsystem` implementation/header
- `UEventSubsystem` implementation/header
- shared domain event definitions:
  - existing event structs/delegates headers
  - or create a focused shared header if none exists
- simulation subsystem files that currently emit ad hoc UI-facing callbacks and should emit typed domain events instead, especially:
  - time/simulation director
  - recording/release
  - charts
  - finance
  - contracts/artists
- any existing news feed model/view-model files if they currently consume raw subsystem state instead of event-derived items
- logging category definitions if needed

Possible new files if the project lacks a clean home for this:

- a shared header for typed domain event payloads, e.g.:
  - `Public/Events/MusicDomainEvents.h`
- a small subscription/helper implementation if needed for `UUIManagerSubsystem`
- tests for event emission/translation/subscription if the repo already has a test pattern

Do not invent a parallel architecture if equivalent files already exist.

# Implementation plan

1. **Inspect current architecture in code**
   - Find:
     - `UUIManagerSubsystem`
     - `UEventSubsystem`
     - any existing delegates/event structs
     - current UI refresh triggers
   - Map where business subsystems currently:
     - call UI directly
     - emit stringly-typed events
     - push news/UI state themselves
   - Document in code comments only where useful; do not add broad docs churn.

2. **Define or normalize typed domain events**
   - Create/reuse typed `USTRUCT` payloads for major state changes relevant to current UI/news flows.
   - At minimum, support events already called out by the story/architecture where present in code, such as:
     - week advanced
     - month/year advanced if already exposed
     - artist signed / contract expired
     - recording started / completed
     - record released
     - chart updated
     - finance balance changed
     - news generated or event created
   - Payload rules:
     - use stable IDs, not display names, for entity references
     - include date/week fields where applicable
     - keep payloads serializable-friendly and lightweight
     - avoid embedding widget/view state
   - Prefer explicit structs and delegates over generic maps/JSON/string payloads.

3. **Ensure simulation subsystems emit domain events, not UI instructions**
   - Refactor any direct UI calls from simulation/business subsystems into event emission.
   - Keep ownership boundaries clear:
     - subsystem mutates state
     - subsystem emits typed event
     - listeners decide presentation response
   - Do not move business validation into `UUIManagerSubsystem`.

4. **Update `UEventSubsystem` to translate domain events into player-facing news**
   - Subscribe `UEventSubsystem` to the relevant domain events.
   - Convert domain events into news/feed items with stable references.
   - Add deduplication by event key where practical and consistent with existing architecture.
   - Keep this translation layer presentation-oriented:
     - summarize
     - prioritize
     - store/display references
     - no business recalculation
   - If there is already a news model, adapt it rather than replacing it.

5. **Update `UUIManagerSubsystem` to subscribe and react**
   - Subscribe `UUIManagerSubsystem` to the same domain events it needs for UI refresh/routing.
   - Implement reactions such as:
     - invalidating/rebuilding relevant screen projections
     - refreshing dashboard summaries
     - updating active detail panels if referenced IDs match current selection
     - routing to notifications/tickers/modals if that pattern already exists
   - Keep reactions thin:
     - no mutation of simulation state
     - no recomputation of finance/chart/release outcomes
   - If selection context exists in UI manager, use it only to decide what to refresh, not to drive business rules.

6. **Preserve separation of transient UI state**
   - Verify selected artist/open modal/current tab remain in UI layer only.
   - Remove or stop using any simulation-side “current selected” dependencies encountered during this task if they block the event cleanup.
   - If full removal is too broad, add a compatibility shim and leave a clear TODO.

7. **Add logging and defensive handling**
   - Use or add `LogMusicUI` and any existing event/news log categories.
   - Log:
     - subscription setup
     - key event handling paths
     - ignored events due to missing references or inactive screens
   - Handle missing IDs gracefully in UI/news translation:
     - warn, skip, and avoid crashes
   - Do not silently fabricate business data.

8. **Keep implementation incremental**
   - Prefer adapting current delegates/subsystems over introducing a large new bus abstraction.
   - If multiple event mechanisms exist, consolidate only enough to satisfy this task and reduce coupling.
   - Avoid touching unrelated screens unless required by compile/runtime flow.

9. **Add tests if the repo supports them**
   - If there are existing automated tests:
     - add focused tests for event payload emission and UI/news subscription behavior
   - If no test harness exists, at least structure code so event translation and refresh decisions are easy to test later.

10. **Definition of done**
    - Core simulation subsystems emit typed events for major state changes in current flows.
    - `UEventSubsystem` translates those events into player-facing news.
    - `UUIManagerSubsystem` subscribes and updates screens without owning business logic.
    - Event payloads include stable IDs and dates sufficient for UI refresh and replay-friendly handling.
    - Build passes.

# Validation steps

1. **Static inspection**
   - Confirm no new direct business-logic ownership was added to `UUIManagerSubsystem`.
   - Confirm simulation subsystems are not directly manipulating widgets/screens for covered flows.
   - Confirm event payloads use IDs and dates, not display-name-only references.

2. **Build**
   - Run the most appropriate build command for the workspace after inspecting how the project is actually built.
   - From provided context, try:
     - `dotnet build`
   - If there are test projects and they are relevant:
     - `dotnet test`

3. **Runtime/manual verification**
   - Exercise at least a few representative flows already available in the project, such as:
     - advancing time
     - signing an artist
     - starting/completing recording
     - releasing a record
     - finance balance change
     - chart update
   - Verify for each:
     - subsystem emits event
     - `UEventSubsystem` creates/updates news as appropriate
     - `UUIManagerSubsystem` refreshes affected screens/panels
     - no business outcome is computed in UI manager

4. **Logging verification**
   - Check logs for:
     - successful event subscriptions
     - event handling traces
     - warnings for unresolved IDs or skipped refreshes
   - Ensure no noisy per-frame spam was introduced.

5. **Regression checks**
   - Confirm existing UI flows still work:
     - dashboard refresh
     - artist detail/selection behavior
     - news feed display
     - navigation from news/chart items if already implemented
   - Confirm transient UI state is not serialized or moved into simulation ownership by this change.

# Risks and follow-ups

- **Risk: mixed event systems already exist**
  - The repo may already use delegates, direct calls, and ad hoc notifications in parallel.
  - Follow-up: consolidate further in a later cleanup task once this story is satisfied.

- **Risk: stable ID migration may be incomplete**
  - Some current UI/news code may still rely on names.
  - Follow-up: normalize remaining references under ST-102/ST-104 cleanup if encountered.

- **Risk: `UUIManagerSubsystem` may already contain business logic**
  - If found, only extract the portions directly blocking this task unless a small safe refactor can remove them cleanly.
  - Follow-up: create a dedicated cleanup task for remaining UI/business coupling.

- **Risk: event payload coverage may be broader than current code supports**
  - Implement the typed events needed for current major state changes and architecture compliance, not speculative future systems.
  - Follow-up: extend event catalog as more subsystems are integrated.

- **Risk: Unreal lifetime/subscription issues**
  - Subsystem initialization order and delegate unbinding may cause duplicate subscriptions or stale handlers.
  - Ensure subscriptions are registered/unregistered safely.
  - Follow-up: add lifecycle tests or guards if the codebase has recurring subsystem init issues.

- **Risk: no automated test harness**
  - If tests are not practical in this repo, keep changes modular and well-logged.
  - Follow-up: add subsystem/event pipeline tests in a future engineering enablement task.