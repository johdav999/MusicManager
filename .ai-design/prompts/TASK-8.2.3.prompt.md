# Goal
Implement backlog task **TASK-8.2.3** for **ST-202 — Contract signing and lifecycle** so that:

- **active** and **expired** contracts are tracked separately
- contract lifecycle state updates automatically when **time advances**
- contract expiration updates associated artist availability/state
- expiration emits the appropriate domain event(s)
- behavior is deterministic, save/load-safe, and aligned with the existing subsystem architecture

This should be implemented as a coding task in the current workspace, favoring the existing project patterns over introducing new frameworks.

# Scope
In scope:

- Find the current contract, artist, and time-advancement implementation
- Add or refine a contract state model that distinguishes:
  - active contracts
  - expired contracts
- Ensure time advancement triggers contract lifecycle evaluation
- Prevent an artist from having more than one active contract at a time
- Update artist state/availability when a contract expires
- Emit a typed event or equivalent notification when a contract expires
- Ensure persistence includes the separated contract lifecycle state if contracts are already saved
- Add or update tests covering lifecycle transitions

Out of scope unless required by existing code structure:

- Full renegotiation flows
- Deep contract negotiation UI
- Option clauses / renewals
- New presentation-heavy UI work
- Broad refactors unrelated to contract lifecycle
- Reworking the entire save system beyond what is necessary for this task

# Files to touch
Inspect the workspace first, then update the smallest correct set of files. Likely areas include:

- **Contract domain/model files**
  - contract structs/classes/enums
  - artist-contract linkage models
- **Time/simulation orchestration**
  - `UGameTimeSubsystem`
  - simulation director / weekly advancement flow
- **Artist management**
  - `UArtistManagerSubsystem`
  - artist availability/status updates
- **Command handling**
  - sign artist / negotiate contract command execution
  - validation for duplicate active contracts
- **Events**
  - domain event definitions
  - event dispatch / notification hooks
- **Persistence**
  - save snapshot structs for contracts/artists
  - load validation if present
- **Tests**
  - unit/integration tests for contract expiration on time advancement

Because the workspace hint says `.net` while the architecture describes Unreal/C++, first determine the actual implementation language and test setup in this repo before editing. If this repo contains a simulation library in C#, implement within that actual stack rather than forcing UE-specific code.

# Implementation plan
1. **Discover the current implementation**
   - Search for:
     - contract models/types
     - artist models/types
     - time advancement logic
     - sign artist command handling
     - save/load snapshots
     - tests around contracts or time progression
   - Identify whether the simulation is currently implemented in:
     - Unreal C++
     - C#/.NET support library
     - hybrid code
   - Follow existing naming and architectural conventions.

2. **Define or refine contract lifecycle state**
   - Introduce or confirm an explicit contract status enum/state such as:
     - `Active`
     - `Expired`
   - Ensure each contract has enough data to evaluate lifecycle:
     - `ContractId`
     - `ArtistId`
     - `LabelId`
     - `StartDate`
     - `EndDate`
     - `Status`
   - If the codebase already stores contracts in collections, prefer one of these patterns:
     - separate active/expired collections, or
     - one master collection plus indexed/queryable active vs expired views
   - Match the task wording by making active and expired contracts clearly tracked separately in state and/or API surface.

3. **Add lifecycle evaluation on time advancement**
   - Hook contract evaluation into the existing time advancement pipeline at the correct deterministic point.
   - On each relevant advance step:
     - inspect active contracts
     - detect contracts whose `EndDate` is now before or equal to current game date according to existing date semantics
     - transition them to expired exactly once
   - Avoid repeated expiration processing for already expired contracts.

4. **Update artist state when contract expires**
   - When a contract expires:
     - clear or update the artist’s active contract reference if applicable
     - update artist availability/status so the artist can be signed again if that matches current rules
     - ensure action eligibility reflects no active contract
   - Preserve consistency between artist state and contract state.

5. **Enforce duplicate-active-contract prevention**
   - In sign/contract creation validation:
     - reject creating a new active contract if the artist already has an active contract
   - If load validation exists, detect and report duplicate active contracts for one artist.

6. **Emit lifecycle events**
   - Add or use a typed event equivalent to:
     - `ArtistContractExpired`
     - or `ContractExpired`
   - Event payload should include stable identifiers and date:
     - `ContractId`
     - `ArtistId`
     - `LabelId` if available
     - expiration/effective date
   - Dispatch the event from the lifecycle transition path, not from UI code.

7. **Persist the lifecycle state**
   - If contracts are already serialized:
     - include status and any separated active/expired representation needed
   - If artists store a contract reference:
     - ensure expired references are not restored as active after load
   - Keep save/load backward-compatible if versioning/migration already exists.

8. **Add tests**
   - Add focused tests for:
     - signing creates an active contract
     - advancing time past end date moves contract from active to expired
     - artist state updates when contract expires
     - expiration event is emitted once
     - duplicate active contracts are rejected
     - save/load preserves expired vs active state if test infrastructure exists
   - Prefer deterministic tests with fixed dates.

9. **Keep implementation minimal and aligned**
   - Do not introduce speculative abstractions.
   - Reuse existing date, event, command, and persistence patterns.
   - If the repo lacks a formal event bus, use the existing notification/delegate mechanism consistently.

# Validation steps
1. **Static inspection**
   - Confirm all contract lifecycle transitions compile cleanly in the repo’s actual stack.
   - Verify no broken references from artist, contract, time, or save models.

2. **Build**
   - Run the repo-appropriate build command after determining the real project structure.
   - Start with:
     - `dotnet build`
   - If there are test projects, also build them explicitly if needed.

3. **Tests**
   - Run:
     - `dotnet test`
   - If there are targeted tests for simulation/contracts, run those first and then the full suite.

4. **Behavior verification**
   - Verify these scenarios through tests or existing harnesses:
     - contract is active immediately after signing
     - contract remains active before end date
     - contract expires when time advances to/past the end date per existing date rules
     - expired contract is no longer returned in active contract queries
     - expired contract is returned in expired/history queries
     - artist becomes available / no longer marked under active contract
     - duplicate active contract creation is blocked
     - expiration event fires once only

5. **Persistence verification**
   - If save/load exists in this repo:
     - save with one active and one expired contract
     - reload
     - verify separation and artist linkage remain correct

6. **Regression check**
   - Ensure signing flow still posts finance/advance behavior if already implemented.
   - Ensure time advancement still works for unrelated systems.

# Risks and follow-ups
- **Repo/stack mismatch risk:** The architecture says Unreal/C++, but workspace hints suggest `.NET`. Confirm the actual implementation location before coding.
- **Date semantics risk:** Expiration may depend on whether contracts expire on `EndDate` or after it. Preserve existing game-date conventions and document the chosen rule in code/tests.
- **State duplication risk:** If both artist and contract objects store lifecycle info, they can drift out of sync. Centralize transition logic in one owner path.
- **Persistence risk:** Older saves may not contain explicit contract status. If save versioning exists, add a minimal migration/defaulting rule.
- **Event duplication risk:** Fast-forward or repeated lifecycle scans can emit duplicate expiration events unless transitions are idempotent.
- **Query/API risk:** Existing code may assume a single contract list. Update query helpers carefully so callers can explicitly request active vs expired contracts.

Follow-up suggestions after this task:
- add contract history projections for UI
- add load-time integrity validation for duplicate active contracts
- add monthly/weekly summary news generation for expirations
- prepare for future renegotiation/renewal flows without implementing them now