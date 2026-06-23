# Goal
Implement `TASK-7.2.4` for `ST-102 Stable entity ID standardization` by adding validation that logs warnings/errors for unresolved or duplicate stable IDs across relevant gameplay/save data paths.

The coding agent should:
- identify where stable entity IDs are defined, loaded, validated, and resolved
- add or extend validation routines so unresolved references and duplicate IDs are detected
- emit clear structured logs using existing or newly added UE log categories aligned with the architecture, especially save/data integrity logging
- keep behavior minimally invasive: validation should improve observability first, without breaking unrelated flows unless an existing fatal validation path already expects failure
- preserve compatibility with current mixed name/ID migration code if present

# Scope
In scope:
- validation for duplicate IDs within entity collections such as artists, songs, records, contracts, tours, labels, events, or equivalent current models
- validation for unresolved cross-references, such as:
  - contract references missing artist/label
  - record references missing artist/song definitions
  - song usage references missing song definitions or record links
  - tour/show/event references missing owning entities
  - save/load snapshot references that cannot be resolved
- warning/error logging for these cases
- reusable validation helpers where appropriate
- tests if the workspace already contains a test project or validation test pattern

Out of scope:
- broad refactors of the entire ID system
- changing all entities to a new ID type unless required for this task
- UI work
- command dispatcher changes
- save migration redesign beyond what is necessary to validate and log
- introducing fatal load failures unless the current validation framework already distinguishes fatal vs recoverable issues

Implementation intent:
- unresolved references should log at least warnings, and errors where the current architecture treats them as integrity failures
- duplicate IDs should log errors because they undermine deterministic lookup and save integrity
- validation should be callable during save/load reconstruction and, where practical, after static content registration

# Files to touch
Inspect the repo first, then update the smallest correct set of files. Likely targets include:

- subsystem/model files that own entity registries or snapshots
- save/load subsystem and save validation code
- any existing ID utility, registry, or resolver classes
- logging category definitions
- tests covering save/load or validation

Probable file patterns to search for:
- `*Save*`
- `*Validation*`
- `*Subsystem*`
- `*Artist*`
- `*Song*`
- `*Record*`
- `*Contract*`
- `*Tour*`
- `*Event*`
- `*Log*`
- `*Types*`
- `*Snapshot*`

Search for these symbols/terms before editing:
- `LogMusicSave`
- `LogMusicArtists`
- `LogMusicRecords`
- `LogMusicTours`
- `LogMusicUI`
- `Validate`
- `Validation`
- `Resolve`
- `FindById`
- `GetById`
- `ArtistId`
- `SongDefId`
- `RecordId`
- `ContractId`
- `TourId`
- `EventId`
- `LabelId`
- `SaveVersion`
- `USaveGame`

If no dedicated validation utility exists, create a focused helper in the most relevant gameplay/save module rather than scattering duplicate logic.

# Implementation plan
1. **Discover current ID and validation flow**
   - Inspect the solution structure and identify whether the active implementation is UE C++, C#, or mixed tooling.
   - Find current entity models and determine how IDs are represented today:
     - `FGuid`
     - `FString`
     - `FName`
     - plain string/name keys
   - Find existing validation entry points:
     - save load
     - subsystem initialization
     - content registration
     - migration code
   - Find existing logging categories and reuse them if available.

2. **Define validation behavior**
   - Add a small validation contract if none exists, e.g. helper methods that:
     - detect duplicate IDs in a collection
     - detect unresolved references against a lookup set/map
     - accumulate counts and emit logs
   - Prefer a reusable pattern like:
     - `ValidateUniqueIds(Collection, EntityTypeName, IdSelector, LogCategory)`
     - `ValidateReference(OwnerType, OwnerId, RefFieldName, RefId, ExistsFn, Severity)`
   - Keep output deterministic and easy to grep.

3. **Add duplicate ID validation**
   - For each relevant entity collection currently loaded or reconstructed, validate uniqueness of the stable ID field.
   - At minimum cover the collections actually present in the codebase for this story.
   - Log an error with enough context to diagnose:
     - entity type
     - duplicate ID
     - conflicting indices/names if available
     - source context such as save slot, table, asset, or subsystem
   - Example log intent:
     - `Error: Duplicate ArtistId 'artist_001' detected in ArtistSnapshot during load.`
   - If the current validation framework returns structured results, append these findings there too.

4. **Add unresolved reference validation**
   - Validate cross-entity references after all relevant collections are loaded into temporary lookup maps, before applying live state where possible.
   - Examples to implement where present in code:
     - contract -> artist
     - contract -> label
     - artist -> contract
     - record -> artist
     - record -> song definitions
     - song usage -> song definition
     - song usage -> record
     - tour -> artist
     - show -> venue/region if modeled
     - event/news -> referenced entity IDs
   - Log warnings or errors based on severity:
     - duplicate IDs: error
     - missing required owner/entity for active gameplay state: error
     - missing optional/back-compat/non-critical references: warning
   - Do not silently drop invalid references unless the current code already uses safe fallback behavior; if it does, log that fallback explicitly.

5. **Integrate into existing load/validation lifecycle**
   - Ensure validation runs in the most useful places:
     - after deserializing save snapshots and migrations
     - before reconstructing subsystem live state
     - after static content catalog registration if song/content IDs are resolved there
   - If there is already a `ValidateSave`, `PostLoad`, or `RebuildState` step, hook into that rather than inventing a parallel path.
   - If validation results are already surfaced to callers, include counts of warnings/errors.

6. **Add or align log categories**
   - Reuse existing categories if present.
   - If missing, add categories consistent with architecture guidance, especially `LogMusicSave` and domain-specific categories as needed.
   - Keep messages concise and structured:
     - what failed
     - which ID
     - which owner/reference field
     - what fallback happened, if any

7. **Add tests**
   - If there is an existing automated test project:
     - add tests for duplicate ID detection
     - add tests for unresolved reference detection
     - add tests that valid data produces no errors
   - If log assertion is awkward, test returned validation results/counters instead of raw log text.
   - If no test harness exists, add the smallest feasible unit-level coverage in the existing pattern and document any gaps.

8. **Keep compatibility and avoid overreach**
   - If old save data still uses names or mixed references, preserve current migration behavior.
   - Validate post-migration IDs rather than removing compatibility shims.
   - Do not convert warnings into fatal failures unless the current load path already treats that condition as fatal.

# Validation steps
1. Inspect and build the workspace:
   - `dotnet build`
   - if tests exist: `dotnet test`

2. Run or execute relevant validation tests for:
   - duplicate entity IDs in a collection
   - unresolved required references
   - unresolved optional references
   - valid snapshot/reference graph

3. Manually verify code paths by constructing or using representative fixtures:
   - a save/snapshot with duplicate `ArtistId`
   - a record referencing a missing `ArtistId`
   - a song usage referencing a missing `SongDefId`
   - a contract referencing a missing `ArtistId` or `LabelId`

4. Confirm logs are emitted through the intended categories and include actionable context.

5. Confirm no regression in normal valid load/initialization flow:
   - valid data still loads/builds cleanly
   - no excessive noisy logging for expected compatibility cases

6. In the final work summary, report:
   - files changed
   - validation entry points added/updated
   - entity/reference types covered
   - tests added and results

# Risks and follow-ups
- **Repo/stack mismatch risk:** workspace hints `.NET`, while architecture is Unreal/C++. First determine the actual implementation layer for this task and work in the real active codepath.
- **Scattered ID logic:** ID handling may be duplicated across subsystems. Prefer a shared helper to avoid inconsistent validation behavior.
- **Mixed legacy references:** old name-based references may still exist. Preserve migration compatibility and validate after normalization.
- **Log spam risk:** avoid repeated per-frame or repeated per-access logs. Validation should run at bounded lifecycle points like load/init.
- **Severity ambiguity:** if the codebase lacks a warning/error distinction in validation results, use logs plus structured counters and keep fatal behavior unchanged unless already established.
- **Coverage gaps:** if some entities from the architecture are not yet implemented in code, validate only what exists and note missing future coverage.

Follow-up recommendations after this task:
- centralize stable ID validation into a shared integrity validator
- add structured validation result objects with warning/error counts
- surface fatal integrity failures to user-facing load errors for `ST-403`
- expand validation to migration paths and static content import tooling