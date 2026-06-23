# Goal

Implement backlog task **TASK-8.1.5 — Keep formulas data-driven where possible** for story **ST-201 Artist state model expansion**.

The objective is to refactor or extend artist-state update logic so that artist condition/personality/career formulas are driven by editable data definitions wherever practical, instead of hardcoded constants embedded in code. This should align with the architecture principle of **data-driven balancing** using Unreal-friendly static content patterns.

Because this task has no explicit acceptance criteria, treat success as:

- artist state update formulas are configurable through data structures/assets/tables rather than scattered magic numbers
- code cleanly separates **formula inputs**, **formula config**, and **formula execution**
- defaults/fallbacks are safe and deterministic
- existing gameplay behavior remains functional
- the implementation is incremental and does not overreach into unrelated systems

Use the existing codebase conventions you find in the repo. Prefer minimal, maintainable changes over speculative framework-building.

# Scope

In scope:

- Find the current artist state model and any weekly/monthly update logic related to:
  - attributes
  - personality traits
  - momentum
  - reputation
  - fatigue
  - burnout risk
  - scandal heat
  - action availability derived from state
- Identify hardcoded formula constants and thresholds used in artist-state calculations.
- Introduce a data-driven configuration layer for those formulas where feasible.
- Wire the artist update logic to read from that configuration.
- Add sensible defaults so the game still works if no authored data exists yet.
- Keep formulas deterministic and inspectable.
- Add lightweight validation/logging for missing or invalid formula config.

Out of scope unless required by existing architecture:

- building a large generic formula engine
- redesigning unrelated simulation systems
- changing save-game schema unless absolutely necessary
- deep UI work beyond what is needed to preserve compatibility
- implementing a full balancing tool/editor UI
- broad refactors outside artist-state logic

If the workspace is actually a partial repo or non-UE host project, still implement the task in the most appropriate place using the existing project structure and patterns you discover.

# Files to touch

Inspect first, then update only the necessary files. Likely candidates include:

- `README.md` only if there is a short developer note section for data-driven balancing conventions
- artist simulation/state model files
- subsystem files for artist management and time-driven updates
- any existing config/data asset/table definitions for simulation formulas
- any shared structs/enums for artist state projections
- tests covering artist state updates, if present

Expected likely targets by intent, though exact paths must be discovered:

- artist state structs/classes
- `ArtistManagerSubsystem` implementation
- weekly/monthly simulation update code
- data definition files for balancing/config
- logging/validation helpers if already present

Do not touch generated/intermediate files.

# Implementation plan

1. **Discover the current implementation**
   - Search for:
     - `ArtistManagerSubsystem`
     - `ST-201` related code
     - `fatigue`, `burnout`, `momentum`, `reputation`, `scandal`
     - weekly/monthly artist update methods
     - hardcoded thresholds or scalar modifiers
   - Map where artist state is stored and where it is mutated.
   - Identify whether the project already uses:
     - `UDataAsset`
     - `UPrimaryDataAsset`
     - `UDataTable`
     - config structs loaded from assets/settings/json
   - Reuse the existing pattern instead of inventing a new one.

2. **Design a minimal data-driven formula config**
   - Introduce a focused config structure for artist-state balancing, for example:
     - base weekly fatigue decay/recovery
     - recording/touring activity modifiers
     - burnout accumulation/recovery rates
     - momentum gain/loss rates
     - reputation drift or event modifiers
     - scandal heat decay
     - thresholds for action availability
   - Prefer plain structs with named fields over opaque expression strings.
   - Keep formulas understandable and deterministic.
   - If the codebase already has a balancing asset/table pattern, plug into that.
   - If not, create a small, local solution appropriate for the project.

3. **Refactor hardcoded constants into config-backed values**
   - Replace magic numbers in artist-state update logic with reads from the new config.
   - Preserve current behavior as closely as possible by using equivalent default values.
   - Centralize defaults in one place.
   - Avoid repeated fallback logic scattered across methods.

4. **Keep formulas “data-driven where possible”**
   - Move tunable values and thresholds into data.
   - Keep truly structural logic in code, such as:
     - update ordering
     - clamping
     - deterministic branching
     - invariant enforcement
   - Do not force every branch into data if it harms readability or reliability.

5. **Add validation and safe fallback behavior**
   - On load/init, validate formula config ranges where reasonable:
     - non-negative decay/recovery rates where expected
     - min/max threshold sanity
     - clamp suspicious values
   - Log warnings for missing config and use defaults.
   - Ensure the simulation never crashes because balancing data is absent.

6. **Preserve UI/read model compatibility**
   - Ensure artist detail projections still expose the same state fields.
   - Do not move presentation state into simulation.
   - If action availability depends on thresholds now sourced from data, keep the public behavior stable.

7. **Add or update tests**
   - If tests exist, add focused coverage for:
     - default config path
     - configured formula path
     - deterministic updates from known inputs
     - fallback behavior when config is missing/invalid
   - If no tests exist, add the smallest practical automated coverage in the project’s current test style.
   - If automated tests are not feasible in this workspace, add a clearly documented validation harness or debug assertion path.

8. **Document assumptions in code**
   - Add concise comments where formula config is loaded and applied.
   - Explain why some logic remains in code while constants move to data.

Implementation guidance:

- Prefer a structure like:
  - `FArtistStateFormulaConfig`
  - `FArtistActivityModifiers`
  - `FArtistActionThresholds`
- Prefer one authoritative accessor/provider for artist formula config.
- Keep config immutable at runtime after load unless the project already supports live reload.
- If Unreal assets are available, a `UPrimaryDataAsset` or `UDataAsset` is preferred for designer-editable balancing.
- If the actual workspace is not UE C++ source, adapt the same design principle to the discovered stack.

# Validation steps

1. **Static/code validation**
   - Build the solution/project with the repo’s supported command:
     - `dotnet build`
   - If tests exist:
     - `dotnet test`

2. **Behavior validation**
   - Verify artist state update code compiles and runs with:
     - no authored formula asset/data present
     - authored/default formula config present
   - Confirm no direct hardcoded balancing constants remain in the main artist update path except true invariants/clamps.

3. **Determinism validation**
   - Run the same artist update inputs twice and confirm identical outputs.
   - Ensure config lookup does not depend on nondeterministic ordering or transient UI state.

4. **Fallback validation**
   - Simulate missing/invalid config and confirm:
     - warning is logged
     - defaults are used
     - no crash occurs

5. **Functional validation**
   - Verify artist state still updates for the core ST-201 fields:
     - momentum
     - reputation
     - fatigue
     - burnout risk
     - scandal heat
   - Verify action availability still derives from artist state and commitments.

6. **Code quality validation**
   - Check that:
     - formula values are centralized
     - naming is clear
     - no unnecessary framework was introduced
     - touched files remain focused on this task

# Risks and follow-ups

- **Risk: repo may not contain the expected Unreal gameplay code**
  - If so, adapt the implementation to the actual project structure and note the mismatch in your final summary.

- **Risk: no existing data asset/table pipeline for balancing**
  - Use a minimal local config abstraction now, but structure it so it can later be sourced from DataAssets/DataTables without rewriting formula consumers.

- **Risk: overengineering**
  - Avoid building a generic expression interpreter or rule engine.
  - Named config fields are preferred.

- **Risk: hidden dependencies on old hardcoded thresholds**
  - Search for duplicated artist-state checks outside the main subsystem and align them with the new config source where practical.

- **Risk: save/load coupling**
  - Do not serialize static formula config into saves.
  - Save mutable artist state only; resolve formula config from cooked/static data at runtime.

Follow-ups to mention if not completed in this task:

- migrate additional artist-related thresholds in command validation to the same config source
- add a dedicated balancing asset/table for all artist formulas if only a temporary config object was possible here
- expose formula breakdowns in debug tooling for ST-404 observability
- extend data-driven formulas to tours/critics/market systems using the same pattern where appropriate