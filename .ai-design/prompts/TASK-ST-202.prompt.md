# Goal

Implement **TASK-ST-202 — Contract signing and lifecycle** for the Unreal-based music management simulation.

Deliver a production-ready vertical slice of artist contract management that fits the existing architecture:

- typed command-driven signing flow
- contract domain/state model with stable IDs
- validation for artist availability and player affordability
- finance posting for signing advances
- lifecycle updates during time advancement
- separation of active vs expired contracts
- contract expiration events that update artist availability/actions

This task should be implemented in a way that is consistent with the architecture principles:

- subsystems own business state
- widgets are thin
- commands mutate state
- events notify UI
- save/load compatibility is preserved or prepared
- deterministic weekly simulation remains intact

There were no explicit acceptance criteria beyond the backlog story, so use the backlog story details as the source of truth and fill in reasonable implementation details conservatively.

# Scope

Implement the minimum complete feature set for **ST-202**:

1. **Contract data model**
   - Add or extend a mutable contract state struct/class with:
     - `ContractId`
     - `ArtistId`
     - `LabelId`
     - `StartDate`
     - `EndDate`
     - `Advance`
     - `RoyaltyRate`
     - `AlbumCommitment` and/or term length fields
     - `Status` at minimum supporting `Active` and `Expired`
   - Use stable IDs, never artist names as keys.

2. **Contract ownership and lookup**
   - Add a subsystem responsible for contract state if one already exists, otherwise place contract ownership in the most appropriate existing simulation subsystem with clear boundaries.
   - Support:
     - create contract
     - query active contract by artist
     - query contracts by artist
     - query active vs expired contracts separately
     - expire contracts deterministically on time advancement

3. **Typed signing command**
   - Add a typed command payload for signing/negotiating an artist with:
     - `ArtistId`
     - `LabelId`
     - `Advance`
     - `RoyaltyRate`
     - term length and/or commitment input
   - Route through the command dispatcher/application layer, not directly from UI.

4. **Validation**
   - Signing must fail cleanly if:
     - artist does not exist
     - artist is unavailable for signing
     - artist already has an active contract
     - player cannot afford the advance
     - contract terms are invalid/out of allowed range
   - Return structured success/failure result with user-facing message.

5. **Finance integration**
   - On successful signing, post the advance as a finance ledger entry and update balance.
   - Do not mutate finance state from UI.

6. **Artist state integration**
   - On successful signing:
     - artist becomes signed/unavailable for unsigned actions
     - artist references the active contract ID if the artist model supports it
   - On expiration:
     - artist availability/actions update appropriately
     - active contract link is cleared or transitioned safely

7. **Lifecycle updates**
   - Hook contract expiration into deterministic time advancement.
   - Contracts whose end date has passed should transition to expired in a predictable simulation phase.
   - Emit expiration events once only.

8. **Domain events**
   - Emit typed events for:
     - contract signed / artist signed
     - contract expired
   - Payloads should include stable IDs and relevant dates.

9. **Persistence readiness**
   - If save/load already exists for contracts, update it.
   - If save/load is not yet implemented for this area, structure code so it is snapshot-friendly and does not block ST-402/ST-403.

10. **UI integration only where necessary**
   - If there is already a roster/detail/command panel flow, wire it to the typed command path.
   - Do not build deep new UI unless required to complete the flow.
   - Prefer compatibility shims over broad UI rewrites.

Out of scope unless already trivial in the codebase:

- renegotiation
- option clauses depth
- complex legal terms
- multi-label bidding wars
- deep contract negotiation UI
- royalties settlement logic beyond storing the rate and posting the advance
- save migration for old contract schemas unless already present and easy to add safely

# Files to touch

Inspect the workspace first and adapt to actual project structure. Touch only the files needed. Likely areas:

- **Command layer**
  - command structs/header(s)
  - command dispatcher subsystem/service
  - command result/error code definitions

- **Simulation/domain**
  - contract state structs/enums
  - artist state structs if they need contract linkage/status updates
  - game date/time integration points
  - relevant subsystem headers/cpps for artist/contracts

- **Finance**
  - finance subsystem/service for posting advance ledger entries
  - ledger entry types/enums if contract advances need a dedicated type

- **Events**
  - domain event structs/delegates
  - event subsystem/news bridge if already present

- **Persistence**
  - save snapshot structs if contracts are already serialized
  - validation helpers if contract integrity checks already exist

- **UI glue**
  - existing artist roster/detail/command panel widgets or view-model adapters only if they currently bypass the command path

- **Tests**
  - any existing unit/integration test project
  - if no tests exist, add focused automated coverage in the most appropriate existing test location

Before editing, identify the concrete equivalents of these likely files in the repo and use those rather than inventing parallel systems.

# Implementation plan

1. **Survey the codebase**
   - Find existing implementations for:
     - `UGameTimeSubsystem`
     - artist manager/subsystem
     - finance manager/subsystem
     - command dispatcher
     - event subsystem
     - save/load structs
   - Determine whether a contract subsystem already exists in partial form.
   - Determine whether the project is currently UE C++ only, mixed with C#, or has placeholder .NET tooling only.
   - Follow existing naming/style conventions.

2. **Define/extend contract domain model**
   - Add a contract status enum, e.g. `Active`, `Expired`.
   - Add a contract state struct with stable IDs and dates.
   - Ensure fields are serializable/snapshot-friendly.
   - Add helper methods for:
     - active check by date/status
     - expiration eligibility
     - basic term validation

3. **Integrate contract storage**
   - If there is an existing contract manager/subsystem, extend it.
   - Otherwise add a focused subsystem/service for contract state ownership.
   - Maintain:
     - map/list of contracts by `ContractId`
     - lookup from `ArtistId` to active contract
     - query helpers for active/expired contracts
   - Prevent duplicate active contracts for the same artist.

4. **Add typed signing command**
   - Create a command struct such as `FSignArtistCommand` or `FNegotiateContractCommand`.
   - Include:
     - artist ID
     - label ID
     - advance
     - royalty rate
     - term length and/or album commitment
   - Add dispatcher entry point such as `ExecuteSignArtist(...)`.

5. **Implement validation**
   - Validate in the command/application layer and/or owning subsystem:
     - artist exists
     - artist is signable
     - no active contract exists
     - label/player has enough funds
     - numeric terms are within sane bounds
     - derived end date is valid
   - Return structured failure with clear message and error code.
   - Do not partially mutate state on failure.

6. **Implement successful signing flow**
   - Generate a stable `ContractId`.
   - Create and register the contract.
   - Update artist state:
     - signed status / current label
     - active contract ID
     - action availability flags if present
   - Post finance ledger entry for the advance.
   - Emit contract signed / artist signed event.
   - Return success result with created contract ID if the result type supports payloads.

7. **Hook lifecycle into time advancement**
   - Add a deterministic contract lifecycle update step to the weekly advancement flow.
   - Prefer a clearly named phase or a call from an existing artist/contracts phase.
   - On each relevant tick:
     - find active contracts whose end date has passed
     - mark them expired
     - update artist state and availability
     - emit expiration event once
   - Ensure this is deterministic and idempotent.

8. **Event integration**
   - Add typed event payloads for:
     - contract signed
     - contract expired
   - Include:
     - `ContractId`
     - `ArtistId`
     - `LabelId`
     - effective/signing/expiration date as appropriate
   - If `UEventSubsystem` or news generation already exists, wire these events into it minimally without overbuilding.

9. **Finance integration**
   - Add or reuse a ledger entry type for contract advances.
   - Ensure balance changes are emitted through existing finance events.
   - Keep finance append-only where possible.

10. **Persistence alignment**
   - If contract snapshots already exist, update serialization/deserialization.
   - If not, at least ensure structs are ready for future save inclusion and references are stable.
   - Add validation helpers for duplicate active contracts and broken artist references if there is an existing validation path.

11. **UI command path cleanup**
   - If any current signing flow mutates artist state directly, reroute it through the command dispatcher.
   - Keep widget changes minimal.
   - Preserve existing UX where possible.

12. **Add tests**
   - Add focused automated tests for the core rules:
     - signing succeeds for valid unsigned artist with sufficient funds
     - signing fails when artist already has active contract
     - signing fails when funds are insufficient
     - advance posts finance ledger entry
     - contract expires on/after expected date during time advancement
     - expiration updates artist availability and emits event once
   - If automated tests are not feasible in current setup, add at least a small deterministic test harness or clearly documented validation path.

13. **Document assumptions**
   - If the codebase lacks pieces assumed by the architecture, implement the smallest compatible version and note it in comments or task notes.
   - Do not introduce speculative generic frameworks.

# Validation steps

Run the most appropriate validation available in the repo after implementation.

1. **Build / compile**
   - Start by discovering the correct build path for this workspace.
   - If the provided commands are the only available automation, run:
     - `dotnet build`
   - If there are Unreal-native build steps or project generation scripts already documented in `README.md`, use those as the primary validation path.

2. **Automated tests**
   - Run:
     - `dotnet test`
   - If there are UE automation tests, run the relevant contract/artist/finance tests as well.

3. **Manual functional verification**
   - Verify a valid signing flow:
     - choose an unsigned artist
     - submit signing command with valid terms
     - confirm success result
     - confirm artist becomes signed
     - confirm active contract exists
     - confirm finance balance decreases by advance
     - confirm ledger entry is posted
   - Verify invalid signing cases:
     - insufficient funds
     - duplicate active contract
     - invalid artist ID
     - invalid term values
   - Verify lifecycle:
     - advance time until contract end
     - confirm contract transitions to expired
     - confirm artist becomes available again
     - confirm expiration event/news path triggers once only

4. **Determinism / integrity checks**
   - Re-run the same signing + time advancement scenario and confirm consistent outcomes.
   - Confirm no duplicate active contracts can be created for one artist.
   - Confirm no direct UI mutation path remains for covered signing flow.

5. **Regression checks**
   - Ensure artist roster/detail screens still load.
   - Ensure finance summaries still update.
   - Ensure time advancement still works without contract-related crashes.
   - Ensure existing save/load code still compiles if touched.

# Risks and follow-ups

- **Repo mismatch risk**
  - Workspace hints show `.NET`, while the architecture is Unreal C++.
  - First confirm actual implementation language and project layout before coding.
  - If Unreal source is absent or partial, implement only in the real active code path and document the gap.

- **Subsystem ownership ambiguity**
  - Contract state may currently live inside artist logic or ad hoc UI code.
  - Prefer extending existing ownership rather than creating duplicate sources of truth.

- **Time/date edge cases**
  - Be explicit about expiration semantics:
    - expires when current date passes end date
    - or expires at week close containing end date
  - Choose one rule consistent with existing game date handling and apply it uniformly.

- **Finance coupling**
  - Avoid circular dependencies between contract, artist, and finance subsystems.
  - Use command/application orchestration if direct subsystem calls become tangled.

- **Event duplication**
  - Ensure expiration events are emitted once only, even across repeated weekly processing or load/rebuild paths.

- **Save/load incompleteness**
  - Full persistence may belong to later stories.
  - If full save integration is not yet present, keep contract structs serializable and add TODO-safe hooks rather than inventing a half-finished save system.

- **UI overreach**
  - Do not build a full negotiation screen unless already scaffolded.
  - Focus on backend correctness and minimal command wiring.

Suggested follow-ups after this task:

- richer contract terms and option clauses
- contract snapshots and validation in full save/load
- UI negotiation screen depth
- royalty settlement logic tied to release revenue
- contract-related news generation and dashboard summaries
- automated migration coverage once save schema is introduced