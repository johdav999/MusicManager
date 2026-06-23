# Goal

Implement backlog task **TASK-7.2.2 — Cross-subsystem APIs accept IDs consistently** for story **ST-102 Stable entity ID standardization**.

The coding agent should update the codebase so that gameplay-facing and subsystem-facing APIs consistently use **stable entity IDs** instead of display names or mixed name/ID parameters when referencing entities across subsystem boundaries.

This task is specifically about **API consistency**, not a full data-model rewrite. The result should reduce ambiguity in calls between systems such as artist, song, record, contract, tour, finance, save/load, UI orchestration, and command dispatch.

Key intent:
- Cross-subsystem method signatures should accept IDs consistently.
- Display names remain presentation-only.
- Existing mixed usage should be migrated with compatibility handling where needed.
- Logging/validation should help detect unresolved or duplicate IDs.

# Scope

Focus on the minimum coherent implementation needed to satisfy this task in the current workspace.

In scope:
- Audit cross-subsystem APIs for entity references currently passed as names, labels, titles, or mixed string fields.
- Standardize those APIs to accept stable IDs consistently for relevant entities:
  - ArtistId
  - SongDefId / SongUseId where applicable
  - RecordId
  - ContractId
  - TourId
  - LabelId
  - EventId
- Update internal callers so subsystem-to-subsystem interactions use IDs.
- Preserve UI/display behavior by resolving names from IDs at the presentation edge.
- Add compatibility shims only where necessary to avoid broad breakage.
- Add warnings/errors for unresolved IDs and duplicate ID conditions where practical.

Out of scope unless required to complete compilation:
- Large-scale save migration implementation beyond lightweight compatibility handling.
- Full replacement of all internal storage structures if not needed for API consistency.
- New gameplay features.
- Broad UI redesign.
- Refactoring unrelated systems.

Implementation guidance:
- Prefer typed ID fields already present in the codebase.
- If the codebase currently uses raw strings for IDs, keep that approach unless a stronger existing type already exists.
- Do not introduce speculative abstractions if the project is still early-stage.
- Keep changes incremental and compile-safe.

# Files to touch

Start by inspecting and likely modifying files in these areas, based on actual repository contents:

- **Subsystem headers/cpps**
  - `*Subsystem.h`
  - `*Subsystem.cpp`
  - Especially:
    - `GameTime`
    - `Artist`
    - `SongCatalog`
    - `Production` / `Record`
    - `Release`
    - `Finance`
    - `Tour`
    - `Event`
    - `UIManager`
    - `CommandDispatcher`

- **Domain/data model files**
  - structs defining artists, songs, records, contracts, tours, events
  - save snapshot structs
  - view/projection structs if they expose names as keys

- **Command/request/result files**
  - command payload structs
  - dispatcher methods
  - validation helpers

- **UI integration files**
  - widgets/view-models only where they call subsystem APIs with names instead of IDs
  - selection context code if it stores names as active references

- **Save/load and validation files**
  - save migration/validation helpers
  - reference resolution utilities
  - logging categories if needed

- **Tests**
  - existing unit/integration tests covering subsystem interactions
  - add focused tests for ID-based API behavior if test infrastructure exists

Do not touch generated, intermediate, or build output files.

# Implementation plan

1. **Inspect the codebase and identify current entity reference patterns**
   - Search for cross-subsystem methods that accept:
     - `Name`
     - `ArtistName`
     - `SongTitle`
     - `RecordTitle`
     - generic `FString`/`string` parameters used as lookup keys
   - Identify where APIs are inconsistent, such as:
     - one subsystem accepting `ArtistId`
     - another accepting artist display name
     - UI or command layer passing names directly into business logic

2. **Define the consistency rule before editing**
   - For each entity type, choose the canonical API parameter:
     - `ArtistId`
     - `SongDefId` / `SongUseId`
     - `RecordId`
     - `ContractId`
     - `TourId`
     - `LabelId`
     - `EventId`
   - Use display names only for:
     - UI labels
     - logs/messages
     - debug output
   - If the project already has naming conventions, follow them exactly.

3. **Refactor subsystem public APIs to accept IDs**
   - Update public methods used across subsystem boundaries so they take IDs consistently.
   - Examples of desired direction:
     - `GetArtistById(ArtistId)` instead of `GetArtistByName(Name)`
     - `GetRecord(RecordId)` instead of `GetRecordByTitle(Title)`
     - `AssignSongToRecord(SongDefId, RecordId)` instead of title/name-based variants
   - Keep method names explicit where helpful.

4. **Update command layer and orchestration layer**
   - Ensure command structs and dispatcher methods use IDs in payloads.
   - If commands currently accept names, migrate them to IDs.
   - If UI still selects by display name, resolve to ID in UI/selection layer before dispatching commands.

5. **Update internal callers**
   - Fix all compile errors by updating subsystem-to-subsystem call sites.
   - Ensure orchestration code, event generation, finance posting, chart updates, and save/load reconstruction all pass IDs consistently.

6. **Add compatibility shims only where needed**
   - If there are many old call sites or transitional dependencies, add temporary wrappers such as:
     - name-based overloads marked deprecated/commented as compatibility-only
     - wrappers that resolve name -> ID, log a warning, then call the ID-based method
   - Keep these minimal and localized.
   - Do not leave business logic duplicated across overloads.

7. **Improve validation and logging**
   - When resolving IDs, log warnings/errors for:
     - unresolved IDs
     - duplicate IDs
     - invalid cross-reference usage
   - Use existing UE/project log categories if present; otherwise add targeted logging in the relevant subsystem.
   - Prefer messages that include both ID and display name when available.

8. **Preserve presentation behavior**
   - Ensure UI-facing projections still expose display names for rendering.
   - Ensure widgets do not need to know internal lookup details beyond stable IDs and display fields.
   - If selection state currently stores names, migrate it to IDs while still showing names in the UI.

9. **Review save/load implications**
   - If save/load or migration code still reconstructs references by name, update it to prefer IDs.
   - If old data paths exist, add compatibility handling where practical:
     - resolve legacy name fields to IDs during load
     - log migration warnings
   - Keep this lightweight unless the codebase already has migration hooks.

10. **Add or update tests**
   - If tests exist, add focused coverage for:
     - ID-based lookup succeeds
     - name-based compatibility path logs warning and resolves correctly, if retained
     - unresolved ID produces expected failure behavior
     - cross-subsystem command flow uses IDs end-to-end
   - If no tests exist, add at least one narrow test around the most central subsystem or dispatcher.

11. **Document assumptions in code comments**
   - Add concise comments where needed to clarify:
     - IDs are canonical for business logic
     - names are presentation-only
     - compatibility wrappers are transitional

# Validation steps

1. **Repository inspection**
   - Confirm actual project structure and identify the relevant source files before editing.

2. **Build**
   - Run the most appropriate build command for the workspace after changes:
     - `dotnet build`
   - If the solution/project requires a more specific target, use that and note it in the summary.

3. **Tests**
   - Run:
     - `dotnet test`
   - If no tests exist or test projects are absent, state that clearly and rely on build validation plus targeted code inspection.

4. **Static verification**
   - Search for remaining cross-subsystem APIs using names as canonical keys.
   - Verify that public subsystem methods now consistently accept IDs for entity references.
   - Verify UI/presentation code still uses display names only for rendering.

5. **Behavior verification**
   - Confirm these flows are ID-based end-to-end where present in code:
     - command dispatch to subsystem
     - subsystem-to-subsystem lookup
     - event payload generation
     - save/load reference resolution
   - Confirm unresolved IDs produce warnings/errors rather than silent failure where practical.

6. **Regression check**
   - Ensure compatibility wrappers, if added, do not change business outcomes.
   - Ensure no new direct widget-to-business mutations were introduced.

# Risks and follow-ups

- **Risk: hidden name-based dependencies**
  - Some systems may still implicitly depend on names as keys, especially UI, save/load, or debug tools.
  - Mitigation: search broadly and add compatibility wrappers where necessary.

- **Risk: inconsistent ID types**
  - The codebase may mix `string`, `FString`, `FName`, GUID-like values, or custom structs.
  - Mitigation: do not force a global type migration in this task; standardize API semantics first and align with existing conventions.

- **Risk: save compatibility**
  - Older save paths may still rely on names.
  - Mitigation: prefer lightweight migration/compatibility handling and log clearly when legacy resolution occurs.

- **Risk: broad compile fallout**
  - Public API changes may affect many callers.
  - Mitigation: refactor incrementally, compile often, and use temporary overloads where needed.

Follow-ups after this task:
- Remove temporary name-based compatibility overloads once callers are fully migrated.
- Standardize storage/indexing internals on IDs where still mixed.
- Add stronger save migration coverage for legacy name-keyed data.
- Add automated validation utilities for duplicate/unresolved IDs across all subsystem snapshots.