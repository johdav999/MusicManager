# Goal
Implement backlog task **TASK-8.2.5 — Initial scope can omit renegotiation and option clauses UI depth** for **ST-202 Contract signing and lifecycle**.

This task should ensure the contract system and contract-related UI support the core story requirements for signing and lifecycle management **without building deep UI flows for renegotiation, renewal negotiation, or option clause management**.

The coding agent should:
- preserve the core contract signing flow
- support contract lifecycle tracking and expiration behavior
- keep UI thin and focused on initial signing inputs only
- explicitly avoid adding complex renegotiation/option-management screens, modal trees, or advanced editing workflows
- leave clean extension points for future renegotiation and option clause features

Use the architecture constraints:
- Unreal Engine gameplay architecture principles still apply conceptually: **subsystems own business state, widgets are thin, commands mutate state, events notify UI**
- if the current workspace is not UE-native, adapt these principles to the existing project structure rather than forcing Unreal-specific code patterns
- prefer typed models/DTOs/view models over stringly-typed UI state

# Scope
In scope:
- identify the current contract signing/lifecycle implementation for ST-202
- simplify or constrain UI/view-model behavior so the initial contract UX only exposes:
  - advance
  - royalty rate
  - term length and/or commitment inputs already required by the story
- ensure contract data model can still contain option-related fields if already present, but the UI does **not** need deep editing/management for them
- ensure no renegotiation workflow is required for completion of this task
- add clear placeholders/TODOs/extension seams for future:
  - renegotiation
  - option exercise flows
  - richer contract amendment UI
- update any validation, command handling, and UI messaging so unsupported actions fail gracefully or remain hidden
- keep acceptance aligned to ST-202:
  - typed signing command
  - validation for availability and affordability
  - active vs expired contract tracking
  - expiration events and artist availability updates
  - prevention of duplicate active contracts

Out of scope:
- full renegotiation command flow
- option clause exercise workflow
- contract amendment history UI
- legal/advanced clause editor
- multi-step negotiation minigame
- deep renewal UX at expiration
- any unrelated finance/save/load refactors unless required to keep build/tests passing

# Files to touch
Inspect the repo first and then touch only the minimum required files. Prioritize files related to:
- contract domain models/entities
- contract signing commands/handlers/services
- artist availability/lifecycle logic
- UI/view models/screens/forms for contract signing
- tests covering contract signing and expiration behavior
- documentation/comments where unsupported future features need to be clarified

Likely file categories to inspect:
- contract models, DTOs, or entities
- command/request models for signing artists/contracts
- service/subsystem classes handling contract lifecycle
- UI form/viewmodel/controller for contract negotiation/signing
- event definitions for contract expiration
- tests for duplicate active contracts, affordability, and expiration

Do **not** broaden changes into unrelated systems unless necessary.

# Implementation plan
1. **Discover the current ST-202 implementation**
   - Find all code related to:
     - contract model/state
     - sign contract command
     - contract UI
     - expiration/lifecycle updates
   - Map where renegotiation or option clause concepts already appear.
   - Determine whether the current codebase is UE/C++ or a .NET support/prototype layer, and implement in the native style of the repo.

2. **Define the reduced initial UX contract scope**
   - Keep the signing UI limited to the minimum story inputs:
     - advance
     - royalty rate
     - term length and/or album commitment
   - If option-related fields already exist in the model:
     - keep them in persistence/domain if needed
     - remove/hide/disable deep UI editing for them
   - If renegotiation actions/buttons/routes exist:
     - hide, disable, or mark not yet supported
     - ensure they do not break navigation or tests

3. **Preserve domain completeness while reducing UI depth**
   - Do not delete useful domain fields unless they are dead and clearly harmful.
   - Prefer:
     - keeping `OptionsRemaining` or similar as passive data
     - omitting advanced UI controls for editing/exercising those fields
   - Add comments/TODO markers where future renegotiation/option workflows should plug in.

4. **Constrain command surface**
   - Ensure the supported command path is the initial signing flow only.
   - If renegotiation commands already exist but are incomplete:
     - either leave them untouched and unreachable from UI
     - or make them return a structured “not supported in initial scope” result
   - Avoid exposing partial workflows that mutate state inconsistently.

5. **Validate lifecycle behavior remains complete**
   - Confirm active and expired contracts are tracked distinctly.
   - Confirm time advancement or lifecycle processing:
     - expires contracts correctly
     - emits expiration events if event infrastructure exists
     - updates artist availability/actions
     - prevents duplicate active contracts
   - If any of this is missing, implement only what is necessary to satisfy ST-202.

6. **Adjust UI/view models**
   - Simplify labels/help text to reflect MVP scope.
   - Remove references implying full renegotiation or option management if those features are not implemented.
   - If there is a contract details screen, it may display passive fields, but should not imply editable advanced clauses.
   - Keep widgets/views thin; move business decisions to service/command layer if currently embedded in UI.

7. **Add or update tests**
   - Add focused tests for:
     - signing with valid core terms succeeds
     - signing fails when artist is unavailable
     - signing fails when player cannot afford advance
     - duplicate active contract is prevented
     - contract expiration transitions active -> expired
   - If renegotiation/option UI actions exist:
     - test they are hidden, disabled, or return a clear unsupported result
   - Do not over-engineer broad test coverage beyond this task.

8. **Document intentional scope cut**
   - Add concise inline comments or developer notes stating:
     - renegotiation UI depth intentionally omitted in initial scope
     - option clause management UI intentionally deferred
     - future extension points should use typed commands and lifecycle-safe updates

# Validation steps
1. **Build**
   - Run:
     - `dotnet build`
   - If there is a narrower project/test target discovered during implementation, use it in addition to the root build.

2. **Test**
   - Run:
     - `dotnet test`
   - If tests are too broad or expensive, run the most relevant contract-related test project(s) first, then the full suite if feasible.

3. **Manual verification**
   - Verify the contract signing flow only asks for core initial terms.
   - Verify no deep renegotiation or option-management UI is exposed.
   - Verify unsupported actions are either:
     - absent
     - disabled with clear messaging
     - or return a structured not-supported result
   - Verify contract expiration still updates lifecycle state correctly.

4. **Regression checks**
   - Confirm no duplicate active contracts can be created for one artist.
   - Confirm finance affordability validation still applies at signing.
   - Confirm artist availability changes correctly on sign/expire.
   - Confirm any existing save/load or serialization code still compiles if contract fields remain present but less exposed in UI.

5. **Implementation summary**
   - In your final report, include:
     - files changed
     - what was simplified in UI
     - whether option fields remain in domain only
     - any deferred hooks/TODOs added
     - test/build results

# Risks and follow-ups
Risks:
- renegotiation or option logic may already be partially intertwined with the signing UI, making “hide only” changes insufficient
- removing UI fields without checking command validation may create mismatches between required inputs and defaults
- contract lifecycle may currently rely on UI-driven assumptions rather than centralized service logic
- if the repo is only a partial scaffold, relevant code may be missing and the task may require creating minimal structure rather than editing existing files

Follow-ups:
- add a dedicated renegotiation command and lifecycle-safe amendment model later
- add option exercise workflow later, likely as a separate command and focused modal/screen
- add richer contract detail/history presentation once the domain rules are finalized
- add explicit acceptance criteria for deferred contract features in a future backlog item
- if unsupported actions currently surface in UX, create a future cleanup task for product copy and navigation polish