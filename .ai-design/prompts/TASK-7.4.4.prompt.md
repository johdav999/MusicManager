# Goal
Implement backlog task **TASK-7.4.4 — Event payloads include stable IDs and dates sufficient for UI refresh and save replay** for story **ST-104 Domain event pipeline cleanup**.

The coding agent should update the domain event pipeline so that major simulation/domain events use **typed, serializable payloads** containing:
- stable entity IDs, never display names as identifiers
- event IDs/keys where appropriate
- event dates / effective simulation dates
- enough reference data for:
  - UI refresh/navigation
  - event/news generation
  - persistence/replay of pending or recent events after save/load

This work must preserve the architecture principle:
- **subsystems own business state**
- **widgets are thin**
- **commands mutate state**
- **events notify UI**
- **save/load serializes subsystem snapshots**

# Scope
In scope:
- Audit current domain event structs/classes/delegates used by simulation-facing subsystems and UI/news integration.
- Standardize event payloads for major state changes to include stable IDs and dates.
- Ensure payloads are serializable with Unreal-friendly `USTRUCT` patterns where practical.
- Add/adjust a common event envelope or base metadata pattern if needed.
- Update event emission sites to populate the new fields.
- Update `UEventSubsystem` and `UUIManagerSubsystem` consumers to use IDs/dates instead of names or transient widget state.
- Ensure pending/replayable event data can be saved/restored if the current architecture already persists event/news queues.
- Add validation/logging for malformed or incomplete event payloads.

Out of scope unless required to complete this task safely:
- Large redesign of the entire command bus.
- Full news deduplication redesign beyond what is needed to support stable event keys.
- Broad save schema overhaul unrelated to event payload persistence.
- UI visual redesign.
- New gameplay features.

Implementation should align with ST-104 acceptance intent even though no explicit task-level acceptance criteria were provided:
- core simulation subsystems emit typed events for major state changes
- `UEventSubsystem` converts domain events into player-facing news
- `UUIManagerSubsystem` subscribes without owning business logic
- event payloads include stable IDs and dates sufficient for UI refresh and save replay

# Files to touch
Start by locating the real equivalents in the repo. Touch only the minimum necessary set.

Likely targets:
- Event/domain model headers and cpp files:
  - files defining domain event structs/enums/delegates
  - any shared types for IDs and game dates
- Subsystems that emit major events:
  - `UGameTimeSubsystem`
  - artist/contract subsystem
  - production/record subsystem
  - chart subsystem
  - finance subsystem
  - tour subsystem
  - event/news subsystem
- Event bus / dispatcher / subscription glue
- `UEventSubsystem`
- `UUIManagerSubsystem`
- Save/load subsystem and save snapshot structs if pending/replayable events are persisted
- Tests covering event payload shape, serialization, and replay behavior

Because the workspace hint may not reflect the actual UE5 C++ layout, first discover files by searching for:
- `UEventSubsystem`
- `UUIManagerSubsystem`
- `UGameTimeSubsystem`
- `DECLARE_MULTICAST_DELEGATE`
- `DECLARE_DYNAMIC_MULTICAST_DELEGATE`
- `USTRUCT()`
- `Event`
- `News`
- `SaveGame`
- `OnWeekAdvanced`
- `RecordReleased`
- `ChartUpdated`
- `ArtistSigned`

If there is an existing event payload type, prefer extending it over introducing parallel patterns.

# Implementation plan
1. **Inspect the current event pipeline**
   - Identify:
     - where domain events are defined
     - which subsystems emit them
     - how `UEventSubsystem` consumes them
     - how `UUIManagerSubsystem` refreshes screens from them
     - whether event/news queues are persisted in save data
   - Produce a quick mapping of current event types and payload deficiencies:
     - missing stable IDs
     - missing event date/effective date
     - use of display names as keys
     - payloads that are not serializable
     - payloads insufficient for replay after load

2. **Define a stable event payload pattern**
   - Introduce or extend a shared metadata struct, Unreal-serializable if possible, for example:
     - `EventId`
     - `EventType`
     - `OccurredOn` / `EffectiveDate`
     - `PrimaryEntityId`
     - optional related IDs
     - optional dedupe key / replay key
   - Keep it lightweight and compatible with existing typed event structs.
   - Prefer `FString`, `FName`, or project-standard stable ID types already used by ST-102 work.
   - Use the project’s game date type if one exists, e.g. `FGameDate`.

3. **Update major typed event structs**
   - For each major event used by UI/news/save replay, ensure payload includes:
     - stable IDs for referenced entities
     - event/effective date
     - enough context to resolve display data from subsystems later
   - Typical events to cover if present:
     - week/month/year advanced
     - artist signed / contract expired
     - recording started / completed
     - record released
     - chart updated
     - critic review published
     - finance balance changed
     - tour started / show completed
     - news generated
   - Remove reliance on display names in payloads except as optional convenience fields; IDs remain authoritative.

4. **Add serialization-friendly support**
   - Ensure event structs intended for persistence/replay are `USTRUCT(BlueprintType)` or project-equivalent serializable structs.
   - Avoid raw UObject pointers in persisted/replayable payloads.
   - If there is a pending event queue or news queue, store event payload snapshots or a replayable projection using IDs and dates only.

5. **Update emitters**
   - Modify subsystem emission sites so every emitted event populates:
     - stable IDs
     - event date/effective date
     - dedupe/replay key if needed
   - Use the authoritative simulation date from `UGameTimeSubsystem` or equivalent, not wall-clock time.
   - Ensure deterministic event IDs/keys where replay/deduplication matters.

6. **Update consumers**
   - In `UEventSubsystem`:
     - consume the new payload fields
     - generate news items keyed by stable event/entity references
     - dedupe by stable event key rather than display text
   - In `UUIManagerSubsystem`:
     - refresh/navigate using stable IDs
     - resolve display names/details from owning subsystems on demand
     - avoid storing business state in UI event handlers

7. **Handle save/load replay**
   - If the game persists pending events/news:
     - update save snapshot structs/schema to include the replayable event payload or equivalent stable references
     - on load, reconstruct event/news state before UI rebuild
   - If only news items are persisted:
     - ensure each news item contains stable references and dates sufficient for navigation and refresh
   - Do not serialize transient widget state.

8. **Add validation and logging**
   - Add warnings/errors for:
     - missing primary IDs
     - invalid/empty event dates
     - unresolved referenced entities during replay
     - duplicate event IDs/keys where uniqueness is expected
   - Use existing or new log categories consistent with architecture, likely `LogMusicUI`, `LogMusicSave`, and event-related categories.

9. **Add or update tests**
   - Add focused tests for:
     - event payloads include IDs and dates
     - event serialization/deserialization round-trip
     - replay/load preserves enough data for UI/news resolution
     - dedupe keys remain stable across save/load
   - If automated UE tests are absent, add the smallest practical coverage in the project’s existing test style.

10. **Keep compatibility where needed**
   - If existing consumers still expect old fields, add temporary compatibility shims rather than breaking many call sites.
   - Mark old name-based access as deprecated in comments if appropriate.

# Validation steps
1. **Code search validation**
   - Confirm major domain event structs now include stable IDs and date fields.
   - Confirm no critical UI/news flow still depends on display names as authoritative identifiers.

2. **Build**
   - Run the appropriate project build command for the repo.
   - If only workspace commands are available, try:
     - `dotnet build`
   - If this is actually a UE C++ project with generated project files, use the repo’s documented build path if present in `README.md`.

3. **Automated tests**
   - Run existing tests:
     - `dotnet test`
   - Plus any newly added tests for event payload serialization/replay.

4. **Manual verification in code paths**
   - Verify at least these flows in code or tests:
     - record released event contains `RecordId`, `ArtistId`, `ReleaseDate`
     - chart update event contains chart identifier and week/date
     - artist/contract event contains `ArtistId`, `ContractId`, effective date
     - finance event contains reference ID and date where applicable
   - Verify `UEventSubsystem` can generate news from IDs and dates without needing display names in payloads.
   - Verify `UUIManagerSubsystem` can route refresh/navigation from stable references.

5. **Save/load replay verification**
   - Save with pending/recent events or news items.
   - Load and confirm:
     - event/news references still resolve
     - UI can navigate to associated artist/record/chart items
     - no broken references caused by name-based lookup
     - next-week simulation behavior is unchanged by the event payload refactor

6. **Logging validation**
   - Confirm malformed payloads produce clear warnings/errors.
   - Confirm unresolved IDs during replay/load are surfaced through logs, not silently ignored.

# Risks and follow-ups
- **Risk: event definitions are fragmented**
  - The repo may have multiple ad hoc event patterns. Prefer a minimal unifying metadata layer rather than a full rewrite.

- **Risk: save schema impact**
  - If pending events/news are persisted, schema changes may require migration hooks. Keep changes additive where possible.

- **Risk: UI currently relies on display strings**
  - Some widgets may still use names as lookup keys. Replace only authoritative lookup paths now; leave optional display text intact.

- **Risk: no shared game date type**
  - If no `FGameDate` exists, use the project’s current simulation date representation consistently and avoid introducing incompatible date models.

- **Risk: delegate signature churn**
  - Changing delegate payloads may affect many subscribers. Use compatibility wrappers if needed to keep the refactor contained.

Follow-ups to note in comments or task output if discovered:
- remaining event types still lacking stable replay metadata
- save migration work needed for old event/news snapshots
- opportunities to introduce a formal event envelope/base struct across all domain events
- any UI screens still coupled to transient state or direct subsystem mutation