# Goal
Implement `TASK-7.2.5 — Display names remain presentation-only` for story `ST-102 Stable entity ID standardization`.

The coding agent should enforce the architectural rule that gameplay/business logic, persistence, cross-subsystem references, and lookups must use stable entity IDs only, while human-readable names/titles remain UI/presentation fields.

This task is specifically about removing any remaining reliance on display names as identifiers or keys, and making that separation explicit and safe in code.

# Scope
In scope:
- Audit the codebase for places where artist/song/record/contract/tour/label/event display names are used as:
  - map keys
  - lookup keys
  - equality identity
  - persistence references
  - command/API inputs for business logic
- Refactor those usages to stable IDs.
- Preserve display names for:
  - widgets
  - labels
  - tooltips
  - logs/messages where appropriate
  - debug output as supplemental context only
- Add compatibility shims only where necessary to avoid broad breakage, but keep the final authoritative path ID-based.
- Add validation/logging for suspicious name-based resolution if encountered.
- Update or add tests for the separation between ID and display name behavior.

Out of scope:
- Large-scale redesign of unrelated subsystems.
- Full save migration framework unless directly required by touched code.
- UI redesign beyond adapting bindings to consume IDs + display fields correctly.
- Introducing a new generic identity framework if existing project conventions already provide a stable ID type.

Implementation intent:
- If the project already has entity structs/classes with both `Id` and `Name`/`DisplayName`, standardize usage around that.
- If stable IDs are inconsistent, use the project’s existing preferred stable identifier type where possible rather than inventing a parallel scheme.
- Keep changes minimal, targeted, and compile-safe.

# Files to touch
Start by inspecting and then touching only the files necessary to complete the task. Prioritize these categories:

1. Core model/entity definitions
- Files defining artist/song/record/contract/tour/label/event data structs/classes.
- Any shared types for IDs, references, or save snapshots.

2. Subsystems and managers
- Files under gameplay/simulation/application layers where entities are stored or queried.
- Especially managers/subsystems that maintain `TMap`, registries, or lookup helpers.

3. Command/API surfaces
- Command structs, dispatcher methods, and subsystem public methods that may still accept names instead of IDs.

4. Persistence
- Save/load snapshot structs and serialization helpers where names may still be persisted as references.

5. UI/view-model glue
- Widget presenters/view models/adapters that may pass names into business logic instead of IDs.
- Keep display text behavior intact.

6. Tests
- Existing unit/integration tests covering entity lookup, save/load, command dispatch, or UI projections.
- Add focused tests near the touched code rather than broad new test infrastructure.

Likely search targets:
- `Name`
- `DisplayName`
- `Title`
- `FindByName`
- `GetByName`
- `TMap<FString`
- `TMap<FName`
- `SelectedArtistName`
- `ArtistName`
- `SongTitle`
- `RecordTitle`
- `LabelName`
- `EventName`
- any string-based equality checks against entity identity

Do not modify generated, intermediate, or build output files.

# Implementation plan
1. Audit current identity usage
- Search the codebase for all entity lookup/storage patterns using display names.
- Build a short internal checklist of each offending usage:
  - file
  - entity type
  - current name-based behavior
  - target ID-based replacement
- Pay special attention to:
  - maps keyed by display name
  - helper methods like `GetArtistByName`
  - save structs storing names as references
  - command handlers receiving names from UI
  - duplicate-name edge cases

2. Identify the canonical stable ID per entity
- For each relevant entity type, confirm the canonical stable identifier already used by the project.
- Prefer existing stable fields/types over introducing new ones.
- If an entity lacks a stable ID but is in scope for this task, add one only if required to complete the refactor cleanly.
- Ensure naming is clear:
  - `ArtistId`, `SongDefId`, `RecordId`, etc.
  - `DisplayName`/`Title` remain clearly non-authoritative.

3. Refactor storage and lookup paths to IDs
- Replace name-keyed registries/maps with ID-keyed registries/maps.
- Replace name-based lookup helpers with ID-based helpers.
- If a name-based helper must temporarily remain for compatibility:
  - mark it clearly as compatibility/presentation-only
  - make it resolve through authoritative ID-backed data
  - log a warning in non-shipping/dev builds if used for business logic paths
- Ensure duplicate display names do not break behavior.

4. Separate presentation fields from identity semantics
- Where entity structs currently expose ambiguous `Name` fields used for both identity and display, clarify usage:
  - keep display text fields for UI
  - ensure comparisons, references, and persistence use IDs
- If helpful, rename local variables/parameters to reduce ambiguity:
  - `ArtistName` -> `ArtistDisplayName` when it is presentation text
  - `NameKey` -> `ArtistId` where it is actually identity
- Avoid unnecessary public API churn unless needed for correctness.

5. Update command and subsystem interfaces
- Ensure business-facing methods accept IDs, not names.
- If UI currently passes names:
  - update UI glue/view-model selection payloads to carry IDs
  - continue displaying names in widgets
- Keep widgets thin:
  - selection and command dispatch should use IDs
  - text rendering should use display fields from projections

6. Update persistence and reference serialization
- Ensure save/snapshot/reference structs store stable IDs for relationships.
- Display names may still be stored as optional denormalized presentation/debug fields only if already needed, but must not be used to reconstruct references.
- If load/restore code currently resolves by name, switch it to ID resolution.
- Add validation/warnings for unresolved IDs and any legacy name-only fallback path still present.

7. Add validation and logging
- Add targeted warnings/errors for:
  - duplicate IDs
  - unresolved IDs
  - attempted name-based business lookup in compatibility paths
- Use existing project log categories if present; otherwise use the nearest appropriate category.
- Keep logs actionable and concise.

8. Add or update tests
- Add focused tests that prove:
  - two entities can share the same display name without identity collision
  - renaming an entity display name does not break references
  - save/load or snapshot reconstruction uses IDs, not names
  - command execution works when display names change
- Prefer small deterministic tests over broad end-to-end coverage if test infrastructure is limited.

9. Keep backward compatibility minimal and explicit
- If legacy code paths still require name-based entry points, keep them as thin adapters only.
- Add TODO comments only where genuinely necessary and tie them to `ST-102` / `TASK-7.2.5`.
- Do not leave silent fallback behavior that could reintroduce name-as-key logic.

10. Document through code clarity
- Add brief comments where needed to state the rule:
  - display names are presentation-only
  - IDs are authoritative for identity, persistence, and cross-system references

# Validation steps
1. Static/code validation
- Build the solution/project after changes.
- Use the workspace’s candidate commands as applicable:
  - `dotnet build`
  - `dotnet test`
- If the Unreal C++ project is not fully buildable via the provided .NET commands, still run any available tests/build steps that validate touched support projects and report limitations clearly.

2. Functional validation by inspection/tests
- Verify no touched business logic path uses display names as authoritative keys.
- Verify maps/registries in touched code are keyed by stable IDs.
- Verify command/subsystem APIs touched by this task accept IDs.
- Verify UI-facing projections still expose display names for rendering.

3. Edge-case validation
- Create or update tests for:
  - duplicate display names across two entities
  - display name change after creation
  - unresolved ID handling
  - legacy compatibility path warning behavior, if such a path remains

4. Persistence validation
- If save/load code is touched, verify references serialize/deserialize by ID.
- Confirm display names are not required to restore relationships.

5. Final change summary
- In your implementation notes/commit summary, explicitly list:
  - every name-based identity usage removed
  - any compatibility shims retained
  - any remaining known hotspots not addressed in this task

# Risks and follow-ups
- Risk: hidden name-based assumptions in UI or legacy helper methods may cause regressions after switching to IDs.
  - Mitigation: keep temporary adapters where necessary, but make them non-authoritative and logged.

- Risk: ambiguous `Name` fields in structs may still confuse future contributors.
  - Mitigation: rename touched variables/parameters to `DisplayName` where practical.

- Risk: save/load compatibility may depend on legacy name references.
  - Mitigation: if legacy fallback is required, isolate it and log when used; do not let it remain the primary path.

- Risk: duplicate-name bugs may only surface in edge cases not currently covered.
  - Mitigation: add explicit duplicate-display-name tests.

Follow-ups to note if discovered:
- Broader story-level cleanup for all remaining stable ID standardization outside the touched files.
- Save migration support for any legacy name-keyed snapshots.
- Additional UI selection-context cleanup if widgets still route commands by display text.
- Consolidation of entity reference types into explicit ID wrappers if the codebase is still inconsistent.