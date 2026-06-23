# Goal
Implement backlog task **TASK-8.1.4** for **ST-201 — Artist state model expansion** by exposing artist detail data to the UI as **read-only projections**.

The coding agent should add or complete a projection/view-model path that lets UI code retrieve artist detail information without directly mutating or depending on raw simulation state. The result should align with the architecture principle:

- **Subsystems own business state**
- **Widgets are thin**
- **Commands mutate state**
- **Events notify UI**
- **Read-only projections feed presentation**

This task is specifically about the acceptance criterion:

- **Artist detail projections expose this data to UI in a read-only form.**

The projection should cover the expanded artist state expected by ST-201, including at minimum:
- stable artist ID
- display name
- artist type/status
- attributes
- personality traits
- career state:
  - momentum
  - reputation
  - fatigue
  - burnout risk
  - scandal heat
- action/availability-related read-only flags if already derivable
- any contract/label summary fields already available and safe to expose as display data

Do **not** implement direct UI-driven mutation in this task. If UI widgets already exist, wire them to consume the projection. If no UI exists yet, implement the projection API and any minimal integration points needed for future UI use.

# Scope
In scope:
- Inspect current artist domain/state structures and existing UI access patterns.
- Add a dedicated read-only projection struct such as `FArtistDetailView`, or extend an existing equivalent if one already exists.
- Add subsystem API(s) on the artist-owning subsystem to retrieve artist detail projections by stable ID.
- Ensure the projection is safe for UI consumption and does not expose mutable internal containers/references.
- Update existing UI/view-model plumbing to use the projection instead of reading raw subsystem maps/objects directly, where practical and low-risk.
- Keep naming and style consistent with the existing codebase.

Out of scope:
- New gameplay formulas for weekly/monthly artist state updates.
- New command handlers.
- Full UI redesign.
- Selection-context refactor unless required for compilation.
- Save/load schema changes unless the current implementation blocks projection access.
- Broad architecture cleanup outside the artist detail read path.

Implementation constraints:
- Prefer minimal, incremental changes.
- Preserve backward compatibility where possible.
- Use stable IDs, not artist names, for lookups.
- Keep widgets thin and read-only.
- Avoid introducing direct widget ownership of simulation state.

# Files to touch
Start by inspecting these areas and then touch only the minimum necessary files:

- `README.md` for project conventions if useful.
- Artist subsystem/manager files, likely under Unreal source folders such as:
  - `Source/.../ArtistManagerSubsystem.*`
  - `Source/.../Artist*.h/.cpp`
  - `Source/.../Subsystems/...`
- Existing artist state/model structs:
  - artist snapshot/state structs
  - attribute/personality/career structs
- Existing UI-facing structs or view models:
  - `FArtistDetailView`
  - roster/detail widget data structs
  - UI manager or selection context files
- Artist detail / roster / inspector widgets if they already bind artist data:
  - `UUserWidget` subclasses
  - presenter/view-model helpers
- Any event or refresh plumbing needed so UI can request or refresh projections cleanly.

If the exact files are not obvious, first search for:
- `ArtistManagerSubsystem`
- `ArtistId`
- `SelectedArtist`
- `ArtistDetail`
- `Roster`
- `UUserWidget`
- `Momentum`
- `Reputation`
- `Fatigue`
- `Burnout`
- `Scandal`

Do not modify generated, intermediate, or build output files.

# Implementation plan
1. **Discover the current artist model and UI access path**
   - Find the authoritative artist-owning subsystem.
   - Identify the current mutable artist state struct(s).
   - Identify whether a projection/view struct already exists.
   - Identify how UI currently gets artist data:
     - direct subsystem map access
     - raw struct copies
     - widget-owned state
     - selection-based lookup

2. **Define or extend a read-only artist detail projection**
   - Introduce a UI-facing struct, preferably Unreal-friendly, e.g.:
     - `USTRUCT(BlueprintType) struct FArtistDetailView`
   - Include only presentation-safe fields.
   - Recommended fields:
     - `ArtistId`
     - `DisplayName`
     - `ArtistType`
     - `Status`
     - attribute values
     - personality values
     - momentum
     - reputation
     - fatigue
     - burnout risk
     - scandal heat
     - optional display summaries for label/contract/action flags if already available
   - If there are nested structs already suitable for read-only copying, reuse them rather than duplicating too much shape.

3. **Add projection builder logic in the artist subsystem**
   - Add a method on the artist subsystem such as:
     - `bool TryGetArtistDetailView(const FName/FString/FGuid& ArtistId, FArtistDetailView& OutView) const;`
     - or equivalent consistent with the codebase
   - The method should:
     - validate the ID
     - resolve the artist from authoritative state
     - map domain state into the projection
     - return failure cleanly if not found
   - Keep it const/read-only.
   - Do not return mutable references to internal state.

4. **Support batch/list projection only if already needed**
   - If roster/detail screens currently need both summary and detail data, consider:
     - keeping existing roster summary projection intact
     - adding detail projection separately
   - Do not over-engineer a generic projection framework unless one already exists.

5. **Wire UI or UI-facing layer to the projection**
   - Update artist detail/inspector widgets or their presenter layer to request `FArtistDetailView` from the subsystem or UI manager.
   - If the architecture already routes through `UUIManagerSubsystem`, prefer that route.
   - Replace direct reads of raw artist state where practical in the touched flow.
   - Keep widget logic display-only.

6. **Preserve stable-ID-based lookup**
   - Ensure artist detail retrieval uses stable IDs only.
   - If current UI still uses names or selected raw objects, add a compatibility shim only as needed, but keep the new projection API ID-based.

7. **Handle missing data safely**
   - If an artist ID is invalid or missing:
     - return false / empty optional / safe default
     - avoid crashes in UI
   - If some expanded state fields are not yet fully implemented in the domain model, expose what exists and add TODO comments only where necessary.
   - Do not fabricate business state beyond safe defaults already used by the codebase.

8. **Keep Blueprint/UI usability in mind**
   - If widgets are Blueprint-backed, ensure the projection struct and accessors are Blueprint-friendly where appropriate.
   - If the codebase is C++-only for UI plumbing, stay consistent.

9. **Add lightweight comments where helpful**
   - Document that the projection is read-only and intended for UI consumption.
   - Avoid excessive comments.

10. **Do a minimal cleanup pass**
   - Remove any newly introduced dead code.
   - Keep includes and forward declarations tidy.
   - Avoid unrelated refactors.

# Validation steps
1. **Code search validation**
   - Confirm there is now a dedicated artist detail projection/view struct or equivalent.
   - Confirm the artist subsystem exposes a read-only getter for artist detail by stable ID.
   - Confirm touched UI code consumes the projection rather than raw mutable state in the updated path.

2. **Build validation**
   - Run:
     - `dotnet build`
   - If there are project-specific Unreal build instructions available in the repo, use them as well, but do not invent unsupported commands.

3. **Test validation**
   - Run:
     - `dotnet test`
   - If no relevant tests exist, note that explicitly in your summary.

4. **Behavior validation**
   - Verify the projection includes the ST-201 artist detail fields relevant to UI:
     - attributes
     - personality
     - momentum
     - reputation
     - fatigue
     - burnout risk
     - scandal heat
   - Verify lookup is by stable ID.
   - Verify missing/invalid artist IDs fail safely.

5. **Regression validation**
   - Check that no direct mutation path was introduced into widgets.
   - Check that existing artist detail or roster screens still compile and bind correctly.
   - Check that no save/load or command code was unintentionally changed.

6. **Final implementation summary**
   - In your final report, include:
     - files changed
     - projection struct added/updated
     - subsystem API added/updated
     - UI integration updated
     - build/test results
     - any gaps due to missing existing UI infrastructure

# Risks and follow-ups
Risks:
- The repo/workspace hint suggests `.NET`, while the architecture is Unreal/C++; project discovery may require extra inspection before locating the real gameplay source.
- Existing UI may still be tightly coupled to raw artist state, making a full conversion too large for this task.
- Expanded artist state fields may be partially implemented or named differently than the backlog language.
- Blueprint bindings may require `BlueprintType`/`BlueprintReadOnly` exposure details not obvious until code inspection.

Follow-ups after this task:
- Add or refine `FArtistRosterItemView` and other read-only projections for consistency across screens.
- Route all artist UI access through `UUIManagerSubsystem` or a dedicated presentation/query layer.
- Add event-driven refresh hooks so artist detail widgets update automatically on artist state changes.
- Add automated tests for projection mapping and invalid-ID handling.
- Continue ST-201 work for the simulation-side weekly/monthly artist state updates if not already complete.