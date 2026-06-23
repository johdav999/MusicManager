# Goal
Implement backlog task **TASK-7.4.2** for story **ST-104 — Domain event pipeline cleanup** by adding or completing **`UEventSubsystem`** so it **converts typed domain events into player-facing news items**.

The coding agent should produce a clean Unreal Engine 5 C++ implementation that fits the existing subsystem-oriented architecture:

- simulation/domain systems emit **typed events**
- `UEventSubsystem` listens to those events
- `UEventSubsystem` translates them into **news feed items / notifications-ready records**
- news generation is **deduplicated**, **stable-ID based**, and **serializable where practical**
- UI can consume generated news without embedding business logic

Because no explicit acceptance criteria were provided for the task itself, use the story-level architecture and acceptance criteria as the source of truth.

# Scope
In scope:

- Add or extend **`UEventSubsystem`** as a `UGameInstanceSubsystem`
- Define a **player-facing news item model** with stable references
- Add a **domain-event-to-news translation pipeline**
- Support at least the major event types most likely already present or needed by ST-104, such as:
  - artist signed
  - contract expired
  - recording completed
  - record released
  - chart updated / milestone
  - critic review published
  - tour started / show completed / standout show
  - finance balance changed only if it represents notable news, not every balance mutation
- Implement **deduplication by event key**
- Preserve enough metadata for:
  - UI rendering
  - navigation to related artist/record/tour
  - save/load replay or persistence later
- Expose read/query APIs for:
  - latest news
  - unread/high-priority items if applicable
  - lookup by related entity ID
- Add logging for event ingestion and deduplication behavior
- Add tests if the workspace already has a test pattern; otherwise add lightweight validation hooks and keep code testable

Out of scope unless trivially necessary:

- full widget implementation
- major refactors of unrelated subsystems
- a generic reflection-based event bus
- deep notification UX polish
- save migration work beyond what is minimally needed for serializable structs
- immersive scene logic

If the codebase already has partial event/news infrastructure, **integrate with it rather than replacing it wholesale**.

# Files to touch
Inspect the repo first and then update the most relevant files. Expected targets include:

- `Source/.../EventSubsystem.h`
- `Source/.../EventSubsystem.cpp`
- `Source/.../...Types.h` or equivalent shared types file for:
  - domain event payload structs
  - news item structs
  - enums for news category/priority
- `Source/.../UIManagerSubsystem.*` only if needed to wire event/news consumption
- `Source/.../GameTimeSubsystem.*` or other subsystem files only if needed to subscribe to existing delegates/events
- `Source/.../SaveGame` or snapshot structs only if there is already an event/news snapshot path and this task naturally plugs into it
- any existing module build file if new headers/classes require registration
- automated test files if a test project/pattern exists

Before editing, identify the actual Unreal module path under `Source/` and use existing naming/style conventions.

# Implementation plan
1. **Survey the current codebase**
   - Find the actual Unreal C++ module under `Source/`.
   - Locate any existing:
     - `UEventSubsystem`
     - domain event structs/delegates
     - news feed models
     - UI manager subscriptions
     - save snapshot structs for pending events/news
   - Determine whether ST-104 is partially implemented already.
   - Prefer extending existing patterns over introducing parallel systems.

2. **Define or normalize the news data model**
   Create a serializable `USTRUCT` for player-facing news items, using stable IDs and dates. Include fields along these lines:

   - `NewsId` or generated stable-ish key
   - `EventKey` for deduplication
   - `Date` / game date / week marker
   - `Headline`
   - `Body` or summary text
   - `Category` (industry, artist, release, charts, critics, tour, finance, system)
   - `Priority` (low/normal/high/critical)
   - related stable IDs:
     - `ArtistId`
     - `RecordId`
     - `TourId`
     - `EventId` if applicable
   - optional navigation target/type
   - `bUnread` if the project already supports read state
   - optional tags or flags for ticker eligibility / summary eligibility

   Keep it lightweight and serializable.

3. **Define translation inputs**
   If typed domain event payloads already exist, use them directly.
   If they do not, add minimal typed payload structs for the major simulation events required by ST-104. Payloads should include:
   - stable IDs
   - event date
   - enough context to build a headline without querying too much mutable UI state

   Do **not** make widgets or UI state part of the event payload.

4. **Implement `UEventSubsystem` responsibilities**
   Add/complete:
   - subsystem initialization and teardown
   - subscription to relevant domain event delegates
   - internal queue/list of generated news items
   - deduplication tracking by `EventKey`
   - query methods for UI/read models

   Suggested public API shape:
   - `GetRecentNewsItems(...)`
   - `GetAllNewsItems()`
   - `GetNewsForArtist(FName/FString ArtistId)`
   - `ClearUnread()` or mark-read helpers only if consistent with current codebase
   - `HandleXxxEvent(const FXxxEvent& EventPayload)` internal handlers

5. **Implement event-to-news translation**
   For each supported domain event type:
   - build a deterministic `EventKey`
   - decide whether it should generate player-facing news
   - map it to:
     - category
     - priority
     - headline/body
     - related IDs
   - append to the news list only if not already seen

   Example dedupe key patterns:
   - `ArtistSigned:{ArtistId}:{ContractId}`
   - `RecordReleased:{RecordId}:{ReleaseDate}`
   - `ChartPeak:{RecordId}:{ChartId}:{WeekStart}:{Rank}`
   - `CriticReview:{RecordId}:{PublicationId}`

   Keep key generation deterministic and stable-ID based.

6. **Prioritization and filtering**
   Add simple prioritization rules so the subsystem can distinguish:
   - major player-facing stories
   - routine informational items

   At minimum:
   - high priority: chart #1, award/win-like milestone if present, major scandal, contract expiration for player artist, release launch
   - normal priority: artist signed, recording completed, critic review, tour started
   - low priority: routine chart update unless milestone-worthy

   Avoid flooding the feed with low-value repetitive items.

7. **Deduplication behavior**
   Implement dedupe in `UEventSubsystem` itself, per architecture guidance.
   Requirements:
   - duplicate domain events with the same semantic key should not create duplicate news items
   - log when duplicates are suppressed
   - preserve deterministic ordering of accepted items

   If there is already an event queue snapshot concept, ensure dedupe state is compatible with future persistence.

8. **UI-facing integration**
   If the project already has a UI manager or dashboard/news widget integration point:
   - expose read-only accessors or a delegate like `OnNewsGenerated`
   - keep UI thin
   - do not move business logic into widgets

   If no UI integration exists yet, at least expose:
   - a multicast delegate/event from `UEventSubsystem` when a news item is generated
   - query methods for polling

9. **Logging and observability**
   Add or use a dedicated log category, ideally aligned with architecture guidance.
   Log:
   - event received
   - news generated
   - duplicate suppressed
   - malformed/insufficient payload skipped

   Keep logs useful for balancing/debugging, not noisy in shipping.

10. **Persistence compatibility**
   If there is already a save snapshot for pending events/news:
   - make the news item struct serializable and compatible
   - ensure only stable IDs and plain data are stored
   - do not store raw asset paths or widget references

   If save integration is not yet present, structure the code so it can be serialized later without redesign.

11. **Code quality constraints**
   - Follow existing Unreal style/macros
   - Prefer `USTRUCT(BlueprintType)` only if the UI layer needs Blueprint access
   - Keep subsystem APIs typed, not stringly-typed
   - Avoid introducing hidden dependencies on selected artist or UI state
   - Keep implementation deterministic and side-effect-light

12. **Document assumptions in code comments**
   Where event sources are missing or partial, add concise comments/TODOs indicating:
   - which subsystem should emit the event
   - what payload fields are required
   - what future milestone can extend the translation rules

# Validation steps
1. **Build/discovery**
   - Inspect the solution/module layout.
   - Run the most appropriate build command available in the workspace:
     - `dotnet build`
   - If there is a UE-specific build path already documented in `README.md`, use that as well.

2. **Static verification**
   Confirm:
   - `UEventSubsystem` compiles
   - all new structs/enums are included in the correct module headers
   - no circular include issues were introduced
   - subsystem initialization/shutdown safely binds/unbinds delegates

3. **Functional verification**
   Add or run lightweight tests/manual harnesses to verify:
   - a supported domain event produces exactly one news item
   - replaying the same event with the same dedupe key does not create duplicates
   - different events for the same artist/record create distinct items when appropriate
   - generated news items contain stable related IDs and date fields
   - ordering is deterministic

4. **Integration verification**
   If UI manager/news feed hooks exist, verify:
   - generated news can be queried by UI
   - UI-facing delegate/event fires when a news item is created
   - no business logic moved into UI classes

5. **Logging verification**
   Check logs for:
   - event ingestion messages
   - duplicate suppression messages
   - warnings for malformed payloads

6. **Regression verification**
   Ensure:
   - no subsystem now depends on transient UI state
   - no direct widget mutation was introduced
   - event payloads remain serializable/plain-data where practical

7. **Final output**
   In your implementation summary, include:
   - files changed
   - event types supported
   - dedupe key strategy
   - any assumptions or follow-up gaps discovered in the repo

# Risks and follow-ups
- **Risk: existing event infrastructure may be fragmented or absent.**
  - Mitigation: implement `UEventSubsystem` with minimal adapters and avoid overbuilding a generic bus.

- **Risk: domain event payloads may not yet include enough context.**
  - Mitigation: add only the missing stable-ID/date fields required for news generation; avoid UI-derived context.

- **Risk: duplicate spam from weekly chart or finance events.**
  - Mitigation: only generate news for milestone-worthy chart/finance events and use deterministic dedupe keys.

- **Risk: save/load integration may not yet exist for news items.**
  - Mitigation: keep structs serializable and persistence-ready, but do not block the task on full save implementation.

- **Risk: workspace stack hint says `.net` while the architecture is Unreal C++.**
  - Mitigation: treat `.sln`/`dotnet build` as workspace discovery/build helpers, but implement in the actual UE C++ module.

Follow-up recommendations after this task:
- wire `UUIManagerSubsystem` to consume `UEventSubsystem` via read-only projections/delegates
- add save snapshot support for pending/generated news if not already present
- add milestone-aware chart/news rules to reduce feed noise during fast-forward
- add automated tests around event replay/deduplication once the project’s test harness is identified