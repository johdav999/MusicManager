# MusicManager Current Feature And GUI Inventory

This document describes the functionality and GUI currently present in the repository based on the C++ source, config, and visible Unreal content assets.

## Assessment Scope

This inventory is based on repository inspection only. Blueprint internals inside `.uasset` files were not opened in the Unreal Editor, so Blueprint behavior is inferred from asset names, C++ backing classes, config references, and binding expectations.

## Current Gameplay Functionality

### Time Simulation

- Weekly time advancement is implemented.
- Weekly time advancement can be executed through a typed command dispatcher command.
- Multi-week batch advancement is implemented.
- Month-close summaries are implemented.
- Year change broadcasts are implemented.
- A deterministic weekly phase order exists.
- Monthly compatibility hooks exist for systems that still update monthly.

Primary files:

- `Source/MusicManager/Public/GameTimeSubsystem.h`
- `Source/MusicManager/Private/GameTimeSubsystem.cpp`

### Command Dispatcher

- A Blueprint-visible command result model exists.
- Command results include success/failure, stable error code, user-facing message, remediation hint, affected entity ids, created entity id, quantity, and result date.
- Stable command error codes exist for invalid references, invalid state, insufficient funds, date conflicts, locked songs, unsigned artists, validation failure, and missing subsystems.
- `UCommandDispatcherSubsystem` exists as the application-layer entry point for implemented player actions.
- Command execution logs through `LogMusicCommands`.
- Commands emit a command-level result delegate after execution.
- Commands emit typed domain events with event id, command type, structured result, affected ids, created entity id, and timestamp.
- Commands expose both Blueprint and native domain-event delegates for UI/news/analytics/testing subscribers.
- The UI manager exposes command notifications with severity, structured result, notification id, and timestamp.
- News generation now binds directly to `UGameTimeSubsystem::OnMonthlySummaryClosed`, so monthly news summaries/events are produced from the deterministic monthly close path instead of depending on a missed world-initialization hookup.
- News event delivery is centralized through `UEventSubsystem::OnNewsEventGenerated`; this avoids double-delivering non-batched events while keeping `UUIManagerSubsystem` as the UI routing subscriber.
- News pipeline logging now covers monthly summary receipt, dedupe decisions, event emission, UI routing/defer/queue behavior, layout card addition, and `UNewsFeedList` card creation/insertion.
- `UGameTimeSubsystem` now starts an automatic real-time timer on initialization and calls `AdvanceMonth()` every 5 seconds. If subsystem initialization happens before a valid `UWorld` exists, the subsystem logs that it is waiting and retries startup from `FWorldDelegates::OnPostWorldInitialization`. `PauseTime(true)` stops the timer and `PauseTime(false)` resumes it; the timer also stops when the simulation reaches the supported end date.
- Monthly news generation now runs from `OnMonthlySummaryClosed` only; the old direct monthly compatibility call into `EventSubsystem` was removed to avoid pre-processing/deduping the same month before the delegate subscriber path sees it.
- Monthly news generation now falls back to a real industry recap event when no unsigned artist event can be generated, so closed months do not silently produce no feed item.
- News UI delivery no longer depends on `OnNewsEventGenerated` multicast dispatch reaching `UUIManagerSubsystem`; `UEventSubsystem::EmitNewsEvent` routes directly to `UUIManagerSubsystem::HandleNewsEvent` and then broadcasts the event for non-UI subscribers.
- `UNewsFeedItemWidget` and `UNewsFeedList` now force inserted cards visible and log widget bindings, parent visibility, headline assignment, and child counts so display-side issues are diagnosable.
- `UNewsFeedList` now exposes a Blueprint-assignable `OnNewsFeedCardAdded` delegate and an overrideable `On News Card Added` Blueprint event that fire after a news card has been created, populated, and inserted into the feed.
- `UNewsFeedList` now clears designer placeholder children from `FeedContainer` on construct by default, preventing an empty starter card from appearing in the runtime feed.
- `UNewsFeedList` now keeps the hover/detail ticker hidden at gameplay start and only shows it after a real news item is hovered or toggled, preventing an empty popup-style news card from appearing during BeginPlay.
- `UNewsFeedItemWidget` now caches incoming news data and reapplies it after `NativeConstruct`, so Blueprint Construct/default values cannot blank a populated news card after `SetupFromEvent`.
- `NewsFeedItemBP` is now rebuilt with an explicit 472x148 root size box, and runtime news cards receive vertical list padding, so the feed allocates the full card width instead of clipping each item down to the icon strip. The newsfeed rebuild automation now recreates the generated newsfeed Widget Blueprints before assembly to clear stale UMG compiler metadata.
- Clicking a `NewUpcomingArtistPerforming` news feed card now resolves the unsigned artist from stable news metadata or source-name fallback and opens the artist audition panel populated with that artist's real audition data.
- Command notifications are buffered for widgets that bind after a command completes.
- Sign artist command exists and validates player label id, unsigned artist availability, and contract term ranges before signing.
- Reject artist command exists and validates unsigned artist state before rotating the audition pool.
- Start recording command exists and wraps `URecordManagerSubsystem::SubmitRecordingIntent`, converting subsystem validation errors into structured command results.
- Schedule release command exists and wraps the record subsystem release scheduling path.
- Launch marketing campaign command exists and wraps the marketing subsystem campaign launch path.
- Advance time command exists and validates positive week counts before calling weekly simulation advancement.
- Audition sign/pass actions now dispatch typed commands instead of mutating artist state directly.
- Recording GUI confirmation now dispatches a typed start-recording command instead of directly submitting the recording intent.
- Command automation tests exist for result construction, invalid command failure/domain-event emission, and UI notification buffering.
- `MusicManager.Commands` automation tests pass through `UnrealEditor-Cmd`.

Primary files:

- `Source/MusicManager/Public/MusicCommandResult.h`
- `Source/MusicManager/Private/MusicCommandResult.cpp`
- `Source/MusicManager/Public/CommandDispatcherSubsystem.h`
- `Source/MusicManager/Private/CommandDispatcherSubsystem.cpp`
- `Source/MusicManager/Private/Tests/CommandDispatcherTests.cpp`
- `Source/MusicManager/Public/ArtistManagerSubsystem.h`
- `Source/MusicManager/Private/ArtistManagerSubsystem.cpp`
- `Source/MusicManager/Public/UIManagerSubsystem.h`
- `Source/MusicManager/Private/UIManagerSubsystem.cpp`
- `Source/MusicManager/Private/AuditionWidget.cpp`
- `Source/MusicManager/Private/UI/RecordWidget.cpp`

### Artist Management

- Unsigned artist pool support exists.
- Artists can be loaded from a DataTable.
- The next unsigned artist can be selected for audition.
- Unsigned artists can be rotated after passing/rejecting.
- Artists can be signed.
- Artists can be rejected.
- Artists can be signed by stable artist id through the command dispatcher.
- Artist rejection/pass now rotates the unsigned artist pool through the artist subsystem.
- Active and expired contracts are tracked.
- Monthly contract financial processing exists.
- Contract expiry is implemented.
- Signed artist data can be queried for UI.
- Selected artist state exists.
- Artist action availability is tracked for UI attention states.
- Artist market modifiers exist for sales simulation.

Primary files:

- `Source/MusicManager/Public/ArtistManagerSubsystem.h`
- `Source/MusicManager/Private/ArtistManagerSubsystem.cpp`

### Contracts

- Contract data structures exist.
- Deal terms exist.
- Sign-up bonus, royalty rate, number of records, and contract years are represented.
- Active contracts and expired contracts are stored.
- Contract widgets can display signed contract data.

Primary files:

- `Source/MusicManager/Public/FArtistContract.h`
- `Source/MusicManager/Public/FArtistDealTerms.h`
- `Source/MusicManager/Public/ContractWidget.h`
- `Source/MusicManager/Private/ContractWidget.cpp`

### Songs

- Song data structure exists.
- Song registry subsystem exists.
- Songs can be loaded from a DataTable.
- Songs can be created for artists.
- Songs can be queried by song id.
- Songs can be queried by artist.
- Eligible songs for recording can be queried.
- Songs can be locked for recording.
- Songs can be unlocked.
- Songs can be marked as recorded.
- Songs can be serialized/deserialized for save games.

Primary files:

- `Source/MusicManager/Public/FSongData.h`
- `Source/MusicManager/Public/Song.h`
- `Source/MusicManager/Private/Song.cpp`
- `Source/MusicManager/Public/SongManagerSubsystem.h`
- `Source/MusicManager/Private/SongManagerSubsystem.cpp`

### Recording And Records

- Record data structures exist.
- Record format enum exists: vinyl, cassette, CD, digital download, streaming.
- Recording intent structure exists.
- Record lifecycle states exist.
- Record lifecycle now distinguishes draft, recording, recorded, scheduled, released, and catalog states.
- Recording intents can be submitted.
- Recording can be started through a typed command dispatcher command.
- Recording intent validation exists.
- Singles require exactly one track.
- LPs require multiple tracks.
- Duplicate songs are rejected.
- Unsigned artists cannot record.
- Songs must belong to the selected artist.
- Already released songs cannot be recorded again.
- Songs are locked during recording.
- Record creation exists.
- Format eligibility is filtered by era.
- Record quality is derived from song quality.
- Release date resolution exists.
- Recorded music can remain unreleased until explicitly scheduled or released now.
- Release scheduling validates record ownership, date, format era support, and target regions.
- Regional release targeting exists on record data.
- Sales simulation ignores unreleased records.
- Scheduled records automatically become released when their release date is reached during monthly processing.
- Record sales are evaluated only in the record's selected target regions.
- Release planner view-model data exists for plannable records, available regions, valid formats, projected reach, and validation warnings.
- Monthly sales simulation exists.
- Lifetime units are tracked.
- Sales history can be queried.

Primary files:

- `Source/MusicManager/Public/RecordManagerSubsystem.h`
- `Source/MusicManager/Private/RecordManagerSubsystem.cpp`

### Market Simulation

- Region data structure exists.
- Market segment profile structure exists.
- Market demand snapshots exist.
- Region DataTable loading exists.
- Market segment profile DataTable loading exists.
- Regions can be queried.
- Region segments can be assigned.
- All regions can be queried.
- Demand snapshots can be generated.
- Runtime artist exposure per region exists.
- Runtime record exposure per region exists.
- Record exposure can be added and queried through a clean market subsystem API.
- Record-specific exposure is included in demand snapshots and affects record sales demand.
- Temporary monthly radio play simulation exists.

Primary files:

- `Source/MusicManager/Public/MarketRegion.h`
- `Source/MusicManager/Public/MarketManagerSubsystem.h`
- `Source/MusicManager/Private/MarketManagerSubsystem.cpp`

### Charts

- Chart data structures exist for chart definitions, ranked entries, weekly snapshots, and per-record chart history.
- `UChartManagerSubsystem` exists as the dedicated chart owner.
- Global records, singles, albums, format-specific, and dynamic genre chart definitions are generated from real record data.
- Regional records, singles, and albums chart definitions are generated from loaded market regions.
- Weekly chart calculation runs during the `ChartCalculation` weekly simulation phase.
- Multi-week time advancement creates ordered weekly chart snapshots because each weekly phase is processed in sequence.
- Chart rows are generated only from real record sales history.
- Regional charts filter sales by market/region id.
- Format-specific charts filter by real record format support and sales format.
- Genre charts filter by real record primary genre.
- Weekly charts now allocate monthly sales-history buckets into weekly units by calendar-day overlap instead of requiring fake weekly sales rows.
- Streaming sales are converted into stream-equivalent units before chart points are calculated.
- Deterministic chart formula profiles exist for era default, physical sales, singles velocity, album longevity, genre specialist, format-weighted, and streaming-era scoring.
- Chart snapshots track current rank, previous rank, peak rank, weeks on chart, chart points, units, and stream-equivalent units.
- Per-record chart history is maintained.
- Duplicate chart snapshots are prevented by chart/week keys.
- Chart milestone news is generated for first entry, Top 40, Top 10, number one, and new peak outcomes.
- Chart milestone news is deduplicated by chart id, record id, milestone type, and week.
- Chart save/load support exists for definitions, weekly snapshots, per-record history, current chart keys, processed chart weeks, and processed milestone keys.
- Chart save validation checks chart ids, regional references, record type/formula/format filters, duplicate chart/week snapshots, rank validity, record references, artist references, point values, unit values, and processed keys.
- Chart view models exist for chart list/detail and record chart history.
- Chart dashboard/status view model exists for current #1, top player-owned charting release, biggest player-owned movement, recent milestones, and no-data status.
- OpenAI Image 2 reference-image workflow has been executed for the chart GUI.
- Chart GUI reference image and workflow notes exist.
- Blueprintable C++ `UChartsWidget` backing class exists for chart list, dashboard/status, selected chart, record history, empty/error states, and reference image path.
- Final chart UMG `.uasset` screen still needs editor assembly and pixel-match comparison against the reference image.
- Automated chart regression tests exist for weekly sales allocation, definition matching, formula profiles, and dashboard view projection.

Primary files:

- `Source/MusicManager/Public/ChartManagerSubsystem.h`
- `Source/MusicManager/Private/ChartManagerSubsystem.cpp`
- `Source/MusicManager/Public/UI/ChartsWidget.h`
- `Source/MusicManager/Private/UI/ChartsWidget.cpp`
- `Source/MusicManager/Private/Tests/ChartTests.cpp`
- `docs/design/references/charts_reference.png`
- `docs/design/references/charts_reference_workflow.md`
- `Source/MusicManager/Public/MusicSaveGame.h`
- `Source/MusicManager/Private/MusicSaveSubsystem.cpp`
- `Source/MusicManager/Public/EventSubsystem.h`
- `Source/MusicManager/Private/EventSubsystem.cpp`
- `Source/MusicManager/Public/GameTimeSubsystem.h`
- `Source/MusicManager/Private/GameTimeSubsystem.cpp`
- `Source/MusicManager/Public/RecordManagerSubsystem.h`
- `Source/MusicManager/Private/RecordManagerSubsystem.cpp`

### Finance

- Label account data exists.
- Cash flow ledger entries exist.
- Transaction types exist.
- Transactions can be registered.
- Label balance can be queried.
- Label ledger can be queried.
- Last month profit can be queried.
- Monthly finance summaries exist.
- Accumulated cash can be queried.
- Record sales revenue registration exists.
- Record sales entries can be processed into finance transactions.
- Marketing campaign spend is booked as `MarketingCost` ledger transactions when a campaign launches.

Primary files:

- `Source/MusicManager/Public/FinanceManagerSubsystem.h`
- `Source/MusicManager/Private/FinanceManagerSubsystem.cpp`

### News And Events

- Music news event data exists.
- Monthly news summary data exists.
- News event generation broadcasts exist.
- Month-advanced and month-closed handlers exist.
- Batch time advancement handling exists.
- Processed news keys avoid duplicate news triggers.
- Layout registration exists so news can be routed to the UI.

Primary files:

- `Source/MusicManager/Public/EventSubsystem.h`
- `Source/MusicManager/Private/EventSubsystem.cpp`

### Audition Flow

- Audition event data exists.
- Audition event actor exists.
- Audition can start from the current unsigned artist.
- Music player component registration is attempted.
- Performance-finished handling exists.
- Deal finalization path exists.
- Audition widget can present artist data and contract sliders.
- Sign/pass events are broadcast by the widget.

Primary files:

- `Source/MusicManager/Public/AuditionTypes.h`
- `Source/MusicManager/Public/AuditionEventActor.h`
- `Source/MusicManager/Private/AuditionEventActor.cpp`
- `Source/MusicManager/Public/AuditionWidget.h`
- `Source/MusicManager/Private/AuditionWidget.cpp`

### Audio Playback

- Music player component exists.
- Audio playback/stop behavior exists.
- Performance finished delegate exists.
- UI manager can register and stop the active music player.

Primary files:

- `Source/MusicManager/Public/MusicPlayerComponent.h`
- `Source/MusicManager/Private/MusicPlayerComponent.cpp`

### Save And Load

- Save subsystem exists.
- Save game object exists.
- Save files now include an explicit save schema version. The current schema version is 5.
- Campaign metadata is stored with slot name, save timestamps, and an optional thumbnail asset reference for save/load UI.
- Supported older saves migrate sequentially to the current schema before validation instead of relying only on version acceptance/rejection.
- Legacy v1 save fields can migrate into the explicit time snapshot, artist contract snapshot, player label snapshot, and finance account snapshot when present.
- A structured save validation result exists with validation errors and warnings.
- A player label snapshot is stored with stable label id, display name, founded date, and reputation.
- Time is stored through an explicit time snapshot with current date and simulation-end state.
- Songs can be saved with song id, artist id, song data, recording lock state, and record linkage.
- Artist manager state can be saved with unsigned artists, active contracts, expired contracts, selected artist, artist-to-song mapping, artist momentum, and artist reputation.
- Records can be saved with record data, lifecycle state, sales history, and lifetime unit totals.
- Finance state can be saved with label accounts, ledgers, monthly summaries, and closed monthly summary keys.
- Market exposure can be saved for regional artist and record exposure.
- Marketing state can be saved with campaigns, generated exposure history, and applied campaign-month keys.
- News state can be saved with monthly news summaries, processed news keys, and closed monthly news keys.
- Load validation checks top-level save version, required subsystems, snapshot validity, duplicate ids, broken record/song/artist/label references, finance consistency, market exposure references, marketing campaign references, news summary keys, and reserved future-system snapshots before applying state.
- Load application is two-phase: validate first, then apply snapshots in dependency order.
- Failed validation aborts load before live subsystem state is mutated.
- UI rebuild after load exists through the UI manager.
- A save slot registry exists for real save/load UI binding.
- Save slot descriptors expose slot name, display label, player label name, in-game date, created/last-saved timestamps, save version, autosave/backup flags, parent slot, thumbnail asset path, loadability, and validation messages.
- Manual saves can store a thumbnail asset reference.
- Autosave writes to a deterministic autosave slot.
- Manual slot overwrites create timestamped backup save slots before replacing the main slot.
- Save slot listing revalidates registered saves and reports unreadable or invalid slots without mock data.
- Save slot deletion removes both the save file and registry entry.
- Reserved future-system persistence buckets exist for tours, awards, critic reviews, rival labels, staff, unlocks, and optional settings/UI state; they are empty until owning gameplay systems exist and validate any populated entries.
- Persistence automation tests exist for legacy migration, future-version rejection, slot descriptors, future snapshot validation, duplicate song validation, and broken record/song reference validation.
- `MusicManager.Persistence` automation tests pass through `UnrealEditor-Cmd`.

Current save coverage now includes the existing implemented management loop: time, player label, songs, artists/contracts, records, record sales history/lifetime units, finance accounts/ledger/monthly summaries, market exposure, charts, and news summaries/processed keys.

Current save coverage now also includes release targeting fields on records, marketing campaign state, chart weekly snapshots, chart history, and chart milestone keys.

Remaining persistence gaps include actual owning gameplay subsystems and UI widgets for future systems that do not exist yet, such as tours, rivals, critics/awards, staff progression, unlock-driven progression, and a production save/load screen. Existing old v1 save fields are retained only as legacy compatibility fields.

Primary files:

- `Source/MusicManager/Public/MusicSaveSubsystem.h`
- `Source/MusicManager/Private/MusicSaveSubsystem.cpp`
- `Source/MusicManager/Public/MusicSaveGame.h`
- `Source/MusicManager/Private/MusicSaveGame.cpp`
- `Source/MusicManager/Private/Tests/PersistenceTests.cpp`
- `Source/MusicManager/Public/PlayerLabelSubsystem.h`
- `Source/MusicManager/Private/PlayerLabelSubsystem.cpp`
- `Source/MusicManager/Public/GameTimeSubsystem.h`
- `Source/MusicManager/Private/GameTimeSubsystem.cpp`
- `Source/MusicManager/Public/SongManagerSubsystem.h`
- `Source/MusicManager/Private/SongManagerSubsystem.cpp`
- `Source/MusicManager/Public/ArtistManagerSubsystem.h`
- `Source/MusicManager/Private/ArtistManagerSubsystem.cpp`
- `Source/MusicManager/Public/RecordManagerSubsystem.h`
- `Source/MusicManager/Private/RecordManagerSubsystem.cpp`
- `Source/MusicManager/Public/FinanceManagerSubsystem.h`
- `Source/MusicManager/Private/FinanceManagerSubsystem.cpp`
- `Source/MusicManager/Public/MarketManagerSubsystem.h`
- `Source/MusicManager/Private/MarketManagerSubsystem.cpp`
- `Source/MusicManager/Public/MarketingManagerSubsystem.h`
- `Source/MusicManager/Private/MarketingManagerSubsystem.cpp`
- `Source/MusicManager/Public/EventSubsystem.h`
- `Source/MusicManager/Private/EventSubsystem.cpp`

### Release Planning

- A typed `FScheduleReleaseCommand` exists.
- `URecordManagerSubsystem::ScheduleRelease` validates ownership, lifecycle state, release date, format validity, duplicate regions, and region existence.
- `URecordManagerSubsystem::ReleaseNow` exists as an explicit immediate-release path.
- Records awaiting release planning can be queried.
- Scheduled releases can be queried.
- Released/catalog records can be queried.
- Release planner view-model data exists for C++/Blueprint UI binding.
- Release planner view models now include human-readable record and artist display names.
- Release dashboard summaries can be queried for awaiting-planning, scheduled, and recently released records.
- A production release planner reference image was generated with OpenAI Image 2 and saved in the repo.
- `UReleasePlannerWidget` exists as a Blueprintable UMG backing class for the release planner screen.
- `UReleasePlannerWidget` refreshes real planner data, selects records, exposes empty/error/disabled state, exposes the reference image path, and schedules releases through `UCommandDispatcherSubsystem`.
- Release scheduling now emits readable news payloads with record/artist names while preserving stable ids in metadata.
- Final release planner `.uasset` assembly and pixel-match comparison inside the Unreal Editor remain to be completed.
- Release/marketing automation tests cover release snapshot validation, dashboard summaries, and readable release news payloads.

Primary files:

- `Source/MusicManager/Public/RecordManagerSubsystem.h`
- `Source/MusicManager/Private/RecordManagerSubsystem.cpp`
- `Source/MusicManager/Public/UI/ReleasePlannerWidget.h`
- `Source/MusicManager/Private/UI/ReleasePlannerWidget.cpp`
- `docs/design/references/release_planner_reference.png`

### Marketing

- `UMarketingManagerSubsystem` exists.
- Marketing channels exist for radio, press, television, posters, social, and playlisting.
- Channel era rules exist and are validated against campaign dates.
- A typed `FLaunchMarketingCampaignCommand` exists.
- Marketing campaign launch validates record marketability, label ownership, target regions, channel availability, campaign dates, positive budget, channel minimum budget, duplicate selections, and affordability.
- Successful marketing launch records exactly one `MarketingCost` finance ledger entry.
- Marketing campaigns are persisted with stable campaign ids.
- Active campaigns resolve monthly into regional record exposure through `UMarketManagerSubsystem`.
- Campaign-month keys prevent duplicate exposure application after save/load or repeated month processing.
- Marketing planner view-model data exists for C++/Blueprint UI binding.
- Marketing planner view models now include human-readable record/artist display names, active campaign ROI summaries, and a forecast summary.
- Campaign ROI summaries estimate spend, generated exposure, unit lift, gross revenue, and ROI from real campaign state.
- Marketing dashboard summaries can be queried for active and completed campaigns.
- A production marketing planner reference image was generated with OpenAI Image 2 and saved in the repo.
- `UMarketingPlannerWidget` exists as a Blueprintable UMG backing class for the marketing planner screen.
- `UMarketingPlannerWidget` refreshes real marketable records, selects records, exposes empty/error/disabled state, exposes the reference image path, and launches campaigns through `UCommandDispatcherSubsystem`.
- Marketing campaign launch now emits readable news payloads with record/artist names and budget while preserving stable ids in metadata.
- Final marketing planner `.uasset` assembly and pixel-match comparison inside the Unreal Editor remain to be completed.
- Release/marketing automation tests cover marketing snapshot validation, campaign ROI summaries, and readable marketing news payloads.

Primary files:

- `Source/MusicManager/Public/MarketingManagerSubsystem.h`
- `Source/MusicManager/Private/MarketingManagerSubsystem.cpp`
- `Source/MusicManager/Public/UI/MarketingPlannerWidget.h`
- `Source/MusicManager/Private/UI/MarketingPlannerWidget.cpp`
- `Source/MusicManager/Public/MarketManagerSubsystem.h`
- `Source/MusicManager/Private/MarketManagerSubsystem.cpp`
- `Source/MusicManager/Public/FinanceManagerSubsystem.h`
- `Source/MusicManager/Private/FinanceManagerSubsystem.cpp`
- `docs/design/references/marketing_planner_reference.png`

### Player Controller And Pawn

- Custom game mode exists.
- Custom player controller exists.
- Custom pawn exists.
- Player controller creates a layout widget at BeginPlay.
- Mouse cursor, click events, and hover events are enabled.
- Input mode is set to game and UI.
- Camera pawn has a spring arm and camera.
- WASD movement is implemented using Enhanced Input.

Primary files:

- `Source/MusicManager/MusicManagerGameMode.h`
- `Source/MusicManager/MusicManagerGameMode.cpp`
- `Source/MusicManager/Public/MusicManagerPlayerController.h`
- `Source/MusicManager/Private/MusicManagerPlayerController.cpp`
- `Source/MusicManager/MusicManagerPawn.h`
- `Source/MusicManager/MusicManagerPawn.cpp`

## Current GUI Functionality

### UI Manager

- Active layout registration exists.
- News routing exists.
- Pending news buffering exists before layout registration.
- Audition screen routing exists.
- Region map routing exists.
- Contract screen routing exists.
- Record/studio screen routing exists.
- Signed artist panel refresh exists.
- Batch UI update suppression exists during fast-forward.
- Status widget registration exists.
- Current label id tracking exists.
- News card selection event exists.
- Layer 3 screen routing exists.
- Main canvas state routing exists.
- Hover tooltip routing exists.
- Artist hover detail routing exists.
- Selected entity routing to inspector exists.

Primary files:

- `Source/MusicManager/Public/UIManagerSubsystem.h`
- `Source/MusicManager/Private/UIManagerSubsystem.cpp`

### Layout

- Main layout backing class exists.
- Layout registers with the UI manager on construct.
- News cards can be added and removed from the feed.
- Audition widget can be shown/hidden.
- Record widget can be shown/hidden.
- Region map can be shown.
- Contract widget can be shown.
- Signed artist panel can be refreshed.
- Hover tooltip can be shown/hidden.
- Artist hover detail can be shown/hidden.
- Layer 2 can be enabled/disabled.
- Artist selection routes into `ArtistManagerSubsystem` and the UI inspector path.
- Layout now owns a top HUD status bar slot. It binds an existing `TopStatusBarWidget` child when present, or creates `/Game/GUI/HUD/TopStatusBarBP` at runtime and attaches it to the top layer.
- The top status bar displays real date, player label name, cash balance, reputation, pause/play state, and command-driven fast-forward controls.
- `TopStatusBarBP` now contains a Blueprint-owned designer hierarchy instead of relying on C++ runtime construction for the main visual tree.
- `TopStatusBarBP` includes the required inherited/bound Blueprint fields for root panels, generated background art, status icons, text blocks, and command buttons.
- The top status bar Blueprint uses imported generated image assets for its background, brand icon, date icon, label icon, cash icon, reputation icon, pause icon, play icon, fast-forward icon, and menu icon.
- The top status bar was regenerated from a dedicated AAA reference image after the earlier absolute-positioned version failed visually at runtime.
- The rebuilt `TopStatusBarBP` uses row-based UMG layout containers with a flexible spacer and fixed-height status pills instead of hardcoded absolute x coordinates.
- The top status bar parent layout slot now uses a 72px height to match the widget's designed height and avoid clipping.
- The top status bar components are positioned to follow `docs/design/references/top_status_bar_reference.png`; final pixel-level tuning should still be done by visual comparison in the Unreal Designer viewport.
- A production left-side artist audition panel now exists as `/Game/GUI/Audition/ArtistAuditionPanelBP`, inheriting `UAuditionWidget`.
- `ULayout` now creates and routes the new audition panel at runtime, keeps it collapsed until an audition is shown, and uses the existing audition sign/pass command flow.
- The audition panel uses generated panel, vinyl frame, portrait fallback, contract box, button, and icon assets from `Content/GUI/Audition`.
- `UAuditionWidget` now refreshes optional portrait/stat-meter bindings, formats deal values as currency/percent/years, and configures contract sliders with production ranges.
- The legacy `Content/GUI/AuditionBP.uasset` remains in the repository, but the new HUD path prefers `ArtistAuditionPanelBP`; final pixel-level tuning should still be checked in the Unreal Designer viewport against `docs/design/references/artist_audition_left_panel_reference.png`.

Primary files:

- `Source/MusicManager/Public/Layout.h`
- `Source/MusicManager/Private/Layout.cpp`
- `Source/MusicManager/Public/AuditionWidget.h`
- `Source/MusicManager/Private/AuditionWidget.cpp`
- `Source/MusicManager/Public/UI/TopStatusBarWidget.h`
- `Source/MusicManager/Private/UI/TopStatusBarWidget.cpp`
- `Source/MusicManager/Public/UI/MusicManagerWidgetBlueprintTools.h`
- `Source/MusicManager/Private/UI/MusicManagerWidgetBlueprintTools.cpp`
- `Content/GUI/HUD/TopStatusBarBP.uasset`
- `Content/GUI/Audition/ArtistAuditionPanelBP.uasset`

### Existing GUI Blueprint Assets

The repository contains these GUI assets under `Content/GUI`:

- `Content/GUI/AuditionBP.uasset`
- `Content/GUI/CommandItemWidgetBP.uasset`
- `Content/GUI/CommandPanelBP.uasset`
- `Content/GUI/CommandPanelL.uasset`
- `Content/GUI/ContractGUIBP.uasset`
- `Content/GUI/DateWidgetBP.uasset`
- `Content/GUI/LayoutBP.uasset`
- `Content/GUI/LayoutBP1.uasset`
- `Content/GUI/NewFeedListBP.uasset`
- `Content/GUI/NewsCardBP.uasset`
- `Content/GUI/NewsFeedItemBP.uasset`
- `Content/GUI/RecordingGUIBP.uasset`
- `Content/GUI/RecordingRecordListBP.uasset`
- `Content/GUI/RecordSongListBP.uasset`
- `Content/GUI/RegionButtonBP.uasset`
- `Content/GUI/USMap.uasset`
- `Content/GUI/Artist/ArtistHoveredBP.uasset`
- `Content/GUI/Artist/SignedArtistItem.uasset`
- `Content/GUI/Artist/SignedArtistPanel.uasset`
- `Content/GUI/Artist/StatusWidgetBP.uasset`
- `Content/GUI/HUD/TopStatusBarBP.uasset`
- `Content/GUI/Audition/ArtistAuditionPanelBP.uasset`
- `Content/GUI/RegionMap/RegionMapBP.uasset`

### Existing GUI Image And Icon Assets

The repository contains these GUI image/icon assets:

- `Content/GUI/Bild3.png`
- `Content/GUI/Bild3.uasset`
- `Content/GUI/Panel Gray.png`
- `Content/GUI/Panel_Gray.uasset`
- `Content/GUI/Icons/ArtistIcon.png`
- `Content/GUI/Icons/Artists.uasset`
- `Content/GUI/Icons/Charts.png`
- `Content/GUI/Icons/Charts.uasset`
- `Content/GUI/Icons/Contract.png`
- `Content/GUI/Icons/Contracts.uasset`
- `Content/GUI/Icons/Financials.png`
- `Content/GUI/Icons/Financials.uasset`
- `Content/GUI/Icons/GenresIcon.png`
- `Content/GUI/Icons/GenreTree.png`
- `Content/GUI/Icons/GenreTree.uasset`
- `Content/GUI/Icons/Market.png`
- `Content/GUI/Icons/Market.uasset`
- `Content/GUI/Icons/Marketing.png`
- `Content/GUI/Icons/Marketing.uasset`
- `Content/GUI/Icons/Radio.png`
- `Content/GUI/Icons/Radio.uasset`
- `Content/GUI/Icons/Record.png`
- `Content/GUI/Icons/Record.uasset`
- `Content/GUI/Icons/Studio.uasset`
- `Content/GUI/Icons/StudioIcon.png`
- `Content/GUI/Icons/Tours.png`
- `Content/GUI/Icons/Tours.uasset`
- `Content/GUI/HUD/TopStatusBarFrame.png`
- `Content/GUI/HUD/TopStatusBarFrame.uasset`
- `Content/GUI/HUD/TopStatusBarSurface.png`
- `Content/GUI/HUD/TopStatusBarSurface.uasset`
- `Content/GUI/HUD/TopStatusBarIconsSheet.png`
- `Content/GUI/HUD/TopStatusBarIconsSheet.uasset`
- `Content/GUI/HUD/TopStatusIcon_BrandRecord.png`
- `Content/GUI/HUD/TopStatusIcon_BrandRecord.uasset`
- `Content/GUI/HUD/TopStatusIcon_DateCalendar.png`
- `Content/GUI/HUD/TopStatusIcon_DateCalendar.uasset`
- `Content/GUI/HUD/TopStatusIcon_LabelPerson.png`
- `Content/GUI/HUD/TopStatusIcon_LabelPerson.uasset`
- `Content/GUI/HUD/TopStatusIcon_CashDollar.png`
- `Content/GUI/HUD/TopStatusIcon_CashDollar.uasset`
- `Content/GUI/HUD/TopStatusIcon_ReputationStar.png`
- `Content/GUI/HUD/TopStatusIcon_ReputationStar.uasset`
- `Content/GUI/HUD/TopStatusIcon_Pause.png`
- `Content/GUI/HUD/TopStatusIcon_Pause.uasset`
- `Content/GUI/HUD/TopStatusIcon_Play.png`
- `Content/GUI/HUD/TopStatusIcon_Play.uasset`
- `Content/GUI/HUD/TopStatusIcon_FastForward.png`
- `Content/GUI/HUD/TopStatusIcon_FastForward.uasset`
- `Content/GUI/HUD/TopStatusIcon_Menu.png`
- `Content/GUI/HUD/TopStatusIcon_Menu.uasset`

### Widget Backing Classes

Current C++ widget classes include:

- `UAuditionWidget`
- `UContractWidget`
- `UEventTickerWidget`
- `UNewsFeedItemWidget`
- `UNewsFeedList`
- `URecordWidget`
- `URecordSongListItemWidget`
- `URegionMapWidget`
- `URegionMapButton`
- `USignedArtistPanelWidget`
- `USignedArtistItemWidget`
- `UStatusWidget`
- `UTopStatusBarWidget`
- `UCommandPanelWidget`
- `UCommandItemWidget`
- `UArtistHoverDetailWidget`
- `UHoverTooltipManagerWidget`
- `UInspectorPanelWidget`
- `UMainCanvasHost`

## Current Content And Data Assets

### Data Assets

The repository contains these game data assets and source data files:

- `Content/Data/ArtistData.uasset`
- `Content/Data/ArtistIcons.uasset`
- `Content/Data/FArtistData.csv`
- `Content/Data/FArtistData.xlsx`
- `Content/Data/FArtistData2.csv`
- `Content/Data/Market Segments.csv`
- `Content/Data/MarketSegmentData.uasset`
- `Content/Data/RegionData.uasset`
- `Content/Data/SongData.uasset`
- `Content/Data/Tes4.csv`
- `Content/Data/Test.csv`
- `Content/Data/Test3.csv`
- `Content/Data/US_MarketRegions.csv`

### Blueprint/Scripting Assets

The repository contains these scripting assets:

- `Content/Scripting/AuditionEventBP.uasset`
- `Content/Scripting/GameInstanceBP.uasset`
- `Content/Scripting/MMBPLib.uasset`
- `Content/Scripting/MusicManagerGameModeBP.uasset`
- `Content/Scripting/MusicManagerPawnBP.uasset`
- `Content/Scripting/MusicManagerPlayerControllerBP.uasset`

### Maps And Scene Content

- Default game map is configured as `/Game/Start.Start`.
- Editor startup map is configured as `/Game/Start.Start`.
- `Content/Start.umap` exists.
- Office/furniture content exists.
- Concert stage content exists.
- Music prop content exists.
- Performer animation content exists.
- Character content exists.

## Web/Server Editor

- A local-first MusicManager web editor now exists under `EditorServer`.
- The editor runs without external package installation using Node's built-in HTTP server and static frontend files.
- `run-editor.ps1` starts the editor at `http://127.0.0.1:5177`.
- Editor architecture and workspace ownership are documented in `docs/editor_architecture.md`.
- Editor configuration is project-local in `EditorServer/config.json`.
- Editor writes are restricted to configured editor data roots, currently `EditorData`.
- Versioned editor schemas exist for artists, songs, regions, market segments, eras, format rules, marketing channels, chart formulas, GUI references, save slot summaries/scenarios.
- The validation engine checks required fields, duplicate ids, numeric ranges, year ranges, cross references, and project/file asset references.
- The editor data store uses project-local JSON records in `EditorData`.
- Existing project CSV data has been imported into the editor store:
  - 10 artist records from `Content/Data/FArtistData.csv`
  - 50 region records from `Content/Data/US_MarketRegions.csv`
  - 5 segment records from `Content/Data/Market Segments.csv`
- `SongData.uasset` import/export is wired through an Unreal Python bridge launched by the editor server, so the web editor reads and writes the real Unreal DataTable through Unreal APIs rather than binary-patching `.uasset` files.
- 1 song record has been imported from `Content/Data/SongData.uasset` into `EditorData/songs`:
  - `song_newrow` / `Jumpin Jive Shoes`
- Import/export APIs exist for editor records, with deterministic JSON manifests written under `EditorData/exports`.
- A production OpenAI Image 2 editor dashboard reference image exists at `docs/design/references/editor_dashboard_reference.png`.
- Editor dashboard reference workflow notes exist at `docs/design/references/editor_dashboard_reference_workflow.md`.
- The web dashboard uses real API data for validation status, record counts, save file counts, GUI reference counts, write roots, exports, and recent audit records.
- Artist and song editor screens exist with selectable record rows, validated update-detail forms, and separate create-new actions.
- Market editor screens exist for regions and market segments with selectable rows, per-entry update-detail forms, and separate create-new actions.
- Rules editor screens exist for eras, format rules, marketing channels, and chart formulas with selectable rows, update-detail forms, and separate create-new actions.
- Release/marketing balance lab exists and uses real editor records for artist, region, and marketing channel references; invalid references fail validation.
- Chart formula lab exists and ranks caller-provided real sales-history rows without generating fake chart rows.
- Read-only save slot/file inspector exists; Unreal binary `.sav` files are listed as read-only and are not mutated or decoded by the Node editor.
- GUI reference manager indexes real reference images from `docs/design/references`.
- Editor-side audit records are written for successful imports, saves, and exports under `EditorData/audit`.
- Automated Node tests cover write safety, CSV import, validation failures, deterministic export, release/marketing preview, and chart preview.
- Live server smoke test passed against `/api/v1/dashboard`.

Primary files:

- `EditorServer/server.js`
- `EditorServer/lib/editorCore.js`
- `EditorServer/public/index.html`
- `EditorServer/public/styles.css`
- `EditorServer/public/app.js`
- `EditorServer/schemas/editor.schema.json`
- `EditorServer/config.json`
- `EditorServer/test/editorCore.test.js`
- `EditorServer/package.json`
- `EditorData/...`
- `run-editor.ps1`
- `docs/editor_architecture.md`
- `docs/design/references/editor_dashboard_reference.png`
- `docs/design/references/editor_dashboard_reference_workflow.md`

Remaining editor limitations:

- Save inspection is read-only metadata/file inspection; binary Unreal save objects are not decoded outside Unreal.
- The web editor does not edit `.uasset` assets directly.
- Era, format, marketing channel, and chart formula stores are implemented but currently empty until authored or imported from future source data.
- Song records are no longer empty in the editor store; the current imported song count is 1 from `SongData.uasset`.
- Balance labs mirror production formulas only at a deterministic editor-analysis level; full parity should eventually come from shared generated rules or Unreal-side validation/export tooling.
- GUI reference tracking indexes existing references but does not yet perform automated pixel comparison.

## Currently Reachable GUI Commands

The C++ command routing currently handles:

- `Contracts`: shows the selected artist's active contract if one exists.
- `Studio`: shows the recording GUI for the selected artist.
- `Market`: shows the region map.

Unsupported command panel categories are no longer included in the default C++ command list.

## Known Gaps Against Professional Target

- No complete campaign setup flow is implemented.
- No professional main menu/load/save campaign GUI was identified in C++.
- Save/load now covers the currently implemented management loop, but does not yet cover future systems that are not implemented, such as tours, rivals, critics/awards, staff progression, unlocks, and optional UI state.
- Chart manager subsystem, chart persistence, chart dashboard projection, chart GUI reference workflow/backing class, and chart regression tests are implemented; the final chart UMG `.uasset` screen still needs editor assembly and pixel-match comparison.
- No critic/review subsystem is implemented.
- No awards subsystem is implemented.
- No tour manager subsystem is implemented.
- No rival label subsystem is implemented.
- No staff/office progression system is implemented.
- Command dispatcher coverage exists for sign artist, reject artist, start recording, schedule release, launch marketing, and advance time. Remaining future systems still need dispatcher commands as they are implemented.
- Command dispatcher automation tests exist for the currently implemented command result/event/notification surface.
- Release planner has a generated production reference image and Blueprintable C++ backing class, but the final UMG `.uasset` screen still needs editor assembly and pixel-match comparison.
- Marketing planner has a generated production reference image and Blueprintable C++ backing class, but the final UMG `.uasset` screen still needs editor assembly and pixel-match comparison.
- No music video planner GUI/system is implemented.
- No finance dashboard GUI beyond status/ledger access patterns was identified.
- Chart GUI backing view models and `UChartsWidget` exist, but no final chart UMG `.uasset` screen is implemented.
- No reviews/critics GUI was identified.
- No awards GUI was identified.
- No rival labels GUI was identified.
- No staff/office GUI was identified.
- No complete tutorial/onboarding system was identified.
- Production-grade OpenAI Image 2 reference-image workflow has been applied to the release planner, marketing planner, charts, and main gameplay HUD targets; other GUI areas still need the same workflow.
- A production OpenAI Image 2 AAA reference image now exists for the main gameplay HUD, covering the top status strip, left artist summary rail, right news feed, and bottom command dock visual target. The top status bar C++/Blueprint widget is implemented and runtime-wired; the left rail, right news-feed restyle, bottom dock restyle, and final pixel-match icon/texture polish remain.
- The bottom command dock has been rebuilt from the production GUI workflow with a saved AAA reference image, generated dock/button/icon PNG assets, imported UI textures, and rebuilt `CommandPanelBP`/`CommandItemWidgetBP` widget trees. The dock now exposes five command buttons: Audition, Market, Contracts, Studio, and Charts. Audition, Market, Contracts, and Studio route through `UUIManagerSubsystem`; Charts is intentionally logged as pending until the final chart UMG screen is assembled and wired. Primary files/assets: `Source/MusicManager/Public/UI/CommandPanelWidget.h`, `Source/MusicManager/Private/UI/CommandPanelWidget.cpp`, `Source/MusicManager/Public/UI/CommandItemWidget.h`, `Source/MusicManager/Private/UI/CommandItemWidget.cpp`, `Source/MusicManager/Private/UIManagerSubsystem.cpp`, `Source/MusicManager/Private/UI/MusicManagerWidgetBlueprintTools.cpp`, `Scripts/RebuildBottomCommandDock.py`, `Content/GUI/CommandPanelBP.uasset`, `Content/GUI/CommandItemWidgetBP.uasset`, `Content/GUI/HUD/CommandDock/*`, and `docs/design/references/bottom_command_dock_reference.png`.
- The bottom command dock Market and Contracts icons have been recropped from the generated atlas using clean component bounds, centered into transparent 128x128 icon canvases, and reimported into Unreal. This removes the adjacent-icon slivers that were visible on the left edge of both icons. Updated assets: `Content/GUI/HUD/CommandDock/BottomCommandDockIcon_Market.png`, `Content/GUI/HUD/CommandDock/BottomCommandDockIcon_Market.uasset`, `Content/GUI/HUD/CommandDock/BottomCommandDockIcon_Contracts.png`, and `Content/GUI/HUD/CommandDock/BottomCommandDockIcon_Contracts.uasset`.
- The artist audition panel has been regenerated from the production GUI workflow with a saved reference image, generated portrait/vinyl/icon/button/slider/meter assets, a scrollable Blueprint widget layout, and custom C++ rendered controls for segmented stat meters and gold sliders. The AuditionBP compiler collision for `PerformanceMeter`, `StagePresenceMeter`, `AudienceEngagementMeter`, `VocalQualityMeter`, and `SongwritingQualityMeter` is fixed by removing those custom meters from the inherited native property contract and resolving them by widget name at refresh time. Native text/button/slider bindings remain real Blueprint variables. Primary files/assets: `Source/MusicManager/Public/AuditionWidget.h`, `Source/MusicManager/Private/AuditionWidget.cpp`, `Source/MusicManager/Public/UI/MusicSegmentedMeterWidget.h`, `Source/MusicManager/Private/UI/MusicSegmentedMeterWidget.cpp`, `Source/MusicManager/Public/UI/MusicGoldSlider.h`, `Source/MusicManager/Private/UI/MusicGoldSlider.cpp`, `Source/MusicManager/Private/UI/MusicManagerWidgetBlueprintTools.cpp`, `Scripts/RebuildArtistAuditionPanel.py`, `Content/GUI/Audition/*`, and `docs/design/references/artist_audition_reference_v2.png`.
- The artist audition panel layout has been tightened so the full contract offer and bottom actions fit within the scrollable panel at runtime. The root cause was a mismatch between the generated scroll content height/section positions and the actual bottom-most controls, combined with an oversized vinyl/portrait block consuming too much vertical space. The generated Blueprint now uses a smaller square vinyl/portrait composition, smaller type, square icon slots to avoid distortion, earlier stat/contract sections, and explicit bottom padding in the scroll content. Rebuilt asset: `Content/GUI/Audition/ArtistAuditionPanelBP.uasset`.
- The artist audition Blueprint root is now a fixed visible shell with clipping, instead of letting the tall scroll content define the whole Widget Blueprint bounds. `RootSizeBox` is 520x720, `RootCanvas` clips to bounds, and `AuditionScrollBox` clips/takes ownership of the taller inner `AuditionContentSizeBox`. This prevents the Blueprint designer view from flowing below the visible screen edge while preserving vertical scrolling for the full contract offer.
- The artist audition Blueprint has been rebuilt again to keep the header outside the scrolling content, preventing the `ARTIST AUDITION` title from scrolling over the panel frame. The vinyl frame and default portrait PNGs now use alpha masking so the record outside area and portrait corners are transparent instead of black square cutouts. The contract offer area has distinct icons for bonus, records, royalty, and term rows, textured gold slider handles/tracks, lower button placement outside the offer surface, and fallback venue/city display text when the audition payload does not provide values. The generated widget tree now keeps legacy/current bonus icon widget names present as real widgets, fixing the stale `SliderSignUpBonusIcon`/`GiftIcon` Blueprint GUID commandlet failure.
- The contract offer sliders in the artist audition panel now use a custom `UMusicGoldSlider` UMG widget backed by a custom Slate `SLeafWidget`, instead of a skinned stock `USlider`. The widget paints its own dark/gold track, filled portion, tick marks, and circular gold knob texture, and handles mouse capture/value updates directly while preserving the `SetValue`, `GetValue`, min/max/step, and `OnValueChanged` API used by `UAuditionWidget`. `UAuditionWidget` now binds these controls as `UMusicGoldSlider*`, so the Blueprint cannot silently use the stock slider renderer for the contract controls.
- The artist audition portrait/vinyl composition has been tightened again: the portrait brush now uses a centered face crop and a smaller 132x132 draw box aligned to the vinyl center window, so tall artist portrait textures fit the record hole instead of showing mostly lower face/body. The custom contract slider knob is reduced from 34px to 26px while keeping the same slider range/value behavior. Rebuilt asset: `Content/GUI/Audition/ArtistAuditionPanelBP.uasset`.
- The artist audition decision buttons are now wired through the production command flow. `UAuditionWidget` continues to execute `SignArtist` and `RejectArtist` via `UCommandDispatcherSubsystem`; `ULayout` now listens for successful `OnSignArtist` and `OnPass` events and closes the audition panel after either decision. Audition opening paths now ensure the production audition panel exists before populating data, so bottom-dock audition and news-driven audition flows use the same widget instance. Artist rejection now has a validated `RejectArtistById` path that reports command failures cleanly instead of only broadcasting list changes. Successful artist signing now publishes an `ArtistSigned` `FMusicNewsEvent` through `UEventSubsystem::PublishNewsEvent`, including artist, label, genre, and deal-term metadata so the signed-artist story appears in the standard news feed/event pipeline. Deal royalties are consistently treated as fractional rates (`0.17` = 17%) for monthly contract financials. Primary files: `Source/MusicManager/Public/Layout.h`, `Source/MusicManager/Private/Layout.cpp`, `Source/MusicManager/Public/ArtistManagerSubsystem.h`, `Source/MusicManager/Private/ArtistManagerSubsystem.cpp`, `Source/MusicManager/Private/CommandDispatcherSubsystem.cpp`, and `Source/MusicManager/Public/FArtistDealTerms.h`.
- The studio recording flow has been upgraded from the prototype Single/LP immediate-recording path toward the production recording system. Records now have a first-class `ERecordType` (`Single`, `EP`, `LP`) while preserving legacy `bIsSingle`/`bIsLP` compatibility. `URecordManagerSubsystem` can build authoritative recording projections with estimated cost, duration, and completion date; active recording sessions now persist through `FRecordManagerSnapshot`; recording completion creates the real record later, marks/links selected songs, unlocks songs, and emits recording-complete news. `UCommandDispatcherSubsystem::ExecuteStartRecording` now validates the projection, checks label cash, books exactly one `RecordingCost` ledger transaction, starts the session, emits recording-start news, and returns the projected completion date/duration. `USongManagerSubsystem` can query real genre-compatible, unreleased, unlocked songs from the song database and can claim unowned catalog songs for the recording artist at session start. `URecordWidget` now supports Single/EP/LP selection, genre-based song population, projected cost/duration/warning display, selected-track count/duration, artist header fields, release overview fields, and the existing song preview route through `UMusicPlayerComponent`; `URecordSongListItemWidget` exposes real metadata/quality text bindings. Chart matching now uses `ERecordType`, with Singles charts matching singles and album charts accepting EP/LP album-like records. A studio recording OpenAI Image 2 reference and workflow note have been added at `docs/design/references/studio_recording_reference.png` and `docs/design/references/studio_recording_reference_workflow.md`. `RecordingGUIBP`, `RecordSongListBP`, and `RecordingRecordListBP` have now been rebuilt as designer-visible, reference-style Blueprint widget trees with a premium dark/gold AAA layout: artist/vinyl header, Single/EP/LP format cards, release overview, dense available/selected track tables, bottom production summary bar, warning surface, confirm/cancel controls, and compact row actions/metadata. The rebuild is automated by `UMusicManagerWidgetBlueprintTools::RebuildStudioRecordingBlueprints` and `Scripts/RebuildStudioRecordingWidgets.py`; the clean Unreal commandlet verification pass completed with 0 errors and 0 warnings, and the C++ build succeeds. Remaining limitation: final in-PIE screenshot comparison against the reference still needs a visual pass. Primary files/assets: `Content/GUI/RecordingGUIBP.uasset`, `Content/GUI/RecordSongListBP.uasset`, `Content/GUI/RecordingRecordListBP.uasset`, `Scripts/RebuildStudioRecordingWidgets.py`, `Source/MusicManager/Public/UI/MusicManagerWidgetBlueprintTools.h`, `Source/MusicManager/Private/UI/MusicManagerWidgetBlueprintTools.cpp`, `Source/MusicManager/Public/RecordManagerSubsystem.h`, `Source/MusicManager/Private/RecordManagerSubsystem.cpp`, `Source/MusicManager/Public/SongManagerSubsystem.h`, `Source/MusicManager/Private/SongManagerSubsystem.cpp`, `Source/MusicManager/Public/CommandDispatcherSubsystem.h`, `Source/MusicManager/Private/CommandDispatcherSubsystem.cpp`, `Source/MusicManager/Public/UI/RecordWidget.h`, `Source/MusicManager/Private/UI/RecordWidget.cpp`, `Source/MusicManager/Public/UI/RecordSongListItemWidget.h`, `Source/MusicManager/Private/UI/RecordSongListItemWidget.cpp`, `Source/MusicManager/Private/ChartManagerSubsystem.cpp`, `Source/MusicManager/Public/MusicSaveGame.h`, and `Source/MusicManager/Private/Tests/PersistenceTests.cpp`.
- Studio artist selection for recording has been hardened. Signed artist rows now select and broadcast the stable `ArtistId` instead of `ArtistName`; the signed artist panel pushes its default selection into `UArtistManagerSubsystem` when needed; `ULayout::ShowRecordWidget` resolves legacy name selections to stable contract ids and defaults to the first signed artist if none is selected. `URecordWidget` now logs the selected artist, resolved contract/genre, and eligible song count when populating the available songs list, and shows a no-eligible-songs message when the real song database has no matching entries. Build note: the code compiled, but final link verification was blocked because `UnrealEditor.exe` had `UnrealEditor-MusicManager.dll` locked; close the editor and rerun the build for final link verification. Primary files: `Source/MusicManager/Public/UI/SignedArtistItemWidget.h`, `Source/MusicManager/Private/UI/SignedArtistItemWidget.cpp`, `Source/MusicManager/Private/UI/SignedArtistPanelWidget.cpp`, `Source/MusicManager/Private/Layout.cpp`, and `Source/MusicManager/Private/UI/RecordWidget.cpp`.
- The studio recording GUI has been rebuilt closer to the AAA reference image using `userwidget.md` and the design workflow. Reference crop sections were saved under `docs/design/references/studio_recording_sections/` for header, artist header, format cards, release overview, available songs, selected tracks, and footer comparisons. A generated OpenAI Image 2 icon sheet has been copied into `Content/GUI/StudioRecording/StudioRecordingIconSheet_OpenAIImage2.png`, with cropped/imported production icons for clock, warning, filter, and search. `RecordingGUIBP` now includes the missing `STUDIO RECORDING` heading with record icon, artist portrait/vinyl composition, signed-artist tablet, icon-backed popularity/fans/reputation stats, release overview gold stem, bordered dark panels, genre dropdown, song search field, aligned table headers/values, play/stop preview button, checkbox-style select control, segmented popularity meter, clock/warning icons, and dotted next-track row. `URecordWidget` now owns real genre/search filtering, selected-track refresh, and available-song cache behavior; `URecordSongListItemWidget` now owns play/stop visual state, selected-state refresh, genre/duration columns, and segmented popularity binding. Clean verification passed: the Studio Recording Blueprint rebuild commandlet completed with 0 errors and 0 warnings, and the C++ build succeeded. Remaining limitation: final in-PIE screenshot comparison still needs a human visual pass against the provided reference after reopening the editor. Primary files/assets: `Source/MusicManager/Public/UI/RecordWidget.h`, `Source/MusicManager/Private/UI/RecordWidget.cpp`, `Source/MusicManager/Public/UI/RecordSongListItemWidget.h`, `Source/MusicManager/Private/UI/RecordSongListItemWidget.cpp`, `Source/MusicManager/Private/UI/MusicManagerWidgetBlueprintTools.cpp`, `Scripts/RebuildStudioRecordingWidgets.py`, `Content/GUI/RecordingGUIBP.uasset`, `Content/GUI/RecordSongListBP.uasset`, `Content/GUI/RecordingRecordListBP.uasset`, and `Content/GUI/StudioRecording/*`.- Studio command routing now creates and displays the production `RecordingGUIBP` even when `LayoutBP1` does not contain a bound child widget named `RecordWidget`. `ULayout` now has `RecordWidgetClass`, loads `/Game/GUI/RecordingGUIBP.RecordingGUIBP_C`, creates it on the HUD layer, keeps it collapsed until needed, and calls this ensure path both during layout construction and when the Studio command is clicked. This fixes Studio button clicks doing nothing when the optional Blueprint binding is missing. Build verification succeeded. Primary files: `Source/MusicManager/Public/Layout.h` and `Source/MusicManager/Private/Layout.cpp`.
- The layout audition launcher now detects the legacy `/Game/GUI/AuditionBP.AuditionBP_C` class if it was serialized into `LayoutBP1` and replaces it at runtime with `/Game/GUI/Audition/ArtistAuditionPanelBP.ArtistAuditionPanelBP_C`. This prevents audition news clicks from opening the old audition Blueprint while preserving the editable `ArtistAuditionPanelWidgetClass` hook for future non-legacy subclasses. Runtime logs now report when the legacy class is replaced and which audition widget class was created.
- Generated widget rebuild scripts now preserve existing Blueprint assets and rebuild their widget trees in place, avoiding transient missing-binding compiler warnings caused by deleting and recreating empty Widget Blueprints. This applies to the artist audition panel and news feed widget scripts. Remaining limitation: final in-editor pixel comparison still needs a visual pass in PIE against the reference images.

## Temporary Or Non-Final Areas

The code currently contains explicit temporary/TODO markers for:

- Temporary radio simulation in market/record systems.
- TODOs for genre-based radio formatting.
- TODOs for payola/promotion hooks.
- TODOs for chart feedback loops and touring boosts.
- TODOs for record-specific exposure differentiation.
- Temporary default record format mapping in the recording GUI.

## Current Practical State

The repository currently looks like an early playable prototype foundation. It has meaningful C++ systems for artist signing, contracts, songs, recording, records, monthly sales, market demand, finance entries, news, expanded validated save/load coverage for the implemented loop, and several UMG-backed GUI areas.

It is not yet a full professional-grade music label management game. The largest missing pieces are chart GUI, final editor-built release/marketing UMG assets, touring, critics, awards, rival labels, staff progression, polished production GUI across all screens, persistence for remaining future systems once they exist, and full integration of all screens into a validated command-driven gameplay loop.
- Active contracts GUI has been added as a production UMG slice following `userwidget.md`. A new OpenAI Image 2 reference image and workflow note were saved at `docs/design/references/active_contracts_reference.png` and `docs/design/references/active_contracts_reference_workflow.md`; a generated OpenAI Image 2 contract icon sheet was copied into `Content/GUI/Contracts/ActiveContractsIconSheet_OpenAIImage2.png` and cropped/imported into individual contract/status/date/royalty/record/revenue icons. `UActiveContractsWidget` now lists real `UArtistManagerSubsystem::ActiveContracts`, refreshes on artist signed, contract expired, and monthly contract financial updates, shows an empty state when no contracts exist, and displays a selected contract dossier with dates, terms, royalties, bonus, upkeep, lifetime revenue/cost, last royalty, record delivery, momentum, and production progress. `UActiveContractItemWidget` renders selectable contract cards backed by real contract data. `ULayout` now creates `/Game/GUI/Contracts/ActiveContractsBP.ActiveContractsBP_C` at runtime, keeps it collapsed until needed, opens it from the Contracts dock command, and preserves legacy `ShowContract` callers by opening the active contracts panel preselected to the requested artist. Clean verification passed: C++ build succeeded, `Scripts/RebuildActiveContractsWidget.py` imported assets and rebuilt `ActiveContractsBP`/`ActiveContractItemBP`, and the second commandlet pass completed with 0 errors and only unrelated engine Interchange/Trellis warnings. Remaining limitation: final in-PIE screenshot comparison against the generated reference still needs a visual pass after reopening the editor. Primary files/assets: `Source/MusicManager/Public/UI/ActiveContractsWidget.h`, `Source/MusicManager/Private/UI/ActiveContractsWidget.cpp`, `Source/MusicManager/Public/UI/ActiveContractItemWidget.h`, `Source/MusicManager/Private/UI/ActiveContractItemWidget.cpp`, `Source/MusicManager/Public/Layout.h`, `Source/MusicManager/Private/Layout.cpp`, `Source/MusicManager/Private/UIManagerSubsystem.cpp`, `Source/MusicManager/Public/UI/MusicManagerWidgetBlueprintTools.h`, `Source/MusicManager/Private/UI/MusicManagerWidgetBlueprintTools.cpp`, `Scripts/RebuildActiveContractsWidget.py`, `Content/GUI/Contracts/*`, and `docs/design/references/active_contracts_reference.png`.
