# Release Planner And Marketing Implementation Slices

This document breaks release planning and marketing into production-ready vertical slices. Each completed implementation slice must update `current.md`.

## Overall Goal

Turn recorded music into a real planned release flow with scheduled releases, regional format choices, marketing campaigns, costs, exposure effects, and GUI screens that follow `docs/design.md`.

## Global Implementation Rules

- Read `AGENTS.md`, `docs/architecture.md`, and `docs/design.md` before implementation.
- For any GUI slice, first generate a production-quality reference image with OpenAI Image 2, then implement the widget against it and compare.
- No mock GUI, placeholder screens, or fake marketing effects.
- Marketing must use real campaign state and affect real market/record outcomes.
- All player actions should go through the command dispatcher once available.
- Update `current.md` after each completed slice.

## Slice 1 - Release Lifecycle Model

### Prompt

Expand the record lifecycle into a production release-planning model that supports recorded, scheduled, released, and catalog states.

### User-Facing Outcome

Recorded music can wait for a planned release date instead of always becoming immediately available.

### Implementation Scope

- Extend record state where needed:
  - `Recorded`
  - `Scheduled`
  - `Released`
  - `Catalog`
- Ensure release dates are authoritative.
- Add helpers to query:
  - records awaiting release planning
  - scheduled releases
  - released/catalog records
- Update monthly/weekly processing to launch records only when release date is reached.
- Preserve existing immediate release behavior only through an explicit "release now" path.

### Acceptance Criteria

- New recordings can remain unreleased until scheduled.
- Sales simulation ignores unreleased records.
- Scheduled records begin sales when the date is reached.
- `current.md` is updated.

### Primary Files

- `RecordManagerSubsystem`
- `MusicSaveGame` if persistence exists
- `GameTimeSubsystem`
- `current.md`

## Slice 2 - Release Planning Command

### Prompt

Add a typed schedule release command that validates record ownership, release date, formats, and target regions.

### User-Facing Outcome

The player can schedule a release safely and get clear errors for invalid dates, formats, or regions.

### Implementation Scope

- Add `FScheduleReleaseCommand`.
- Include:
  - record id
  - label id
  - release date
  - selected regions
  - selected formats
  - release type if needed
- Validate:
  - record exists and belongs to player label
  - record is recorded but not released
  - release date is not before recorded/current date
  - formats are valid for era
  - regions exist
  - selected songs are still valid
- Execute through `URecordManagerSubsystem`.

### Acceptance Criteria

- Invalid record id fails cleanly.
- Past release date fails cleanly.
- Invalid region/format fails cleanly.
- Valid command schedules the record.
- `current.md` is updated.

### Primary Files

- `CommandDispatcherSubsystem`
- `RecordManagerSubsystem`
- `MarketManagerSubsystem`
- `current.md`

## Slice 3 - Regional Release State

### Prompt

Add regional release state so records can target specific markets and sell only in selected regions.

### User-Facing Outcome

The player can choose where a release launches, and sales respond to those choices.

### Implementation Scope

- Add selected region ids to record/release state.
- Update sales simulation to only evaluate selected release regions.
- Add validation that selected regions exist in `UMarketManagerSubsystem`.
- Persist region selections when persistence support is available.

### Acceptance Criteria

- Record sales only occur in selected regions.
- Empty region selection is rejected or replaced with a deliberate all-regions policy.
- Region choices survive save/load when persistence slice is complete.
- `current.md` is updated.

### Primary Files

- `RecordManagerSubsystem`
- `MarketManagerSubsystem`
- `MusicSaveGame` if relevant
- `current.md`

## Slice 4 - Marketing Campaign Data Model

### Prompt

Create a real marketing campaign model for record releases.

### User-Facing Outcome

Marketing choices become persistent campaign state instead of a single generic exposure multiplier.

### Implementation Scope

- Add `UMarketingManagerSubsystem` or a clearly bounded marketing area in an existing subsystem if architecture favors it.
- Define campaign data:
  - campaign id
  - record id
  - label id
  - target regions
  - channels
  - budget
  - start/end dates
  - status
  - generated exposure by region/channel
- Define marketing channels for current era support:
  - radio
  - press
  - TV
  - posters
  - social
  - playlisting
- Use era/date rules to allow or disallow channels.

### Acceptance Criteria

- Marketing campaigns can be created and queried.
- Campaign data is stable-id based.
- Campaign channels are validated against current era/date.
- No campaign uses fake effects; every channel has a defined calculation hook.
- `current.md` is updated.

### Primary Files

- New `MarketingManagerSubsystem`
- `RecordManagerSubsystem`
- `MarketManagerSubsystem`
- `MusicSaveGame` if persistence exists
- `current.md`

## Slice 5 - Launch Marketing Command

### Prompt

Add a typed launch marketing campaign command that validates budget, record state, regions, channels, and dates.

### User-Facing Outcome

The player can buy marketing campaigns only when the label can afford them and the campaign is valid for the release.

### Implementation Scope

- Add `FLaunchMarketingCampaignCommand`.
- Validate:
  - player label owns record
  - record is recorded/scheduled/released but still marketable
  - selected regions are valid
  - selected channels are valid for date/era
  - budget is positive
  - label has enough cash
  - campaign dates are valid
- Register marketing spend in `FinanceManagerSubsystem`.
- Create campaign in marketing subsystem.

### Acceptance Criteria

- Invalid budget/funds fail without creating campaign or spending cash.
- Successful command creates campaign and records expense once.
- Campaign can be queried by record.
- `current.md` is updated.

### Primary Files

- `CommandDispatcherSubsystem`
- `MarketingManagerSubsystem`
- `FinanceManagerSubsystem`
- `RecordManagerSubsystem`
- `current.md`

## Slice 6 - Marketing Exposure Resolution

### Prompt

Resolve active marketing campaigns into real regional record exposure that affects sales simulation.

### User-Facing Outcome

Marketing spend changes release outcomes through market exposure, not just UI text.

### Implementation Scope

- Add weekly/monthly marketing resolution.
- Convert campaign budget/channel/region/date into record exposure.
- Feed exposure into `UMarketManagerSubsystem` or directly into record sales demand snapshots through a clean API.
- Apply decay over time.
- Prevent duplicate application after save/load and month advancement.

### Acceptance Criteria

- Campaign exposure increases sales potential for targeted regions.
- Expired campaigns stop adding new exposure.
- Exposure values are queryable for UI.
- Re-advancing a loaded month does not duplicate campaign effects.
- `current.md` is updated.

### Primary Files

- `MarketingManagerSubsystem`
- `MarketManagerSubsystem`
- `RecordManagerSubsystem`
- `GameTimeSubsystem`
- `current.md`

## Slice 7 - Release Planner View Model

### Prompt

Create a release planner view model that exposes only real records ready for planning and their valid release options.

### User-Facing Outcome

The release planner can show real available records, valid dates, valid regions, valid formats, and projected implications.

### Implementation Scope

- Add `FReleasePlannerView`.
- Include:
  - record summary
  - artist summary
  - current lifecycle state
  - valid formats for date
  - available regions
  - selected/default region policy
  - simple projected reach based on market snapshots
  - validation warnings
- Keep projection conservative and based on real data.

### Acceptance Criteria

- View model returns no fake records.
- Empty state is represented explicitly when no records are ready.
- Format/region options reflect current data.
- `current.md` is updated.

### Primary Files

- `RecordManagerSubsystem`
- `MarketManagerSubsystem`
- possible `ReleasePlannerViewModels.h`
- `current.md`

## Slice 8 - Release Planner GUI Reference And Widget

### Prompt

Generate a production-quality reference image for the release planner GUI, then implement the UMG-backed release planner widget to match it as closely as practical.

### User-Facing Outcome

The player has a polished production release planner screen in the MusicManager black/gold vinyl style.

### Implementation Scope

- Generate the reference image using OpenAI Image 2.
- Save the reference image in an appropriate docs/design reference location.
- Implement the release planner widget:
  - record overview
  - release date selector
  - region selector
  - format selector
  - projected reach
  - validation warnings
  - schedule/release buttons
  - empty state
  - error/disabled states
- Wire it to real view models and `FScheduleReleaseCommand`.
- Compare implemented GUI against the reference and iterate.

### Acceptance Criteria

- GUI uses real data only.
- GUI has empty/error/disabled states.
- Scheduling uses the command dispatcher.
- Widget visually follows `docs/design.md`.
- Reference image comparison has been performed.
- `current.md` is updated.

### Primary Files/Assets

- New release planner widget C++ files
- New or updated UMG assets
- `CommandDispatcherSubsystem`
- `UIManagerSubsystem`
- `Layout`
- reference image asset/documentation
- `current.md`

## Slice 9 - Marketing Planner View Model

### Prompt

Create a marketing planner view model that exposes valid campaigns, channels, budget ranges, projected exposure, and affordability.

### User-Facing Outcome

The marketing planner can show the real consequences and constraints of marketing choices.

### Implementation Scope

- Add `FMarketingPlannerView`.
- Include:
  - selected record/release summary
  - valid channels for date/era
  - target regions
  - current label cash
  - budget limits
  - estimated exposure by channel/region
  - active campaigns for the record
  - validation warnings
- Ensure projection uses real market/campaign formulas.

### Acceptance Criteria

- View model does not fabricate records/campaigns.
- Invalid channels are omitted or disabled with reason.
- Affordability is visible.
- `current.md` is updated.

### Primary Files

- `MarketingManagerSubsystem`
- `FinanceManagerSubsystem`
- `MarketManagerSubsystem`
- possible `MarketingPlannerViewModels.h`
- `current.md`

## Slice 10 - Marketing Planner GUI Reference And Widget

### Prompt

Generate a production-quality reference image for the marketing planner GUI, then implement the UMG-backed marketing planner widget to match it as closely as practical.

### User-Facing Outcome

The player can launch real marketing campaigns from a polished production GUI.

### Implementation Scope

- Generate the reference image using OpenAI Image 2.
- Save the reference image in an appropriate docs/design reference location.
- Implement:
  - channel selector
  - budget controls
  - region targeting
  - exposure projection
  - affordability display
  - active campaign list
  - launch/adjust buttons
  - empty/error/disabled states
- Wire launch to `FLaunchMarketingCampaignCommand`.
- Compare implemented GUI against the reference and iterate.

### Acceptance Criteria

- GUI uses real data.
- Invalid campaigns cannot be launched.
- Spending is recorded in finance once.
- Visual style matches `docs/design.md`.
- Reference comparison has been performed.
- `current.md` is updated.

### Primary Files/Assets

- New marketing planner widget C++ files
- New or updated UMG assets
- `CommandDispatcherSubsystem`
- `MarketingManagerSubsystem`
- `UIManagerSubsystem`
- reference image asset/documentation
- `current.md`

## Slice 11 - Dashboard And News Integration

### Prompt

Integrate scheduled releases and marketing campaigns into dashboard/news feedback.

### User-Facing Outcome

The player can see upcoming releases, active campaigns, release launches, and campaign results without digging through subsystems.

### Implementation Scope

- Add upcoming release summaries to a dashboard/view model if dashboard exists.
- Add news events for:
  - release scheduled
  - release launched
  - marketing campaign launched
  - campaign completed
- Ensure no duplicate news.
- Show major changes after time advancement.

### Acceptance Criteria

- Release/marketing events appear in the news/feed path.
- Upcoming releases can be queried for UI.
- Time advancement produces understandable feedback.
- `current.md` is updated.

### Primary Files

- `EventSubsystem`
- `UIManagerSubsystem`
- `RecordManagerSubsystem`
- `MarketingManagerSubsystem`
- dashboard/status files if present
- `current.md`

## Slice 12 - Persistence And Regression Coverage

### Prompt

Persist release planner and marketing campaign state and add regression coverage for release/marketing flows.

### User-Facing Outcome

Scheduled releases, selected regions/formats, active campaigns, exposure, and campaign spending survive save/load.

### Implementation Scope

- Extend save snapshots for release fields and marketing campaigns.
- Validate campaign references to records, labels, regions, and channels.
- Add tests or documented automation for:
  - schedule release success/failure
  - launch marketing success/failure
  - campaign spend booked once
  - exposure affects sales after time advance
  - save/load round trip

### Acceptance Criteria

- Scheduled releases survive save/load.
- Marketing campaigns survive save/load.
- Invalid marketing snapshots fail validation.
- Tests compile or verification command is documented.
- `current.md` is updated.

### Primary Files

- `MusicSaveGame`
- `MusicSaveSubsystem`
- `RecordManagerSubsystem`
- `MarketingManagerSubsystem`
- tests
- `current.md`

## Slice 13 - Production Release Planner UMG Screen Backing

### Prompt

Generate and store the release planner reference image, then implement a production UMG backing class that exposes the real release planner view model, command-driven scheduling, empty/error states, and reference-image metadata for Blueprint UMG assembly.

### User-Facing Outcome

The release planner has a real screen contract for UMG: it can show recorded releases, selectable release options, validation warnings, and schedule releases through the command dispatcher.

### Implementation Scope

- Generate the reference image with OpenAI Image 2 and save it under `docs/design/references`.
- Add a release planner widget backing class with Blueprint-callable methods to refresh from real records, select a record, expose the current `FReleasePlannerView`, submit `FScheduleReleaseCommand`, expose empty/error/disabled state, and expose the reference image path used for visual matching.
- Do not fabricate UMG assets or placeholder data.
- The Blueprint UMG asset must be assembled against this backing class and the saved reference image in the Unreal Editor.

### Acceptance Criteria

- Reference image exists in the repo.
- Backing class compiles and is Blueprintable.
- Scheduling goes through `UCommandDispatcherSubsystem`.
- Empty/error/disabled states are explicit.
- `current.md` is updated.

### Primary Files/Assets

- `docs/design/references/release_planner_reference.png`
- new `Source/MusicManager/Public/UI/ReleasePlannerWidget.h`
- new `Source/MusicManager/Private/UI/ReleasePlannerWidget.cpp`
- `current.md`

## Slice 14 - Production Marketing Planner UMG Screen Backing

### Prompt

Generate and store the marketing planner reference image, then implement a production UMG backing class that exposes the real marketing planner view model, command-driven campaign launch, ROI projections, empty/error states, and reference-image metadata for Blueprint UMG assembly.

### User-Facing Outcome

The marketing planner has a real screen contract for UMG: it can show marketable records, channel choices, budgets, exposure projections, ROI forecasts, active campaigns, and launch campaigns through the command dispatcher.

### Implementation Scope

- Generate the reference image with OpenAI Image 2 and save it under `docs/design/references`.
- Add a marketing planner widget backing class with Blueprint-callable methods to refresh from real marketable records, select a record, expose the current `FMarketingPlannerView`, launch `FLaunchMarketingCampaignCommand`, expose empty/error/disabled state, and expose the reference image path used for visual matching.
- Wire launch through `UCommandDispatcherSubsystem`.
- Do not fabricate UMG assets or placeholder campaigns.

### Acceptance Criteria

- Reference image exists in the repo.
- Backing class compiles and is Blueprintable.
- Campaign launch goes through the command dispatcher.
- Empty/error/disabled states are explicit.
- `current.md` is updated.

### Primary Files/Assets

- `docs/design/references/marketing_planner_reference.png`
- new `Source/MusicManager/Public/UI/MarketingPlannerWidget.h`
- new `Source/MusicManager/Private/UI/MarketingPlannerWidget.cpp`
- `current.md`

## Slice 15 - Campaign ROI, Dashboard Summaries, And Tuning Surface

### Prompt

Add real campaign ROI and dashboard summary projections for release and marketing planning, using existing finance, record sales, market exposure, and campaign data.

### User-Facing Outcome

The player can evaluate upcoming releases and marketing campaigns with useful professional-grade summaries: budget, spend, exposure, expected unit lift, revenue estimate, and ROI.

### Implementation Scope

- Add ROI summary structs for campaigns and marketing planner views.
- Calculate campaign spend, generated exposure, estimated unit lift, estimated gross revenue, and ROI.
- Add dashboard summary structs for upcoming releases, scheduled releases, active campaigns, and completed campaign outcomes.
- Expose Blueprint-callable dashboard summary functions.
- Keep formulas deterministic, documented in code, and based on existing real data.

### Acceptance Criteria

- Campaign ROI can be queried per campaign and per record.
- Marketing planner view includes ROI forecasts.
- Dashboard summary view includes upcoming releases and active campaigns.
- No mock campaign data is used.
- `current.md` is updated.

### Primary Files

- `MarketingManagerSubsystem`
- `RecordManagerSubsystem`
- `FinanceManagerSubsystem`
- `current.md`

## Slice 16 - Human-Readable Release And Marketing News

### Prompt

Polish release and marketing generated text so player-facing news and planner summaries use human-readable artist and record names rather than mostly stable ids.

### User-Facing Outcome

News entries and planner summaries read like real label reports, naming records and artists clearly while preserving ids in metadata for logic.

### Implementation Scope

- Add helper resolution for record display name and artist display name.
- Populate readable names in release and marketing planner views.
- Emit news events for release scheduling and marketing campaign launch with readable headlines/body.
- Preserve record/artist/campaign ids in metadata.
- Deduplicate news with stable keys.

### Acceptance Criteria

- Release scheduling news names the record and artist.
- Marketing launch news names the record and campaign spend.
- Metadata still contains stable ids.
- `current.md` is updated.

### Primary Files

- `RecordManagerSubsystem`
- `MarketingManagerSubsystem`
- `EventSubsystem`
- `CommandDispatcherSubsystem`
- `current.md`

## Slice 17 - Release And Marketing Regression Tests

### Prompt

Add automated regression coverage for release scheduling and marketing campaign flows.

### User-Facing Outcome

Release and marketing behavior has automated protection for valid/invalid scheduling, campaign validation, ROI calculations, and readable event payloads.

### Implementation Scope

- Add Unreal automation tests for schedule release snapshot validation success/failure, marketing campaign snapshot validation success/failure, ROI summary calculation from real campaign data, dashboard summary calculation from real records/campaigns, and generated release/marketing news payloads containing human-readable names and stable ids.
- Use production structs/APIs only.
- Avoid fake UI or mock data paths.

### Acceptance Criteria

- Tests compile.
- Tests run through `UnrealEditor-Cmd`.
- Tests fail if validation or readable-name payload generation is removed.
- `current.md` is updated.

### Primary Files

- new `Source/MusicManager/Private/Tests/ReleaseMarketingTests.cpp`
- release/marketing subsystem files touched by earlier slices
- `current.md`

## Suggested Execution Order

1. Slice 1 - Release lifecycle model
2. Slice 2 - Release planning command
3. Slice 3 - Regional release state
4. Slice 4 - Marketing campaign data model
5. Slice 5 - Launch marketing command
6. Slice 6 - Marketing exposure resolution
7. Slice 7 - Release planner view model
8. Slice 8 - Release planner GUI reference and widget
9. Slice 9 - Marketing planner view model
10. Slice 10 - Marketing planner GUI reference and widget
11. Slice 11 - Dashboard and news integration
12. Slice 12 - Persistence and regression coverage
13. Slice 13 - Production release planner UMG screen backing
14. Slice 14 - Production marketing planner UMG screen backing
15. Slice 15 - Campaign ROI, dashboard summaries, and tuning surface
16. Slice 16 - Human-readable release and marketing news
17. Slice 17 - Release and marketing regression tests

## Definition Of Done

- Records can be scheduled and released in selected regions/formats.
- Marketing campaigns spend real money and affect real exposure/sales.
- Release planner and marketing planner use real view models.
- GUI follows `docs/design.md` and reference-image workflow.
- Release/marketing state persists.
- `current.md` accurately documents release and marketing coverage.
