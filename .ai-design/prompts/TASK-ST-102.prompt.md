# Goal
Implement backlog task **TASK-ST-102 — Stable entity ID standardization** for the Unreal-based MusicManager project.

The coding agent should standardize gameplay entity identity usage so that **stable IDs** are the authoritative keys across simulation, persistence, and cross-subsystem APIs, while **display names remain presentation-only**.

This task maps to story **ST-102 — Stable entity ID standardization** and must satisfy these backlog acceptance criteria:

- Artists, songs, records, contracts, tours, labels, and events use stable IDs instead of display names as keys.
- Cross-subsystem APIs accept IDs consistently.
- Existing mixed name/ID references are migrated with compatibility handling where needed.
- Validation logs warnings/errors for unresolved or duplicate IDs.

No explicit acceptance criteria were provided on the task itself, so implement to the story and architecture intent above.

# Scope
Focus only on the minimum coherent implementation needed to establish stable entity IDs as a project-wide standard in the current codebase.

In scope:
- Audit current entity models, subsystem APIs, save/load structures, and lookup containers for name-keyed or mixed key usage.
- Introduce or normalize stable ID fields for core entities:
  - ArtistId
  - SongDefId / SongUseId if applicable
  - RecordId
  - ContractId
  - TourId
  - LabelId
  - EventId
- Refactor internal maps, registries, and lookup helpers to key by stable IDs instead of display names where practical.
- Update subsystem/public APIs to accept IDs consistently.
- Preserve display names as non-authoritative UI fields only.
- Add compatibility/migration shims where old code still passes names.
- Add validation and logging for:
  - missing IDs
  - duplicate IDs
  - unresolved references
  - legacy name-based fallback resolution
- Update save/load serialization paths to persist and restore stable IDs correctly.
- Add or update tests for ID-based lookup, migration fallback, and validation behavior.

Out of scope unless required to complete compilation:
- Large UI redesigns
- New gameplay features
- Full command bus implementation
- Broad save version migration framework beyond what is necessary for this story
- Refactoring unrelated systems that do not participate in entity identity or references

Implementation guidance:
- Prefer **stable string-like IDs / `FName` / `FGuid`-backed fields** based on existing code patterns in the repo.
- Do not over-engineer a generic identity framework if a lightweight project-specific approach is sufficient.
- If the current codebase already has partial ID support, consolidate around that instead of introducing a second pattern.

# Files to touch
Start by auditing and then modify the relevant files you actually find. Expect to touch files in these categories:

- **Entity/state model headers and cpp files**
  - structs/classes representing artists, songs, records, contracts, tours, labels, events
- **Subsystems**
  - artist manager
  - song catalog
  - production/record manager
  - tour manager
  - finance/save/load/event systems if they store references
- **Save/load**
  - `USaveGame` models
  - serialization/deserialization helpers
  - validation/migration code
- **UI/view models**
  - only where they still pass names as authoritative references
- **Tests**
  - unit/integration tests covering lookup and migration behavior
- **Logging**
  - add or extend UE log categories if needed for validation output

Given the workspace context, first inspect:
- `README.md`
- solution/project structure under `MusicManager.sln`
- source folders for Unreal module code
- any save model, subsystem, or gameplay state files
- any existing tests reachable from `dotnet test`

If there is a mixed Unreal/.NET tooling setup, use the .NET commands only for whatever test/build coverage exists in this workspace, but prioritize the actual source-of-truth gameplay code.

# Implementation plan
1. **Audit current identity usage**
   - Search the codebase for:
     - maps keyed by `Name`, `FString`, or display title
     - methods like `GetArtistByName`, `FindRecordByTitle`, etc.
     - save structs storing names as references
     - UI calls that pass names into business logic
   - Produce a concise internal checklist of all affected entity types and call sites before editing.

2. **Choose and normalize the stable ID type**
   - Reuse the project’s existing ID representation if one already exists.
   - Otherwise prefer a lightweight Unreal-friendly type:
     - `FName` or `FString` for serialized stable IDs if the project is already string-ID oriented
     - `FGuid` only if the codebase already uses it consistently
   - Keep the choice consistent across entities and references.
   - If needed, define small typed aliases/wrappers only if they improve safety without causing widespread churn.

3. **Update core entity models**
   - Ensure each core mutable entity has an explicit stable ID field.
   - Ensure display name/title fields remain separate and are not used as authoritative keys.
   - For static song definitions, ensure the definition ID is distinct from mutable usage state if both exist.

4. **Refactor authoritative containers and lookup paths**
   - Convert subsystem-owned maps/registries to key by stable ID.
   - Add canonical lookup helpers such as:
     - `FindArtistById`
     - `FindRecordById`
     - `FindContractById`
   - Keep temporary compatibility helpers like `FindArtistByLegacyName` only where needed during migration.
   - Where old code depends on name lookup, route it through:
     1. exact ID lookup if possible
     2. legacy name fallback
     3. warning/error log on fallback or failure

5. **Standardize cross-subsystem APIs**
   - Update public/internal subsystem methods so references are passed by stable ID, not display name.
   - Replace ambiguous string parameters with clearly named ID parameters.
   - Update call sites accordingly.
   - Avoid leaving mixed semantics like `FString ArtistKey` if it can mean either name or ID.

6. **Add compatibility handling**
   - For legacy save data or old call paths, support migration/fallback where practical.
   - If old serialized data stores names:
     - attempt deterministic resolution to IDs
     - log a warning when fallback migration occurs
     - fail clearly if multiple entities match or no entity matches
   - Do not silently bind the wrong entity.

7. **Add validation**
   - On load and/or subsystem initialization, validate:
     - every entity has a non-empty stable ID
     - IDs are unique within their entity domain
     - references point to existing entities
     - no duplicate active entities share the same ID
   - Emit warnings/errors through appropriate UE log categories.
   - Prefer explicit validation helpers that can be reused by save/load and tests.

8. **Update persistence**
   - Ensure save snapshots serialize stable IDs for all cross-entity references.
   - Remove dependence on display names for reconstruction.
   - If a save migration/version hook already exists, plug legacy name-to-ID migration into it.
   - If no migration framework exists, add the smallest safe compatibility layer in load code and document it.

9. **Update UI/view-model boundaries only as needed**
   - UI may still display names, but any command/query into business logic must use IDs.
   - If selection models currently store names, switch them to IDs while preserving display behavior.

10. **Add tests**
   - Add or update tests for:
     - entity lookup by stable ID
     - duplicate ID detection
     - unresolved reference validation
     - legacy name-based save/reference migration
     - ensuring display name changes do not break references
   - Prefer focused tests around the most central subsystem(s) and save validation path.

11. **Document assumptions in code comments**
   - Add brief comments where compatibility shims exist, noting they are transitional and name-based references are deprecated.

Implementation quality bar:
- Keep changes cohesive and compile-safe.
- Prefer small helper functions over repeated ad hoc fallback logic.
- Do not leave new code introducing additional name-keyed business logic.

# Validation steps
1. **Codebase audit verification**
   - Confirm all core entity types in scope have explicit stable ID fields.
   - Confirm display names are no longer used as authoritative keys in touched systems.

2. **Build**
   - Run the most relevant available build command(s):
     - `dotnet build`
   - If there are targeted project/test builds in the repo, use those as appropriate.

3. **Tests**
   - Run:
     - `dotnet test`
   - If no tests exist for the touched area, add them and run the smallest relevant suite.

4. **Behavioral validation**
   - Verify that:
     - lookups succeed by ID
     - changing an entity display name does not break references
     - duplicate IDs are detected and logged
     - unresolved references are detected and logged
     - legacy name-based references, if still supported, resolve with warnings rather than silently

5. **Save/load validation**
   - If save/load code is present in the workspace, verify:
     - saved references use IDs
     - load reconstructs references from IDs
     - legacy name-based save data either migrates safely or fails clearly

6. **API consistency check**
   - Spot-check touched subsystem methods to ensure parameter naming and semantics clearly indicate IDs.
   - Remove or deprecate ambiguous name-based methods where possible.

7. **Final change summary**
   - In your final implementation notes, include:
     - which entity types were standardized
     - which legacy paths remain temporarily supported
     - any unresolved areas that need follow-up stories

# Risks and follow-ups
- **Risk: broad refactor surface**
  - Identity usage often spreads across subsystems, save/load, and UI. Keep the refactor incremental and compatibility-aware.

- **Risk: ambiguous legacy name resolution**
  - Duplicate display names can cause incorrect migration. If ambiguity exists, log an error and fail safely rather than guessing.

- **Risk: partial API conversion**
  - Mixed name/ID semantics can persist if only some call sites are updated. Prioritize canonical APIs and route all touched code through them.

- **Risk: save compatibility**
  - Old saves may rely on names. Add the smallest safe migration path now and document any remaining gaps.

- **Risk: Unreal/.NET workspace mismatch**
  - The workspace hints at .NET build commands, but the architecture is Unreal/C++. Use available tooling pragmatically, but do not assume the gameplay code is .NET.

Recommended follow-ups after this task:
- Story **ST-103** command dispatcher should build on these stable IDs exclusively.
- Story **ST-104** domain events should carry stable IDs in payloads.
- Story **ST-402/ST-403** should formalize save versioning and migration around the ID standard.
- Add a dedicated validation pass/tool for content and save integrity once the core ID migration is complete.