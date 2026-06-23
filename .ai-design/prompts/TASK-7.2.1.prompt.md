# Goal
Implement `TASK-7.2.1` for `ST-102 Stable entity ID standardization` by replacing display-name-keyed references with stable IDs for core gameplay entities: artists, songs, records, contracts, tours, labels, and events.

The implementation should ensure:
- Stable IDs are the canonical keys in runtime state, save/load state, and cross-subsystem APIs.
- Display names remain presentation-only.
- Existing mixed name/ID usage is migrated with compatibility handling where practical.
- Validation/logging detects unresolved, missing, or duplicate IDs.
- Changes fit the existing Unreal/C++ architecture and do not introduce widget-owned business state.

# Scope
In scope:
- Audit current entity models and subsystem APIs for any use of display names as keys or foreign references.
- Introduce/standardize stable ID fields and key types for:
  - `ArtistId`
  - `SongId` / `SongDefId`
  - `RecordId`
  - `ContractId`
  - `TourId`
  - `LabelId`
  - `EventId`
- Refactor maps, lookup tables, and relationship fields to use stable IDs instead of names.
- Update serialization/save-facing structs to persist IDs, not display names, for identity and references.
- Add compatibility migration/shims where old name-based references may still exist.
- Add validation and logging for duplicate IDs, unresolved references, and fallback migration behavior.

Out of scope unless required to complete compilation:
- Large UI redesigns.
- New gameplay features beyond ID standardization.
- Full command bus implementation.
- Broad save migration framework beyond minimal compatibility needed for this task.
- Refactoring unrelated systems that already use stable IDs correctly.

# Files to touch
Inspect first, then update the smallest correct set. Likely targets include:

- `README.md` only if there is a developer note or migration note already maintained there.
- Core gameplay module headers/cpps under the main source tree, especially:
  - entity/state structs for artists, songs, records, contracts, tours, labels, events
  - subsystem classes managing those entities
  - save/load snapshot structs and serializers
  - validation helpers
  - logging category definitions
- Any DataTable/DataAsset row structs that currently rely on names as identity.
- Any utility or registry classes that resolve entities by name.
- Any tests covering save/load, entity lookup, or subsystem references.

Because the workspace listing is shallow, first discover the actual source layout and then touch only relevant files. Prefer likely locations such as:
- `Source/**`
- `Source/**/Public/**`
- `Source/**/Private/**`
- test projects if present

# Implementation plan
1. **Discover current model and usage**
   - Search the codebase for likely name-key anti-patterns:
     - maps keyed by `Name`, `ArtistName`, `SongName`, `RecordName`, etc.
     - APIs like `GetArtistByName`, `FindRecordByTitle`, `TMap<FString, ...>`, `TMap<FName, ...>` where the key is actually an entity display name
     - save structs storing names as foreign references
   - Build a short internal inventory of:
     - entity structs/classes
     - owning subsystems
     - save/load structs
     - compatibility-sensitive call sites

2. **Define canonical ID approach**
   - Reuse existing ID types if already present.
   - If no shared convention exists, standardize on the project’s current practical type for serialized IDs, preferring the least disruptive stable type already in use:
     - existing `FGuid`, or
     - stable `FString`/`FName` ID fields used as opaque identifiers
   - Do **not** use display names as IDs.
   - Keep naming explicit, e.g. `ArtistId`, `RecordId`, `ContractId`.

3. **Update entity models**
   - Ensure each relevant entity has:
     - a stable ID field
     - a separate display field such as `Name`/`Title`
   - Replace foreign-reference fields that currently store names with ID fields.
   - Where needed for compatibility, temporarily retain deprecated legacy name fields with comments and migration handling.

4. **Refactor subsystem storage and lookup**
   - Change canonical registries/maps to be keyed by stable IDs.
   - Add or update lookup helpers such as:
     - `FindArtistById`
     - `FindSongById`
     - `FindRecordById`
   - If UI or legacy code still needs name lookup, keep it as a non-canonical helper layered on top of ID-backed storage and clearly mark it as compatibility-only.

5. **Refactor cross-subsystem APIs**
   - Update method signatures, command payloads, event payloads, and internal calls to pass IDs consistently.
   - Remove mixed usage where one subsystem accepts names and another accepts IDs for the same entity.
   - Ensure event payloads and save-facing references use IDs.

6. **Handle save/load compatibility**
   - Update save snapshot structs to serialize stable IDs for identity and relationships.
   - If old save fields or legacy runtime data may still contain names:
     - add migration/compatibility resolution during load
     - resolve legacy names to IDs where possible
     - log warnings when fallback resolution is used
     - fail clearly when resolution is impossible and the reference is required
   - Never treat raw display names as canonical after load.

7. **Add validation and logging**
   - Add validation checks for:
     - duplicate entity IDs
     - missing/empty IDs
     - unresolved foreign references
     - legacy name fallback collisions
   - Use existing log categories if present; otherwise add targeted logs in the appropriate gameplay/save categories.
   - Keep logs actionable and include entity type plus offending ID/name.

8. **Minimize UI breakage**
   - Preserve display behavior by continuing to expose names/titles for presentation.
   - Only update UI-facing code where compilation requires it, ensuring widgets consume IDs for selection/navigation and names for display.

9. **Add/adjust tests**
   - Add or update automated coverage where the project already has tests.
   - Prioritize:
     - entity lookup by ID
     - save/load reference reconstruction
     - migration from legacy name-based references if test infrastructure allows
     - duplicate/unresolved ID validation behavior

10. **Document assumptions in code comments**
   - Mark any temporary compatibility fields or fallback paths as transitional.
   - Add concise comments where a display field must never be used as a key.

# Validation steps
1. **Codebase audit validation**
   - Confirm all core entities in scope have explicit stable ID fields.
   - Confirm display names/titles are no longer used as canonical map keys or foreign references.

2. **Build**
   - Run:
     - `dotnet build`
   - If there are test projects:
     - `dotnet test`

3. **Static/manual verification**
   - Search for remaining problematic patterns:
     - `ByName`
     - `ArtistName` used as key/reference
     - `SongName` used as key/reference
     - `RecordName` used as key/reference
     - `ContractName`
     - `TourName`
     - `LabelName`
     - `EventName`
     - `TMap<FString, ...>` / `TMap<FName, ...>` in entity registries where the string/name is a display field
   - Verify remaining name usage is presentation-only or explicit compatibility code.

4. **Runtime/data validation**
   - Exercise or simulate load/initialization paths and verify:
     - duplicate IDs emit warnings/errors
     - unresolved references emit warnings/errors
     - legacy name fallback resolves when possible
     - impossible legacy references fail clearly rather than silently corrupting state

5. **Regression checks**
   - Verify existing flows still compile and function conceptually:
     - artist lookup
     - song assignment
     - record ownership/reference
     - contract-to-artist linkage
     - tour-to-artist linkage
     - event references
     - save/load reconstruction

6. **Acceptance alignment**
   - Confirm the implementation satisfies the story intent:
     - artists, songs, records, contracts, tours, labels, and events use stable IDs instead of display names as keys
     - cross-subsystem APIs accept IDs consistently
     - mixed references are migrated or compatibility-handled
     - validation logs unresolved/duplicate IDs

# Risks and follow-ups
- **Risk: hidden name-based coupling**
  - Some systems may implicitly rely on display names for lookup, especially UI glue or older save code.
  - Mitigation: keep narrow compatibility helpers during refactor and log all fallback usage.

- **Risk: save compatibility**
  - Old saves may not contain enough information to resolve ambiguous names.
  - Mitigation: implement best-effort migration, warn on fallback, and fail clearly on ambiguous/unresolvable required references.

- **Risk: partial refactor inconsistency**
  - Mixed ID/name APIs can persist if not fully audited.
  - Mitigation: update canonical subsystem interfaces first, then compile-fix all call sites.

- **Risk: over-standardizing the wrong ID type**
  - Introducing a brand-new ID wrapper everywhere may be too disruptive for this task.
  - Mitigation: prefer the project’s existing stable serialized ID type and standardize usage before introducing abstractions.

Follow-ups after this task:
- Add a shared ID utility/type alias layer if the codebase lacks one.
- Add stronger save migration/versioning coverage under `ST-403`.
- Align command payloads and domain events fully with stable IDs under `ST-103` and `ST-104`.
- Add dedicated automated integrity tests for cross-subsystem reference graphs.