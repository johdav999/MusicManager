# Persistence Expansion Implementation Slices

This document breaks the save/load expansion into production-ready vertical slices. Each slice should leave the project buildable, avoid mock data/placeholders, and update `current.md` when complete.

## Overall Goal

Expand `UMusicSaveGame` with versioned snapshots for:

- time
- player label
- songs
- contracts/artists
- records
- record sales history/lifetime units
- finance accounts/ledger/monthly summaries
- market exposure
- news summaries / processed news keys

Add load validation so broken references fail cleanly.

## Global Implementation Rules

- Read `AGENTS.md` and `docs/architecture.md` before starting each slice.
- Keep persistence owned by subsystems; `UMusicSaveSubsystem` coordinates save/load but should not duplicate business logic.
- Save mutable campaign state only. Static data remains in DataTables/DataAssets.
- Use stable IDs for cross-references. Do not use display names as persistent keys.
- Every load must validate before mutating live subsystem state.
- Failed loads must leave the currently running game state unchanged when practical.
- Every completed slice must update `current.md`.
- No mock data, no placeholder functions, no scaffolding.

## Slice 1 - Save Schema Versioning And Validation Result Foundation

### Prompt

Implement a versioned save-game schema foundation for `UMusicSaveGame` and a structured load validation result that future subsystem snapshots can use.

### User-Facing Outcome

Save files have an explicit version and invalid/corrupt saves fail with clear logged reasons instead of partially loading broken campaign state.

### Implementation Scope

- Add `SaveVersion` and campaign metadata fields to `UMusicSaveGame`.
- Add constants for current/minimum supported save versions.
- Add a validation result type, for example `FMusicSaveValidationResult`, with:
  - `bIsValid`
  - validation errors
  - validation warnings
- Add helper methods to append errors/warnings.
- Update `UMusicSaveSubsystem::LoadGame` so it validates the save object before applying state.
- Keep old fields temporarily only if needed for migration from existing saves, but mark their purpose clearly.
- Add logging through the existing save/load path.

### Acceptance Criteria

- Save files include a version.
- Unsupported save versions fail cleanly.
- Null or wrong-class save objects fail cleanly.
- Validation errors are logged.
- Live subsystem state is not mutated when top-level save validation fails.
- `current.md` is updated with the new save versioning foundation.

### Primary Files

- `Source/MusicManager/Public/MusicSaveGame.h`
- `Source/MusicManager/Private/MusicSaveGame.cpp`
- `Source/MusicManager/Public/MusicSaveSubsystem.h`
- `Source/MusicManager/Private/MusicSaveSubsystem.cpp`
- `current.md`

## Slice 2 - Player Label Snapshot

### Prompt

Introduce a real player label campaign snapshot and persist it through save/load so records, finance, and future systems can reference a stable label id.

### User-Facing Outcome

The campaign has a durable player label identity instead of relying on placeholder artist ids or transient UI state.

### Implementation Scope

- Add `FPlayerLabelSnapshot` or equivalent to `UMusicSaveGame`.
- Include at minimum:
  - `LabelId`
  - display name
  - founded/current start date if already known
  - reputation/prestige if already represented, otherwise a real initial value
  - starting/current cash linkage strategy
- Decide ownership location:
  - If a label subsystem already exists, use it.
  - If none exists, create a minimal production-ready `UPlayerLabelSubsystem` that owns the player label state.
- Initialize the player label deterministically for a new campaign.
- Update systems that need current label id to read from the player label source, not hardcoded or placeholder data.
- Keep UI manager label id behavior compatible but make it resolve from the player label state where appropriate.

### Acceptance Criteria

- Player label state saves and loads.
- The player label has a stable non-empty id.
- Existing finance/UI label lookups can resolve the current player label.
- No new placeholder label id behavior is introduced.
- `current.md` is updated.

### Primary Files

- `Source/MusicManager/Public/MusicSaveGame.h`
- `Source/MusicManager/Private/MusicSaveGame.cpp`
- `Source/MusicManager/Public/MusicSaveSubsystem.h`
- `Source/MusicManager/Private/MusicSaveSubsystem.cpp`
- possible new `PlayerLabelSubsystem` files
- `Source/MusicManager/Public/UIManagerSubsystem.h`
- `Source/MusicManager/Private/UIManagerSubsystem.cpp`
- `current.md`

## Slice 3 - Time Snapshot Validation

### Prompt

Convert time persistence into an explicit time snapshot with validation and restore behavior.

### User-Facing Outcome

The current campaign date and simulation cadence survive save/load safely.

### Implementation Scope

- Add `FTimeSnapshot` or equivalent to `UMusicSaveGame`.
- Persist:
  - current game date
  - simulation end/reached-end state if required
  - any deterministic cadence state needed by `UGameTimeSubsystem`
- Move existing `SavedGameDate` usage into the new snapshot path.
- Add `UGameTimeSubsystem::BuildSaveSnapshot`, `ValidateSaveSnapshot`, and `ApplySaveSnapshot` style methods, or equivalent existing-pattern methods.
- Validate impossible dates:
  - unset/min date
  - dates before campaign start if a start date exists
  - dates beyond supported simulation end unless explicitly allowed

### Acceptance Criteria

- Time saves through the new snapshot.
- Time loads only after validation.
- Invalid dates fail cleanly.
- Existing weekly/monthly delegates are not fired during raw state restore unless intentionally documented.
- `current.md` is updated.

### Primary Files

- `Source/MusicManager/Public/GameTimeSubsystem.h`
- `Source/MusicManager/Private/GameTimeSubsystem.cpp`
- `Source/MusicManager/Public/MusicSaveGame.h`
- `Source/MusicManager/Private/MusicSaveSubsystem.cpp`
- `current.md`

## Slice 4 - Song Snapshot And Usage Validation

### Prompt

Harden song persistence by validating saved song records, recording locks, and record linkage before applying loaded song state.

### User-Facing Outcome

Saved songs restore reliably, and broken song ids or invalid artist links are rejected instead of corrupting recording/release state.

### Implementation Scope

- Review existing `FSongSaveRecord`.
- Extend it if needed to persist:
  - song id
  - owning artist id
  - song data
  - recording lock state if locks should survive save/load
  - record linkage for recorded songs
- Add validation for:
  - non-empty song ids
  - unique song ids
  - non-empty artist ids for artist-owned songs
  - valid record linkage when record snapshot support exists
- If record linkage requires Slice 5 to validate fully, stage the validation so song-level checks happen now and cross-reference checks happen in Slice 9.

### Acceptance Criteria

- Song save/load still works.
- Duplicate song ids fail validation.
- Invalid saved song records fail cleanly.
- No song state is lost compared with the current implementation.
- `current.md` is updated.

### Primary Files

- `Source/MusicManager/Public/SongManagerSubsystem.h`
- `Source/MusicManager/Private/SongManagerSubsystem.cpp`
- `Source/MusicManager/Public/MusicSaveGame.h`
- `Source/MusicManager/Private/MusicSaveSubsystem.cpp`
- `current.md`

## Slice 5 - Artist And Contract Snapshot Validation

### Prompt

Expand artist/contract persistence into explicit validated snapshots that preserve active/expired contracts, selected artist if appropriate, and artist runtime caches needed by simulation.

### User-Facing Outcome

Signed artists, contract status, expired contracts, and artist simulation modifiers survive save/load consistently.

### Implementation Scope

- Replace or wrap existing `SavedContracts` with explicit artist/contract snapshots.
- Persist:
  - active contracts
  - expired contracts
  - unsigned artist pool state if currently mutable
  - selected artist only if it is considered optional UI state; otherwise leave it out and document
  - artist momentum/reputation caches
  - artist-to-song mapping if not fully derivable from song snapshots
  - action availability cache if not recomputed on load
- Validate:
  - unique active contract per artist where required
  - non-empty artist ids
  - contract dates are valid
  - active/expired flags agree with stored collections
  - each contract references an artist known from saved artist/song data or static data

### Acceptance Criteria

- Active and expired contracts restore.
- Artist momentum/reputation behavior is not reset accidentally.
- Broken contract references fail validation.
- Action availability is correct after load, either restored or recomputed.
- `current.md` is updated.

### Primary Files

- `Source/MusicManager/Public/ArtistManagerSubsystem.h`
- `Source/MusicManager/Private/ArtistManagerSubsystem.cpp`
- `Source/MusicManager/Public/FArtistContract.h`
- `Source/MusicManager/Public/MusicSaveGame.h`
- `Source/MusicManager/Private/MusicSaveSubsystem.cpp`
- `current.md`

## Slice 6 - Record Snapshot And Label Ownership

### Prompt

Persist records and replace placeholder label ownership resolution with the real player label id from the player label state.

### User-Facing Outcome

Recorded releases survive save/load, and records belong to the actual player label instead of using artist ids as label ids.

### Implementation Scope

- Add `FRecordSnapshot` or equivalent for:
  - record id
  - artist id
  - label id
  - album/record name
  - single/LP flags
  - song ids
  - recorded date
  - release date
  - primary genre
  - formats
  - record quality
  - marketing exposure
  - lifecycle state
  - active recording intents/start/completion dates if active recordings must survive save/load
- Add `URecordManagerSubsystem` save/load snapshot methods.
- Replace `ResolveLabelForArtist` placeholder with real label ownership from the player label/contract state.
- Validate:
  - non-empty record ids
  - unique record ids
  - valid artist ids
  - valid label ids
  - song ids exist
  - no duplicate song ids within a record
  - valid date ordering
  - formats are recognized

### Acceptance Criteria

- Records survive save/load.
- Record lifecycle state survives save/load.
- Existing records use a stable player label id.
- Broken record/song/artist/label references fail validation.
- Placeholder label resolution is removed.
- `current.md` is updated.

### Primary Files

- `Source/MusicManager/Public/RecordManagerSubsystem.h`
- `Source/MusicManager/Private/RecordManagerSubsystem.cpp`
- `Source/MusicManager/Public/MusicSaveGame.h`
- `Source/MusicManager/Private/MusicSaveSubsystem.cpp`
- player label files from Slice 2
- `current.md`

## Slice 7 - Record Sales History And Lifetime Units Snapshot

### Prompt

Persist record sales history and lifetime unit totals so release performance does not reset after loading.

### User-Facing Outcome

Recorded sales history, monthly sales entries, and lifetime units are preserved across saves.

### Implementation Scope

- Add sales history snapshot structures:
  - record id
  - sales entries
  - lifetime units
- Persist `SalesHistory` and `LifetimeUnits` from `URecordManagerSubsystem`.
- Validate:
  - each sales record id exists
  - each entry record id matches a known record
  - market id is non-empty and preferably known
  - units sold are non-negative
  - dates are valid
  - lifetime units are not less than summed persisted entries unless intentionally documented

### Acceptance Criteria

- Sales history survives save/load.
- Lifetime units survive save/load.
- Invalid sales entries fail validation.
- Finance summaries and record stats can rely on restored sales data.
- `current.md` is updated.

### Primary Files

- `Source/MusicManager/Public/RecordManagerSubsystem.h`
- `Source/MusicManager/Private/RecordManagerSubsystem.cpp`
- `Source/MusicManager/Public/MusicSaveGame.h`
- `Source/MusicManager/Private/MusicSaveSubsystem.cpp`
- `current.md`

## Slice 8 - Finance Snapshot

### Prompt

Persist finance accounts, ledgers, and monthly summaries with validation.

### User-Facing Outcome

Cash balances, ledger history, monthly summaries, and financial reporting survive save/load.

### Implementation Scope

- Add finance snapshot structures for:
  - label accounts
  - current balances
  - ledger entries
  - monthly summaries
  - closed monthly summary keys if needed to prevent duplicates
- Add `UFinanceManagerSubsystem` save/load snapshot methods.
- Validate:
  - non-empty label ids
  - no duplicate label accounts
  - finite transaction amounts
  - valid timestamps
  - valid transaction types
  - monthly summary period start/end ordering
  - monthly summary net equals income minus expenses within tolerance
  - ledger references point to existing records/contracts where applicable, if cross-reference validation is available

### Acceptance Criteria

- Label balances survive save/load.
- Ledger entries survive save/load.
- Monthly summaries survive save/load.
- Closed monthly summaries are not duplicated after loading and advancing time.
- Invalid finance data fails validation.
- `current.md` is updated.

### Primary Files

- `Source/MusicManager/Public/FinanceManagerSubsystem.h`
- `Source/MusicManager/Private/FinanceManagerSubsystem.cpp`
- `Source/MusicManager/Public/MusicSaveGame.h`
- `Source/MusicManager/Private/MusicSaveSubsystem.cpp`
- `current.md`

## Slice 9 - Market Exposure Snapshot

### Prompt

Persist market exposure state for artists and records so market momentum and radio/exposure effects survive save/load.

### User-Facing Outcome

Regional artist and record exposure does not reset when the player reloads a campaign.

### Implementation Scope

- Add market snapshot structures for:
  - region artist exposure
  - region record exposure
  - any additional mutable market trend state currently present
- Add `UMarketManagerSubsystem` save/load snapshot methods.
- Validate:
  - region ids exist in loaded/static region data
  - artist ids are known
  - record ids are known
  - exposure values are finite and non-negative or within the intended valid range
- Ensure static region/segment data is loaded before applying market snapshots.

### Acceptance Criteria

- Market exposure survives save/load.
- Invalid region/artist/record references fail validation.
- Static market data remains DataTable-driven and is not serialized redundantly.
- `current.md` is updated.

### Primary Files

- `Source/MusicManager/Public/MarketManagerSubsystem.h`
- `Source/MusicManager/Private/MarketManagerSubsystem.cpp`
- `Source/MusicManager/Public/MusicSaveGame.h`
- `Source/MusicManager/Private/MusicSaveSubsystem.cpp`
- `current.md`

## Slice 10 - News Summary And Processed Key Snapshot

### Prompt

Persist news summaries, processed news keys, and any pending batch news state needed to avoid duplicate or missing news after load.

### User-Facing Outcome

The news feed state and monthly news history remain consistent after loading a campaign.

### Implementation Scope

- Add event/news snapshot structures for:
  - monthly news summaries
  - processed news keys
  - closed monthly news keys
  - pending batch news events if they should survive save/load
- Add `UEventSubsystem` save/load snapshot methods.
- Validate:
  - summary keys are non-empty and unique
  - generated news ids are valid
  - news event ids are unique where full events are persisted
  - event timestamps are valid
  - news types are valid enum values
- Decide whether currently visible news cards are presentation state or simulation state. Persist only simulation state unless UI state is explicitly needed.

### Acceptance Criteria

- Monthly news summaries survive save/load.
- Processed news keys survive save/load.
- Loading and advancing time does not duplicate already generated monthly news.
- Invalid news data fails validation.
- `current.md` is updated.

### Primary Files

- `Source/MusicManager/Public/EventSubsystem.h`
- `Source/MusicManager/Private/EventSubsystem.cpp`
- `Source/MusicManager/Public/MusicSaveGame.h`
- `Source/MusicManager/Private/MusicSaveSubsystem.cpp`
- `current.md`

## Slice 11 - Cross-Subsystem Save Validation And Atomic Load

### Prompt

Add final cross-subsystem validation and make load application atomic enough that broken cross-references fail before live state is mutated.

### User-Facing Outcome

Broken saves fail cleanly with actionable log errors, and valid saves restore the complete existing management loop.

### Implementation Scope

- Add cross-reference validation in `UMusicSaveSubsystem` or a dedicated save validator:
  - contracts reference known artists/player label
  - songs reference known artists
  - records reference known artists, songs, and labels
  - sales entries reference known records and markets
  - finance entries reference known labels and records/contracts where applicable
  - market exposure references known regions, artists, and records
  - news references valid ids/timestamps
- Use a two-phase load:
  - validate all snapshots
  - apply snapshots in dependency order only after validation passes
- Suggested apply order:
  1. static data availability check
  2. player label
  3. time
  4. songs/artists/contracts
  5. records/sales
  6. finance
  7. market
  8. news
  9. UI rebuild/refresh
- Ensure failed validation does not partially apply state.
- Add clear logging for each validation failure.

### Acceptance Criteria

- A save with a bad record song id fails before applying any live state.
- A save with a bad finance label id fails before applying any live state.
- A save with a bad market region id fails before applying any live state.
- A valid save restores the full existing loop:
  - signed artist
  - songs
  - created record
  - sales history
  - finance ledger/balance
  - market exposure
  - news summary
  - current date
- `current.md` is updated.

### Primary Files

- `Source/MusicManager/Public/MusicSaveSubsystem.h`
- `Source/MusicManager/Private/MusicSaveSubsystem.cpp`
- all subsystem snapshot files from prior slices
- `current.md`

## Slice 12 - Automated Save/Load Regression Coverage

### Prompt

Add automated coverage for the expanded persistence system using real subsystem data and validation failure cases.

### User-Facing Outcome

The save/load system has regression protection against losing campaign state or accepting broken references.

### Implementation Scope

- Add Unreal automation tests or the existing repo test pattern if one exists.
- Cover at minimum:
  - successful round-trip for time/player label/songs/contracts/records/sales/finance/market/news
  - invalid version rejection
  - duplicate song id rejection
  - bad record song reference rejection
  - bad finance label reference rejection
  - bad market region reference rejection
  - no partial mutation on failed validation
- Use real subsystem APIs and real data structures.
- Do not rely on mock gameplay data. Build minimal valid in-memory state through production APIs.

### Acceptance Criteria

- Tests compile.
- Tests fail before the fix if the relevant validation is removed.
- Tests can be run from terminal or documented Unreal automation command.
- `current.md` is updated with test coverage status.

### Primary Files

- likely `Source/MusicManager/Private/Tests/...`
- `Source/MusicManager/MusicManager.Build.cs` if test dependencies are needed
- save/subsystem files touched in prior slices
- `current.md`

## Slice 13 - Save Version Migrations

### Prompt

Implement real save schema migrations between supported versions before validation, so old save objects are upgraded into the current schema instead of only being accepted or rejected by version number.

### User-Facing Outcome

Older campaign saves continue to load when their data can be migrated safely, and failed migrations produce clear validation errors without mutating live game state.

### Implementation Scope

- Add a migration entry point on `UMusicSaveGame` or `UMusicSaveSubsystem`.
- Support sequential migrations from every minimum supported version to `CurrentSaveVersion`.
- Migrate legacy fields into explicit snapshots where possible:
  - legacy saved game date into `TimeSnapshot`
  - legacy contracts into artist snapshots
  - legacy money into the player label finance account
- Preserve created/last-saved metadata when present.
- Log migration warnings and errors through the save/load path.
- Reject unknown future versions and unsupported older versions cleanly.

### Acceptance Criteria

- A supported older save is upgraded to `CurrentSaveVersion` before snapshot validation.
- Migration failure aborts load before live state is applied.
- Current-version saves are not modified except for normal validation.
- Automated tests cover at least one successful migration and one unsupported version.
- `current.md` is updated.

### Primary Files

- `Source/MusicManager/Public/MusicSaveGame.h`
- `Source/MusicManager/Private/MusicSaveGame.cpp`
- `Source/MusicManager/Public/MusicSaveSubsystem.h`
- `Source/MusicManager/Private/MusicSaveSubsystem.cpp`
- `Source/MusicManager/Private/Tests/...`
- `current.md`

## Slice 14 - Save Slot Metadata, Autosaves, Backups, And Thumbnail References

### Prompt

Add a production-ready save slot management surface for future save/load UI, including slot descriptors, autosave slots, rolling backups, and thumbnail asset references.

### User-Facing Outcome

The game can present real save slots with campaign metadata, dates, labels, validation status, thumbnails, autosaves, and backups instead of hardcoded or placeholder save entries.

### Implementation Scope

- Add a saved slot registry that tracks real save slots written by the game.
- Add a Blueprint-visible slot descriptor with:
  - slot name
  - display label
  - player label name
  - in-game date
  - created/last saved timestamps
  - save version
  - autosave/backup flags
  - thumbnail soft object path
  - validation status and validation messages
- Add APIs for:
  - listing save slots
  - saving a normal slot
  - autosaving
  - creating backups before overwriting a slot
  - loading a selected slot
  - deleting a slot from the registry when a delete API exists
- Do not create a shipping GUI widget until the required OpenAI Image 2 reference-image workflow is performed.

### Acceptance Criteria

- Slot descriptors are generated from real save data.
- Autosave writes to a deterministic autosave slot.
- Overwriting a manual slot creates a backup descriptor from the previous save.
- Slot list does not require mock data.
- Thumbnail fields store production asset references, not temporary screenshots.
- `current.md` is updated.

### Primary Files

- `Source/MusicManager/Public/MusicSaveGame.h`
- `Source/MusicManager/Private/MusicSaveGame.cpp`
- `Source/MusicManager/Public/MusicSaveSubsystem.h`
- `Source/MusicManager/Private/MusicSaveSubsystem.cpp`
- `current.md`

## Slice 15 - Reserved Persistence Buckets For Future Systems

### Prompt

Add explicit validated save snapshot buckets for future professional-grade systems: tours, awards, critics, rivals, staff, unlocks, and settings/UI state.

### User-Facing Outcome

The save schema has stable, validated locations for future campaign systems so those features can be added without another disruptive save format rewrite.

### Implementation Scope

- Add typed snapshot structs for:
  - tours
  - awards
  - critic reviews
  - rival labels
  - staff
  - unlocks
  - settings/UI state
- Persist empty snapshots when the owning systems do not exist yet.
- Validate any populated entries for stable ids, valid dates, finite numeric values, and legal ranges.
- Do not invent gameplay behavior, fake records, mock entries, or placeholder subsystem state.

### Acceptance Criteria

- Current saves include the future-system snapshot buckets.
- Empty future snapshots validate cleanly.
- Invalid populated future snapshot entries fail validation.
- Existing load behavior remains atomic.
- `current.md` is updated.

### Primary Files

- `Source/MusicManager/Public/MusicSaveGame.h`
- `Source/MusicManager/Private/MusicSaveGame.cpp`
- `Source/MusicManager/Private/MusicSaveSubsystem.cpp`
- `current.md`

## Slice 16 - Automated Persistence Round-Trip Tests

### Prompt

Add automated save/load round-trip tests that exercise real save objects, migrations, slot descriptors, future snapshot validation, and broken-reference rejection.

### User-Facing Outcome

Persistence has regression coverage for schema migration, slot metadata, and validation failures before new professional-grade systems build on it.

### Implementation Scope

- Add Unreal automation tests following the repository's existing test pattern.
- Cover:
  - save object migration from an older supported version
  - unsupported future version rejection
  - slot descriptor generation from real save metadata
  - invalid future snapshot validation
  - duplicate song id validation
  - bad record song reference rejection where subsystem validation is available
- Use production structs and APIs only.
- Avoid mock save slots or fake UI entries.

### Acceptance Criteria

- Tests compile.
- Tests run from terminal through `UnrealEditor-Cmd`.
- Tests fail if migration or validation is removed.
- `current.md` is updated with test coverage status.

### Primary Files

- `Source/MusicManager/Private/Tests/PersistenceTests.cpp`
- `Source/MusicManager/Public/MusicSaveGame.h`
- `Source/MusicManager/Private/MusicSaveGame.cpp`
- `Source/MusicManager/Public/MusicSaveSubsystem.h`
- `Source/MusicManager/Private/MusicSaveSubsystem.cpp`
- `current.md`

## Suggested Execution Order

1. Slice 1 - Save schema versioning and validation result foundation
2. Slice 2 - Player label snapshot
3. Slice 3 - Time snapshot validation
4. Slice 4 - Song snapshot and usage validation
5. Slice 5 - Artist and contract snapshot validation
6. Slice 6 - Record snapshot and label ownership
7. Slice 7 - Record sales history and lifetime units snapshot
8. Slice 8 - Finance snapshot
9. Slice 9 - Market exposure snapshot
10. Slice 10 - News summary and processed key snapshot
11. Slice 11 - Cross-subsystem save validation and atomic load
12. Slice 12 - Automated save/load regression coverage
13. Slice 13 - Save version migrations
14. Slice 14 - Save slot metadata, autosaves, backups, and thumbnail references
15. Slice 15 - Reserved persistence buckets for future systems
16. Slice 16 - Automated persistence round-trip tests

## Definition Of Done For The Full Persistence Expansion

- `UMusicSaveGame` has explicit versioning.
- Save/load covers time, player label, songs, contracts/artists, records, sales history, finance, market exposure, and news summaries/processed keys.
- Load validation catches broken references before live state is mutated.
- Existing management loop state survives a save/load round trip.
- Placeholder label ownership is removed from record creation.
- Tests or documented verification cover success and failure paths.
- `current.md` accurately describes the implemented persistence coverage.
