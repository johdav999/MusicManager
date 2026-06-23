# Goal
Implement backlog task **TASK-8.1.3 — Action availability derives from artist state and active commitments** for story **ST-201 Artist state model expansion**.

The coding agent should update the project so that artist actions are no longer enabled by ad hoc UI assumptions or isolated booleans alone, but are instead derived from:
- the artist’s current simulation state
- active commitments such as recording, release, tour, or contract constraints
- a centralized, read-only action availability evaluation path suitable for both simulation and UI use

This work should fit the architecture direction:
- business state owned by subsystems
- widgets remain thin
- commands validate against subsystem-owned rules
- UI reads projections/view models rather than mutating or inferring business logic

Because this task has no explicit acceptance criteria beyond the story note, implement the smallest coherent vertical slice that establishes a durable pattern for action availability and exposes it to UI-facing projections.

# Scope
In scope:
- Identify the current artist state model and extend or adapt it so action availability can be computed from artist condition and commitments.
- Add a centralized action availability evaluation mechanism in the artist/domain layer.
- Support at least the core artist actions that are already present or clearly implied by the codebase, such as:
  - record / start recording
  - release planning or schedule release if applicable
  - tour / plan tour if applicable
  - sign / negotiate if applicable for unsigned artists
- Represent action availability with:
  - available/unavailable status
  - machine-readable reason code(s)
  - optional user-facing explanation text if the project already has a result/message pattern
- Ensure active commitments are considered, such as:
  - active recording
  - scheduled or active tour
  - contract status
  - artist fatigue / burnout / scandal / other state fields if already present or introduced by ST-201 work
- Expose action availability through an artist detail/roster projection or equivalent read-only API for UI consumption.
- Update command validation to use the same centralized availability rules where practical, avoiding duplicated logic.

Out of scope unless trivial and already adjacent:
- Large UI redesigns
- New gameplay systems not needed for availability derivation
- Deep balancing/formula tuning
- Save migration work beyond what is strictly necessary for compile/runtime correctness
- Full implementation of every future action in the architecture docs

If the codebase is missing some of the expected systems, implement a minimal compatible version that:
- compiles cleanly
- is easy to extend later
- does not over-engineer beyond current project patterns

# Files to touch
Inspect first, then touch only the files needed. Prioritize existing gameplay/domain files over creating parallel systems.

Likely areas to inspect and update:
- Artist subsystem/manager classes
  - files likely named similar to `ArtistManagerSubsystem.*`, `ArtistSubsystem.*`, `ArtistManager.*`
- Artist state structs/models
  - files containing artist data structs, enums, flags, or save models
- Command dispatcher / command validation files
  - especially commands related to signing, recording, release scheduling, touring
- Projection/view-model files
  - artist detail view, roster item view, command panel view, or similar
- UI integration files only where needed to consume the new projection fields
- Shared enums/result types
  - for action types and availability reason codes
- Tests, if present
  - unit/integration tests around artist state, command validation, or projections

Possible new files if the project lacks a clean home:
- `ArtistActionTypes.h/.cpp`
- `ArtistActionAvailability.h/.cpp`
- `ArtistAvailabilityEvaluator.h/.cpp`

Do not create redundant abstractions if an existing pattern already covers:
- command result codes
- read-only projections
- subsystem query APIs

# Implementation plan
1. **Survey the existing implementation**
   - Find the current artist model, artist subsystem, and any existing action gating logic.
   - Identify where UI currently decides whether actions are enabled.
   - Identify where commands currently validate artist eligibility.
   - Identify existing concepts for:
     - artist status
     - contract state
     - recording state
     - tour state
     - fatigue/burnout/reputation/scandal
     - flags like `CanRecord` / `CanTour`

2. **Define a centralized action availability model**
   - Introduce or reuse an enum for artist actions, for example:
     - `Sign`
     - `StartRecording`
     - `ScheduleRelease`
     - `PlanTour`
   - Introduce or reuse a reason code enum for unavailability, for example:
     - `None`
     - `ArtistNotFound`
     - `Unsigned`
     - `AlreadySigned`
     - `NoActiveContract`
     - `ContractExpired`
     - `RecordingInProgress`
     - `TourInProgress`
     - `ScheduledCommitmentConflict`
     - `Fatigued`
     - `BurnoutRiskTooHigh`
     - `ScandalLocked`
     - `ArtistStateBlocked`
   - Add a struct representing action availability, e.g.:
     - action type
     - `bAvailable`
     - reason code(s)
     - optional display text
   - Keep this serializable/read-only friendly if the project uses `USTRUCT`.

3. **Implement evaluation in the artist/business layer**
   - Add a subsystem method or helper such as:
     - `GetArtistActionAvailability(ArtistId, ActionType)`
     - `GetArtistAvailableActions(ArtistId)`
   - This logic should derive availability from current state, not UI state.
   - Use existing subsystem-owned data and cross-subsystem queries as needed.
   - Prefer deterministic checks in a stable order so results are predictable and testable.
   - Suggested evaluation order:
     1. artist exists
     2. artist status / signedness / contract validity
     3. active commitments
     4. state-based blockers like fatigue/burnout/scandal
     5. action-specific prerequisites
   - If multiple blockers exist, either:
     - return the highest-priority reason, or
     - return a small ordered list of reasons
   - Keep the behavior consistent and documented in code comments.

4. **Use active commitments as first-class inputs**
   - Derive commitments from existing systems rather than duplicating state where possible.
   - Examples:
     - active recording from production/record subsystem
     - active or scheduled tour from tour subsystem
     - contract status from contract data
   - If cross-subsystem querying is awkward, add a narrow query interface rather than hardcoding UI assumptions.
   - Avoid circular ownership; the artist subsystem may aggregate availability, but should not duplicate authoritative state from other subsystems unless that is already the project pattern.

5. **Integrate with artist projections**
   - Update the artist detail/roster projection to include action availability data.
   - Expose enough information for UI to:
     - enable/disable buttons
     - show tooltip/reason text
   - Keep projections read-only and derived.
   - If there is already a command panel projection, populate it there instead of adding UI-side logic.

6. **Align command validation with the same rules**
   - Update relevant commands so they consult the centralized availability evaluation before mutating state.
   - Avoid duplicated eligibility logic drifting apart between UI and command execution.
   - If commands need stricter validation than UI availability, layer it as:
     - shared availability check first
     - command-specific validation second
   - Return existing structured failure results using the mapped reason code/message pattern.

7. **Preserve compatibility with existing flags**
   - If the current artist model has flags like `CanRecord` or `CanTour`, do not blindly remove them unless clearly safe.
   - Instead:
     - reinterpret them as coarse capability gates
     - combine them with dynamic state and commitments
   - Example:
     - `CanRecord == true` means recording is generally supported
     - actual availability still depends on fatigue, contract, and active commitments

8. **Add lightweight tests or assertions**
   - If automated tests exist, add focused coverage for availability derivation.
   - At minimum cover:
     - signed artist with no blockers can record
     - unsigned artist cannot record
     - artist in active recording cannot start another recording
     - artist on active tour cannot take conflicting action if that rule exists
     - fatigued/burnout-blocked artist action becomes unavailable
   - If no test framework is practical, add temporary debug logging in the subsystem and keep logs concise.

9. **Keep UI changes minimal**
   - Only update UI bindings necessary to consume the new projection fields.
   - Do not embed business rules in widgets.
   - If no UI currently consumes availability, still expose the projection/API cleanly for follow-up tasks.

10. **Document assumptions in code**
   - Add short comments where rules are intentionally simplified due to current codebase limitations.
   - Example:
     - “Release planning availability currently depends only on mastered record existence and contract validity; date conflict checks remain command-level until release scheduling refactor.”

# Validation steps
1. **Build and compile**
   - Run:
     - `dotnet build`
   - If there are tests:
     - `dotnet test`

2. **Static validation in code**
   - Confirm there is a single authoritative path for artist action availability queries.
   - Confirm UI-facing projections read from that path rather than recomputing rules.
   - Confirm command validation uses the same path where applicable.

3. **Functional validation**
   - Verify these scenarios in code or tests:
     - a valid signed artist with no active commitments reports recording/tour actions correctly
     - an unsigned artist cannot access signed-only actions
     - an artist with an expired or missing contract is blocked where appropriate
     - an artist with an active recording is blocked from starting another recording
     - an artist with an active tour or conflicting commitment is blocked from conflicting actions
     - artist state blockers such as fatigue/burnout/scandal affect availability if those fields exist

4. **Projection validation**
   - Confirm artist detail or roster projection includes action availability data.
   - Confirm unavailable actions include a reason code and, if supported, a user-facing message.

5. **Regression validation**
   - Ensure existing command flows still compile and return structured failures instead of silent no-ops.
   - Ensure no widget now directly mutates artist state to decide availability.
   - Ensure no save/load schema is broken unintentionally by transient derived fields being serialized incorrectly.

6. **Logging/debug validation**
   - If log categories exist, emit concise debug logs only where useful.
   - Avoid noisy per-frame logging.

# Risks and follow-ups
- **Risk: duplicated authority across subsystems**
  - If artist availability depends on production/tour/contract state spread across systems, there is a risk of duplicated logic.
  - Mitigation: centralize evaluation in one query path and use narrow cross-subsystem reads.

- **Risk: existing UI may rely on old booleans**
  - Some widgets may still read legacy flags directly.
  - Mitigation: preserve legacy fields where needed, but route new bindings through the projection.

- **Risk: incomplete systems in current workspace**
  - The workspace hint suggests `.NET`, while the architecture targets Unreal/C++.
  - Mitigation: inspect the actual repository structure first and adapt implementation to the real codebase conventions. Do not force Unreal-specific patterns if the current implementation is in C# tooling or a hybrid setup.

- **Risk: over-scoping into full command refactors**
  - This task is about deriving availability, not rebuilding every command flow.
  - Mitigation: update only the commands directly impacted and leave clear TODOs for adjacent stories.

Follow-up recommendations after this task:
- Add a shared mapping from availability reason codes to localized UI text/tooltips.
- Expand availability to all command panel actions once more systems are implemented.
- Add automated tests around cross-subsystem commitment conflicts.
- Consider a dedicated artist detail projection struct if one does not yet exist.
- In later stories, ensure weekly artist state updates feed directly into these availability rules.