# Goal
Implement backlog task **TASK-8.1.1 / ST-201 Artist state model expansion** by expanding the artist simulation state so each artist includes:

- core **attributes**
- **personality traits**
- **momentum**
- **reputation**
- **fatigue**
- **burnout risk**
- **scandal heat**

The implementation must fit the existing architecture direction:

- deterministic simulation
- subsystem-owned business state
- stable IDs
- read-only projections for UI
- save-friendly mutable state
- no widget-owned business logic

This task should establish the **artist state model foundation**, not a full balancing pass. Prefer a clean, extensible data model and minimal, safe integration points over speculative systems.

# Scope
In scope:

- Expand the mutable artist state/data model to include the new fields.
- Ensure unsigned and signed artists use the same compatible base state shape.
- Add or update any supporting enums/structs/view models needed to expose this data cleanly.
- Integrate the new state into the artist manager/subsystem layer.
- Add basic initialization/defaulting for newly created or loaded artists.
- Add lightweight update hooks or placeholders if the current simulation already has artist update flow, but do **not** invent large new simulation systems unless required for compilation/integration.
- Ensure the data is serializable/persistable in the project’s current model.
- Expose the new fields through read-only artist detail/summary projections if such projections already exist.

Out of scope unless required by existing code structure:

- Full weekly/monthly balancing formulas for fatigue/burnout/scandal.
- New UI screens or major widget redesign.
- Contract logic changes.
- Tour/recording logic changes beyond compile-safe references to the new artist state.
- Save migration framework beyond adding safe defaults where needed.
- Deep relationship/cohesion simulation.

If the codebase is incomplete or partially scaffolded, implement the smallest coherent vertical slice that matches the architecture and compiles.

# Files to touch
Inspect the repo first and then touch only the minimum necessary files. Likely targets include:

- Artist domain/state structs/classes:
  - files containing artist model definitions
  - files containing artist snapshot/save structs
  - files containing artist detail/summary projection structs
- Artist subsystem/manager:
  - `UArtistManagerSubsystem` or equivalent
  - any artist repository/state container
- Save/load serialization files if artist state is explicitly serialized separately
- Tests covering artist state, serialization, or projections

Search for likely symbols/filenames such as:

- `Artist`
- `ArtistState`
- `ArtistSnapshot`
- `ArtistManager`
- `ArtistSubsystem`
- `ArtistDetailView`
- `Roster`
- `SaveGame`
- `Snapshot`

Do not modify unrelated UI or gameplay systems unless a compile error or direct dependency requires it.

# Implementation plan
1. **Discover current artist model and ownership**
   - Find the canonical mutable artist state type.
   - Identify where artist data is created, stored, queried, and serialized.
   - Identify whether there is already a split between:
     - static artist template/archetype data
     - mutable campaign artist state
     - UI projection/view structs

2. **Expand the artist state schema**
   Add fields for the story requirement in the canonical mutable artist state. Prefer grouped structs if the codebase already uses them; otherwise keep changes minimal and readable.

   Target conceptual shape:

   - `Attributes`
     - examples: talent, charisma, reliability, market appeal
   - `PersonalityTraits`
     - examples: ego, teamwork, lifestyle risk, activism
   - `Momentum`
   - `Reputation`
   - `Fatigue`
   - `BurnoutRisk`
   - `ScandalHeat`

   Follow existing naming conventions. If there is no existing grouped struct pattern, introduce small `USTRUCT`s only if they improve clarity and do not create broad churn.

3. **Use safe scalar ranges**
   Unless the codebase already defines a stat type, use simple scalar numeric values with clear defaults. Prefer normalized/intuitive ranges already implied by the architecture and backlog, e.g. 0–100 style values.

   Add clamping/helpers if the project already has utility patterns for stat normalization. Avoid overengineering.

4. **Initialize defaults consistently**
   Ensure all artist creation paths populate the new fields:
   - generated artists
   - unsigned discovery pool artists
   - signed roster artists
   - deserialized/load fallback paths

   If no authored values exist yet, use sensible defaults that preserve gameplay neutrality, for example:
   - momentum/reputation: moderate baseline
   - fatigue/burnout/scandal: low baseline
   - attributes/personality: zeroed only if the project already fills them later; otherwise use explicit defaults

5. **Preserve compatibility**
   If existing saves or constructors may omit the new fields:
   - add backward-safe defaults
   - avoid assuming non-initialized nested structs are valid without checks
   - do not break existing artist lookup or roster iteration code

6. **Expose read-only projections**
   If the project has artist detail or roster projection/view structs, include the new fields there so UI can read them without direct subsystem mutation.

   Keep projections read-only and derived from subsystem state. Do not add widget logic.

7. **Integrate with artist manager queries**
   Update any artist getter/build-projection methods so the new state is surfaced consistently.
   If there is an “action eligibility” or “can record/can tour” style method already depending on artist condition, only make minimal compile-safe adjustments. Do not invent new gameplay rules unless clearly expected by existing code comments/tests.

8. **Serialization/save support**
   If artist state is serialized explicitly:
   - include the new fields in save snapshots
   - ensure load paths restore them
   - ensure missing values default safely

   If serialization is automatic through reflected structs, verify the new fields are reflected correctly.

9. **Add lightweight validation/logging**
   If the project has logging categories or validation helpers, add minimal validation for impossible values and clamp where appropriate. Keep this non-invasive.

10. **Add or update tests**
   Prefer focused tests for:
   - artist state construction/defaults
   - projection includes new fields
   - serialization round-trip if test infrastructure exists

11. **Document assumptions in code comments**
   Add brief comments only where needed to clarify:
   - these are mutable campaign-state artist stats
   - formulas are intentionally lightweight/placeholders for now
   - future weekly/monthly update logic can build on this model

# Validation steps
1. **Codebase inspection**
   - Confirm the canonical artist state type and all references compile after changes.
   - Confirm no duplicate parallel artist state definitions were left inconsistent.

2. **Build**
   Run the most appropriate available build command from workspace root:

   - `dotnet build`

   If there are tests:
   - `dotnet test`

3. **Static verification**
   Verify:
   - artist state now includes attributes, personality traits, momentum, reputation, fatigue, burnout risk, and scandal heat
   - unsigned and signed artists share the same compatible base model
   - projections/read models expose the new fields where applicable
   - serialization paths include or safely default the new fields

4. **Behavior verification**
   If there is a local harness/test or easy entry point:
   - create/load an artist
   - inspect returned artist detail/summary data
   - confirm all new fields are present and initialized
   - confirm no null/default crashes in roster/detail flows

5. **Regression check**
   Ensure existing flows still work at compile level:
   - roster retrieval
   - artist lookup by ID
   - save/load of artist-containing state
   - any existing command/query paths touching artist data

6. **Report**
   In your final implementation summary, include:
   - files changed
   - exact new fields added
   - any defaults chosen
   - whether tests were added/updated
   - any assumptions due to missing acceptance criteria

# Risks and follow-ups
- **Risk: duplicate artist models**
  - The repo may contain multiple artist structs for UI, save, and runtime. Keep them aligned and identify the canonical source of truth.

- **Risk: save compatibility**
  - Adding nested structs/fields can expose missing-default issues on load. Use safe initialization and avoid brittle assumptions.

- **Risk: premature formula design**
  - This task is about state model expansion, not final simulation tuning. Do not hardcode complex burnout/scandal systems unless already scaffolded.

- **Risk: UI coupling**
  - Avoid pushing business logic into widgets. Expose read-only projections instead.

Recommended follow-ups after this task:
- implement weekly/monthly artist condition updates based on activity/neglect
- derive action availability from artist state and commitments
- add debug/stat inspection support for artist hidden values
- add save migration coverage if legacy save support is required
- add balancing data assets for default stat ranges and update formulas