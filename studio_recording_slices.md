# Studio Recording GUI And Recording System Slices

These prompts implement the production-grade studio recording flow for MusicManager. Follow `AGENTS.md`, `docs/architecture.md`, `docs/design.md`, and `userwidget.md` before implementing any slice.

Current starting point:

- `URecordWidget` exists, but it is a prototype studio screen.
- Current UI supports only Single/LP, not EP.
- Current recording completion is immediate; no real duration is modeled.
- Current recording cost is not projected in the GUI and is not booked as a recording-cost ledger entry by the recording flow.
- Current song selection uses songs already assigned to the artist, not the broader song database filtered by the artist genre.
- Song preview playback exists in `URecordSongListItemWidget` through `UMusicPlayerComponent`.
- `URecordManagerSubsystem`, `USongManagerSubsystem`, `UCommandDispatcherSubsystem`, finance, sales, charts, and market systems provide partial reusable infrastructure.

## Prompt 1 - Recording Domain Model: Single, EP, LP, Cost, Duration

Implement a production recording domain model that supports `Single`, `EP`, and `LP` as first-class record types.

User-facing outcome:

- The game can calculate whether a selected track list is a valid Single, EP, or LP.
- The game can show projected recording cost and completion date before the player confirms.
- Recording completion is no longer immediate; it is based on a deterministic duration derived from record type, song count, and production scope.

Affected systems:

- `RecordManagerSubsystem`
- `CommandDispatcherSubsystem`
- `MusicSaveGame` / persistence snapshots
- `FinanceManagerSubsystem`
- `current.md`

Implementation requirements:

- Add a record type enum, for example `ERecordType { Single, EP, LP }`.
- Replace or augment `bIsSingle` / `bIsLP` with the enum while preserving migration/backward compatibility for existing save data.
- Define production validation:
  - Single: exactly 1 song.
  - EP: 2-5 songs.
  - LP: 6-14 songs.
- Add deterministic projection data:
  - `EstimatedRecordingCost`
  - `EstimatedDurationDays`
  - `EstimatedCompletionDate`
  - `ValidationWarnings`
- Cost should scale by record type, track count, artist quality/reputation, and current era. Do not use fake placeholder constants without documenting them as balancing defaults in a production rules location.
- Duration should complete on the simulation timeline and be processed by the existing weekly/monthly time pipeline.
- Active recording sessions must persist across save/load with song locks intact.
- Add automated tests for record-type validation, cost/duration projection, and completion after time advances.

Acceptance criteria:

- A valid EP can be recorded.
- Invalid song counts fail with clear command errors.
- Recording sessions complete only when the projected completion date is reached.
- Save/load preserves active recording sessions and locked songs.
- Build succeeds.
- `current.md` is updated.

## Prompt 2 - Genre-Based Song Database Selection For Studio

Implement real studio song candidate selection from the game song database by artist genre.

User-facing outcome:

- When the player opens the studio for an artist, the available song list shows unreleased, unlocked songs from the song database matching the artist genre.
- The player can preview each song, inspect song quality metadata, and select/deselect songs for the pending record.
- Songs that are locked, recorded, released, or genre-incompatible are excluded or clearly disabled with a real reason.

Affected systems:

- `SongManagerSubsystem`
- `ArtistManagerSubsystem`
- `RecordWidget` / future studio GUI backing class
- `RecordSongListItemWidget`
- save validation for song-to-record references
- `current.md`

Implementation requirements:

- Add a query such as `BuildRecordingSongCandidateView(ArtistId, RecordType, OutView)` or `GetAvailableSongsForArtistGenre`.
- Resolve artist genre from the signed artist contract/artist data.
- Candidate rows must include:
  - stable `SongId`
  - song name
  - genre
  - quality metrics
  - hit potential
  - duration estimate if available, otherwise a real derived estimate
  - `SoundWave` availability
  - disabled reason
  - whether selected
- Do not create fake songs in the GUI.
- If the current data table has too few songs for a genre, fail cleanly with a no-data state and a clear log; do not generate placeholder rows at runtime.
- Keep song ownership rules explicit: either selected genre songs become assigned/locked to the recording artist at recording start, or only songs with matching artist id are valid. Choose one production rule and enforce it consistently in validation and persistence.
- Preserve preview playback through `UMusicPlayerComponent`.

Acceptance criteria:

- Studio candidates come from real `USongManagerSubsystem` data.
- Genre filtering works for signed artists.
- Preview button plays real `SoundWave` data when present and logs a clear warning when missing.
- Selected songs are locked during recording and linked to the created record on completion.
- Build succeeds.
- `current.md` is updated.

## Prompt 3 - Start Recording Command, Finance, News, And Completion Integration

Upgrade the start-recording command into a full production command path.

User-facing outcome:

- Confirming a studio recording validates real data, charges the label, starts a recording session, and emits news/notification events.
- When recording completes, a real record is created and becomes available for release planning.

Affected systems:

- `UCommandDispatcherSubsystem`
- `URecordManagerSubsystem`
- `UFinanceManagerSubsystem`
- `UEventSubsystem`
- `UArtistManagerSubsystem`
- `URecordManagerSubsystem` persistence
- `current.md`

Implementation requirements:

- Extend `FStartRecordingCommand` to carry:
  - record type enum
  - selected song ids
  - selected/derived formats
  - projected cost and duration validation hash or re-computed authoritative projection
- Dispatcher must validate affordability before starting.
- On success, book a `RecordingCost` finance ledger transaction exactly once.
- Emit a `RecordingSession` news event when recording starts, with metadata for artist id/name, record type, song ids, cost, start date, and estimated completion date.
- Emit a second `RecordingSession` or record-complete event when the session completes.
- Record creation must update:
  - record database
  - song-to-record links
  - artist record delivery/progress where contract terms require records
  - release-planner availability
  - save-game snapshots
- Do not directly write chart rows. Charts should pick up released records from real sales history after release.
- Do not directly grant radio play. Radio/market systems should read released record data and exposure state after release/marketing.

Acceptance criteria:

- Starting a recording reduces cash by the real projected cost.
- Insufficient funds fails with a structured command result.
- Recording start and completion appear as news events.
- Completed records are visible to release planning.
- Charts remain driven by sales history, not by recording creation.
- Build and command regression tests pass.
- `current.md` is updated.

## Prompt 4 - Production Studio Recording UMG Screen

Create the production GUI for recording a record in the studio, using `userwidget.md`.

User-facing outcome:

- The player opens Studio for the selected signed artist and sees a polished studio recording panel.
- The player chooses Single, EP, or LP.
- The player previews songs, selects/deselects songs, sees selected track count, projected recording cost, projected completion date, validation warnings, and can confirm/cancel.

Required GUI workflow:

- Generate a AAA reference image with OpenAI Image 2 before implementation.
- Save the reference under `docs/design/references/studio_recording_reference.png`.
- Save a workflow note under `docs/design/references/studio_recording_reference_workflow.md`.
- Create or replace the C++ widget backing class as production-ready:
  - `URecordWidget` can be upgraded if appropriate, or a new `UStudioRecordingWidget` can be introduced if cleaner.
- Create/rebuild Blueprint widget assets that inherit from the C++ classes:
  - main studio recording screen
  - song candidate row widget
  - selected-track row widget if separate
- Blueprint must contain the full designer hierarchy and all inherited/bound variables.
- Generated image assets must be assigned to the Blueprint fields.
- Use real list views for dynamic song rows; do not hardcode song rows into a background image.

Required states:

- No signed artist selected.
- Artist has no genre-compatible songs.
- Valid selection.
- Invalid selection with warning.
- Insufficient funds.
- Recording already in progress / songs locked.
- Missing audio preview.

Acceptance criteria:

- The Studio command opens the production recording widget for the selected artist.
- Single/EP/LP controls update validation, cost, and duration immediately.
- Song preview, select, deselect, confirm, and cancel all work with real data.
- Widget matches the generated reference image at a pixel-like level.
- Build succeeds.
- `current.md` is updated.

## Prompt 5 - Downstream Recording Data Integration: Release, Radio, Charts, Save, Tests

Close the downstream integration gaps so recorded music flows through the professional management loop.

User-facing outcome:

- A record created in the studio becomes part of the full game economy: release planning, marketing, radio/market exposure, sales, finance, charts, and news.

Affected systems:

- `RecordManagerSubsystem`
- `ReleasePlannerWidget` / release planner view models
- `MarketingManagerSubsystem`
- `MarketManagerSubsystem`
- `ChartManagerSubsystem`
- `FinanceManagerSubsystem`
- `MusicSaveSubsystem`
- automation tests
- `current.md`

Implementation requirements:

- Ensure completed records appear in release planner with:
  - record type
  - artist display name
  - selected songs
  - primary genre
  - record quality
  - valid formats for the current era
- Ensure marketing planner can target completed/scheduled/released records as currently intended.
- Ensure market/radio systems consume only real released records and real exposure data.
- Replace or isolate temporary radio simulation so it does not pretend to be the final radio system; create a production radio-exposure interface if needed.
- Ensure chart definitions classify Single, EP, and LP correctly:
  - Singles chart includes only singles.
  - Albums chart includes LPs and any configured album-like EP behavior.
  - Genre charts use `PrimaryGenre`.
  - Format charts use real selected formats.
- Add round-trip persistence tests for active recordings, completed records, song locks, record type, selected songs, cost/duration fields, and downstream references.
- Add end-to-end tests:
  - select genre songs
  - start recording
  - advance time to completion
  - schedule release
  - simulate sales
  - verify chart eligibility through sales history

Acceptance criteria:

- No system depends on mock record data.
- No chart rows are generated before real sales exist.
- No radio exposure is granted without real release/market data.
- Save/load preserves all recording and downstream state.
- Build and automation tests pass.
- `current.md` is updated.
