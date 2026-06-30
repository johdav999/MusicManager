# Charts Implementation Slices

This document breaks the chart system into production-ready vertical slices. Each completed implementation slice must update `current.md`.

## Overall Goal

Implement weekly music charts that rank released records using real sales/streaming/market data, preserve chart history, generate milestones/news, and expose polished chart GUI.

## Global Implementation Rules

- Read `AGENTS.md`, `docs/architecture.md`, and `docs/design.md` before implementation.
- Charts must be deterministic and data-driven.
- Charts must use real record/sales/market data; no fake chart rows.
- Use stable IDs for chart entries.
- For any chart GUI, generate a production-quality OpenAI Image 2 reference first, implement against it, compare, and iterate.
- Update `current.md` after each completed slice.

## Slice 1 - Chart Data Model

### Prompt

Create chart data structures for weekly ranked chart entries, chart definitions, and chart history.

### User-Facing Outcome

The game can represent charts as durable, queryable simulation data.

### Implementation Scope

- Add structures:
  - `FChartDefinition`
  - `FChartEntry`
  - `FWeeklyChartSnapshot`
  - `FRecordChartHistory`
- Include:
  - chart id
  - region id or global marker
  - chart type: singles/albums/all records initially as supported by existing data
  - week start date
  - rank
  - previous rank
  - peak rank
  - weeks on chart
  - record id
  - artist id
  - chart points
  - units/streams where available
- Keep types Blueprint-visible where GUI needs them.

### Acceptance Criteria

- Chart structs compile.
- Structs are stable-id based.
- No chart rows are generated yet unless using real data in later slices.
- `current.md` is updated.

### Primary Files

- New `ChartManagerSubsystem` files or chart model headers
- `current.md`

## Slice 2 - Chart Manager Subsystem

### Prompt

Create `UChartManagerSubsystem` to own chart definitions, weekly chart snapshots, and chart history.

### User-Facing Outcome

Charts have a dedicated system separated from market and record simulation.

### Implementation Scope

- Add `UChartManagerSubsystem : UGameInstanceSubsystem`.
- Add default chart definitions for the current available market data:
  - global records chart
  - regional records charts where regions exist
- Add query APIs:
  - get current chart
  - get chart by id/week
  - get chart history for record
  - get top record for dashboard/news
- Do not generate fake charts.

### Acceptance Criteria

- Subsystem initializes.
- Chart definitions are queryable.
- Empty charts return explicit empty results.
- `current.md` is updated.

### Primary Files

- `Source/MusicManager/Public/ChartManagerSubsystem.h`
- `Source/MusicManager/Private/ChartManagerSubsystem.cpp`
- `Source/MusicManager/MusicManager.Build.cs`
- `current.md`

## Slice 3 - Chart Point Formula

### Prompt

Implement deterministic chart point calculation from real record sales history and market data.

### User-Facing Outcome

Released records earn chart positions based on actual performance.

### Implementation Scope

- Add formula for chart points:
  - unit sales from `RecordManagerSubsystem`
  - stream-equivalent handling if existing formats include streaming
  - region filtering
  - release recency/lifecycle if needed
  - marketing/radio effects only as already reflected in sales/exposure data
- Keep formula documented in code.
- Avoid chart points that depend on random per-call state.

### Acceptance Criteria

- Same inputs produce same chart points.
- Unreleased records are excluded.
- Records with no sales are excluded or rank at zero only if intentionally supported.
- Regional charts use region-specific sales entries.
- `current.md` is updated.

### Primary Files

- `ChartManagerSubsystem`
- `RecordManagerSubsystem`
- `MarketManagerSubsystem`
- `current.md`

## Slice 4 - Weekly Chart Resolution

### Prompt

Wire chart calculation into weekly simulation so charts update once per week.

### User-Facing Outcome

Advancing time updates music charts on a weekly cadence.

### Implementation Scope

- Add chart phase handling from `UGameTimeSubsystem`.
- Calculate each active chart once per week.
- Store weekly snapshots.
- Track previous rank, movement, peak rank, and weeks on chart.
- Ensure multi-week fast-forward processes each weekly chart in order.

### Acceptance Criteria

- Weekly time advancement creates chart snapshots.
- Multi-week advancement creates ordered weekly snapshots.
- Previous rank/movement is correct.
- No duplicate snapshot is created for the same chart/week.
- `current.md` is updated.

### Primary Files

- `GameTimeSubsystem`
- `ChartManagerSubsystem`
- `current.md`

## Slice 5 - Chart Milestones And News

### Prompt

Generate news/events for meaningful chart milestones.

### User-Facing Outcome

The player is notified when releases chart, climb, hit number one, or reach milestones.

### Implementation Scope

- Add milestone detection:
  - first chart entry
  - top 40
  - top 10
  - number 1
  - new peak
  - major drop if desired
- Emit news through `UEventSubsystem`.
- Deduplicate milestones by chart id, record id, milestone type, and week.
- Keep news grounded in real chart snapshots.

### Acceptance Criteria

- Milestone news appears only for real chart outcomes.
- Duplicate news is prevented.
- News payload references record/artist ids.
- `current.md` is updated.

### Primary Files

- `ChartManagerSubsystem`
- `EventSubsystem`
- `current.md`

## Slice 6 - Chart Persistence

### Prompt

Persist chart definitions, weekly snapshots, chart history, and processed chart milestones.

### User-Facing Outcome

Chart history survives save/load.

### Implementation Scope

- Extend save snapshots for:
  - chart definitions if mutable, otherwise save only history/snapshots
  - weekly chart snapshots
  - per-record chart history
  - processed milestone keys
- Validate:
  - chart ids are non-empty
  - week dates are valid
  - ranks are positive and unique within a chart/week
  - record ids exist
  - artist ids exist
  - chart points are finite/non-negative
- Add chart snapshot load/apply methods.

### Acceptance Criteria

- Chart history survives save/load.
- Invalid chart references fail validation.
- Loading and advancing time does not duplicate already processed chart weeks.
- `current.md` is updated.

### Primary Files

- `ChartManagerSubsystem`
- `MusicSaveGame`
- `MusicSaveSubsystem`
- `current.md`

## Slice 7 - Chart View Models

### Prompt

Create chart view models for chart list, chart detail, and record chart history GUI.

### User-Facing Outcome

GUI can display current charts and history without querying multiple subsystems directly.

### Implementation Scope

- Add view models:
  - `FChartListView`
  - `FChartEntryView`
  - `FRecordChartHistoryView`
- Include:
  - display rank
  - rank movement
  - artist name
  - record title
  - units/points
  - peak rank
  - weeks on chart
  - player-owned highlight flag
- Resolve display data from subsystem ids.
- Provide explicit empty state when no chart exists.

### Acceptance Criteria

- View models use real chart snapshots.
- Missing display references are handled as validation/log warnings, not crashes.
- Empty charts produce an empty view model.
- `current.md` is updated.

### Primary Files

- `ChartManagerSubsystem`
- `ArtistManagerSubsystem`
- `RecordManagerSubsystem`
- possible `ChartViewModels.h`
- `current.md`

## Slice 8 - Charts GUI Reference And Widget

### Prompt

Generate a production-quality reference image for the charts GUI, then implement the UMG-backed chart screen to match it as closely as practical.

### User-Facing Outcome

The player can inspect weekly charts in a polished production interface.

### Implementation Scope

- Generate the reference image using OpenAI Image 2.
- Save the reference image in an appropriate docs/design reference location.
- Implement chart screen:
  - chart selector
  - week/current view
  - ranked entries
  - movement indicators
  - units/points
  - peak/weeks columns
  - player-owned highlight
  - empty state
  - error/disabled states
- Wire screen to chart view models.
- Compare implemented GUI against reference and iterate.

### Acceptance Criteria

- GUI uses real chart data only.
- Empty state appears if no charts are available.
- Visual style follows `docs/design.md`.
- Reference comparison has been performed.
- `current.md` is updated.

### Primary Files/Assets

- New chart widget C++ files
- New or updated UMG assets
- `UIManagerSubsystem`
- `Layout`
- `ChartManagerSubsystem`
- reference image asset/documentation
- `current.md`

## Slice 9 - Dashboard Integration

### Prompt

Integrate chart highlights into dashboard/status/news surfaces.

### User-Facing Outcome

The player can see the top charting release and major chart movement from the main operating view.

### Implementation Scope

- Add dashboard query for:
  - top player-owned release
  - current number one
  - biggest player-owned movement
  - new milestones
- Route through existing UI manager/dashboard/status paths where appropriate.
- Do not create fake dashboard values.

### Acceptance Criteria

- Dashboard can show real chart highlights.
- No chart data produces a clear empty state.
- Major chart events are visible after time advance.
- `current.md` is updated.

### Primary Files

- `ChartManagerSubsystem`
- dashboard/status/UI files
- `EventSubsystem`
- `current.md`

## Slice 10 - Chart Regression Coverage

### Prompt

Add automated coverage for chart calculation, weekly resolution, persistence, and milestone generation.

### User-Facing Outcome

Chart behavior is protected from regressions.

### Implementation Scope

- Add tests or documented automation for:
  - deterministic chart point calculation
  - weekly snapshot creation
  - rank movement from one week to the next
  - regional filtering
  - no duplicate chart snapshots
  - milestone news deduplication
  - save/load round trip
  - invalid chart snapshot rejection
- Use real subsystem data structures.

### Acceptance Criteria

- Tests compile or verification command is documented.
- Tests use real records/sales data, not fake GUI rows.
- `current.md` is updated with chart test status.

### Primary Files

- `Source/MusicManager/Private/Tests/...`
- `ChartManagerSubsystem`
- `RecordManagerSubsystem`
- `MusicSaveSubsystem`
- `current.md`

## Slice 11 - Production Charts GUI And Reference Workflow

### Prompt

Generate the production OpenAI Image 2 reference for the Charts GUI and implement the real UMG backing class against that reference.

### User-Facing Outcome

The player can open a polished charts screen that presents real weekly chart data, movement, units, points, peak rank, weeks-on-chart, and player-owned highlights.

### Implementation Scope

- Generate and store the reference image in `docs/design/references`.
- Add reference workflow documentation with the exact prompt used.
- Implement a Blueprintable chart widget backing class.
- Expose:
  - chart selector data
  - current chart table
  - dashboard/status highlights
  - selected record chart history
  - empty/error states
  - reference image path
- Keep the widget thin; all simulation data must come from `UChartManagerSubsystem`.

### Acceptance Criteria

- No mock chart rows are introduced.
- GUI backing uses real chart view models only.
- The reference image follows `docs/design.md`.
- The UMG asset can compare against `charts_reference.png`.
- `current.md` is updated.

### Primary Files/Assets

- `Source/MusicManager/Public/UI/ChartsWidget.h`
- `Source/MusicManager/Private/UI/ChartsWidget.cpp`
- `docs/design/references/charts_reference.png`
- `docs/design/references/charts_reference_workflow.md`
- `current.md`

## Slice 12 - Chart Dashboard And Status Integration

### Prompt

Expose chart dashboard/status projections for the main operating dashboard and chart GUI.

### User-Facing Outcome

The player can see the current #1, best player-owned charting release, biggest player-owned mover, and recent chart milestones without opening every chart manually.

### Implementation Scope

- Add dashboard view models to `UChartManagerSubsystem`.
- Include empty state/status message when no chart week exists.
- Resolve chart display names and entry display names from real subsystem state.
- Highlight player-owned records using the player label id.
- Keep dashboard values grounded in current chart snapshots only.

### Acceptance Criteria

- Dashboard projection returns meaningful empty state before charts resolve.
- Dashboard projection returns real highlights after charts resolve.
- No fake dashboard/status values are generated.
- `current.md` is updated.

### Primary Files

- `ChartManagerSubsystem`
- `UI/ChartsWidget`
- `current.md`

## Slice 13 - Expanded Chart Types And Era Formulas

### Prompt

Add professional-grade chart definitions for singles, albums, genre charts, format-specific charts, regional singles/albums, and era/formula-specific scoring.

### User-Facing Outcome

Charts feel like a real music industry system instead of a single generic ranking.

### Implementation Scope

- Extend chart definitions with:
  - genre filters
  - format filters
  - formula profile
- Add global singles and albums charts.
- Add global format-specific charts for active record formats.
- Add dynamic global genre charts from real record genres.
- Add regional singles and albums charts where market regions exist.
- Apply deterministic era/formula weights for physical, format-weighted, streaming, singles velocity, album longevity, and genre-specialist charts.

### Acceptance Criteria

- Expanded chart definitions are queryable.
- Records only enter charts that match their real type/genre/format data.
- Formula weights are deterministic and testable.
- Invalid saved chart definition enum/filter values fail validation.
- `current.md` is updated.

### Primary Files

- `ChartManagerSubsystem`
- `MusicSaveGame`
- `current.md`

## Slice 14 - Weekly Sales Granularity From Monthly Sales

### Prompt

Improve weekly chart scoring so monthly record sales history can feed weekly charts without requiring fake weekly rows.

### User-Facing Outcome

Weekly charts update consistently even when sales are currently produced as monthly history entries.

### Implementation Scope

- Treat each monthly sales entry as a month bucket.
- Allocate units into the requested chart week by calendar-day overlap.
- Preserve region and format filtering.
- Use allocated weekly units for points, units, and stream-equivalent units.
- Add regression tests for month overlap and non-overlap behavior.

### Acceptance Criteria

- A week inside a monthly sales bucket receives proportional units.
- A week outside the bucket receives zero units.
- Streaming and physical sales still score through the correct formula paths.
- No fake sales rows are persisted.
- `current.md` is updated.

## Suggested Execution Order

1. Slice 1 - Chart data model
2. Slice 2 - Chart manager subsystem
3. Slice 3 - Chart point formula
4. Slice 4 - Weekly chart resolution
5. Slice 5 - Chart milestones and news
6. Slice 6 - Chart persistence
7. Slice 7 - Chart view models
8. Slice 8 - Charts GUI reference and widget
9. Slice 9 - Dashboard integration
10. Slice 10 - Chart regression coverage
11. Slice 11 - Production Charts GUI And Reference Workflow
12. Slice 12 - Chart Dashboard And Status Integration
13. Slice 13 - Expanded Chart Types And Era Formulas
14. Slice 14 - Weekly Sales Granularity From Monthly Sales

## Definition Of Done

- Weekly charts are generated from real sales/market data.
- Chart history, ranks, movement, peaks, and weeks-on-chart are tracked.
- Chart milestones generate deduplicated news.
- Chart history persists through save/load.
- Chart GUI follows `docs/design.md` and reference-image workflow.
- `current.md` accurately documents chart coverage.
