# Goal
Implement backlog task **TASK-7.4.5 — Do not store selected artist/open modal in simulation subsystems** for story **ST-104 Domain event pipeline cleanup**.

Refactor the code so that **transient UI state** such as:
- selected artist
- selected record/song if applicable
- open modal/dialog state
- current inspector target
- other presentation-only selection context

is **not owned or persisted by simulation/business subsystems**.

The target architecture is:
- **simulation subsystems own deterministic gameplay state only**
- **UI manager or a dedicated selection context subsystem owns transient UI state**
- **domain events remain typed and business-focused**
- **save/load excludes transient UI state by default**
- **existing UI flows continue to work**

If the current codebase does not yet have a dedicated selection subsystem, prefer the smallest safe implementation:
1. move transient selection/modal state into `UUIManagerSubsystem`, or
2. introduce a lightweight `USelectionContextSubsystem` only if clearly cleaner and low-risk.

Do not invent unrelated architecture. Keep the change focused on removing UI state from simulation subsystems and preserving behavior.

# Scope
In scope:
- Find all simulation subsystems currently storing UI/transient state, especially selected artist and modal/open-panel state.
- Remove that state from simulation-layer ownership.
- Add/adjust UI-facing APIs so widgets/screens can read/write selection context through the UI layer.
- Update event handling so UI refresh/navigation uses stable IDs from events rather than hidden “current selected” state in simulation.
- Ensure save/load does not serialize transient selection/modal state by default.
- Add compatibility shims only where necessary to avoid broad breakage during refactor.
- Update references/callers/tests affected by the ownership move.

Out of scope:
- Large UI redesigns.
- Rewriting the full event bus.
- Broad command-dispatcher refactors unless directly required by this task.
- Persisting optional UI state snapshots unless already established and clearly separated from simulation saves.
- New gameplay features.

Implementation constraints:
- Preserve current gameplay behavior.
- Prefer stable IDs over names.
- Keep widgets thin.
- Do not let widgets directly mutate simulation state as a workaround.
- Do not leave duplicate sources of truth for selection state.

# Files to touch
Inspect the repo first, then touch only the files needed. Likely candidates include:

- `UArtistManagerSubsystem` implementation/header
- any other simulation subsystem storing:
  - selected artist
  - selected entity
  - current modal/open dialog
  - current tab used for business logic
- `UUIManagerSubsystem` implementation/header
- any existing selection-context or screen-routing subsystem
- save/load snapshot structs and serialization code
- widgets/view models that currently query simulation for selected artist/modal state
- event subscription/translation code related to artist selection or modal opening
- tests covering subsystem state, save/load, or UI orchestration

Also inspect:
- domain event structs/payloads
- any compatibility helper methods like `GetSelectedArtist()`
- any save snapshot types that may currently include transient UI state in simulation snapshots

Do not modify generated/intermediate files.

# Implementation plan
1. **Audit current ownership of transient UI state**
   - Search for fields/properties/methods indicating UI state in simulation subsystems, such as:
     - `SelectedArtist`
     - `CurrentArtist`
     - `FocusedArtist`
     - `OpenModal`
     - `CurrentModal`
     - `Selected*`
     - `Hovered*`
     - `CurrentTab`
   - Classify each found state as:
     - true business state
     - transient presentation state
   - Only move transient presentation state.

2. **Identify the correct destination for selection/modal state**
   - Prefer existing `UUIManagerSubsystem` if it already handles routing/modals/selection.
   - If there is already a dedicated selection subsystem, use it.
   - Only create a new lightweight `USelectionContextSubsystem` if the current UI manager would become awkward or overloaded.

3. **Move selected artist ownership out of simulation**
   - Remove selected-artist storage from `UArtistManagerSubsystem` and any other simulation subsystem.
   - Replace with UI-layer ownership:
     - selected artist ID
     - selected record ID if needed
     - selected song ID if needed
   - Expose explicit UI-layer APIs such as:
     - `SetSelectedArtistId(...)`
     - `GetSelectedArtistId()`
     - `ClearSelectedArtist()`
   - If needed, emit UI-layer delegates/events for selection changes.

4. **Move modal/open-dialog ownership out of simulation**
   - Remove modal/open-panel state from simulation subsystems.
   - Put modal orchestration in `UUIManagerSubsystem` or equivalent.
   - Expose APIs like:
     - `OpenModal(EMusicModalType, Payload)`
     - `CloseModal(...)`
     - `GetCurrentModal()`
   - Keep payloads presentation-oriented and avoid embedding business mutation logic.

5. **Refactor callers**
   - Update widgets, controllers, and screen logic that previously read selected artist/modal state from simulation subsystems.
   - Route those reads/writes through the UI layer.
   - Where simulation actions previously relied on implicit “current selected artist,” change them to use:
     - explicit artist IDs from command payloads, or
     - UI selection context resolved before dispatch.
   - Avoid hidden dependencies on “current artist.”

6. **Preserve domain event cleanliness**
   - Ensure domain events remain business-focused and include stable IDs/dates as needed.
   - Do not add UI-only state to domain event payloads.
   - If UI needs to react to events by changing selection or opening a modal, do that in the UI layer after receiving the event.

7. **Remove transient state from persistence**
   - Inspect save snapshots for simulation subsystems.
   - Remove selected artist/open modal/transient selection state from simulation save data.
   - If there is an optional UI snapshot mechanism, keep it clearly separate and non-authoritative.
   - Ensure load reconstructs simulation first, then UI can initialize independently.

8. **Add compatibility shims where necessary**
   - If many callers currently use simulation APIs like `GetSelectedArtist()`, add temporary forwarding methods in the UI layer or a thin deprecated shim to minimize breakage.
   - Mark/refactor toward explicit ownership rather than keeping long-term duplication.
   - Do not keep mirrored state in both places.

9. **Update tests or add focused coverage**
   - Add or update tests to verify:
     - simulation subsystems no longer own transient selection/modal state
     - selection survives normal UI flow during runtime
     - save/load does not persist transient UI state in simulation snapshots
     - commands/business logic do not require hidden selected-artist state

10. **Keep the change minimal and coherent**
   - Prefer a focused refactor over broad architectural churn.
   - If you discover adjacent issues, note them in follow-up comments rather than expanding scope unless required to complete this task safely.

# Validation steps
Run the most relevant validation available in the workspace.

1. **Static/code validation**
   - Build the solution/project after refactor.
   - Use the available workspace commands first:
     - `dotnet build`
   - If there are tests:
     - `dotnet test`

2. **Codebase verification**
   - Confirm simulation subsystems no longer declare/store transient UI state for:
     - selected artist
     - open modal
     - similar presentation-only state
   - Confirm UI manager/selection subsystem now owns that state.

3. **Behavior verification**
   - Verify existing UI flows still work conceptually in code:
     - selecting an artist updates detail/inspector through UI-owned selection context
     - opening/closing modal routes through UI manager
     - business commands use explicit IDs rather than implicit simulation selection

4. **Persistence verification**
   - Confirm save/load paths do not serialize transient selection/modal state as part of simulation snapshots.
   - Confirm no simulation load logic depends on selected artist/modal state being present.

5. **Event pipeline verification**
   - Confirm domain events remain typed and business-oriented.
   - Confirm UI reactions to events happen in UI orchestration code, not by storing UI state in simulation subsystems.

6. **Regression scan**
   - Search for remaining references to old patterns such as simulation-owned:
     - `SelectedArtist`
     - `CurrentArtist`
     - `OpenModal`
     - `CurrentModal`
   - Resolve or document any intentional exceptions.

In the final implementation notes/summary, include:
- what state was moved
- where it now lives
- any compatibility shims added
- any remaining follow-up items

# Risks and follow-ups
Risks:
- Hidden coupling may exist where business logic implicitly depends on selected artist state.
- Save/load code may accidentally still serialize deprecated fields.
- Widgets may directly query simulation subsystems for convenience, causing partial refactor breakage.
- Event handlers may assume modal/selection changes happen inside simulation code.

Mitigations:
- Search broadly before editing.
- Prefer explicit IDs in commands and event handling.
- Remove old fields rather than leaving mirrored state.
- Add temporary compatibility shims only when necessary and keep them one-way/non-authoritative.

Follow-ups to note if encountered:
- Introduce a dedicated `USelectionContextSubsystem` if `UUIManagerSubsystem` becomes too overloaded.
- Standardize all UI navigation/selection on stable IDs.
- Add deprecation cleanup for temporary shim APIs.
- Add focused tests around UI selection context and event-driven screen refresh.
- Review other transient UI state (`hovered item`, `current tab`, filters) to ensure it is not leaking into simulation ownership.