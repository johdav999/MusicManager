# Goal
Implement backlog task **TASK-8.1.2 — Weekly/monthly updates adjust artist condition based on activity and neglect** for story **ST-201 Artist state model expansion**.

Add deterministic artist condition progression to the simulation so that, on weekly advancement and monthly rollup, each artist’s mutable career state updates based on recent activity and neglect. This should fit the existing architecture where simulation logic lives in subsystems, time progression is orchestrated centrally, and UI reads artist state through projections rather than mutating it directly.

The implementation should:
- update artist condition during the simulation cadence
- account for both **activity** and **neglect/inactivity**
- remain deterministic and data-driven where practical
- avoid introducing UI-owned business logic
- preserve compatibility with current save/load and existing artist flows as much as possible

# Scope
In scope:
- Extend artist simulation state and/or supporting tracking needed to evaluate recent activity
- Hook artist condition updates into the weekly and monthly simulation flow
- Implement formulas/rules for condition changes from:
  - recording / production activity
  - touring or scheduled live activity if already represented
  - release-related workload if already represented
  - inactivity / neglect over time
- Update artist read-only projections/view models so UI can display the resulting values
- Add logging/debug visibility for artist condition updates
- Add or update automated tests for deterministic weekly/monthly artist state progression

Out of scope unless required by existing code structure:
- Large UI redesigns
- New widgets/screens
- Deep personality relationship simulation
- New save migration framework beyond the minimum needed for added fields
- Refactoring unrelated systems
- Implementing full data-asset balancing pipeline if the project currently uses constants; prefer a minimal extensible config approach

# Files to touch
Inspect the repo first and adjust to actual names, but expect to touch files in these areas:

- **Time/simulation orchestration**
  - `Source/.../GameTimeSubsystem.*`
  - or equivalent simulation director / time advancement service

- **Artist domain**
  - `Source/.../ArtistManagerSubsystem.*`
  - artist state structs/models, likely under:
    - `Source/.../Models/...`
    - `Source/.../Simulation/...`
    - `Source/.../Artists/...`

- **Shared simulation data / config**
  - constants, tuning structs, or data asset definitions for artist condition rules
  - if no config exists, add a small internal tuning struct in the artist subsystem first

- **Read-only projections / view models**
  - artist detail/roster projection structs or query methods
  - any DTOs returned to UI

- **Persistence**
  - save snapshot structs if artist mutable state gains new serialized fields
  - load defaults for backward compatibility if applicable

- **Tests**
  - unit/integration tests around weekly advancement and artist state updates
  - deterministic simulation tests if present

- **Logging/debug**
  - artist log category usage or debug dump helpers

Do not invent broad new folders if the project already has an established layout. Follow existing naming and module conventions.

# Implementation plan
1. **Discover current simulation and artist model**
   - Find the authoritative artist state struct/class.
   - Identify existing mutable fields for:
     - momentum
     - reputation
     - fatigue
     - burnout risk
     - scandal heat
     - availability / commitments
   - Find where weekly and monthly advancement currently happens.
   - Find how artist activity is currently represented:
     - active recording
     - active release
     - active tour
     - recent events
     - last active date/week if already present

2. **Define minimal condition-update inputs**
   Implement the smallest robust model that supports “activity and neglect” without overreaching. Prefer adding explicit tracking fields if missing, such as:
   - `LastMeaningfulActivityWeek` or equivalent
   - current activity flags derived from subsystem state:
     - recording this week
     - touring/show this week
     - release campaign active this week
   - optional monthly counters:
     - weeks active this month
     - weeks neglected this month

   If direct cross-subsystem reads are needed, keep them query-based and deterministic. Avoid circular ownership.

3. **Add/update artist condition rule function**
   In `UArtistManagerSubsystem` or the artist simulation owner, add a focused method such as:
   - `ProcessWeeklyArtistCondition(...)`
   - `ApplyWeeklyConditionUpdate(...)`
   - `ProcessMonthlyArtistConditionRollup(...)`

   The weekly update should:
   - inspect each artist in deterministic order
   - derive activity level for the current week
   - increase fatigue/burnout risk for heavy activity
   - reduce momentum or adjust condition for prolonged neglect
   - optionally allow mild recovery when inactive but not neglected
   - clamp all scalar values to valid ranges

   Suggested baseline behavior if no prior formulas exist:
   - **Recording active:** small fatigue increase, slight momentum support
   - **Tour/show active:** larger fatigue increase, possible burnout risk increase
   - **Recent release/promo active:** small fatigue increase, slight momentum/reputation support
   - **Inactive for short period:** slight fatigue recovery
   - **Neglected for sustained period:** momentum decay and possible reputation drift down
   - **Very high fatigue:** increase burnout risk
   - **Low fatigue over time:** gradual burnout risk recovery

   Keep formulas simple, transparent, and easy to tune.

4. **Add monthly rollup behavior**
   Since architecture calls for weekly ticks with monthly summaries, implement a monthly pass that either:
   - aggregates the weekly effects into a monthly summary event/log, or
   - applies additional neglect/conditioning adjustments once per month

   Good minimal monthly behavior:
   - if artist had no meaningful activity for the full month, apply an extra neglect penalty
   - if artist was overworked most weeks, apply extra burnout risk
   - reset monthly counters after processing

   Ensure this runs from the time subsystem’s month-close event/phase, not from UI.

5. **Integrate into simulation order**
   Hook the artist condition update into the simulation phase order under the artist/personality/condition phase described in architecture.

   Requirements:
   - run before downstream systems that depend on artist condition, if applicable
   - be deterministic across saves/reloads
   - avoid hidden side effects in widget code

   If there is already a weekly phase callback in `UGameTimeSubsystem`, register/call the artist subsystem there. If monthly close exists, add the monthly rollup there too.

6. **Update projections for UI**
   Ensure artist detail/roster projections expose the updated condition values read-only. If useful and already consistent with the codebase, add derived display fields such as:
   - current activity status
   - neglect status / inactive weeks
   - condition summary text or enum

   Do not put simulation calculations in widgets.

7. **Persistence compatibility**
   If new mutable fields are added to artist state:
   - serialize them in save snapshots
   - provide safe defaults on load for older saves or missing data
   - avoid breaking existing campaigns if possible

   If the project does not yet have formal migration hooks, implement defensive defaults rather than a large migration system.

8. **Observability**
   Add concise logs under the artist log category for weekly/monthly condition processing, especially in development builds. Include:
   - artist ID
   - activity classification
   - fatigue delta
   - burnout delta
   - neglect delta
   - resulting values

   Keep logs structured and not overly noisy in shipping builds.

9. **Tests**
   Add deterministic tests covering at least:
   - active artist gains fatigue from work
   - inactive artist recovers some fatigue short-term
   - neglected artist loses condition/momentum after sustained inactivity
   - monthly rollup applies expected extra adjustment
   - values clamp correctly
   - save/load preserves new artist condition fields if persistence tests exist

10. **Implementation constraints**
   - Follow existing code style and Unreal patterns
   - Prefer small, composable methods over one large tick function
   - Do not let widgets directly mutate artist condition
   - Do not key anything by artist display name
   - Keep behavior deterministic and testable

# Validation steps
1. **Static/code review checks**
   - Confirm artist condition updates are owned by simulation/business layer, not UI.
   - Confirm weekly and monthly hooks are wired through the time/simulation subsystem.
   - Confirm stable artist IDs are used in logs/events/queries.

2. **Build**
   - Run:
     - `dotnet build`
   - If there are test projects:
     - `dotnet test`

3. **Behavior verification**
   Create or use test fixtures for artists in these scenarios:
   - **Recording artist:** advance 1–4 weeks and verify fatigue increases predictably
   - **Touring artist:** verify stronger fatigue/burnout pressure than recording-only
   - **Idle but recently active artist:** verify some recovery rather than immediate neglect penalty
   - **Neglected artist:** advance several weeks/month close and verify momentum/condition penalties
   - **Overworked artist:** verify burnout risk trends upward and clamps correctly
   - **Recovered artist:** verify burnout risk can trend down when workload drops

4. **Determinism verification**
   - Run the same simulation setup twice and confirm identical artist outputs after N weeks.
   - If save/load exists, save before week advance, reload, advance, and confirm same result as uninterrupted simulation.

5. **Projection/UI verification**
   - Verify artist detail/roster query methods return updated condition values after time advances.
   - Confirm no widget-side recalculation is required for correctness.

6. **Logging/debug verification**
   - In development mode, confirm artist condition updates emit readable logs/debug summaries.
   - Ensure logs are not spammy or dependent on non-deterministic ordering.

# Risks and follow-ups
- **Risk: unclear existing ownership of artist state**
  - The repo may already split artist data across multiple systems. Keep the authoritative mutation point singular, even if activity inputs come from other subsystems.

- **Risk: monthly-only legacy logic**
  - Existing code may still be monthly-driven. If so, wrap legacy behavior carefully and avoid double-applying updates on both weekly and monthly paths.

- **Risk: no explicit activity tracking**
  - If the codebase lacks “last active week” or similar fields, add the minimum necessary tracking rather than inferring from UI state or transient selections.

- **Risk: save compatibility**
  - New artist fields may require defaults for old saves. Be defensive during deserialization.

- **Risk: formula creep**
  - Keep formulas simple in this task. Do not overdesign a full morale/personality simulation.

Follow-ups after this task:
- move tuning values into DataAssets/DataTables if currently hardcoded
- expose a richer artist condition summary in UI projections
- add debug panel support for artist hidden-state breakdowns
- integrate condition more deeply into action eligibility and contract/risk systems
- add domain events/news for major burnout or neglect milestones if desired