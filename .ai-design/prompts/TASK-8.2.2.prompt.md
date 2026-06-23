# Goal
Implement `TASK-8.2.2` for `ST-202 — Contract signing and lifecycle` by adding contract validation that enforces:

- **artist availability** before signing
- **player affordability** for contract advance/payment obligations

The implementation must fit the existing architecture principles:

- business rules live in subsystems / command layer
- UI remains thin
- commands return structured success/failure
- stable IDs are used for lookups
- finance posting occurs only after validation succeeds

Deliver a minimal, production-quality change that makes contract signing reject invalid attempts with clear error results and logs.

# Scope
In scope:

- Find the existing contract signing flow, likely command-dispatch or subsystem based.
- Add or complete validation for:
  - artist exists
  - artist is available to sign
  - artist does not already have an active conflicting contract
  - player/label has enough funds to cover the advance
- Ensure failed validation returns structured failure instead of mutating state.
- Ensure successful signing:
  - creates/activates contract
  - updates artist contract state/availability
  - posts finance advance cost
  - emits any existing relevant event(s)
- Add or update automated tests around validation behavior.

Out of scope unless required by existing code structure:

- deep contract negotiation UI
- renegotiation / option clauses
- save migration changes unrelated to this validation
- broad refactors of unrelated systems
- adding new screens/widgets unless needed for compile correctness

If the codebase does not yet have a dedicated command object for signing, implement the smallest aligned version consistent with current patterns rather than overbuilding.

# Files to touch
Inspect first, then modify only the minimum necessary set. Likely candidates include files related to:

- contract signing command / dispatcher
- artist manager subsystem
- finance manager subsystem
- contract data/state models
- command result / error code definitions
- tests covering contract signing

Search for likely symbols and files such as:

- `SignArtist`
- `NegotiateContract`
- `Contract`
- `ArtistManagerSubsystem`
- `FinanceManagerSubsystem`
- `CommandDispatcherSubsystem`
- `FCommandResult`
- `ECommandErrorCode`
- `ArtistSigned`
- `ContractExpired`

Prefer touching:

1. command validation entry point
2. owning subsystem method for contract creation
3. finance posting integration point
4. unit/integration tests

Avoid touching UI files unless they must be updated to compile against changed result/error enums.

# Implementation plan
1. **Discover the current signing flow**
   - Locate the exact path used to sign an artist.
   - Identify:
     - command/request struct
     - validation location
     - contract creation location
     - finance posting location
     - event emission location
   - Confirm whether the project is C++ only, C# tooling only, or mixed. Follow the actual implementation language used by gameplay code.

2. **Identify the authoritative validation boundary**
   - Validation should happen at the command/application layer or the owning subsystem entry point.
   - Ensure UI is not the source of truth.
   - If validation is duplicated in multiple places, centralize shared checks in one helper/service method and keep UI-side checks advisory only.

3. **Implement artist availability validation**
   - Add a clear availability check that rejects signing when any of the following are true:
     - artist ID is invalid / artist not found
     - artist already has an active contract
     - artist status indicates unavailable for signing
     - artist is already signed to a label
   - Prefer existing artist state fields such as:
     - `CurrentLabelId`
     - `ContractId`
     - `Status`
     - lifecycle flags
   - If no helper exists, add a small method such as:
     - `IsArtistAvailableForContract(...)`
     - or `CanSignArtist(...)`
   - Keep logic deterministic and based on stable IDs, not display names.

4. **Implement player affordability validation**
   - Before contract creation, verify the player label has enough available funds for the advance.
   - Use the finance subsystem/account balance as the authoritative source.
   - Reject when:
     - advance is negative or invalid
     - player balance is insufficient
   - If there is an existing reserve/ledger pattern, follow it.
   - Do not post finance entries on failed validation.

5. **Return structured failure results**
   - Use existing result types if present.
   - Add specific error codes/messages if needed, such as:
     - `InvalidArtistState`
     - `InsufficientFunds`
     - `InvalidReference`
     - `DuplicateActiveContract`
   - Ensure user-facing messages are clear, e.g.:
     - “This artist is not available for signing.”
     - “You do not have enough funds to pay the contract advance.”

6. **Preserve successful signing behavior**
   - On success:
     - create the contract
     - associate it with artist and label
     - update artist state to signed/unavailable
     - post finance ledger entry for advance
     - emit existing domain event(s)
   - Ensure ordering is safe:
     1. validate
     2. create/update state
     3. post finance
     4. emit event
   - If the codebase supports transactional rollback patterns, use them. Otherwise keep mutation ordering simple and guarded.

7. **Prevent duplicate active contracts**
   - Explicitly guard against creating a second active contract for the same artist.
   - If contracts are stored separately from artist state, validate both:
     - artist-linked contract reference
     - contract repository/list for active entries by artist ID
   - This aligns with story notes and save validation expectations.

8. **Add logging**
   - Use existing UE/game log categories if available.
   - Log validation failures at warning/debug level with stable IDs and reason.
   - Do not spam logs from UI retries.

9. **Add or update tests**
   - Cover at least:
     - signing succeeds when artist is available and funds are sufficient
     - signing fails when artist already has active contract
     - signing fails when artist is otherwise unavailable
     - signing fails when funds are insufficient
     - failed signing does not create contract or post finance entry
   - If there is no test project yet, add the smallest appropriate automated coverage in the existing test style.

10. **Keep implementation minimal and aligned**
   - Do not introduce a generic framework.
   - Do not refactor unrelated contract lifecycle behavior unless required for correctness.
   - If you discover missing lifecycle hooks for expiration, note them in follow-up comments but keep this task focused on validation checks.

# Validation steps
1. **Code search and compile**
   - Search for all signing entry points and ensure the authoritative one is covered.
   - Build the solution/project using the appropriate existing command:
     - `dotnet build`
   - If gameplay code is Unreal C++, use the project’s normal compile path if available in workspace conventions.

2. **Automated tests**
   - Run:
     - `dotnet test`
   - If tests are filtered or split, run the relevant contract/artist/finance test suite.

3. **Behavior verification**
   - Verify success case:
     - available artist
     - sufficient funds
     - contract created
     - artist marked signed/unavailable
     - finance advance posted
   - Verify failure case: unavailable artist
     - no contract created
     - no finance posting
     - structured failure returned
   - Verify failure case: insufficient funds
     - no contract created
     - no finance posting
     - structured failure returned

4. **State integrity checks**
   - Confirm no duplicate active contracts can be created for one artist.
   - Confirm stable ID lookups are used.
   - Confirm no UI-only state is required for validation.

5. **Regression check**
   - Ensure existing signing flow callers still compile.
   - Ensure any changed enums/result types do not break unrelated commands.

# Risks and follow-ups
- **Risk: signing logic may be split across UI and subsystem code**
  - Mitigation: consolidate authoritative validation in one backend path and leave UI checks as optional mirrors only.

- **Risk: finance subsystem may not expose a clean affordability API**
  - Mitigation: add a minimal helper like `CanAfford(amount)` or equivalent rather than duplicating balance logic.

- **Risk: artist availability may be represented inconsistently**
  - Mitigation: validate both explicit status fields and active contract presence; document any inconsistency found.

- **Risk: no existing tests for this flow**
  - Mitigation: add focused tests around command/subsystem behavior without building a large new harness.

- **Follow-up recommended**
  - Add explicit contract lifecycle tests for expiration and artist availability restoration on time advancement.
  - Add save/load validation coverage for duplicate active contracts and broken artist-contract references.
  - If not already present, standardize command error codes/messages across sign/start-recording/plan-tour flows.