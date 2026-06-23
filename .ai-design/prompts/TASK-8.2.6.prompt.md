# Goal
Implement backlog task **TASK-8.2.6 — Finance posting for advances must occur at signing** for story **ST-202 Contract signing and lifecycle**.

Ensure that when an artist contract is successfully signed, the contract advance is posted immediately to the finance system at signing time, rather than deferred to a later simulation phase or omitted entirely.

This change must preserve the architecture principles:
- business state owned by subsystems
- commands mutate state
- finance ledger remains authoritative for cash movement
- UI stays thin
- emitted events reflect the committed result

# Scope
In scope:
- Find the existing artist signing / contract creation flow.
- Ensure the sign-contract command validates affordability against current available funds.
- On successful signing, immediately create the contract and post a finance ledger entry for the advance in the same operation.
- Update label cash/balance immediately.
- Emit any existing finance balance changed event if the finance subsystem already does so.
- Keep ledger posting append-only and traceable to the created contract and/or artist.
- Add or update automated tests covering:
  - successful signing posts advance immediately
  - insufficient funds prevents signing
  - no duplicate/double-posting of the advance
- Add minimal logging if the codebase already uses logging categories.

Out of scope:
- renegotiation
- contract expiration behavior beyond ensuring this change does not break it
- UI redesign
- broader finance summary work outside the signing flow
- save migration unless required by an existing persistence schema change

# Files to touch
Inspect and update the actual files that implement these responsibilities. Likely candidates include equivalents of:

- contract signing command / application layer
  - `*CommandDispatcher*`
  - `*Contract*Command*`
  - `*SignArtist*`
- contract domain/state management
  - `*Contract*Subsystem*`
  - `*ArtistManager*`
- finance posting
  - `*FinanceManagerSubsystem*`
  - ledger entry models / balance update helpers
- event definitions if needed
  - finance balance changed event
  - artist signed event payloads
- tests
  - unit/integration tests around sign artist + finance behavior

Because the workspace hint says `.NET`, first locate the real implementation files before editing. Prefer touching the smallest set of files necessary.

# Implementation plan
1. **Locate the signing flow**
   - Search for the command or method that signs an artist / creates a contract.
   - Identify:
     - where affordability is checked
     - where the contract object/state is created
     - whether finance posting already exists elsewhere
     - whether there is any weekly/monthly finance settlement incorrectly handling advances

2. **Locate finance ledger posting API**
   - Find the finance subsystem/service method used to post ledger entries and update balance.
   - Reuse an existing append-only ledger mechanism if present.
   - If no dedicated method exists, add a focused helper for posting an advance entry without redesigning the subsystem.

3. **Define the correct transactional order**
   Implement the signing flow so the operation behaves atomically from gameplay perspective:
   - validate artist availability
   - validate no duplicate active contract
   - validate advance amount and affordability
   - create contract
   - post finance ledger entry for the advance immediately
   - update artist/contract ownership state
   - emit success/domain events

   If the code structure requires it, prefer one orchestrating method that performs both contract creation and finance posting together. Avoid a design where the contract is created successfully but finance posting can silently fail afterward.

4. **Post the advance at signing**
   Add immediate finance posting with:
   - type/category: `Advance` or equivalent existing finance type
   - amount: negative cash movement from label perspective if that is the ledger convention
   - reference: `ContractId` and optionally `ArtistId`
   - memo: clear human-readable description such as `"Contract advance for <ArtistName>"`

   Follow existing sign conventions in the ledger:
   - if expenses are stored as negative amounts, use negative
   - if type determines direction, follow existing subsystem rules consistently

5. **Prevent double-posting**
   - Ensure the advance is posted exactly once per successful signing.
   - Remove or disable any later deferred posting path for contract advances if one exists.
   - If there is a weekly finance settlement that currently posts advances, update it so signed-contract advances are not reposted.

6. **Preserve command result behavior**
   - If finance affordability fails, return the existing structured failure result.
   - Do not create an active contract on failure.
   - Keep user-facing messages aligned with existing patterns.

7. **Events and observability**
   - Ensure balance-changed notifications fire through the finance subsystem’s normal path.
   - Keep or add logging under the finance/signing categories if available.
   - Do not add UI-specific logic.

8. **Tests**
   Add or update tests for at least these cases:
   - **Successful signing**
     - starting balance known
     - sign artist with advance
     - contract exists
     - ledger contains one advance entry tied to contract/artist
     - balance decreased immediately by advance amount
   - **Insufficient funds**
     - starting balance below advance
     - sign attempt fails
     - no contract created
     - no ledger entry created
     - balance unchanged
   - **No double-post**
     - sign artist successfully
     - run any subsequent tick/settlement path if relevant
     - verify only one advance ledger entry exists

9. **Keep changes minimal and architecture-aligned**
   - Do not let widgets mutate finance or contract state directly.
   - Keep the command/application layer as the orchestrator.
   - Keep subsystem ownership clear.

# Validation steps
1. Search the codebase to identify the real signing and finance files.
2. Implement the change.
3. Run build/tests:
   - `dotnet build`
   - `dotnet test`
4. If there are targeted tests, run those first/also.
5. Manually verify in tests or a small harness:
   - before signing: capture balance and ledger count
   - after signing: balance reduced immediately, ledger count incremented, contract active
   - after any time advance/settlement: no second advance posting
6. Review for regressions:
   - duplicate active contract prevention still works
   - insufficient funds still blocks signing
   - event flow remains intact

# Risks and follow-ups
- **Atomicity risk:** if contract creation and finance posting are split across services without rollback, partial success could corrupt state. Prefer a single orchestrated success path.
- **Ledger sign convention risk:** confirm whether expenses are negative amounts or typed positive amounts before posting.
- **Deferred posting duplication risk:** there may already be a finance settlement path that posts advances later; remove or guard it.
- **Reference consistency risk:** ledger entries should use stable IDs (`ContractId`, `ArtistId`) rather than display names as keys.
- **Test coverage gap:** if the project has weak existing tests around simulation commands, add focused tests now and note broader command/finance integration coverage as a follow-up.
- **Follow-up suggestion:** if not already present, consider a dedicated finance posting helper for contract advances and a domain event payload that includes `ContractId`, `ArtistId`, `AdvanceAmount`, and effective signing date.