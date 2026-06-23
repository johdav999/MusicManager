# Goal
Implement backlog task **TASK-7.2.7 — Include migration support for old save data if present** for story **ST-102 Stable entity ID standardization**.

Add backward-compatible save loading so older save files that still use legacy name-based or mixed references can be loaded and migrated into the current stable-ID-based model before live subsystem state is rebuilt.

The implementation must:
- detect older save versions or legacy save shapes
- migrate legacy entity references to stable IDs where possible
- preserve campaign continuity without silently corrupting data
- log clear warnings/errors for unresolved, ambiguous, or duplicate mappings
- keep migration logic isolated from gameplay subsystem business logic
- avoid trusting legacy raw asset paths or UI/transient state

No explicit acceptance criteria were provided for this task, so align to the architecture and story notes for:
- stable IDs for all gameplay entities
- compatibility handling for mixed name/ID references
- validation logs for unresolved or duplicate IDs
- migration support for old save data if present

# Scope
In scope:
- Inspect current save/load pipeline and identify current save versioning support.
- Introduce or extend save schema version metadata if missing.
- Add migration path(s) from legacy save data to current stable-ID save data.
- Support migration of legacy references for at least:
  - artists
  - songs/song usage
  - records/releases
  - contracts
  - tours
  - labels/events if present in save schema
- Add validation after migration and before applying state to live subsystems.
- Add structured logging under the save/load log category.
- Add automated coverage for at least one legacy-to-current migration path if a test project exists; otherwise add the smallest practical validation harness or documented test seam.

Out of scope:
- broad refactors unrelated to save/load
- changing current gameplay rules
- UI changes beyond surfacing load failure if already supported
- introducing cloud save support
- serializing new transient UI state
- rewriting all save schemas if a targeted migration layer is sufficient

Implementation constraints:
- Prefer additive, minimal-risk changes.
- Do not deserialize legacy data directly into live subsystem state without migration + validation.
- Resolve entities through stable IDs and registries/maps, not display names after migration completes.
- If exact migration is impossible, fail safely with explicit logs and a clear load error rather than inventing bad state.

# Files to touch
Inspect and update the actual equivalents in the repo. Likely candidates include save/load, snapshot, and entity model files.

Primary targets:
- `README.md` if save versioning/migration behavior is documented there
- Save subsystem classes, likely Unreal C++ equivalents such as:
  - `*MusicSaveSubsystem*`
  - `*SaveGame*`
  - `*SaveManager*`
  - `*LoadManager*`
- Save snapshot structs/classes:
  - campaign snapshot
  - artist snapshot
  - contract snapshot
  - song usage snapshot
  - record snapshot
  - tour snapshot
  - event/news snapshot
- Stable ID model/utility files:
  - entity ID helpers
  - lookup/index builders
  - validation utilities
- Logging definitions:
  - `LogMusicSave` category or equivalent
- Tests:
  - existing unit/integration test project if present
  - otherwise add a focused migration test in the nearest existing test location

Also inspect for legacy reference fields such as:
- `Name`
- `ArtistName`
- `SongTitle`
- `RecordTitle`
- `SelectedArtist`
- string keys used as dictionary/map keys
- mixed `Id` + display-name fallback logic

# Implementation plan
1. **Discover the current save pipeline**
   - Find the authoritative save entry point and load flow.
   - Identify:
     - current save object/schema
     - version field presence/absence
     - where subsystem snapshots are serialized/deserialized
     - where validation currently happens, if anywhere
   - Document the effective current version in code comments/constants.

2. **Define explicit save versioning**
   - Add or normalize a `SaveVersion` field on the root save object if not already present.
   - Introduce named version constants, e.g.:
     - `InitialVersion`
     - `StableIdVersion`
     - `CurrentVersion`
   - Treat missing version as legacy version `0` or equivalent.
   - Ensure newly written saves always persist the current version.

3. **Create a migration layer isolated from subsystem logic**
   - Add a dedicated migration utility/service, e.g.:
     - `FMusicSaveMigration`
     - `UMusicSaveMigrationLibrary`
     - or equivalent existing pattern
   - Responsibilities:
     - inspect loaded raw save data
     - apply sequential migrations from old version to current version
     - return migrated snapshot + warnings/errors
   - Keep migration code out of artist/record/finance subsystem business logic.

4. **Support legacy reference resolution**
   - Build migration-time lookup tables from available save content, for example:
     - artist name -> artist stable ID
     - record title + artist context -> record stable ID
     - contract legacy references -> artist/label IDs
     - song legacy identifiers -> song definition ID
   - Prefer exact deterministic mappings.
   - If multiple matches exist:
     - log warning/error with enough context
     - mark migration failure for fatal ambiguity where state integrity would be compromised
   - If a legacy object lacks a stable ID but can be uniquely identified:
     - generate/assign a stable ID if architecture permits for mutable entities
     - update all internal references consistently
   - If static content references are involved:
     - resolve through cooked content IDs/registries only
     - never trust arbitrary legacy asset paths

5. **Migrate root and nested snapshots**
   - For each mutable entity snapshot type present in the save:
     - ensure stable ID field exists/populates
     - migrate legacy cross-references to stable IDs
     - stop using display names as authoritative keys
   - Typical examples:
     - artist snapshots: ensure `ArtistId`
     - contracts: replace `ArtistName`/legacy label refs with `ArtistId`/`LabelId`
     - records: replace artist/title-based links with `RecordId`, `ArtistId`, `SongDefIds`
     - song usage: replace title/name references with `SongDefId` and usage IDs if applicable
     - tours/shows/events: replace artist/record references with stable IDs
   - If maps are keyed by names, convert them to arrays or ID-keyed maps as appropriate for the current schema.

6. **Add post-migration validation**
   - Before applying migrated data to live state, validate:
     - all required IDs are present
     - all references resolve
     - no duplicate IDs for same entity type
     - no duplicate active contracts for one artist
     - no impossible dates introduced by migration
     - no invalid ranges caused by malformed legacy data
   - Categorize issues:
     - recoverable: log warning and apply safe default/fallback
     - fatal: abort load with clear error
   - Ensure load path does not partially apply invalid migrated state.

7. **Integrate migration into load flow**
   - Update load sequence to:
     1. deserialize raw save
     2. detect version
     3. migrate to current schema
     4. validate migrated snapshot
     5. rebuild subsystem state
     6. rebuild UI after successful state restore
   - Ensure failed migration/validation returns a clear failure result and does not mutate live campaign state.

8. **Add observability**
   - Use `LogMusicSave` or equivalent for:
     - detected legacy version
     - each migration step applied
     - counts of migrated entities by type
     - unresolved/ambiguous references
     - final success/failure summary
   - Keep logs actionable and specific.

9. **Add tests**
   - If automated tests exist:
     - add at least one migration test fixture representing an old save with legacy name-based references
     - assert migration produces stable IDs and valid cross-references
     - add one failure-path test for ambiguous or unresolved legacy references if practical
   - If no test harness exists:
     - add the smallest possible deterministic test seam around migration functions
     - document manual verification steps in code comments or README if needed

10. **Preserve forward behavior**
   - Confirm newly saved campaigns write only current stable-ID schema.
   - Do not keep writing legacy compatibility fields unless already required for a staged rollout.
   - If temporary dual-read support is needed, keep it read-only and clearly marked for later removal.

# Validation steps
1. **Code inspection**
   - Verify the load path always routes through migration + validation before subsystem reconstruction.
   - Verify save path writes `CurrentVersion`.

2. **Build**
   - Run:
     - `dotnet build`
   - If there are tests:
     - `dotnet test`

3. **Legacy save migration test**
   - Create or use a representative old save payload with:
     - missing `SaveVersion` or old version
     - legacy artist name references
     - at least one record/contract/tour reference using names instead of IDs
   - Load it through the migration path.
   - Confirm:
     - stable IDs are populated
     - cross-references now use IDs
     - migrated save validates successfully
     - no live state is applied before migration completes

4. **Ambiguity/failure test**
   - Use a legacy save where a name-based reference maps to multiple possible entities or none.
   - Confirm:
     - warning/error logs are emitted
     - fatal ambiguity aborts load safely
     - no partial campaign state is applied

5. **Round-trip test**
   - Load a migrated legacy save, then save again.
   - Confirm the rewritten save:
     - uses current version
     - no longer depends on legacy name-based references
     - can be loaded again without invoking legacy migration logic

6. **Regression check on current saves**
   - Load a current-version save.
   - Confirm no migration changes are applied unnecessarily and behavior remains unchanged.

7. **Logging verification**
   - Confirm `LogMusicSave` output clearly shows:
     - detected version
     - migration steps
     - validation result
     - failure reason when applicable

# Risks and follow-ups
- **Risk: ambiguous legacy name matching**
  - Multiple artists/records may share names or titles.
  - Mitigation: require deterministic context-based matching; fail safely when ambiguous.

- **Risk: hidden legacy references**
  - Some old fields may exist in nested snapshots or map keys not obvious at first glance.
  - Mitigation: audit all serialized structs and search for display-name-based keys/usages.

- **Risk: partial migration corrupting state**
  - Applying migrated data incrementally to live subsystems can leave the campaign inconsistent.
  - Mitigation: migrate and validate into an isolated snapshot first, then apply atomically.

- **Risk: generated IDs breaking external assumptions**
  - If new IDs are generated for legacy mutable entities, all internal references must be updated consistently.
  - Mitigation: centralize ID assignment and reference remapping in one migration pass.

- **Risk: static content mismatch**
  - Old saves may reference songs/content no longer present.
  - Mitigation: resolve only through current registries; treat missing required content as validation failure.

Follow-ups:
- Add more migration fixtures for each historical save version once actual legacy formats are identified.
- Consider exporting a developer-only migration report for troubleshooting.
- If current code still writes mixed name/ID compatibility fields, schedule cleanup after migration support is proven.
- Add explicit user-facing load error messaging if the current UI only logs failures silently.