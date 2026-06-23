# Goal
Implement backlog task **TASK-7.4.6 — Deduplicate news generation by event key** for story **ST-104 Domain event pipeline cleanup**.

The coding agent should update the domain event/news pipeline so that `UEventSubsystem` (or the equivalent event/news manager in this codebase) does **not generate duplicate player-facing news items** for the same underlying domain event.

This work must align with the architecture and story intent:

- keep simulation/domain events separate from UI state
- convert typed domain events into player-facing news
- ensure event payloads use stable IDs and dates
- deduplicate news generation by a stable **event key**
- preserve deterministic behavior suitable for weekly simulation and save/load

Because no explicit acceptance criteria were provided for this task beyond the story notes, treat the following as the implementation target:

- each domain event that can produce news must resolve to a deterministic, stable deduplication key
- repeated processing of the same event must not create duplicate news entries
- distinct events that happen to share similar text must still be allowed if their keys differ
- deduplication must work during normal runtime and after load/replay/rebuild flows where events may be reprocessed
- UI/news feed behavior should remain event-driven and compatible with existing systems

# Scope
Focus only on the minimum code changes needed to support **news deduplication by event key** in the existing project.

In scope:

- inspect the current event/news pipeline and identify:
  - where domain events are emitted
  - where they are transformed into news items
  - how news items are stored, queued, or displayed
- introduce a stable **event/news deduplication key** concept
- ensure the event subsystem checks for prior generation before adding a news item
- use stable IDs/date data from event payloads where available
- add or update persistence if news/event state is already saved and dedup state must survive save/load
- add tests if the repo has an existing test pattern; otherwise add lightweight validation hooks/logging

Out of scope unless required by existing code structure:

- broad refactors of the entire event bus
- redesigning UI widgets
- changing unrelated simulation logic
- adding a generic reflection-based event framework
- rewriting all event payloads if a compatibility shim can be used

If the current codebase is incomplete or differs from the architecture docs, implement the feature in the **closest existing event/news path** and document assumptions in code comments or logs.

# Files to touch
Start by inspecting these likely areas, then touch only what is necessary:

- `README.md` only if there is a developer-facing note about event/news behavior that must be updated
- event/news subsystem files, likely Unreal C++ classes such as:
  - `UEventSubsystem`
  - news feed manager
  - notification manager
  - domain event bus or event translator
- structs/classes for:
  - domain event payloads
  - news item models
  - save snapshot models for pending events/news
- any subsystem save/load code if dedup state must persist
- any existing tests covering event/news generation

Look for files matching patterns like:

- `*EventSubsystem*`
- `*News*`
- `*Notification*`
- `*GameTimeSubsystem*`
- `*Save*`
- `*Subsystem*`

If the workspace is actually a .NET support/tooling project rather than the UE runtime code, still locate the event/news implementation and apply the same design in the actual source files present.

# Implementation plan
1. **Discover the current implementation**
   - Search the repo for:
     - `UEventSubsystem`
     - `News`
     - `Notification`
     - `ENewsGenerated`
     - `OnWeekAdvanced`
     - domain event structs
     - any queue/list of generated news
   - Identify:
     - the source event type(s)
     - the translation point from domain event to news item
     - the storage model for generated news
     - whether save/load already persists event/news state

2. **Define a deterministic deduplication key**
   - Add a stable key field or computed method for generated news, e.g.:
     - `EventKey`
     - `DedupKey`
     - `SourceEventKey`
   - The key should be deterministic and derived from stable event identity data, not display text.
   - Prefer composition from:
     - event type
     - primary stable entity IDs such as `ArtistId`, `RecordId`, `TourId`, `EventId`
     - simulation date/week where appropriate
     - optional publication/source ID if multiple distinct news items are valid for the same entity
   - Do **not** use localized text/title/body as the dedup key.
   - If payloads already include a stable `EventId`, prefer that as the base key.
   - If not, compute a canonical key string/value from payload fields in one place.

3. **Centralize key generation**
   - Implement a single helper/function responsible for generating the dedup key from a domain event or news candidate.
   - Keep this logic close to the event/news subsystem so future event types follow the same rule.
   - If there are multiple event payload types, add overloads or a translator layer rather than duplicating key logic across callers.

4. **Prevent duplicate news insertion**
   - Before adding a generated news item to the subsystem’s queue/history/feed:
     - check whether the dedup key already exists in:
       - current in-memory news items
       - pending queue if separate
       - any persisted dedup index restored from save
   - If the key already exists:
     - skip insertion
     - log a verbose/debug message indicating duplicate suppression
   - If the key is new:
     - insert the news item
     - register the key in a lookup structure optimized for membership checks, e.g. set/map

5. **Choose the right dedup lifetime**
   - Match behavior to likely gameplay expectations:
     - dedup should prevent the same underlying event from generating the same news item more than once
     - it should survive save/load if old news items are persisted and replay/reprocessing can occur
   - If the subsystem stores historical news:
     - rebuild the dedup set from stored news on initialization/load
   - If only pending/recent news is stored:
     - persist the dedup keys explicitly if needed to avoid duplicates after load
   - Keep the implementation deterministic and simple.

6. **Preserve valid distinct stories**
   - Ensure the dedup key is specific enough that separate events are not collapsed incorrectly.
   - Examples:
     - two different chart updates in different weeks should not dedup unless they are truly the same source event
     - two different critic reviews for the same record from different publications should not dedup if both are intended
     - one release event and one chart event for the same record must remain distinct

7. **Add compatibility handling**
   - If existing news items do not have a stored key:
     - compute it on load/init where possible
     - or add a fallback migration path
   - Avoid breaking existing saves if save models already exist.
   - If save versioning exists, update version/migration only if required by the current persistence design.

8. **Add observability**
   - Add logging in the relevant log category for:
     - generated key
     - duplicate suppression
     - malformed events that cannot produce a stable key
   - Use warnings only for invalid/missing identity data; use verbose/log for normal duplicate suppression.

9. **Add tests or verification hooks**
   - If automated tests exist:
     - add coverage for duplicate suppression and non-duplicate distinct events
   - If not:
     - add a small deterministic validation path or debug helper that can be exercised manually

10. **Keep code style aligned with the project**
   - Follow existing naming, subsystem boundaries, and serialization patterns.
   - Prefer minimal invasive changes over speculative abstractions.

# Validation steps
1. **Build/discovery**
   - Restore/build the solution to confirm the workspace compiles before changes:
     - `dotnet build`
   - If there are tests:
     - `dotnet test`

2. **Static verification**
   - Confirm there is exactly one central place where news dedup is enforced.
   - Confirm dedup keys are based on stable identifiers, not display strings.

3. **Behavior verification**
   - Validate these scenarios in code or tests:
     - processing the exact same domain event twice produces only one news item
     - processing two distinct events with different keys produces two news items
     - processing similar events with same text but different entity IDs still produces separate news items
     - if applicable, loading saved news state and reprocessing the same event does not duplicate the news item

4. **Persistence verification**
   - If news/event state is persisted:
     - save a state after generating a news item
     - reload
     - trigger or replay the same source event
     - verify no duplicate news is added

5. **Logging verification**
   - Confirm duplicate suppression emits a useful debug/verbose log entry.
   - Confirm malformed events without enough identity data either:
     - produce a safe fallback key deterministically, or
     - are rejected with a clear warning

6. **Regression verification**
   - Ensure existing news feed/UI still receives and displays generated news.
   - Ensure no unrelated event types stop generating news accidentally.

# Risks and follow-ups
- **Risk: no clear event/news subsystem exists in the current repo**
  - Mitigation: implement in the nearest existing event-to-notification/news translation path and document the mapping.

- **Risk: event payloads lack stable identity fields**
  - Mitigation: add the smallest possible stable key composition using available IDs/date/type; if necessary, extend payload structs with stable IDs rather than using display names.

- **Risk: dedup is too aggressive**
  - Mitigation: make keys specific to event type and source identity; include week/date/source publication where needed.

- **Risk: dedup does not survive save/load**
  - Mitigation: rebuild the dedup index from persisted news history or persist the key set explicitly.

- **Risk: existing saves/news items lack the new key field**
  - Mitigation: compute keys lazily for legacy entries or add a lightweight migration path.

Follow-ups to note if encountered but not required for this task:
- standardize all domain event payloads to include explicit stable `EventId`/entity IDs/date
- add a formal typed event-to-news translator layer if current logic is scattered
- add automated tests around event replay and save/load determinism
- review ticker/notification dedup separately if it is implemented independently from the news feed