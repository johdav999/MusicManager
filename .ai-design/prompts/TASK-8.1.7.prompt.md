# Goal
Implement backlog task **TASK-8.1.7 — Ensure unsigned and signed artists share a compatible base model** for story **ST-201 Artist state model expansion**.

The coding agent should update the codebase so that:
- unsigned/discovery-pool artists and signed/roster artists are represented through a **shared core artist state model**
- common artist data is not duplicated across separate incompatible structs/classes
- signing an artist transitions ownership/contract state without requiring lossy conversion or ad hoc field copying
- downstream systems can safely read artist identity, attributes, personality, career state, and availability from a consistent base representation

This work should align with the architecture direction:
- deterministic simulation
- stable IDs
- subsystem-owned business state
- thin UI
- mutable campaign state in save data
- signed vs unsigned status expressed as state, not divergent entity shapes

# Scope
In scope:
- inspect current artist-related models, especially any split between unsigned artist candidates and signed roster artists
- introduce or refactor toward a **shared base artist data structure** used by both states
- preserve or improve compatibility with:
  - `UArtistManagerSubsystem`
  - contract/signing flow
  - artist detail/read models
  - save/load serialization
  - any discovery/audition pipeline that creates unsigned artists
- represent signed/unsigned differences via explicit state fields such as:
  - status
  - current label id
  - contract id
  - action flags / availability
  rather than separate incompatible models
- add minimal migration/shim logic if existing code expects old shapes
- keep changes focused on model compatibility, not a broad redesign of all artist systems

Out of scope unless required to keep build/tests passing:
- major UI redesign
- full contract lifecycle implementation beyond compatibility points
- deep formula tuning for artist progression
- unrelated subsystem refactors

# Files to touch
Start by locating and updating the actual artist model and its consumers. Likely targets include files matching these patterns:

- `Source/**/Artist*.h`
- `Source/**/Artist*.cpp`
- `Source/**/ArtistManager*.h`
- `Source/**/ArtistManager*.cpp`
- `Source/**/Contract*.h`
- `Source/**/Contract*.cpp`
- `Source/**/Save*.h`
- `Source/**/Save*.cpp`
- `Source/**/Audition*.h`
- `Source/**/Audition*.cpp`
- `Source/**/Discovery*.h`
- `Source/**/Discovery*.cpp`
- `Source/**/Roster*.h`
- `Source/**/View*.h`
- `Source/**/View*.cpp`

Also inspect:
- any `USTRUCT` definitions for artist save snapshots
- any DataTable row structs for generated/discovered artists
- any command payloads or event payloads that assume signed artists use a different type than unsigned artists

If the repo structure differs, prefer touching the smallest set of files that centralize:
1. artist state definition
2. artist manager storage
3. sign/convert flow
4. save/load serialization

# Implementation plan
1. **Discover the current split**
   - Search for all artist-related structs/classes/enums.
   - Identify whether there are separate types such as:
     - unsigned artist candidate
     - signed artist state
     - roster entry
     - audition result artist
   - Document the current overlap and divergence before editing.
   - Find all places where an unsigned artist is “converted” into a signed artist.

2. **Define a shared base model**
   - Introduce a common `USTRUCT` for core mutable artist state if one does not already exist.
   - The shared model should cover, at minimum where supported by the codebase:
     - stable `ArtistId`
     - display name
     - artist type
     - attributes
     - personality traits
     - career state
     - genre affinities/tags
     - availability/action flags
     - signed/unsigned status
     - optional `CurrentLabelId`
     - optional `ContractId`
   - Prefer composition over inheritance for Unreal data models unless the codebase already uses inheritance safely.
   - If there are currently two structs, refactor them so both embed/reference the same base core struct, or collapse them into one canonical state struct plus status fields.

3. **Model signed vs unsigned as state**
   - Add or normalize an enum like `EArtistStatus` with values such as:
     - `Unsigned`
     - `Signed`
     - `Retired`
     - `Disbanded`
     - others only if already present
   - Ensure signed-only fields are nullable/optional and valid for unsigned artists:
     - unsigned artist: no label id, no contract id
     - signed artist: label id present, contract id present if active
   - Remove assumptions that only signed artists have full stats.

4. **Refactor manager storage to one canonical representation**
   - Update `UArtistManagerSubsystem` or equivalent so artist lookup/storage uses one canonical artist state type keyed by stable ID.
   - If the system currently stores separate collections for unsigned and signed artists with different element types, keep separate indexes/views if useful, but back them with the same underlying model.
   - Add helper queries like:
     - get artist by id
     - get signed artists
     - get unsigned artists
     - is artist signable
   - Avoid duplicating artist data between pools.

5. **Update signing flow**
   - Refactor the sign artist path so it mutates artist status and contract/label references on the existing artist entity instead of constructing a different signed-only object.
   - Preserve existing identity and state:
     - same `ArtistId`
     - same attributes/personality/career values
   - Ensure no data is lost during signing.

6. **Update projections/read models**
   - Any artist detail or roster projection should read from the shared base model.
   - If UI projections differ for signed vs unsigned artists, derive those differences from status and related fields rather than separate source types.
   - Keep widgets thin; do not move business logic into UI.

7. **Update save/load compatibility**
   - Ensure the canonical artist model serializes correctly in save snapshots.
   - If old save snapshots or legacy structs exist, add a migration/translation path where practical.
   - Validate that unsigned artists can be saved/loaded with the same schema as signed artists.
   - Do not serialize raw asset paths; preserve stable IDs only.

8. **Add validation guards**
   - Add lightweight validation in manager/load paths:
     - signed artist should not have empty label/contract state if contract is required by current rules
     - unsigned artist should not appear in signed roster queries
     - duplicate artist IDs should warn/error
   - Use existing log categories or add artist/save warnings if needed.

9. **Minimize breakage with compatibility shims**
   - If many call sites expect old types, add temporary conversion/accessor helpers rather than rewriting unrelated systems.
   - Mark obvious follow-up cleanup points with TODO comments only if the codebase convention allows it.

10. **Keep implementation idiomatic for Unreal**
   - Use `USTRUCT(BlueprintType)` / `UPROPERTY` where existing serialization/editor exposure requires it.
   - Preserve reflection/serialization compatibility.
   - Avoid introducing patterns that fight Unreal’s save/GC/data tooling.

# Validation steps
1. **Static inspection**
   - Confirm there is now one canonical/shared artist core model used by both unsigned and signed artists.
   - Confirm signing no longer requires lossy field-by-field conversion into a different incompatible type.

2. **Build**
   - Run the most appropriate build command available from workspace context:
     - `dotnet build`
   - If there are tests/projects available:
     - `dotnet test`

3. **Code-path validation**
   - Verify these scenarios in code:
     - create/load an unsigned artist
     - query unsigned artist details
     - sign that artist
     - verify same `ArtistId` and core stats remain intact
     - query signed roster and confirm artist appears there
     - save/load and confirm artist remains valid

4. **Behavioral checks**
   - Ensure unsigned artists expose the same base fields needed by ST-201:
     - attributes
     - personality
     - momentum/reputation/fatigue/burnout/scandal or current equivalents
   - Ensure signed-only state is represented as optional/conditional fields, not a separate incompatible model.

5. **Regression checks**
   - Check any audition/discovery flow still produces valid artist entities.
   - Check any roster/detail projection still compiles and reads artist data correctly.
   - Check save/load code still serializes artist collections without broken references.

6. **If automated tests exist nearby**
   - Add or update focused tests for:
     - unsigned-to-signed transition preserves core state
     - shared model serialization round-trip
     - signed/unsigned query filters

# Risks and follow-ups
- **Risk: legacy code assumes separate types**
  - Mitigation: add compatibility accessors/shims and refactor only central paths now.

- **Risk: save compatibility breaks**
  - Mitigation: preserve property names where possible and add migration/translation logic if old snapshot shapes exist.

- **Risk: UI or command code depends on signed-only artist structs**
  - Mitigation: update projections to consume the shared model and derive status-specific behavior from fields.

- **Risk: manager currently duplicates artist data across pools**
  - Mitigation: consolidate to one canonical store plus filtered views/indexes.

Follow-ups to note if encountered but not required for this task:
- add explicit artist validation helpers for load-time integrity checks
- unify artist view-model generation across roster/discovery screens
- add tests around contract expiration and status transitions
- remove deprecated legacy artist structs once all call sites are migrated