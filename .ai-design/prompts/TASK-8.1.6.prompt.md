# Goal
Implement backlog task **TASK-8.1.6** for **ST-201 Artist state model expansion** by ensuring the artist state model uses **simple scalar fields only** for now, with no deep relationship/cohesion simulation. The implementation should support current gameplay needs while leaving clear extension points for future richer interpersonal systems.

# Scope
Focus only on the part of ST-201 implied by this task:

- Represent artist condition/personality/career state with scalar values such as:
  - attributes
  - personality traits
  - momentum
  - reputation
  - fatigue
  - burnout risk
  - scandal heat
- Ensure any current or new artist state update logic operates on these scalar values.
- Avoid implementing:
  - artist-to-artist relationship graphs
  - band member relationship matrices
  - social network simulation
  - internal cohesion event chains
  - dependency on deep relationship state for action availability
- If the codebase already contains placeholders or partial relationship concepts, keep them inert, optional, or clearly marked as future work without wiring them into active simulation logic.
- Preserve compatibility with the architecture:
  - subsystem-owned business state
  - deterministic simulation
  - read-only UI projections
  - stable IDs
- Keep unsigned and signed artists on the same compatible base state model.

Do not expand into unrelated story work unless required to compile or preserve behavior.

# Files to touch
Inspect the workspace first and then update the minimum necessary files. Likely candidates include:

- `README.md` only if there is a developer-facing model note that must be corrected
- Any artist domain model files, likely named similar to:
  - `Artist*.cs`
  - `ArtistState*.cs`
  - `ArtistModel*.cs`
  - `ArtistManager*.cs`
  - `ArtistSubsystem*.cs`
- Any simulation tick/update files that modify artist condition over time
- Any DTO/view/projection files for artist detail/roster display
- Any save/load snapshot files if artist state serialization must include the scalar fields
- Any tests covering artist state, simulation updates, or serialization

Because the workspace hint says `.net`, prefer existing C# project structure and naming conventions over the Unreal/C++ target architecture language in the backlog. Implement the task in the actual codebase style you find.

# Implementation plan
1. **Discover current artist model and simulation flow**
   - Search for artist-related types, managers, services, and tests.
   - Identify where artist mutable state is defined.
   - Identify whether relationship/cohesion/member-link concepts already exist.
   - Identify where weekly/monthly updates are applied.

2. **Define or normalize a scalar-only artist state shape**
   - Ensure the mutable artist state contains scalar fields for the ST-201 baseline, using existing names where possible.
   - Prefer a compact structure with numeric values and simple flags/status fields.
   - If needed, add or normalize fields such as:
     - `Momentum`
     - `Reputation`
     - `Fatigue`
     - `BurnoutRisk`
     - `ScandalHeat`
     - personality/attribute scalar stats
   - Keep ranges and defaults sensible and deterministic.

3. **Remove active dependency on deep relationship simulation**
   - If relationship collections, nested member dynamics, or cohesion systems exist, do not delete them unless clearly dead and safe to remove.
   - Instead:
     - stop using them in active calculations
     - replace any required outputs with scalar substitutes
     - document with a concise TODO/FUTURE note
   - Example: if action availability currently depends on a relationship graph, switch it to depend on scalar state like fatigue, burnout risk, status, and commitments.

4. **Update simulation logic to use scalar-only calculations**
   - Ensure artist updates during time advancement are based only on scalar state and deterministic inputs.
   - Keep formulas simple and inspectable.
   - Avoid introducing randomness unless the surrounding system already uses a deterministic seeded approach.
   - If no update logic exists yet, add only the minimum needed scaffolding to support scalar state without overbuilding.

5. **Keep UI/read models compatible**
   - Ensure artist detail or roster projections expose the scalar fields in read-only form if projections already exist.
   - Do not add UI-owned business logic.
   - Do not add relationship UI unless already present and trivial to disable.

6. **Preserve save/load compatibility**
   - If artist state is serialized, ensure the scalar fields are included.
   - If older saves may omit new fields, use safe defaults.
   - Do not introduce fragile nested relationship serialization for this task.

7. **Add or update tests**
   - Add focused tests that verify:
     - artist state can be created with scalar defaults
     - simulation updates operate without relationship data
     - action eligibility, if covered, depends on scalar state rather than deep relationship structures
     - serialization/deserialization preserves scalar fields
   - Keep tests narrow and deterministic.

8. **Document future extension point**
   - Add a short code comment or developer note where appropriate:
     - current implementation intentionally uses scalar artist state
     - deeper relationship simulation can be layered later without breaking the base model

Implementation constraints:
- Prefer minimal, incremental changes.
- Do not invent a full relationship subsystem.
- Do not refactor broad architecture unless necessary for this task.
- Follow existing code style and patterns in the repository.

# Validation steps
1. **Codebase inspection**
   - Confirm where artist state is defined and where it is updated.
   - Confirm no active gameplay path now requires deep relationship simulation.

2. **Build**
   - Run:
     - `dotnet build`

3. **Tests**
   - Run:
     - `dotnet test`

4. **Behavior verification**
   - Verify artist entities can exist and function with scalar-only state.
   - Verify signed and unsigned artists share the same compatible base model.
   - Verify any artist update logic uses scalar values only.
   - Verify no UI/projection path breaks due to removed or bypassed relationship assumptions.
   - Verify save/load or snapshot tests still pass if present.

5. **Regression check**
   - Search for references to relationship/cohesion/member-dynamics concepts and confirm they are not required by current execution paths.
   - Confirm no compile-time dead references remain after the change.

# Risks and follow-ups
- **Risk: hidden coupling to relationship concepts**
  - Existing code may reference band cohesion or member relationships indirectly in eligibility or simulation updates.
  - Mitigation: search broadly and replace active dependencies with scalar checks.

- **Risk: save/schema drift**
  - If artist state is serialized, adding or renaming fields may affect existing snapshots.
  - Mitigation: preserve field names where possible and use safe defaults.

- **Risk: over-implementation**
  - It is easy to start building a richer social simulation because the architecture mentions future relationship depth.
  - Mitigation: explicitly stop at scalar state and leave extension notes only.

- **Risk: UI assumptions**
  - Some views may expect richer nested artist data.
  - Mitigation: provide stable scalar projections and avoid breaking contracts unless necessary.

Follow-ups for later stories/tasks:
- Add optional relationship/cohesion simulation as a separate layer on top of the scalar base model.
- Introduce band/member interpersonal dynamics only when there is clear gameplay use and acceptance coverage.
- Consider data-driven scalar ranges and tuning tables if not already present.