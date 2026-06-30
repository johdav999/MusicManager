# Architecture overview

Target stack inferred from the idea and provided architecture:

- **Platform:** PC/console game built in **Unreal Engine 5**
- **Language:** **C++** for core systems, **UMG/UUserWidget** for UI
- **Rendering/character tech:** **UE5 + MetaHumans**
- **Runtime style:** primarily **single-player, local simulation**
- **Persistence:** local save-game files plus cooked data assets/data tables
- **Audio/content pipeline:** pre-generated song catalog imported as game content, with metadata-driven simulation

This product should use a **layered gameplay architecture inside a single Unreal game client**, centered on deterministic management simulation and optional immersive presentation scenes.

At a high level:

1. **Simulation Layer**
   - Owns time progression, artists, songs, records, market, charts, tours, critics, finance, rival labels, and events.
   - Runs on a fixed in-game cadence, ideally weekly with monthly rollups.
   - Must be deterministic and data-driven.

2. **Presentation Layer**
   - UI dashboards, artist panels, charts, finance screens, contracts, release planning, tour planner.
   - Immersive scenes for auditions, concerts, music videos, studio sessions.
   - Reads state from simulation services and sends player commands back.

3. **Content/Data Layer**
   - Static era data, genre definitions, market rules, song metadata, region profiles, event templates, chart formulas.
   - Stored in DataTables / PrimaryDataAssets / cooked assets.
   - Save-game stores mutable campaign state only.

4. **Orchestration Layer**
   - GameInstanceSubsystems coordinate simulation and UI.
   - A central command/event approach prevents widgets from directly mutating game state.

Recommended architectural principle:

- **Subsystems own business state**
- **Widgets are thin**
- **Commands mutate state**
- **Events notify UI**
- **Save/load serializes subsystem snapshots**
- **Immersive scenes consume simulation outputs, not own simulation logic**

---

# Key decisions

1. **Keep a single-player authoritative local simulation**
   - No backend is required for core gameplay.
   - This reduces complexity and fits the management sim genre.
   - Future cloud saves or telemetry can be added later without changing core simulation.

2. **Use Unreal `UGameInstanceSubsystem` as the main service boundary**
   - Matches the existing codebase direction.
   - Good lifetime for campaign-wide systems.
   - Keeps logic independent from map loads and presentation scenes.

3. **Introduce a dedicated Simulation Director**
   - Current `UGameTimeSubsystem` is the orchestrator; formalize this into a stricter simulation pipeline.
   - It should execute ordered phases:
     - trend updates
     - artist state
     - production progress
     - release/chart resolution
     - tour resolution
     - finance settlement
     - event/news generation
   - This avoids hidden side effects and makes balancing easier.

4. **Move from monthly-only simulation to weekly ticks with monthly summaries**
   - Weekly charts and release cadence are core to music management fantasy.
   - Tours, critics, radio, streaming, and virality feel better on weekly resolution.
   - Monthly/quarterly reports can still be generated for finance UX.

5. **Standardize all entities on stable IDs**
   - Never use artist names as keys.
   - Use `FGuid` or stable `FName`/string IDs for:
     - ArtistId
     - SongId
     - RecordId
     - ContractId
     - TourId
     - EventId
     - LabelId
   - Names become display fields only.

6. **Separate simulation state from presentation state**
   - Selected artist, hovered item, open modal, current tab should not live in simulation subsystems.
   - Create a `USelectionContextSubsystem` or keep this in `UUIManagerSubsystem`.
   - This reduces save/load bugs and hidden coupling.

7. **Treat concerts/music videos as generated presentations of simulation outcomes**
   - The concert scene should be driven by a `FPerformancePackage` generated from simulation:
     - artist lineup
     - venue size
     - crowd energy
     - setlist
     - stage theme
     - era styling
   - The scene should not decide ticket sales or artist fatigue; simulation already did.

8. **Use data-driven balancing**
   - Era rules, genre popularity, media channels, format economics, chart formulas, and audience segments should be editable without code changes.
   - Prefer DataAssets/DataTables over hardcoded values.

9. **Expand save-game coverage to full campaign snapshots**
   - Current save omissions are risky.
   - Save all mutable subsystem state required to resume exactly:
     - time
     - artists/contracts
     - songs/locks
     - records/releases/sales history
     - finance ledger/balance
     - market exposure/trends
     - tours
     - rival labels
     - pending events/news
     - unlocked content

10. **Use a command bus pattern for player actions**
    - UI sends typed commands.
    - Subsystems validate and execute.
    - Results are returned as success/failure + emitted domain events.
    - This makes testing and future automation easier.

---

# Components (diagram in text + responsibilities)

```text
[Player Input]
    |
    v
[PlayerController]
    |
    v
[UILayout / Screen Router] <-----------------------------+
    |                                                    |
    +--> [Dashboard Widgets]                             |
    +--> [Artist/Contract Widgets]                       |
    +--> [Record/Release Widgets]                        |
    +--> [Tour Widgets]                                  |
    +--> [Charts/News Widgets]                           |
    +--> [Finance Widgets]                               |
    +--> [Region/Market Widgets]                         |
    +--> [Immersive Scene Launchers]                     |
    |                                                    |
    v                                                    |
[UI Manager Subsystem]                                   |
    |                                                    |
    +--> sends commands ---------------------------------+
    |
    v
[Command Dispatcher / Application Layer]
    |
    +--> [Game Time / Simulation Director]
    +--> [Artist Manager]
    +--> [Song Catalog Manager]
    +--> [Production & Record Manager]
    +--> [Release & Catalog Manager]
    +--> [Market & Trend Manager]
    +--> [Chart Manager]
    +--> [Critic & Awards Manager]
    +--> [Tour Manager]
    +--> [Finance Manager]
    +--> [Rival Label AI Manager]
    +--> [Event/News Manager]
    +--> [Save/Load Manager]
    |
    v
[Domain Events Bus]
    |
    +--> [UI Manager]
    +--> [News Feed]
    +--> [Notification System]
    +--> [Analytics/Telemetry]
    +--> [Immersive Performance Builder]
    |
    v
[Persistence Layer]
    |
    +--> [SaveGame snapshots]
    +--> [Static DataTables/DataAssets]
    +--> [Audio/Visual content assets]
```

## Core runtime components

### 1. `UGameTimeSubsystem` / Simulation Director
Responsibilities:
- Own in-game date and tick cadence
- Advance week/month/year
- Execute simulation phases in deterministic order
- Broadcast `OnWeekAdvanced`, `OnMonthClosed`, `OnYearAdvanced`

Recommended phases per week:
1. trend drift
2. artist condition/personality updates
3. production progress
4. release launch processing
5. market exposure updates
6. chart calculation
7. tour show resolution
8. finance posting
9. critic/news generation
10. UI notifications

### 2. `UArtistManagerSubsystem`
Responsibilities:
- Artist roster and discovery pool
- Band/member composition
- Attributes and personality traits
- Career momentum, reputation, burnout, scandals
- Contract ownership and artist availability
- Artist action eligibility

Subdomains:
- unsigned talent pool
- signed roster
- retired/disbanded artists
- artist relationships and internal band cohesion

### 3. `USongCatalogSubsystem`
Responsibilities:
- Registry of all pre-authored songs
- Song metadata and eligibility
- Ownership/usage state
- Song-to-record linkage
- Covers/licensing flags later

Important distinction:
- **Song definition** = static content metadata
- **Song instance/use** = mutable campaign usage state

### 4. `UProductionSubsystem` / `URecordManagerSubsystem`
Responsibilities:
- Recording intents
- Studio booking
- Producer/engineer modifiers
- Production quality outcomes
- Release package creation: single, EP, album
- Release state machine:
  - planned
  - recording
  - mastered
  - scheduled
  - released
  - catalog

### 5. `UMarketManagerSubsystem`
Responsibilities:
- Region demand
- audience segments
- genre popularity by era/region
- media channel effectiveness
- exposure accumulation
- streaming/radio/press/social effects

### 6. `UChartManagerSubsystem`
Responsibilities:
- Weekly chart ranking
- chart points formulas by era
- sales vs streams weighting
- regional and global charts
- chart history and milestones

This should be separated from market simulation for clarity.

### 7. `UCriticAwardsSubsystem`
Responsibilities:
- critic reviews
- publication profiles
- review score generation
- awards nominations and wins
- prestige effects on catalog and artist reputation

### 8. `UTourManagerSubsystem`
Responsibilities:
- Tour planning
- venue booking
- routing
- ticket demand simulation
- show-by-show outcomes
- fatigue, morale, and profitability
- generation of performance packages for immersive scenes

### 9. `UFinanceManagerSubsystem`
Responsibilities:
- Label cash accounts
- ledger entries
- royalties
- advances
- marketing spend
- studio/tour/distribution costs
- loans/investor obligations
- P&L and cash flow summaries

### 10. `URivalLabelSubsystem`
Responsibilities:
- Competing labels
- AI signing behavior
- release competition
- poaching attempts
- market pressure and chart rivalry

### 11. `UEventSubsystem`
Responsibilities:
- Convert domain events into player-facing news
- deduplicate and prioritize stories
- trigger opportunities/scenarios
- feed notifications and ticker cards

### 12. `UUIManagerSubsystem`
Responsibilities:
- screen routing
- modal orchestration
- selection context
- command dispatch from widgets
- event-to-UI translation
- hover/tooltip/detail presentation

### 13. `UPerformancePresentationSubsystem`
Responsibilities:
- Build concert/music video/audition scene payloads
- Spawn MetaHumans, stage setup, crowd profile, lighting presets
- Sync selected song audio and animation package
- Return to management UI after scene completion

### 14. `UMusicSaveSubsystem`
Responsibilities:
- campaign save/load
- versioned snapshot serialization
- migration between save versions
- rebuild subsystem state after load

---

# Data model & storage (include persistence choice and schemas)

## Persistence choice

Use a hybrid model:

1. **Static content:** Unreal `UDataTable`, `UPrimaryDataAsset`, and cooked assets
   - eras
   - genres
   - regions
   - audience segments
   - media channels
   - venue definitions
   - song catalog metadata
   - event templates
   - chart rules
   - award definitions

2. **Mutable campaign state:** Unreal `USaveGame`
   - serialized subsystem snapshots
   - versioned schema
   - one save slot = one campaign

3. **Optional analytics/debug logs:** local JSON/CSV in development builds only

This is the right fit because:
- static balancing data should be editable by designers
- mutable state should be compact and resumable
- no relational DB is needed for runtime

## Entity model

## Core IDs

```text
LabelId: string/guid
ArtistId: string/guid
SongDefId: string/guid
SongUseId: string/guid (optional if needed)
RecordId: string/guid
ContractId: string/guid
TourId: string/guid
ShowId: string/guid
EventId: string/guid
RegionId: string
GenreId: string
ChannelId: string
VenueId: string
ChartId: string
ReviewId: string/guid
AwardSeasonId: string/guid
```

## Static schemas

### Artist archetype / generated artist template
```json
{
  "ArtistTemplateId": "artist_tpl_rock_band_60s",
  "Era": "1960s",
  "DefaultType": "Band",
  "GenreAffinities": ["rock", "blues"],
  "AttributeRanges": {
    "Talent": [40, 90],
    "Charisma": [30, 85],
    "Reliability": [20, 80],
    "MarketAppeal": [25, 75]
  },
  "PersonalityRanges": {
    "Ego": [10, 80],
    "Teamwork": [20, 90],
    "LifestyleRisk": [0, 70]
  }
}
```

### Song definition
```json
{
  "SongDefId": "song_000123",
  "Title": "Midnight Signal",
  "Era": "1980s",
  "GenreTags": ["synthpop", "pop"],
  "Mood": "Romantic",
  "Energy": 72,
  "ConsumerAppeal": ["Teenagers", "YoungAdults"],
  "BaseQuality": 68,
  "DurationSec": 214,
  "Language": "en",
  "AudioAssetRef": "/Game/Audio/Songs/song_000123",
  "PerformanceProfileId": "perf_synthpop_midtempo"
}
```

### Region definition
```json
{
  "RegionId": "US",
  "DisplayName": "United States",
  "MarketSize": 1.0,
  "GenreBias": {
    "rock": 1.1,
    "country": 1.2,
    "jazz": 0.9
  },
  "ChannelStrengths": {
    "radio": 1.0,
    "tv": 0.9,
    "streaming": 1.1
  }
}
```

### Era rules
```json
{
  "EraId": "1990s",
  "StartYear": 1990,
  "EndYear": 1999,
  "PrimaryFormats": ["CD", "Cassette"],
  "EmergingFormats": ["MP3"],
  "ChannelWeights": {
    "radio": 0.8,
    "tv": 0.7,
    "magazines": 0.6,
    "internet": 0.2
  },
  "ChartFormulaProfile": "chart_90s_standard"
}
```

## Mutable schemas

### Artist state
```json
{
  "ArtistId": "artist_001",
  "Name": "The Neon Hearts",
  "Type": "Band",
  "CurrentLabelId": "label_player",
  "Status": "Signed",
  "Attributes": {
    "Talent": 78,
    "Charisma": 66,
    "Reliability": 54,
    "MarketAppeal": 71
  },
  "Personality": {
    "Ego": 62,
    "Teamwork": 48,
    "LifestyleRisk": 35,
    "Activism": 20
  },
  "Career": {
    "Momentum": 58,
    "Reputation": 44,
    "Fatigue": 21,
    "BurnoutRisk": 12,
    "ScandalHeat": 0
  },
  "GenreAffinities": ["new_wave", "pop"],
  "ContractId": "contract_001",
  "ActiveRecordIds": ["record_010"],
  "Flags": ["CanTour", "CanRecord"]
}
```

### Contract
```json
{
  "ContractId": "contract_001",
  "ArtistId": "artist_001",
  "LabelId": "label_player",
  "StartDate": "1983-03-01",
  "EndDate": "1986-02-28",
  "Advance": 50000,
  "RoyaltyRate": 0.14,
  "TourSplit": 0.1,
  "AlbumCommitment": 2,
  "OptionsRemaining": 1,
  "Status": "Active"
}
```

### Record/release
```json
{
  "RecordId": "record_010",
  "ArtistId": "artist_001",
  "LabelId": "label_player",
  "Type": "Album",
  "Title": "Electric Youth",
  "SongDefIds": ["song_000123", "song_000124"],
  "Production": {
    "StudioTier": 2,
    "ProducerId": "producer_03",
    "QualityScore": 74,
    "Experimentalism": 35,
    "RadioFriendliness": 68
  },
  "Release": {
    "PlannedDate": "1983-09-02",
    "ActualDate": "1983-09-02",
    "Formats": ["Vinyl", "Cassette"],
    "Regions": ["US", "UK", "DE"]
  },
  "Commercial": {
    "MarketingBudget": 120000,
    "InitialExposure": 18,
    "LifetimeUnits": 0,
    "LifetimeStreams": 0,
    "PeakChartPositions": {}
  },
  "State": "Released"
}
```

### Weekly chart entry
```json
{
  "ChartId": "US_ALBUMS",
  "WeekStart": "1983-09-05",
  "Entries": [
    {
      "Rank": 1,
      "RecordId": "record_010",
      "ArtistId": "artist_001",
      "ChartPoints": 9321,
      "Units": 84000,
      "Streams": 0
    }
  ]
}
```

### Tour
```json
{
  "TourId": "tour_001",
  "ArtistId": "artist_001",
  "Name": "Electric Youth Tour",
  "StartDate": "1983-10-01",
  "EndDate": "1983-11-20",
  "Shows": [
    {
      "ShowId": "show_001",
      "Date": "1983-10-01",
      "RegionId": "US",
      "VenueId": "venue_arena_01",
      "Capacity": 12000,
      "TicketsSold": 9800,
      "GrossRevenue": 196000,
      "Cost": 110000,
      "CrowdEnergy": 74
    }
  ],
  "Status": "Active"
}
```

### Finance ledger entry
```json
{
  "EntryId": "fin_0001",
  "Date": "1983-09-10",
  "LabelId": "label_player",
  "Type": "RecordRevenue",
  "ReferenceId": "record_010",
  "Amount": 245000,
  "Currency": "USD",
  "Memo": "Week 1 album sales"
}
```

## SaveGame structure

```text
UMusicSaveGame
- SaveVersion
- CampaignMeta
  - SlotName
  - CreatedAt
  - LastPlayedAt
  - Difficulty
  - Mode
- TimeSnapshot
- PlayerLabelSnapshot
- ArtistSnapshot[]
- ContractSnapshot[]
- SongUsageSnapshot[]
- RecordSnapshot[]
- TourSnapshot[]
- FinanceSnapshot
- MarketSnapshot
- ChartHistorySnapshot
- CriticSnapshot
- RivalLabelSnapshot
- EventQueueSnapshot
- UnlocksSnapshot
- UIOptionalSnapshot (selected tab, filters) [optional]
```

## Versioning strategy

Every save should include:
- `SaveVersion`
- migration functions:
  - v1 -> v2
  - v2 -> v3

Never deserialize directly into live subsystem state without validation.

---

# Interface design (APIs, commands, events, UI interactions, and integrations as appropriate)

## Internal API style

Use typed C++ service interfaces between subsystems, not stringly-typed calls.

Examples:

```cpp
struct FSignArtistCommand;
struct FCreateRecordingIntentCommand;
struct FReleaseRecordCommand;
struct FPlanTourCommand;
struct FAllocateMarketingBudgetCommand;

struct FCommandResult
{
    bool bSuccess;
    FText UserMessage;
    TArray<FName> ErrorCodes;
};
```

## Command layer

Recommended command dispatcher:

```text
UCommandDispatcherSubsystem
  - ExecuteSignArtist(...)
  - ExecuteNegotiateContract(...)
  - ExecuteStartRecording(...)
  - ExecuteScheduleRelease(...)
  - ExecuteLaunchMarketingCampaign(...)
  - ExecutePlanTour(...)
  - ExecuteTakeLoan(...)
  - ExecuteAdvanceTime(...)
```

This layer:
- validates inputs
- checks preconditions
- calls owning subsystem
- emits domain events
- returns user-facing results

## Example commands

### Sign artist
Input:
```json
{
  "ArtistId": "artist_001",
  "LabelId": "label_player",
  "Advance": 50000,
  "RoyaltyRate": 0.14,
  "AlbumCommitment": 2
}
```

Validation:
- artist exists
- artist unsigned or contract-expired
- player has enough cash for advance
- terms within allowed range

Output:
- success/failure
- created `ContractId`
- emitted `ArtistSigned`

### Start recording
Input:
```json
{
  "ArtistId": "artist_001",
  "SongDefIds": ["song_000123", "song_000124"],
  "StudioTier": 2,
  "ProducerId": "producer_03",
  "TargetType": "Album"
}
```

Validation:
- artist signed
- songs available and not locked
- artist can record
- enough funds

Output:
- `RecordingStarted`
- expected completion date
- finance reservation posted

### Plan tour
Input:
```json
{
  "ArtistId": "artist_001",
  "Shows": [
    {"Date": "1983-10-01", "VenueId": "venue_arena_01", "RegionId": "US"}
  ]
}
```

Validation:
- artist available
- no conflicting recording/tour dates
- venue capacity appropriate
- estimated cost affordable

## Domain events

Use multicast delegates or an internal event bus with typed payloads.

Core events:
- `EGameWeekAdvanced`
- `EGameMonthClosed`
- `EArtistSigned`
- `EArtistContractExpired`
- `EArtistScandalTriggered`
- `ERecordingStarted`
- `ERecordingCompleted`
- `ERecordReleased`
- `EChartUpdated`
- `ECriticReviewPublished`
- `EAwardNominated`
- `EAwardWon`
- `ETourStarted`
- `EShowCompleted`
- `EFinanceBalanceChanged`
- `ENewsGenerated`

Event payload example:
```cpp
USTRUCT()
struct FRecordReleasedEvent
{
    GENERATED_BODY()

    FString RecordId;
    FString ArtistId;
    FGameDate ReleaseDate;
    TArray<FString> RegionIds;
};
```

## UI interactions

## UI principles
- widgets do not directly mutate subsystem state
- widgets gather input and dispatch commands
- widgets subscribe to view-model style data or events
- top-level layout owns screen composition

## Main screens
- Dashboard
- Artist Roster
- Talent Discovery / Auditions
- Contract Negotiation
- Recording / Production
- Release Planner
- Marketing & Distribution
- Charts & Critics
- Tour Planner
- Finance
- Rival Labels / Industry News
- Region Market View
- Settings / Save Load

## Example interaction flow: release an album
1. Player selects artist in roster
2. UI requests artist detail projection from `ArtistManager`
3. Player opens recording screen
4. UI requests eligible songs from `SongCatalogSubsystem`
5. Player confirms recording command
6. Command dispatcher validates and calls `RecordManager`
7. `RecordManager` emits `RecordingStarted`
8. UI shows progress card
9. On completion, event triggers release planner availability
10. Player schedules release and marketing
11. Weekly simulation resolves charts and reviews
12. News feed and dashboard update

## View model recommendation

For complex widgets, expose read-only projections:
- `FArtistDetailView`
- `FRecordPlannerView`
- `FFinanceDashboardView`
- `FChartSummaryView`

This avoids widgets querying many subsystems directly.

## Integrations

### Audio integration
- Song metadata references imported audio assets
- `UMusicPlayerComponent` handles preview playback
- performance scenes use synchronized audio + animation cues

### MetaHuman/performance integration
- `UPerformancePresentationSubsystem` receives a `FPerformancePackage`
- package includes:
  - performer appearance refs
  - song/audio refs
  - venue/stage preset
  - crowd density
  - camera style
  - lighting profile

### Optional future integrations
- Steam/Epic achievements
- cloud saves
- telemetry
- mod support for song metadata packs

---

# Security & privacy

For a local single-player game, security is mostly about integrity and safe content handling rather than network defense.

## Security priorities

1. **Save-game integrity**
   - Validate all loaded save data
   - Reject malformed IDs, impossible dates, negative balances where invalid, broken references
   - Use versioned migrations and sanity checks

2. **Asset reference safety**
   - Never trust arbitrary external paths from save files
   - Save only stable IDs, then resolve to cooked assets through registries

3. **Command validation**
   - All gameplay actions must validate server-style even in single-player
   - Prevent UI from bypassing rules

4. **Cheat/debug separation**
   - Keep debug commands behind dev flags
   - Do not ship unrestricted console commands that corrupt campaign state

5. **Privacy**
   - If no telemetry is enabled, no personal data is collected
   - If telemetry is added later:
     - opt-in
     - anonymized
     - no song/audio personal data
     - clear privacy notice

6. **Third-party content compliance**
   - Ensure rights and licensing for generated music/audio assets are legally cleared for shipped use
   - Store provenance metadata in content pipeline, even if not exposed in runtime

---

# Error handling & observability

## Error handling strategy

Use three levels:

1. **Player-facing validation errors**
   - Example: “Not enough funds to start this recording.”
   - Returned from command execution as structured results

2. **Recoverable runtime warnings**
   - Example: missing optional review template, unresolved non-critical UI asset
   - Log warning, use fallback behavior

3. **Fatal data integrity errors**
   - Example: save references missing artist for active contract
   - Abort load with a clear message and fallback to main menu or backup recovery

## Result pattern

```cpp
enum class ECommandErrorCode
{
    None,
    InsufficientFunds,
    InvalidArtistState,
    SongLocked,
    ContractRequired,
    DateConflict,
    InvalidReference
};
```

Return:
- success flag
- error code
- localized message
- optional remediation hint

## Observability

### Logging categories
Create dedicated UE log categories:
- `LogMusicSimTime`
- `LogMusicArtists`
- `LogMusicRecords`
- `LogMusicMarket`
- `LogMusicCharts`
- `LogMusicTours`
- `LogMusicFinance`
- `LogMusicSave`
- `LogMusicUI`

### Metrics/debug counters
In development builds, track:
- simulation tick duration
- chart calculation duration
- number of active artists
- number of active releases
- event queue size
- save/load duration
- UI rebuild counts

### Debug tools
Add in-game debug panels for:
- current era modifiers
- artist hidden stats
- market demand by region/genre
- chart point breakdown
- finance ledger explorer
- event trace for last tick

### Replayable simulation traces
For balancing, optionally dump weekly simulation summaries to JSON in dev builds:
```json
{
  "Week": "1983-W36",
  "ArtistId": "artist_001",
  "ExposureDelta": 12,
  "SalesUnits": 84000,
  "ChartPoints": 9321,
  "Revenue": 245000
}
```

This is extremely useful for tuning formulas.

---

# Performance considerations

This game is mostly simulation + UI, with occasional heavy 3D presentation scenes.

## Simulation performance
- Weekly simulation should be **batched and deterministic**
- Avoid per-frame business logic
- Use compact structs and maps keyed by stable IDs
- Precompute era/channel/region modifiers where possible
- Cache derived values for current week only

## UI performance
- Avoid rebuilding large widget trees every tick
- Use event-driven refreshes
- Virtualize long lists if roster/catalog grows large
- Use view models to reduce repeated subsystem queries

## Save/load performance
- Serialize snapshots in chunks by subsystem
- Avoid storing redundant derived data if it can be recomputed
- Keep chart history summarized if full history becomes too large

## Content performance
- Song previews should stream audio efficiently
- Concert scenes should load asynchronously
- MetaHuman scenes should use LODs and scalable presets
- Separate management maps from performance maps to control memory

## Performance scene strategy
Concerts/music videos are expensive compared to management UI. Use:
- prebuilt venue templates
- reusable animation sets
- parameterized lighting/crowd systems
- async loading screen before immersive scenes
- lower-cost fallback presentation mode on weaker hardware

## Time acceleration
If player fast-forwards:
- simulation should process multiple weeks without UI redraw between each step
- emit summarized notifications instead of every micro-event
- allow interruption on major events:
  - chart #1
  - bankruptcy risk
  - scandal
  - award win
  - contract expiration

---

# Incremental delivery plan (phases)

## Phase 1 — Core management sim foundation
Goal: playable label management loop without immersive performances.

Deliver:
- `GameTimeSubsystem` with weekly/monthly progression
- `ArtistManagerSubsystem`
- `SongCatalogSubsystem`
- `RecordManagerSubsystem`
- `FinanceManagerSubsystem`
- `MarketManagerSubsystem` basic version
- `ChartManagerSubsystem` basic version
- `UIManagerSubsystem` + dashboard/artist/record/finance screens
- save/load v1
- static era/genre/region/song data pipeline

Playable loop:
- sign artist
- record single/album
- release
- spend marketing
- see sales/charts
- manage cash

## Phase 2 — Contracts, critics, and richer market behavior
Goal: deepen strategy and replayability.

Deliver:
- contract negotiation terms
- artist morale/fatigue/reliability
- critic review system
- awards system
- region-specific demand
- media channel evolution by era
- better chart formulas
- news/event feed improvements
- rival label AI v1

Playable impact:
- more meaningful release timing
- reputation and prestige matter
- competition emerges

## Phase 3 — Tours and live business
Goal: complete the label fantasy.

Deliver:
- `TourManagerSubsystem`
- venue and routing data
- ticket demand simulation
- merchandise revenue
- artist burnout from touring
- tour planner UI
- show summaries and profitability reports

Playable impact:
- second major revenue stream
- risk/reward planning
- stronger artist lifecycle management

## Phase 4 — Immersive performance presentation
Goal: connect management outcomes to visual payoff.

Deliver:
- `PerformancePresentationSubsystem`
- audition scene flow cleanup
- concert scene generation from simulation package
- MetaHuman performer binding
- stage/crowd/lighting presets
- music video presentation mode
- low/high fidelity presentation options

Playable impact:
- player can watch artists perform songs they released
- stronger emotional reward loop

## Phase 5 — Campaign depth across decades
Goal: fulfill the full 1950s–2020s fantasy.

Deliver:
- era transitions and unlocks
- format shifts: vinyl/cassette/CD/MP3/streaming
- changing chart formulas by decade
- evolving discovery channels
- scenario/challenge mode
- sandbox mode
- balancing pass across all decades

Playable impact:
- long-form campaign becomes compelling
- each era feels distinct

## Phase 6 — Polish, balancing, and production hardening
Goal: ship-ready quality.

Deliver:
- full save migration/versioning
- data validation tools
- debug/balancing dashboards
- performance optimization
- accessibility and UX polish
- tutorial/onboarding
- achievements/platform integration
- telemetry optional

## Recommended MVP cut
If scope must be reduced, ship first with:
- 1980s–2000s only
- singles/albums only
- no rival labels at launch
- no immersive concerts at launch
- simplified critics/awards
- one global chart + a few major regions

That MVP still delivers the core fantasy and leaves room for later expansion.