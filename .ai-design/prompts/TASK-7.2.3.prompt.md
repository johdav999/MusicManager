# Goal
Implement `TASK-7.2.3` for `ST-102 Stable entity ID standardization` by migrating existing mixed name/ID references to stable entity IDs, while preserving compatibility where needed so current content, saves, and call sites continue to function during transition.

This task should ensure:
- gameplay/runtime logic uses stable IDs as the authoritative reference
- display names remain presentation-only
- legacy name-based references are resolved through compatibility shims or migration paths
- unresolved or duplicate references are surfaced through validation logs
- save/load compatibility is preserved where practical

# Scope
Focus only on the migration/compatibility slice of stable ID standardization.

In scope:
- identify mixed name/ID references for core entities covered by `ST-102`:
  - artists
  - songs
  - records
  - contracts
  - tours
  - labels
  - events
- convert internal lookup and cross-subsystem references to prefer stable IDs
- add compatibility resolution for legacy name-based references where existing code/data still passes names
- add validation/logging for unresolved, ambiguous, or duplicate legacy references
- add save/load migration or load-time compatibility handling if legacy save fields exist
- keep existing behavior working with minimal breakage

Out of scope unless required to complete compilation:
- broad UI redesign
- command dispatcher work from `ST-103`
- event pipeline cleanup from `ST-104`
- unrelated refactors
- replacing every string in UI routing or display code if it is not used as a business key

Implementation should be incremental and low-risk:
- prefer adapters/shims over sweeping rewrites
- preserve backward compatibility at boundaries
- avoid changing display-facing names or UX text
- do not introduce hidden business logic into widgets

# Files to touch
Start by inspecting and then updating the smallest relevant set of files. Likely targets include:

- `README.md`
  - only if there is a developer-facing note about ID migration or save compatibility worth updating

- Core gameplay/runtime classes in the Unreal project
  - subsystem headers/cpps for artist, song, record, tour, finance, save/load, events, UI manager
  - shared model/struct headers containing entity references
  - save game schema/versioning files
  - validation or logging utility files
  - any registry/index classes that currently key by display name

Likely file categories to search for:
- entity structs with fields like:
  - `Name`
  - `ArtistName`
  - `SongName`
  - `RecordName`
  - `LabelName`
  - `EventName`
  - `SelectedArtist`
  - `CurrentArtist`
- maps keyed by display strings/names
- functions that accept names for lookup instead of IDs
- save serialization code storing names as references
- load code reconstructing relationships from names
- UI-to-subsystem calls passing names into business logic
- DataTable/DataAsset row structs where a stable ID may be missing or not used

Search patterns to use:
- `ArtistName`
- `SongName`
- `RecordName`
- `Contract`
- `TourName`
- `LabelName`
- `EventName`
- `TMap<FString`
- `TMap<FName`
- `FindByName`
- `GetByName`
- `SelectedArtist`
- `CurrentArtist`
- `SaveVersion`
- `Load`
- `Serialize`

If the codebase is not yet in UE C++ and instead contains .NET tooling/tests only, still implement the task in the actual gameplay code if present in the repo; do not force a .NET-only solution for a UE architecture task.

# Implementation plan
1. **Discover current mixed-reference usage**
   - Audit the codebase for all places where entity display names are used as authoritative keys or foreign references.
   - Build a short internal inventory grouped by entity type:
     - authoritative storage keyed by name
     - cross-subsystem API parameters using name
     - save/load fields using name
     - UI/business logic boundaries using name
   - Prioritize runtime integrity paths first:
     - save/load
     - subsystem registries
     - cross-entity references
     - command/business entry points

2. **Define authoritative stable reference fields**
   - For each affected entity model, ensure there is a stable ID field used as the canonical reference.
   - If an entity already has both name and ID, make the ID authoritative and treat the name as display-only.
   - Do not remove legacy name fields if they are still needed for compatibility or UI display.
   - Prefer minimal schema changes that compile cleanly and preserve serialized compatibility.

3. **Add compatibility resolution helpers**
   - Introduce centralized helper logic for resolving legacy references:
     - resolve by ID first
     - if missing, optionally resolve by legacy name
     - detect duplicate name matches and treat as ambiguous
   - Keep this logic in subsystem/registry/save migration layers, not scattered across widgets.
   - Return structured outcomes where possible:
     - resolved by ID
     - resolved by legacy name fallback
     - unresolved
     - ambiguous duplicate
   - Emit warnings when fallback-by-name is used so remaining migration work is visible.

4. **Migrate subsystem lookups to ID-first**
   - Update core subsystem APIs and internal lookups to accept/use stable IDs consistently.
   - Where existing callers still pass names, add temporary overloads/shims or conversion at the boundary.
   - Replace maps keyed by display name with maps keyed by stable ID where feasible.
   - If a name-keyed index is still useful for compatibility/search, keep it as a secondary non-authoritative index.

5. **Handle legacy save/load compatibility**
   - Inspect save schema/versioning.
   - If old save data may contain name-based references:
     - add migration logic for older versions, or
     - add load-time compatibility resolution that upgrades references into IDs before applying live state
   - Preserve old fields only as long as needed for migration.
   - Ensure load validation logs:
     - unresolved legacy references
     - duplicate name collisions
     - invalid/missing IDs
   - Do not silently bind to the wrong entity on duplicate names.

6. **Add validation and logging**
   - Use existing log categories if present, especially save/data integrity categories.
   - Add warnings/errors for:
     - fallback resolution from name to ID
     - unresolved references
     - duplicate/ambiguous names
     - missing canonical IDs on live entities
   - Keep logs actionable and include entity type, legacy value, and context.

7. **Preserve compatibility at UI/business boundaries**
   - If UI or older code paths still pass names, adapt them at the boundary rather than letting names flow deeper into business logic.
   - Avoid changing user-facing display behavior.
   - Keep any string command IDs only for routing, not entity identity.

8. **Add/adjust tests**
   - Add focused automated coverage for the migration behavior where test infrastructure exists.
   - At minimum cover:
     - ID lookup success
     - legacy name fallback success
     - duplicate-name ambiguity failure
     - unresolved legacy reference warning/failure path
     - legacy save migration/load compatibility
   - Prefer small unit tests around resolver/migration logic over broad integration rewrites.

9. **Document assumptions in code comments**
   - Add concise comments where compatibility shims exist, noting they are transitional and ID-first is authoritative.
   - If save migration version changes, document the migration intent clearly.

Implementation guidance:
- favor surgical changes over broad renames
- avoid introducing new global state
- do not key business state by display names after this change
- if duplicate display names are currently possible, treat that as expected and unsafe for authoritative lookup
- if a stable ID is missing for some entity type, add one in the least disruptive way consistent with existing serialization

# Validation steps
Run the most relevant validation available in the workspace after implementation.

1. **Code search validation**
   - Re-scan for obvious business-critical name-keyed references and confirm they are either:
     - removed
     - converted to ID-first
     - explicitly marked compatibility-only

2. **Build**
   - Run the project build command(s) that succeed for the workspace.
   - Start with:
     - `dotnet build`
   - If there are test projects:
     - `dotnet test`
   - If Unreal build tooling is available in the repo/environment, also use the appropriate project build path for the gameplay module.

3. **Behavioral validation**
   - Verify these scenarios in code/tests:
     - entity resolves directly by stable ID
     - legacy name reference resolves when uniquely matched
     - duplicate display names do not silently resolve to an arbitrary entity
     - unresolved legacy references produce warnings/errors
     - save/load upgrades or resolves legacy references correctly

4. **Regression checks**
   - Confirm display names still appear correctly in UI-facing projections/data
   - Confirm no subsystem now depends on display name as a primary key
   - Confirm compatibility shims do not bypass validation

5. **Logging validation**
   - Ensure warnings/errors are emitted through the appropriate log category and include enough context to diagnose bad data

# Risks and follow-ups
- **Risk: duplicate display names already exist**
  - Name fallback may become ambiguous. Do not auto-pick one. Log and fail safely.

- **Risk: hidden name-based coupling in UI or save code**
  - Some flows may still compile but behave incorrectly if they assume names are authoritative. Audit save/load and selection-heavy flows carefully.

- **Risk: partial migration leaves mixed semantics**
  - If some subsystems are ID-first and others still mutate by name, subtle bugs can remain. Prefer boundary shims and centralized resolution.

- **Risk: save compatibility edge cases**
  - Older saves may lack enough information to resolve references uniquely. Handle with explicit validation and user-facing load failure where necessary rather than corrupting state.

- **Risk: over-scoped refactor**
  - Keep this task focused on compatibility migration, not full architecture cleanup.

Follow-ups after this task:
- remove temporary name-based compatibility shims once all callers and save versions are migrated
- add stronger automated save migration coverage if current test support is limited
- standardize typed entity reference/value objects if the codebase currently uses raw strings everywhere
- align remaining cross-subsystem APIs under the broader `ST-102` acceptance criteria so all business interfaces are consistently ID-based