# Goal
Implement backlog task **TASK-8.2.4 — Contract expiration emits events and updates artist availability/actions** for story **ST-202 Contract signing and lifecycle**.

The coding agent should add the missing contract lifecycle behavior so that when an active contract reaches its end date during time advancement:

- the contract is transitioned out of active state into expired state
- the artist’s contract linkage and signed/availability state are updated consistently
- artist action eligibility is recalculated to reflect no active contract
- a typed domain event is emitted for contract expiration
- downstream systems/UI can react through the existing event pipeline patterns

This should be implemented in a way that matches the architecture:
- **subsystems own business state**
- **commands mutate state**
- **events notify UI**
- **weekly deterministic simulation**
- **stable IDs only**

Because explicit acceptance criteria were not provided for this task, use the story-level acceptance criteria from **ST-202**, especially:
- active and expired contracts are tracked separately with lifecycle updates on time advancement
- contract expiration emits events and updates artist availability/actions

# Scope
In scope:

- Find the current contract and artist state implementation.
- Add or complete contract expiration processing during simulation time advancement.
- Ensure expiration is deterministic and based on current in-game date.
- Update contract status from active to expired.
- Ensure expired contracts are no longer treated as active in queries/validation.
- Update the associated artist so they becomes available according to the game’s artist-state model.
- Recompute or refresh artist action flags/eligibility after expiration.
- Emit a typed domain event for expiration with stable IDs and date payload.
- Hook event emission into the existing event/news/UI pipeline if infrastructure already exists.
- Add/adjust tests covering expiration behavior.

Out of scope unless required by existing code structure:

- Full renegotiation flows
- New UI screens
- Deep news content authoring
- Save migration work beyond any minimal schema/status compatibility needed
- Refactoring unrelated contract negotiation logic
- Adding generic event bus infrastructure if a simpler existing pattern already exists

Implementation should prefer minimal, targeted changes over broad architecture rewrites.

# Files to touch
Inspect the workspace and update the actual files that own these responsibilities. Likely candidates include files related to:

- **time advancement / simulation orchestration**
  - `UGameTimeSubsystem`
  - simulation director / weekly tick processing
- **artist state ownership**
  - `UArtistManagerSubsystem`
  - artist state structs / projections / action eligibility helpers
- **contract ownership**
  - contract subsystem or artist manager if contracts are currently stored there
  - contract state structs / enums
- **domain events**
  - event payload structs
  - event dispatcher / subsystem delegates
- **tests**
  - unit/integration tests for contract lifecycle and time advancement

Search for classes/structs/enums with names similar to:

- `UGameTimeSubsystem`
- `UArtistManagerSubsystem`
- `Contract`
- `FContractState`
- `EContractStatus`
- `ArtistSigned`
- `ContractExpired`
- `OnWeekAdvanced`
- `UEventSubsystem`
- `UUIManagerSubsystem`

If no dedicated contract subsystem exists, implement within the current owning subsystem rather than inventing a new one.

# Implementation plan
1. **Discover current contract lifecycle flow**
   - Locate where contracts are created and stored.
   - Identify current contract fields:
     - `ContractId`
     - `ArtistId`
     - `LabelId`
     - `StartDate`
     - `EndDate`
     - `Status`
   - Identify how active vs expired contracts are currently queried.
   - Identify where weekly or monthly time advancement occurs.

2. **Confirm the authoritative expiration point**
   - Use the simulation’s authoritative in-game date from the time subsystem.
   - Decide expiration rule based on existing date semantics:
     - typically contract expires when current date is later than or equal to `EndDate`, depending on how the game defines end-of-period ownership
   - Preserve consistency with any existing release/tour/record date comparisons.
   - Document the chosen rule in code comments if not obvious.

3. **Add/complete contract status transition logic**
   - During time advancement, process all active contracts in a deterministic order.
   - For each active contract whose end date has been reached:
     - set status to `Expired`
     - remove it from active-contract views/indexes
     - keep it in historical/expired storage if the model separates them
   - Prevent duplicate expiration processing on later ticks.

4. **Update artist state on expiration**
   - For the linked artist:
     - clear or invalidate `ContractId` if the model expects no active contract after expiration
     - update artist signed/unsigned/availability state consistently
     - refresh action flags/eligibility such as whether the artist can be signed again, can record for player label, can be assigned label-only actions, etc.
   - Do not leave stale “signed” state after contract expiration.
   - If artist action availability is derived rather than stored, ensure derivation now reflects expired contracts correctly.

5. **Emit a typed domain event**
   - Add or use an event such as `EArtistContractExpired` / `FArtistContractExpiredEvent`.
   - Payload should include stable IDs and date, at minimum:
     - `ContractId`
     - `ArtistId`
     - `LabelId` if available
     - expiration/effective date
   - Emit the event exactly once per expiration.
   - Follow existing event/delegate conventions already used in the codebase.

6. **Integrate with event/news/UI pipeline**
   - If there is an existing event subsystem or delegate subscription model:
     - publish the expiration event through it
     - ensure UI/news listeners can observe it
   - If there is already a news generation layer, add only the minimal mapping needed for contract expiration to become visible there.
   - Do not add presentation logic into simulation subsystems.

7. **Ensure command validation respects expired contracts**
   - Review sign/contract validation logic.
   - Confirm an expired contract does not block signing the artist again.
   - Confirm duplicate active contract prevention still works.
   - Confirm active-contract queries ignore expired contracts.

8. **Preserve save/load compatibility**
   - If contract status is already serialized, ensure expired status persists correctly.
   - If artist state stores `ContractId`, ensure load does not restore stale active linkage for expired contracts.
   - Add minimal validation if needed to avoid impossible state:
     - artist marked signed with expired contract
     - duplicate active contracts for one artist

9. **Add tests**
   - Add focused tests for:
     - contract expires when time advances past end date
     - expiration updates artist state
     - expiration emits event once
     - expired contract no longer counts as active
     - artist becomes signable/available again after expiration
   - Prefer deterministic subsystem-level tests over UI tests.

10. **Keep implementation small and idiomatic**
   - Reuse existing enums, delegates, and subsystem APIs.
   - Avoid introducing speculative abstractions.
   - Keep changes localized to contract lifecycle and artist availability.

# Validation steps
1. **Build and test discovery**
   - Inspect solution/projects and determine the correct build/test commands.
   - Start with:
     - `dotnet build`
     - `dotnet test`
   - If this repo is Unreal/C++ with limited .NET only for tooling, identify the actual test/build path and still run any available automated checks.

2. **Static verification**
   - Confirm there is a contract status enum/value for `Expired`, or add one.
   - Confirm active-contract queries exclude expired contracts.
   - Confirm artist state no longer reports active contract after expiration.

3. **Behavioral verification**
   - Create or use a test fixture with:
     - one artist
     - one active contract ending on a known date
     - time advanced to/past that date
   - Verify:
     - contract status changes to expired
     - artist contract linkage/state updates
     - artist action availability changes appropriately
     - expiration event is emitted once with correct IDs/date

4. **Regression verification**
   - Verify signing logic still prevents duplicate active contracts.
   - Verify signing logic allows signing an artist whose previous contract is expired.
   - Verify no repeated expiration event fires on subsequent ticks.

5. **Persistence sanity check**
   - If save/load tests exist, verify expired contracts and artist state round-trip correctly.
   - If no automated persistence tests exist, at least validate serialization fields are still coherent.

6. **Run available commands**
   - Execute the applicable commands from workspace context:
     - `dotnet build`
     - `dotnet test`
   - Include a concise summary of results in the final agent report.

# Risks and follow-ups
- **Date boundary ambiguity:** The biggest risk is expiring contracts one tick too early or too late. Match existing date comparison semantics used elsewhere in simulation.
- **State duplication:** If both contract and artist store overlapping signed/availability state, stale data can occur. Update both or centralize derivation where feasible.
- **Event duplication:** If expiration processing runs in multiple phases or both load-time and tick-time reconciliation, ensure the event is emitted only once.
- **Hidden dependencies:** Other systems may assume `Artist.ContractId` is always populated for signed artists. Audit signability/action checks after expiration.
- **Save integrity:** Existing saves may contain inconsistent contract/artist linkage. Add minimal defensive validation if needed.
- **UI/news gaps:** If event consumers are incomplete, emit the domain event now and leave richer player-facing news as a follow-up rather than bloating this task.

Suggested follow-ups if gaps are discovered but not necessary for this task:
- add explicit contract lifecycle reconciliation during load validation
- add a dedicated contract query API for active vs historical contracts
- add player-facing notification/news content for contract expiration
- add tests for multiple contracts expiring in the same week and deterministic ordering