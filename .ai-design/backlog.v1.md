# Epics

## EP-1 Core simulation foundation
Goal: Establish a deterministic, weekly-driven management simulation with stable IDs and clear subsystem orchestration.

## EP-2 Artist, contract, and song workflow
Goal: Implement the playable roster loop for discovering, signing, selecting, and preparing artists and songs for production.

## EP-3 Recording, release, market, and charts
Goal: Deliver the core commercial gameplay loop from recording through release, exposure, sales, and chart outcomes.

## EP-4 Finance, save/load, and data integrity
Goal: Make campaigns financially playable and resumable with full snapshot persistence and validation.

## EP-5 UI orchestration and management screens
Goal: Provide thin, event-driven UMG screens routed through the UI manager for core management actions and feedback.

## EP-6 Tours, critics, and immersive presentation hooks
Goal: Add mid-depth progression systems and connect simulation outputs to future/initial performance presentation flows.

# User stories

## EP-1 Core simulation foundation

### ST-101 — Weekly simulation director
**User story:** As a player, I want the game to advance in predictable weekly steps so that releases, charts, and events feel timely and consistent.

**Acceptance criteria:**
- `UGameTimeSubsystem` supports weekly advancement and emits `OnWeekAdvanced`, `OnMonthClosed`, and `OnYearAdvanced`.
- Simulation phases execute in a fixed order each week.
- Fast-forward can process multiple weeks without rebuilding UI every step.
- Existing monthly-only logic is migrated or wrapped so current systems still function during transition.

**Notes:**
- Keep deterministic ordering across subsystems.
- Preserve a monthly summary layer for finance/news UX.
- Avoid per-frame simulation logic.

### ST-102 — Stable entity ID standardization
**User story:** As a developer, I want all gameplay entities to use stable IDs so that saves, references, and cross-subsystem lookups are reliable.

**Acceptance criteria:**
- Artists, songs, records, contracts, tours, labels, and events use stable IDs instead of display names as keys.
- Cross-subsystem APIs accept IDs consistently.
- Existing mixed name/ID references are migrated with compatibility handling where needed.
- Validation logs warnings/errors for unresolved or duplicate IDs.

**Notes:**
- Display names remain presentation-only.
- Prefer `FGuid`/stable string-like IDs in serialized state.
- Include migration support for old save data if present.

### ST-103 — Command dispatcher for gameplay actions
**User story:** As a developer, I want UI actions to go through typed commands so that validation and state mutation are centralized.

**Acceptance criteria:**
- A command dispatcher subsystem/application layer is added for core actions: sign artist, start recording, schedule release, allocate marketing, plan tour, advance time.
- Commands return structured success/failure results with user-facing messages.
- Widgets no longer directly mutate simulation subsystem state for covered actions.
- Command execution emits domain events consumed by UI/news systems.

**Notes:**
- Keep implementation lightweight; no need for a generic reflection-based bus.
- Use typed structs for command payloads.
- Validation should be “server-style” even in single-player.

### ST-104 — Domain event pipeline cleanup
**User story:** As a developer, I want domain events separated from UI state so that simulation remains decoupled from presentation.

**Acceptance criteria:**
- Core simulation subsystems emit typed events for major state changes.
- `UEventSubsystem` converts domain events into player-facing news items.
- `UUIManagerSubsystem` subscribes to events and updates screens without owning business logic.
- Event payloads include stable IDs and dates sufficient for UI refresh and save replay.

**Notes:**
- Do not store selected artist/open modal in simulation subsystems.
- Deduplicate news generation by event key.
- Keep event payloads serializable where practical.

---

## EP-2 Artist, contract, and song workflow

### ST-201 — Artist state model expansion
**User story:** As a player, I want artists to have meaningful career and personality states so that management decisions have consequences.

**Acceptance criteria:**
- Artist state includes attributes, personality traits, momentum, reputation, fatigue, burnout risk, and scandal heat.
- Weekly/monthly updates adjust artist condition based on activity and neglect.
- Action availability derives from artist state and active commitments.
- Artist detail projections expose this data to UI in a read-only form.

**Notes:**
- Keep formulas data-driven where possible.
- Start with scalar values; deeper relationship simulation can come later.
- Ensure unsigned and signed artists share a compatible base model.

### ST-202 — Contract signing and lifecycle
**User story:** As a player, I want to negotiate and sign artists with contract terms so that roster growth has financial and strategic tradeoffs.

**Acceptance criteria:**
- Signing uses a typed command with advance, royalty rate, term length/commitment inputs.
- Contract validation checks artist availability and player affordability.
- Active and expired contracts are tracked separately with lifecycle updates on time advancement.
- Contract expiration emits events and updates artist availability/actions.

**Notes:**
- Initial scope can omit renegotiation and option clauses UI depth.
- Finance posting for advances must occur at signing.
- Prevent duplicate active contracts for one artist.

### ST-203 — Song catalog split between static definitions and campaign usage
**User story:** As a developer, I want static song metadata separated from mutable campaign usage so that content remains data-driven and saves stay clean.

**Acceptance criteria:**
- Static song definitions load from DataTable/DataAsset content.
- Mutable song usage state tracks lock status, ownership/use linkage, and record assignment.
- Eligible song queries support artist, genre affinity, and current lock state.
- Save/load serializes usage state without duplicating static content.

**Notes:**
- Preserve existing preview playback integration.
- Song definition IDs must resolve safely from cooked content only.
- Missing static references should fail validation on load.

### ST-204 — Selection context moved out of simulation
**User story:** As a developer, I want selected artist and similar UI state moved out of simulation subsystems so that saves and business logic are cleaner.

**Acceptance criteria:**
- Selected entity state is owned by `UUIManagerSubsystem` or a dedicated selection subsystem.
- `UArtistManagerSubsystem` no longer requires selected artist state for core business rules.
- Existing UI flows continue to work via selection context APIs.
- Save-game excludes transient selection state by default.

**Notes:**
- Keep compatibility shims during refactor to reduce breakage.
- Avoid hidden dependencies on “current artist”.
- Inspector and command panels should read from selection context.

---

## EP-3 Recording, release, market, and charts

### ST-301 — Recording workflow refactor
**User story:** As a player, I want to start and complete recordings with clear constraints so that production planning feels strategic.

**Acceptance criteria:**
- Recording start uses a typed command and validates artist eligibility, song availability, and funds.
- Recording state tracks planned duration, start/completion dates, studio tier, and production modifiers.
- Song locks are applied on recording start and released/linked appropriately on completion/cancel.
- Completion emits an event that unlocks release planning UI.

**Notes:**
- Support single/EP/album target types.
- Cancellation behavior must not orphan locked songs.
- Finance can post reservation/upfront cost at start.

### ST-302 — Release planning and lifecycle states
**User story:** As a player, I want to schedule releases by format and region so that timing and market reach matter.

**Acceptance criteria:**
- Records support lifecycle states: planned, recording, mastered, scheduled, released, catalog.
- Release scheduling captures date, formats, target regions, and marketing budget.
- Era rules constrain available formats/channels.
- Released records become eligible for market, chart, critic, and finance processing.

**Notes:**
- Start with player label releases only.
- Use data-driven format availability by era.
- Prevent scheduling before recording completion.

### ST-303 — Market exposure and era/channel rules
**User story:** As a player, I want marketing and region differences to affect outcomes so that release strategy changes by decade.

**Acceptance criteria:**
- Market simulation applies region, audience segment, genre, and era/channel modifiers to exposure.
- Marketing allocation influences exposure generation by available channels for the current era.
- Region map/data queries expose market snapshots for UI.
- Exposure outputs feed sales/streams and chart calculations.

**Notes:**
- Keep formulas inspectable in debug logs.
- Initial implementation can use a limited region set.
- Region map widget should bind to actual market state, not placeholder selection only.

### ST-304 — Weekly chart subsystem
**User story:** As a player, I want weekly charts so that I can measure release success and rivalry over time.

**Acceptance criteria:**
- A dedicated chart subsystem calculates weekly rankings from sales/streams/chart points.
- Chart formulas can vary by era/profile.
- Chart history stores weekly entries and peak positions per record.
- Chart update events feed UI summaries and news generation.

**Notes:**
- Separate chart ranking from market simulation logic.
- MVP can ship with one global chart plus key regional charts.
- Ensure deterministic tie-breaking.

### ST-305 — Critic review generation
**User story:** As a player, I want releases to receive critic reviews so that prestige and reputation influence strategy beyond raw sales.

**Acceptance criteria:**
- Released records can receive critic reviews generated from record quality, artist reputation, and genre/era fit.
- Reviews produce a score/result and reputation impact.
- Critic events/news appear in the feed and record detail views.
- Review generation occurs in a defined simulation phase after release.

**Notes:**
- Awards can remain a later extension of this subsystem.
- Keep publication profiles data-driven.
- Avoid generating duplicate reviews for the same publication/record.

---

## EP-4 Finance, save/load, and data integrity

### ST-401 — Expanded finance ledger and summaries
**User story:** As a player, I want clear financial tracking so that I can understand whether my label strategy is sustainable.

**Acceptance criteria:**
- Finance subsystem posts ledger entries for advances, recording costs, marketing spend, release revenue, royalties, and tour results.
- Label balance updates emit events for UI refresh.
- Monthly summaries aggregate income, costs, and net result.
- Finance dashboard projections can be queried without widgets reading raw maps directly.

**Notes:**
- Keep ledger append-only where possible.
- Support at least one player label account cleanly.
- Negative balance handling should be explicit for future loans/investors.

### ST-402 — Full campaign snapshot save/load
**User story:** As a player, I want to save and resume a campaign exactly so that long-form play across decades is reliable.

**Acceptance criteria:**
- Save-game includes time, artists, contracts, song usage, records, sales history, finance, market state, charts, tours, pending events/news, and unlocks.
- Loading reconstructs subsystem state before UI rebuild.
- Save versioning is included in the snapshot schema.
- A loaded campaign reproduces the same next-week simulation outcome as before save.

**Notes:**
- Do not serialize transient widget state unless explicitly optional.
- Resolve assets from IDs, not raw paths from save data.
- Validate references before applying live state.

### ST-403 — Save migration and validation
**User story:** As a developer, I want versioned save migration and integrity checks so that updates do not silently corrupt campaigns.

**Acceptance criteria:**
- Save files include a version number and migration path hooks.
- Load validation checks broken references, impossible dates, duplicate active contracts, and invalid balances/ranges.
- Fatal integrity failures abort load with a clear user-facing error.
- Recoverable issues log warnings and apply safe defaults/fallbacks.

**Notes:**
- Add dedicated save/load log category output.
- Keep migration code isolated from subsystem business logic.
- Include automated test coverage for at least one migration path.

### ST-404 — Debug observability for simulation balancing
**User story:** As a developer, I want logs and debug summaries so that I can tune formulas and diagnose campaign issues.

**Acceptance criteria:**
- Dedicated UE log categories exist for time, artists, records, market, charts, tours, finance, save, and UI.
- Development builds can output weekly simulation summaries/traces.
- Debug counters expose tick duration, active entities, and save/load duration.
- A lightweight in-game debug panel or console dump is available for current era/market/chart breakdowns.

**Notes:**
- Keep debug features gated from shipping builds where appropriate.
- Prefer structured logs for balancing.
- Avoid heavy trace generation during normal release play.

---

## EP-5 UI orchestration and management screens

### ST-501 — Dashboard and status screen refresh model
**User story:** As a player, I want a dashboard that updates from simulation events so that I can quickly assess label health and priorities.

**Acceptance criteria:**
- Dashboard/status widgets display current date, balance, key roster stats, active recordings/releases, and top news.
- UI refreshes are event-driven rather than full rebuilds on every tick/frame.
- Monthly close and major weekly events trigger summary updates.
- Data is supplied through read-only view/projection structs where needed.

**Notes:**
- Reuse existing `UStatusWidget` patterns where possible.
- Keep widgets thin and free of business calculations.
- Fast-forward should batch updates into summaries.

### ST-502 — Artist roster, detail, and command panel integration
**User story:** As a player, I want to browse my roster and issue context-sensitive actions so that artist management is efficient.

**Acceptance criteria:**
- Signed artist list binds to roster projections keyed by stable IDs.
- Selecting an artist updates inspector/detail and command availability via selection context.
- Command panel actions dispatch typed commands instead of string-only business mutations.
- Hover/detail widgets continue to show artist summaries without direct subsystem coupling.

**Notes:**
- String command IDs may remain as UI routing keys only.
- Preserve current hover UX.
- Avoid rebuilding the full roster list on minor stat changes if possible.

### ST-503 — Recording and release planner screens
**User story:** As a player, I want dedicated screens for recording and release planning so that I can manage songs, formats, and budgets clearly.

**Acceptance criteria:**
- Recording screen shows eligible songs, production options, estimated cost/time, and validation feedback.
- Release planner shows schedule date, formats, regions, and marketing allocation.
- Confirm actions call command dispatcher and display structured success/failure messages.
- Screen state rebuilds correctly after load and after simulation events.

**Notes:**
- Keep song preview playback support.
- Disable invalid actions rather than allowing silent failure.
- Use selected artist context only from UI layer.

### ST-504 — Charts, news, and region market screens
**User story:** As a player, I want to inspect charts, news, and regional market data so that I can react to trends and outcomes.

**Acceptance criteria:**
- News feed displays generated stories with stable references to artists/records/events.
- Chart screen shows current rankings and recent history.
- Region market screen displays demand/exposure summaries from market subsystem data.
- Clicking relevant news/chart items can navigate to associated artist/record detail.

**Notes:**
- Region map should become functionally connected to market snapshots.
- Keep ticker/news deduplication behavior.
- Navigation should go through UI manager routing.

---

## EP-6 Tours, critics, and immersive presentation hooks

### ST-601 — Tour planning and weekly show resolution
**User story:** As a player, I want to plan tours and see their financial and artist impact so that live performance becomes a meaningful strategy.

**Acceptance criteria:**
- Tour planning supports venue/region/date selection with affordability and availability validation.
- Weekly simulation resolves scheduled shows into attendance, revenue, cost, fatigue, and crowd energy outcomes.
- Tour state and show history are persisted in saves.
- Tour completion and standout shows emit events/news.

**Notes:**
- Start with a simplified routing model.
- Prevent conflicts with recording/release commitments.
- Merchandise can be included as a simple revenue modifier initially.

### ST-602 — Performance package generation
**User story:** As a developer, I want simulation to generate performance packages so that immersive scenes can present outcomes without owning gameplay logic.

**Acceptance criteria:**
- A performance package struct is generated from tours/auditions/releases with artist, song, venue, crowd, era styling, and stage parameters.
- Package generation uses simulation outcomes such as crowd energy and venue scale.
- Presentation package creation is decoupled from ticket sales/finance calculations.
- Packages can be consumed by a future/current presentation subsystem entry point.

**Notes:**
- No need to fully ship concert scenes in the first release using this story.
- Include stable asset/content references only.
- Support low-fidelity fallback presentation later.

### ST-603 — Audition flow cleanup
**User story:** As a player, I want auditions to feel integrated with management systems so that discovering talent is coherent and maintainable.

**Acceptance criteria:**
- Audition UI and actor flow use command/event patterns rather than mixed UI/simulation ownership.
- Audition outcomes create or reference unsigned artist entities with stable IDs.
- Song preview playback remains available through shared audio component integration.
- Audition-triggered news/actions route through UI manager consistently.

**Notes:**
- Preserve existing audition scene entry where possible.
- Keep immersive audition logic presentation-only.
- Avoid direct widget ownership by world actors beyond explicit interfaces.

### ST-604 — Awards and prestige extension
**User story:** As a player, I want awards and nominations to boost prestige so that long-term label legacy matters.

**Acceptance criteria:**
- Awards season processing can nominate and award eligible artists/records based on reviews, charts, and era rules.
- Wins/nominations affect artist reputation and label prestige metrics.
- Award outcomes generate news items and appear in artist/record history.
- Award definitions are data-driven.

**Notes:**
- Can launch with a single simplified annual award event.
- Keep formulas transparent for balancing.
- Ensure no duplicate award wins for the same category/season entry.

# Milestones

## M1 — Core management loop MVP
Stories: `ST-101`, `ST-102`, `ST-103`, `ST-201`, `ST-202`, `ST-203`, `ST-301`, `ST-302`, `ST-401`, `ST-501`, `ST-502`, `ST-503`

## M2 — Market, charts, and reliable persistence
Stories: `ST-104`,