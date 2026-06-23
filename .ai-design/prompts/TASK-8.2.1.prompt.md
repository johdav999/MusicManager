# Goal
Implement backlog task **TASK-8.2.1** for **ST-202 Contract signing and lifecycle** by introducing or updating a **typed contract-signing command** that includes:
- **Advance**
- **Royalty rate**
- **Term length**
- **Commitment input** (album/record commitment)

The implementation must align with the architecture’s command-driven pattern:
- UI/presentation sends a typed command
- command/application layer validates inputs
- owning subsystem mutates state
- finance posting occurs for advances at signing
- contract lifecycle remains compatible with weekly time advancement and future expiration handling

This task should produce a clean, testable implementation that replaces or avoids ad hoc/stringly signing flows.

# Scope
In scope:
- Add or update a typed command struct for signing with the required inputs
- Add validation for:
  - artist existence
  - artist availability / no duplicate active contract
  - affordability of advance
  - valid royalty/term/commitment ranges
- Create contract state using stable IDs
- Post finance impact for the advance at signing
- Return structured command results
- Emit a domain event for successful signing
- Update any existing signing call sites to use the typed command path
- Add/adjust tests for command validation and successful signing

Out of scope unless required by existing code coupling:
- Full negotiation UI redesign
- Renegotiation flows
- Option clauses depth
- Full contract expiration simulation beyond preserving compatibility
- Broad refactors unrelated to signing
- Save migration unless the current contract schema must change for this task

# Files to touch
Inspect the repo first and then touch only the minimum necessary files in the relevant areas below.

Likely targets:
- Command/application layer
  - command structs
  - command dispatcher / application service
  - command result/error code definitions
- Contract/artist domain
  - contract model/state structs
  - artist manager / contract-owning subsystem
- Finance
  - finance subsystem/service for advance posting
- Events
  - domain event definitions / event bus hooks
- UI integration
  - any signing action entry point currently bypassing typed commands
- Tests
  - unit/integration tests covering signing command behavior

Search for likely files/classes such as:
- `*Command*`
- `*Dispatcher*`
- `*Artist*Subsystem*`
- `*Contract*`
- `*Finance*`
- `*Event*`
- `*SignArtist*`
- `*NegotiateContract*`

If the project is not yet in UE C++ and instead uses a .NET prototype layer, apply the same architecture in the existing stack rather than inventing a parallel system.

# Implementation plan
1. **Discover current signing flow**
   - Find all existing artist signing and contract creation code paths.
   - Identify whether signing currently:
     - directly mutates artist state from UI
     - uses string command IDs
     - omits term length
     - omits commitment
     - posts finance separately or not at all
   - Document the actual owning service/subsystem before editing.

2. **Define the typed command payload**
   - Add or update a command struct/class named consistently with the codebase, e.g. `FSignArtistCommand` or equivalent.
   - Required fields:
     - `ArtistId`
     - `LabelId` if applicable in current architecture
     - `Advance`
     - `RoyaltyRate`
     - `TermLength` or `TermLengthWeeks/Months/Years` based on existing date model
     - `AlbumCommitment` or generic commitment count if the codebase uses a broader term
   - Prefer explicit numeric/domain types already used in the project.
   - Do not use display names as identifiers.

3. **Define/extend structured result handling**
   - Ensure command execution returns a structured result object, not just bool/string.
   - Include:
     - success flag
     - user-facing message
     - machine-readable error code(s)
     - created `ContractId` on success if the result pattern supports payloads
   - Reuse existing result/error infrastructure if present.

4. **Implement validation in the command/application layer**
   - Validate at minimum:
     - artist exists
     - artist is unsigned or otherwise eligible to sign
     - no duplicate active contract exists for the artist
     - advance is non-negative and affordable
     - royalty rate is within allowed bounds
     - term length is valid and positive
     - commitment is valid and positive or within allowed minimums
   - If the architecture already has centralized validation helpers, use them.
   - Keep validation “server-style”: UI must not be trusted.

5. **Create/update contract creation logic**
   - Ensure successful execution creates a contract record with stable `ContractId`.
   - Contract should capture the new signing terms:
     - start date
     - end date or term representation
     - advance
     - royalty rate
     - commitment
     - status active
   - Update artist state to reference the active contract.
   - Prevent multiple active contracts for the same artist.

6. **Post finance for the advance**
   - On successful signing, create the finance ledger/balance effect for the advance.
   - Use the finance subsystem/service rather than mutating balance inline if such a boundary exists.
   - Ensure failure to post finance does not leave partially applied contract state; keep the operation atomic as much as current architecture allows.
   - If no transaction abstraction exists, order operations carefully and roll back on failure where feasible.

7. **Emit domain event**
   - Emit a typed event such as `ArtistSigned` / `EArtistSigned` with stable IDs and relevant dates.
   - Include enough payload for UI/news refresh:
     - `ArtistId`
     - `ContractId`
     - `LabelId` if applicable
     - signing date
   - Reuse existing event pipeline patterns.

8. **Update call sites**
   - Replace any direct signing mutation path with the typed command execution path.
   - If UI already has a signing panel/modal, ensure it passes:
     - advance
     - royalty rate
     - term length
     - commitment
   - Keep widgets thin; no business-rule duplication in UI.

9. **Preserve lifecycle compatibility**
   - If contract lifecycle code already exists, ensure the new term fields integrate cleanly.
   - If expiration checks depend on end date, compute/store it consistently from term length.
   - Do not implement broad lifecycle refactors unless necessary, but avoid introducing schema inconsistencies.

10. **Add tests**
   - Add focused tests for:
     - successful signing creates active contract
     - insufficient funds fails
     - already-signed artist fails
     - invalid royalty rate fails
     - invalid term length fails
     - invalid commitment fails
     - finance posting occurs on success
     - event emission occurs on success
   - Prefer deterministic tests with explicit IDs and dates.

11. **Keep implementation minimal and idiomatic**
   - Follow existing naming/style conventions.
   - Avoid speculative abstractions.
   - Add concise comments only where intent is not obvious.

# Validation steps
1. **Static inspection**
   - Confirm all signing entry points now route through the typed command path.
   - Confirm no new code uses artist display names as keys.

2. **Build**
   - Run:
     - `dotnet build`
   - If there is a specific solution/project file required, use it.

3. **Tests**
   - Run:
     - `dotnet test`
   - If tests are scoped by project, run the relevant test project as well.

4. **Behavior verification**
   - Verify successful signing:
     - returns success result
     - creates contract with advance, royalty rate, term length, commitment
     - updates artist active contract reference/status
     - posts finance deduction/ledger entry
     - emits signing event
   - Verify failure cases:
     - insufficient funds
     - artist unavailable / already signed
     - invalid term values
     - invalid royalty values
     - invalid commitment

5. **Data integrity checks**
   - Confirm only one active contract can exist per artist.
   - Confirm contract IDs are stable/generated properly.
   - Confirm term length maps correctly to stored lifecycle fields.

6. **Regression check**
   - Verify existing roster/artist detail flows still function after signing changes.
   - Verify no direct UI mutation path remains for covered signing behavior.

# Risks and follow-ups
- **Unclear current stack**: workspace hints `.NET`, while target architecture is Unreal C++. First inspect the actual implementation layer and apply the pattern in the real codebase rather than forcing UE-specific constructs.
- **Existing schema mismatch**: current contract model may already store `EndDate` but not explicit term length. If so, compute `EndDate` from term input and avoid unnecessary schema churn unless tests/data require it.
- **Atomicity risk**: if finance posting and contract creation are split across services without transaction support, partial failure could leave inconsistent state. Minimize this risk and note any remaining gap.
- **UI coupling risk**: existing widgets may still assume direct mutation or older signing parameters. Update only the necessary integration points for this task.
- **Acceptance criteria gap**: no explicit task-level AC beyond the story context, so treat the story’s contract-signing AC as the source of truth.
- **Follow-up recommended**:
  - add explicit contract term range constants/data-driven rules
  - add expiration/renewal tests tied to weekly advancement
  - add save/load coverage for any new contract fields if not already present
  - add user-facing validation messaging for contract negotiation UI