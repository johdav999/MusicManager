# Goal
Implement backlog task **TASK-7.2.6 — Prefer `FGuid`/stable string-like IDs in serialized state** for story **ST-102 Stable entity ID standardization**.

The coding agent should update persistence-facing models and serialization paths so that **serialized mutable state uses stable IDs (`FGuid` where appropriate, otherwise stable string-like IDs such as `FName`/`FString`) instead of display names or transient references**.

This task is specifically about **serialized state correctness and forward compatibility**, not a full gameplay-wide ID refactor unless required to make save/load and snapshot serialization safe and consistent.

Key intent:
- Save data must not depend on display names.
- Serialized references between entities must use stable IDs.
- Existing mixed name/ID save data should be handled with compatibility/migration where feasible.
- Validation/logging should surface unresolved or duplicate IDs.

# Scope
In scope:
- Audit save/snapshot structs and any serialized DTO/state models used by save/load.
- Replace name-keyed or display-name-backed serialized references with stable IDs.
- Prefer:
  - `FGuid` for runtime-generated mutable entities like artists, records, contracts, tours, events, etc.
  - stable string-like IDs (`FName`/`FString`) for static content identifiers where appropriate, such as song definition IDs, region IDs, genre IDs, chart IDs, venue IDs.
- Update serialization/deserialization and reconstruction logic accordingly.
- Add compatibility handling for older serialized data if old fields exist.
- Add validation and logging for:
  - missing IDs
  - duplicate IDs
  - unresolved references during load
- Keep display names as presentation-only fields.

Out of scope unless directly required:
- Large UI refactors.
- Rewriting every subsystem API to IDs if not needed for serialized state.
- Broad command/event pipeline changes.
- Non-persistence gameplay redesign.

# Files to touch
Inspect and modify the actual relevant files in the repo, likely including equivalents of the following:
- Save game classes / snapshot structs
- Save/load subsystem or manager
- Entity state structs for mutable campaign data:
  - artist state
  - contract state
  - record/release state
  - song usage state
  - tour/show state
  - finance references
  - event/news references
- Serialization helpers / migration utilities
- Validation utilities
- Logging category definitions if save/load logging is missing or incomplete
- Automated tests covering save/load or snapshot migration

If present, prioritize files matching patterns like:
- `*Save*`
- `*Snapshot*`
- `*State*`
- `*Subsystem*`
- `*Manager*`
- `*Migration*`
- `*Validation*`

Also inspect:
- any structs using fields like `ArtistName`, `RecordTitle`, `SongTitle`, etc. as keys/references
- any `TMap` keyed by display names in serialized state
- any load logic that resolves entities by name

Do not rename files or move architecture around unless necessary.

# Implementation plan
1. **Audit serialized models**
   - Find all `USTRUCT`/`UCLASS` save-facing models and identify fields that currently serialize:
     - display names as references
     - raw object pointers/soft paths where a stable ID should be used
     - maps keyed by names for mutable entities
   - Build a short internal mapping of:
     - entity type
     - current serialized identifier/reference
     - target stable identifier type

2. **Define ID usage rules**
   - Apply this convention consistently:
     - runtime mutable entities: `FGuid`
       - `ArtistId`
       - `RecordId`
       - `ContractId`
       - `TourId`
       - `ShowId` if persisted
       - `EventId` if persisted
       - `LabelId` if mutable/runtime-defined
     - static content/data-driven entities: stable string-like IDs
       - `SongDefId`
       - `RegionId`
       - `GenreId`
       - `ChannelId`
       - `VenueId`
       - `ChartId`
   - If the codebase already has an established type alias/wrapper, reuse it instead of inventing a new abstraction.

3. **Update save-facing structs**
   - Replace serialized reference fields that use names with stable ID fields.
   - Examples:
     - `ArtistName` used as a foreign key -> `ArtistId`
     - `RecordTitle` used as a reference -> `RecordId`
     - song content references by title/path -> `SongDefId`
   - Preserve display fields only if they are explicitly presentation metadata and not used for lookup.
   - Where backward compatibility is needed, keep deprecated legacy fields temporarily, clearly marked and only used during migration/load fallback.

4. **Update save/load serialization logic**
   - Ensure save writes the stable ID fields.
   - Ensure load reconstructs subsystem state by resolving IDs, not names.
   - Replace any lookup logic like “find artist by name” with “find artist by ID”.
   - For static content, resolve through registries/data tables using stable content IDs only.

5. **Add migration/compatibility handling**
   - If old save versions or legacy fields exist:
     - on load, if new ID field is absent/invalid and legacy name field exists, attempt to resolve once via compatibility mapping
     - populate the new ID field
     - log a warning indicating legacy migration occurred
   - If save versioning exists, hook this into the migration path rather than scattering ad hoc conversions.
   - If no formal migration system exists but legacy fields are present, implement minimal compatibility in load helpers with TODO notes for centralization.

6. **Add validation**
   - During load or snapshot validation, detect and log:
     - invalid/empty `FGuid` where required
     - duplicate IDs in collections that should be unique
     - unresolved foreign-key references
     - static content IDs that do not resolve
   - Treat failures according to existing error-handling patterns:
     - recoverable: warning + skip/default where safe
     - fatal integrity issue: fail load with clear error path if existing architecture supports it

7. **Preserve determinism and minimal churn**
   - Do not change gameplay behavior beyond identifier handling.
   - Avoid introducing random ID generation during load for missing references unless there is an explicit existing migration rule.
   - Prefer failing loudly over silently inventing IDs for broken data.

8. **Add or update tests**
   - Add focused tests for:
     - save/load round-trip preserves stable IDs
     - legacy serialized name-based data migrates to ID-based fields if supported
     - duplicate/unresolved IDs trigger validation failures or warnings as expected
   - Keep tests narrow and persistence-focused.

9. **Document with code comments where needed**
   - Add concise comments on legacy fields/migration paths.
   - Mark deprecated name-based serialized fields for future removal after migration window.

# Validation steps
1. **Static code audit**
   - Confirm serialized mutable entity references no longer rely on display names.
   - Confirm display names are not used as primary keys in save-facing structs.

2. **Build**
   - Run the most appropriate available build command from workspace root:
     - `dotnet build`
   - If there are test projects, also run:
     - `dotnet test`

3. **Round-trip verification**
   - Create or use a representative save/snapshot path and verify:
     - entities serialize with stable IDs
     - cross-references deserialize correctly by ID
     - no display-name lookup is required in the normal path

4. **Legacy compatibility verification**
   - If legacy support is implemented, verify an old/legacy payload with name-based references:
     - loads or migrates successfully when uniquely resolvable
     - emits a warning log
     - rewrites/uses the new ID field internally

5. **Validation/error-path verification**
   - Verify unresolved IDs produce warnings/errors through the save/load log category.
   - Verify duplicate IDs are detected.
   - Verify invalid static content IDs fail resolution safely.

6. **Regression check**
   - Ensure presentation fields like artist/record names still display correctly and are not removed where UI depends on them.
   - Ensure no unrelated subsystem behavior changes.

# Risks and follow-ups
- **Risk: mixed existing conventions**
  - The repo may already use a blend of `FGuid`, `FString`, and names. Avoid over-refactoring; standardize serialized state first.

- **Risk: legacy save ambiguity**
  - Name-based migration may be ambiguous if duplicate display names exist. In that case, do not guess silently; log and fail or skip according to existing integrity rules.

- **Risk: hidden name-based lookups**
  - Some subsystems may still reconstruct relationships by name during load. Audit carefully for indirect helper methods.

- **Risk: static vs mutable ID confusion**
  - Do not convert static content IDs to generated GUIDs. Static content should remain stable authored IDs.

Follow-ups after this task:
- Broaden ID standardization across cross-subsystem APIs if serialized state still depends on adapter shims.
- Introduce stronger typed ID wrappers if the codebase would benefit and churn is acceptable.
- Centralize save migration if compatibility logic is currently scattered.
- Add editor/data validation tooling to catch duplicate or missing IDs before runtime.