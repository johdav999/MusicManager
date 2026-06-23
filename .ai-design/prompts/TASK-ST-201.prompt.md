# Goal
Implement backlog task **TASK-ST-201 — Artist state model expansion** for story **ST-201 Artist state model expansion**.

Expand the artist simulation model so artists have meaningful, persistent state that supports management gameplay and future systems. The implementation must align with the provided architecture: simulation state owned by subsystems, deterministic updates driven by time progression, stable IDs, thin UI, and read-only projections for presentation.

Deliver a vertical slice that introduces:
- richer artist state data
- deterministic artist condition updates on weekly/monthly cadence
- action availability derived from state and commitments
- read-only artist detail projections for UI consumption

Because no explicit acceptance criteria were provided on the task itself, implement against the story acceptance criteria and architecture notes.

# Scope
In scope:
- Expand the core artist state model to include:
  - attributes
  - personality traits
  - momentum
  - reputation
  - fatigue
  - burnout risk
  - scandal heat
- Ensure unsigned and signed artists share a compatible base model.
- Add deterministic weekly/monthly artist state update logic in the artist subsystem or the simulation phase it owns.
- Derive artist action availability from artist state plus active commitments.
- Expose artist detail data through read-only projection/view structs for UI.
- Preserve stable ID usage for artist references.
- Keep formulas simple and scalar-based, but structured so they can become data-driven later.
- Add logging where useful for debugging state changes.

Out of scope unless required by existing code coupling:
- Full relationship/cohesion simulation
- Deep scandal event generation
- Contract negotiation overhaul
- Tour system implementation
- UI redesign beyond wiring to new projections
- Save migration beyond minimal compatibility handling if current serialized artist state changes require it
- New generic command bus work outside what is necessary to keep artist actions queryable

Implementation expectations:
- Prefer additive, minimally disruptive changes.
- Reuse existing subsystem patterns and naming conventions in the repo.
- If the current codebase is earlier than the target architecture, introduce the smallest clean abstraction that moves it toward the desired design.
- Do not invent unrelated systems.

# Files to touch
Inspect the repo first and then update the exact files that own artist state, time progression, and artist-facing UI/view models. Likely targets include files matching these responsibilities/names:

- Artist domain model files
  - `*Artist*.h`
  - `*Artist*.cpp`
  - any structs representing artist save/state/data
- Artist subsystem/manager
  - `UArtistManagerSubsystem` or nearest equivalent
  - files such as `ArtistManagerSubsystem.h/.cpp`
- Time/simulation orchestration
  - `UGameTimeSubsystem` or equivalent weekly/monthly tick owner
  - files such as `GameTimeSubsystem.h/.cpp`
- UI projection/view model files
  - structs/classes for artist detail/roster display
  - files such as `ArtistDetailView*`, `ArtistViewModel*`, `StatusWidget*`, roster/detail widgets if they currently read raw mutable state
- Save/load files if artist state is serialized
  - `*Save*.h/.cpp`
  - snapshot structs for artists
- Logging/category declarations if artist simulation logs are added
- Tests if present
  - unit/integration tests around artist subsystem or simulation advancement

Before editing, identify the actual equivalents in this workspace and adapt naming to the existing codebase.

# Implementation plan
1. **Discover current artist model and simulation flow**
   - Find the current artist entity/state struct/class and document:
     - current fields
     - how artists are keyed
     - where signed/unsigned status is stored
     - where action eligibility is currently decided
     - how time advancement currently triggers artist updates
   - Find the current UI read path for artist details and roster rows.
   - Find save/load serialization for artist state if present.
   - If there is no dedicated artist subsystem, identify the nearest owner and keep changes localized.

2. **Design the expanded artist state model**
   - Introduce or extend nested structs to keep the model organized, ideally along these lines:
     - `FArtistAttributes`
       - Talent
       - Charisma
       - Reliability
       - MarketAppeal
     - `FArtistPersonality`
       - Ego
       - Teamwork
       - LifestyleRisk
       - optional placeholder/extensible traits if the codebase already supports them
     - `FArtistCareerState`
       - Momentum
       - Reputation
       - Fatigue
       - BurnoutRisk
       - ScandalHeat
   - Keep values scalar and bounded, preferably normalized or integer ranges consistent with the existing codebase.
   - Ensure both unsigned and signed artists use the same base state shape, with contract/label fields remaining optional or status-driven.
   - If the current model is flat, you may keep storage flat for compatibility but still expose grouped accessors/projections.

3. **Add bounded helpers and deterministic update utilities**
   - Implement helper functions for clamping and state mutation, e.g.:
     - clamp stat ranges
     - apply fatigue changes
     - derive burnout risk from fatigue/reliability/activity
     - decay scandal heat over time
     - momentum drift from activity/neglect
   - Keep formulas simple, deterministic, and inspectable.
   - Avoid randomness unless the existing simulation already uses a deterministic seeded source in this phase.
   - Prefer pure helper functions where possible so they are easy to test.

4. **Implement weekly/monthly artist state updates**
   - Add artist update entry points called by the simulation cadence, for example:
     - `AdvanceArtistsOneWeek(...)`
     - optional monthly rollup/update hook
   - Weekly updates should account for activity and neglect using currently available signals. Use existing commitments if present, such as:
     - active recording
     - active release cycle
     - active tour
     - idle/neglected state
   - Minimum expected behavior:
     - fatigue increases with active commitments and decreases modestly with rest
     - burnout risk tracks sustained fatigue and/or poor reliability
     - momentum rises with active productive/recent success states and decays with inactivity
     - reputation changes slowly and should not fluctuate wildly without explicit events
     - scandal heat decays over time unless another system raises it
   - If some upstream systems do not yet exist, implement graceful defaults and TODO hooks rather than blocking the story.

5. **Derive action availability from state and commitments**
   - Centralize artist action eligibility in the artist subsystem or a dedicated query helper.
   - Add a read-only action availability result/projection, e.g.:
     - can record
     - can tour
     - can sign
     - can schedule release-related actions if relevant
     - blocked reasons/messages or enum codes if the codebase supports them
   - Derive availability from:
     - artist status (signed/unsigned/etc.)
     - active commitments/conflicts
     - fatigue/burnout thresholds
     - scandal/availability flags if already modeled
   - Do not let widgets infer business rules themselves.

6. **Expose read-only artist detail projections**
   - Add or extend a projection struct such as `FArtistDetailView` / `FArtistDetailProjection`.
   - It should expose:
     - stable artist ID
     - display name
     - status
     - grouped attributes/personality/career state
     - derived action availability
     - any active commitment summary already available in the codebase
   - Ensure UI reads this projection rather than mutable internal maps/structs where practical.
   - Keep projections read-only and presentation-safe.

7. **Wire projections into existing UI access paths**
   - Update artist detail/roster/status UI code to request projections from the subsystem instead of directly reading mutable state, where feasible within task scope.
   - Keep widget changes minimal:
     - bind to new fields if already displayed
     - avoid adding business logic to widgets
   - If full UI migration is too broad, at minimum provide the projection API and update the primary artist detail path.

8. **Handle serialization compatibility**
   - If artist state is saved, update snapshot/save structs to include the new fields.
   - Preserve backward compatibility if there is an existing save schema:
     - initialize missing fields to safe defaults
     - avoid breaking deserialization
   - If there is no formal migration layer yet, add defensive defaulting and clear comments/TODOs.

9. **Add logging and debug visibility**
   - Use or add an artist log category consistent with the architecture, ideally `LogMusicArtists` if categories exist.
   - Log meaningful state transitions in development-friendly form:
     - weekly artist update summaries
     - action availability changes if useful
     - invalid/clamped values
   - Keep logs concise and gated appropriately.

10. **Keep implementation aligned with architecture**
   - Subsystem owns mutable artist business state.
   - Time subsystem/orchestrator triggers updates in deterministic order.
   - UI consumes projections only.
   - Stable IDs remain the lookup key.
   - No widget-driven mutation.

11. **Document assumptions in code comments**
   - Where formulas are placeholders, note that they are intentionally simple and should later become data-driven.
   - Mark integration hooks for tours/contracts/releases if those systems are not fully present yet.

# Validation steps
1. **Build and compile**
   - Run the appropriate build for the workspace after code changes.
   - Start with:
     - `dotnet build`
   - If the repo includes Unreal-generated managed tooling only and not the actual game build path, still ensure the touched code is internally consistent and note any missing native build path in your summary.

2. **Static code review checks**
   - Confirm artist state now includes:
     - attributes
     - personality traits
     - momentum
     - reputation
     - fatigue
     - burnout risk
     - scandal heat
   - Confirm unsigned and signed artists use a compatible base model.
   - Confirm stable IDs are still used for artist lookup and projection payloads.

3. **Simulation behavior checks**
   - Verify weekly advancement triggers artist updates exactly once per week.
   - Verify monthly advancement either:
     - triggers a monthly artist rollup, or
     - cleanly relies on weekly updates with documented monthly behavior
   - Verify deterministic behavior:
     - same starting state + same advancement sequence => same resulting artist state

4. **Action availability checks**
   - Validate at least these scenarios:
     - unsigned artist can be identified as signable
     - signed idle artist can record if not blocked
     - artist with active conflicting commitment is blocked appropriately
     - artist with excessive fatigue/burnout risk has restricted availability if thresholds are implemented
   - Ensure blocked reasons are consistent and not UI-derived.

5. **Projection/UI checks**
   - Verify artist detail projection returns the new state fields.
   - Verify UI-facing code reads projection data without mutating subsystem state.
   - If any widget was updated, confirm it still loads and handles missing/default values safely.

6. **Serialization checks**
   - If save/load exists for artists:
     - save an artist with non-default expanded state
     - load it back
     - verify values persist
   - If backward compatibility is relevant, verify missing fields default safely.

7. **Logging/debug checks**
   - Confirm artist update logs appear under the correct category if logging is enabled.
   - Ensure no noisy per-frame logging was introduced.

8. **Regression checks**
   - Existing artist roster/detail flows still function.
   - Time advancement still works.
   - No direct widget-to-subsystem mutation was added.

# Risks and follow-ups
- **Risk: current codebase may not yet match the target architecture**
  - The repo context suggests `.NET`, while the architecture is Unreal/C++.
  - First confirm whether the relevant Unreal source files are present in the workspace.
  - If not, implement only in the actual available project structure and clearly note any mismatch.

- **Risk: existing artist model may be tightly coupled to UI**
  - Minimize breakage by adding projection APIs first, then migrating the main detail path.
  - Leave compatibility shims where necessary.

- **Risk: save schema changes may break old campaigns**
  - Use safe defaults for newly added fields.
  - Avoid hard failures unless the existing load path already enforces strict schema matching.

- **Risk: missing upstream systems for commitments**
  - If recording/tour/release commitments are not fully modeled yet, derive availability from whatever authoritative state exists and leave explicit TODO hooks for ST-202/ST-301/ST-601 integration.

- **Risk: overcomplicated formulas too early**
  - Keep formulas simple, deterministic, and bounded.
  - Prefer maintainable placeholder logic over speculative realism.

Follow-ups after this task:
- Make artist formulas data-driven via DataAssets/DataTables.
- Integrate contract lifecycle effects more deeply in ST-202.
- Feed recording/release/tour outcomes into momentum/reputation/scandal systems.
- Add tests around artist weekly updates and action eligibility.
- Expand UI to show richer artist state trends/history once projections are stable.