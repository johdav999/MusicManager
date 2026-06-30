# MusicManager Web Editor Implementation Slices

This document breaks a local web based/server editor for MusicManager into production-ready vertical slices. The editor must support real game data, real validation, and real import/export paths for the Unreal 5.6 project.

## Overall Goal

Create a local-first web editor and server for authoring, validating, inspecting, and balancing MusicManager data. The editor should manage static content, inspect saves, preview selected simulation outcomes, and support GUI/reference workflow without bypassing Unreal's production rules.

The editor is a development and content-production tool, not a replacement for the in-game runtime. Unreal remains the authoritative simulation runtime. The editor must use stable IDs, versioned schemas, validation reports, and deterministic preview calculations that match or are generated from game rules where practical.

## Global Implementation Rules

- Read `AGENTS.md`, `docs/architecture.md`, `docs/design.md`, `target.md`, and `current.md` before implementation.
- Keep the editor local-first by default, bound to `localhost`, with explicit configuration for any remote mode.
- Do not create mock datasets, placeholder APIs, fake validation, or stub screens.
- All edited data must serialize to explicit project files or validated editor data stores.
- Every write path must validate before saving.
- All entities must use stable IDs; display names are never primary keys.
- Static content authoring should target Unreal-friendly source data: JSON, CSV, DataTables, or import manifests.
- Runtime save inspection should be read-only until a safe, validated edit/write path exists.
- GUI work must follow `docs/design.md`. For editor GUI screens intended to ship as production tools, generate reference images first and compare implementation against them.
- Every completed implementation slice must update `current.md`.

## Recommended Stack

- Backend: ASP.NET Core or Node/TypeScript. Prefer one strongly typed API boundary with OpenAPI output.
- Frontend: React + TypeScript with a dense dashboard/admin UI.
- Storage: project-local editor data under a dedicated folder such as `EditorData/`, plus import/export adapters for existing Unreal data files.
- Validation: shared schema validation in the editor server plus generated or mirrored rules from Unreal structs where practical.
- Execution mode: local process launched from terminal, with clear port/configuration and no network dependency for core editing.

## Slice 1 - Editor Architecture And Workspace Contract

### Prompt

Create the architecture foundation for a local web/server editor, including project workspace discovery, configuration, API boundaries, and file ownership rules.

### User-Facing Outcome

The team can launch a local editor server that knows which Unreal project it is editing and which folders it is allowed to read/write.

### Implementation Scope

- Add an editor architecture document or initial server module describing:
  - local-first execution
  - workspace root detection
  - safe writable folders
  - static data source folders
  - generated/imported asset folders
  - save-inspection folders
  - API versioning strategy
- Define the high-level modules:
  - static data authoring
  - validation
  - save inspector
  - simulation preview
  - GUI/reference manager
  - import/export pipeline
- Add a configuration file for editor server settings:
  - project root
  - data source paths
  - output paths
  - port
  - allowed origins
- Keep configuration deterministic and project-local.

### Acceptance Criteria

- The editor has a documented architecture and workspace contract.
- The server refuses to write outside configured project/editor output folders.
- API versioning is documented.
- No gameplay data is fabricated.
- `current.md` is updated.

### Primary Files

- `editortarget.md`
- possible `docs/editor_architecture.md`
- possible `EditorServer/...`
- `current.md`

## Slice 2 - Editor Domain Schema Foundation

### Prompt

Define typed editor schemas for core static content and runtime inspection projections.

### User-Facing Outcome

The editor can represent MusicManager data consistently before any UI edits or exports are added.

### Implementation Scope

- Add schema definitions for:
  - artist templates
  - song definitions
  - region definitions
  - market segment definitions
  - era definitions
  - format rules
  - marketing channel rules
  - chart formula definitions
  - GUI reference records
  - save slot summaries
- Include shared primitives:
  - stable id
  - display name
  - localized/authoring text where needed
  - asset references
  - source file provenance
  - validation state
- Keep schemas explicit and versioned.
- Add JSON schema, TypeScript types, C# DTOs, or equivalent chosen-stack types.

### Acceptance Criteria

- Schemas compile in the chosen stack.
- Every entity has a stable ID field.
- Display name fields are separate from IDs.
- Schemas include version/provenance metadata.
- `current.md` is updated.

### Primary Files

- `EditorServer` schema files
- possible `EditorData/schema`
- `current.md`

## Slice 3 - Validation Engine And Reference Graph

### Prompt

Implement the editor validation engine that checks data integrity across all known editor schemas.

### User-Facing Outcome

Content creators can detect broken IDs, invalid enum values, missing asset references, and out-of-range tuning values before importing data into Unreal.

### Implementation Scope

- Add validation result structures:
  - severity
  - code
  - message
  - entity type
  - entity id
  - source path
  - suggested fix where safe
- Build a reference graph for:
  - songs to artists
  - records to artists/songs where runtime snapshots are inspected
  - regions to market segments
  - eras to formats/channels/chart formulas
  - GUI references to image files and screen ids
- Validate:
  - duplicate ids
  - missing references
  - invalid enum values
  - invalid dates/ranges
  - impossible era overlaps
  - broken asset paths
  - empty required display fields
- Add an API endpoint for full-project validation.

### Acceptance Criteria

- Validation runs without mutating data.
- Validation returns structured errors and warnings.
- Broken references are reported with source location/provenance.
- Valid empty datasets are allowed only where the schema explicitly permits it.
- `current.md` is updated.

### Primary Files

- `EditorServer` validation module
- schema files from Slice 2
- `current.md`

## Slice 4 - Editor Data Store And Import/Export Pipeline

### Prompt

Implement a project-local editor data store with validated import/export to Unreal-friendly source files.

### User-Facing Outcome

The editor can persist authored data and export it to files Unreal can import or consume.

### Implementation Scope

- Define the editor data store layout, for example:
  - `EditorData/artists/*.json`
  - `EditorData/songs/*.json`
  - `EditorData/regions/*.json`
  - `EditorData/eras/*.json`
  - `EditorData/chart_formulas/*.json`
- Add read/write APIs for versioned editor records.
- Add import from existing CSV/JSON data where available.
- Add export to Unreal-oriented CSV/JSON/DataTable source files.
- Preserve source provenance and generated-file warnings.
- Prevent overwriting hand-authored source files unless explicitly configured.

### Acceptance Criteria

- Editor records can be loaded and saved from project-local files.
- Writes validate before commit.
- Exported files are deterministic.
- No mock records are generated.
- `current.md` is updated.

### Primary Files

- `EditorData/...`
- `EditorServer` storage/import/export modules
- existing `Content/Data` source files where exports target them
- `current.md`

## Slice 5 - Web Shell, Navigation, And Project Dashboard

### Prompt

Create the production web editor shell with navigation, project status, validation summary, and recent data activity.

### User-Facing Outcome

Opening the editor in a browser shows a real project dashboard instead of a placeholder landing page.

### Implementation Scope

- Generate a production-quality reference image for the editor dashboard following `docs/design.md`.
- Implement the web shell:
  - left navigation
  - top project/status bar
  - validation health panel
  - data category cards
  - recent changed records
  - import/export status
  - server/workspace status
- Connect all displayed data to real API endpoints.
- Add empty/error/loading states.

### Acceptance Criteria

- The dashboard uses real project status and validation data.
- Navigation includes only implemented screens or disabled entries with explicit unavailable state.
- No placeholder metrics or fake recent activity.
- Visual style follows the black/gold production direction.
- `current.md` is updated.

### Primary Files

- frontend web app files
- editor dashboard reference image/workflow notes
- editor API endpoints
- `current.md`

## Slice 6 - Artist Template Editor

### Prompt

Implement the artist template editor for authoring unsigned artist pools and artist archetypes.

### User-Facing Outcome

Designers can create and edit artist templates used by auditions, discovery, contracts, and future roster generation.

### Implementation Scope

- Add artist list/table with filtering by:
  - era
  - genre
  - role/type
  - risk
  - validation status
- Add artist detail form:
  - artist id
  - display name
  - type/band/solo metadata
  - genre affinities
  - talent/commercial/reliability stats
  - personality/risk ranges
  - expected contract cost fields
  - portrait/asset reference
- Validate every edit before save.
- Export to the current artist data source or a documented import manifest.

### Acceptance Criteria

- Artists can be listed, edited, created, and validated.
- Duplicate artist ids are rejected.
- Invalid stat ranges are rejected.
- Export uses real edited data only.
- `current.md` is updated.

### Primary Files

- editor artist schema/store/API
- artist editor frontend screen
- existing artist data export target
- `current.md`

## Slice 7 - Song Catalog Editor

### Prompt

Implement the song catalog editor for authoring song definitions and artist/song relationships.

### User-Facing Outcome

Designers can manage the song catalog with stable song IDs, metadata, quality tuning, and artist ownership.

### Implementation Scope

- Add song table with filters:
  - artist
  - genre
  - era
  - quality range
  - validation status
- Add song detail editor:
  - song id
  - title
  - artist id
  - genre tags
  - mood
  - energy
  - quality/hit potential
  - duration
  - audio asset reference
  - performance profile id
- Validate artist references and asset references.
- Export to the song DataTable/import source.

### Acceptance Criteria

- Songs can be listed, edited, created, and validated.
- Missing artist references fail validation.
- Duplicate song ids are rejected.
- Export is deterministic.
- `current.md` is updated.

### Primary Files

- editor song schema/store/API
- song editor frontend screen
- existing song data export target
- `current.md`

## Slice 8 - Market, Region, And Segment Editor

### Prompt

Implement the market editor for region definitions, audience segments, genre demand, channel strengths, and format adoption.

### User-Facing Outcome

Designers can tune regional demand and market behavior through a validated GUI instead of manual spreadsheet edits.

### Implementation Scope

- Add region list/table and detail editor.
- Add market segment editor.
- Add matrix-style editing for:
  - genre demand by region
  - channel strength by region
  - format adoption by era/region
  - market size
  - audience segment distribution
- Validate:
  - segment percentages where applicable
  - non-negative demand values
  - known genre/channel/format ids
  - unique region ids
- Export to current region/segment data sources.

### Acceptance Criteria

- Region and segment data can be edited through real forms.
- Broken segment/region references fail validation.
- Matrix edits persist and export deterministically.
- `current.md` is updated.

### Primary Files

- editor region/segment schemas
- market editor frontend screen
- region/segment export targets
- `current.md`

## Slice 9 - Era, Format, Marketing Channel, And Chart Formula Editor

### Prompt

Implement the rules editor for eras, record formats, marketing channels, and chart formula profiles.

### User-Facing Outcome

Designers can tune how the music industry changes over time: formats, media channels, economics, and chart formulas.

### Implementation Scope

- Add era timeline editor.
- Add format rule editor:
  - format id
  - active years
  - price/cost assumptions
  - sales/streaming semantics
- Add marketing channel rule editor:
  - channel id
  - active years
  - minimum budget
  - exposure multipliers
  - region modifiers
- Add chart formula editor:
  - formula id
  - sales weight
  - streaming conversion
  - radio/marketing factors
  - recency/longevity modifiers
  - genre/format modifiers
- Validate overlapping rules and impossible dates.

### Acceptance Criteria

- Era/rule records can be edited and validated.
- Formula edits are versioned and exportable.
- Invalid active-year ranges fail validation.
- No hardcoded fake rule values are introduced.
- `current.md` is updated.

### Primary Files

- editor era/rules schemas
- rules editor frontend screen
- export targets for rules/formulas
- `current.md`

## Slice 10 - Release And Marketing Balance Lab

### Prompt

Implement a release and marketing balance lab that previews release reach, campaign exposure, estimated unit lift, revenue, and ROI using real rules and edited data.

### User-Facing Outcome

Designers can compare release and marketing scenarios before changing game balance.

### Implementation Scope

- Add scenario input:
  - artist/template
  - record type
  - release date
  - regions
  - formats
  - marketing channels
  - budget
- Use existing or mirrored production formulas for:
  - market demand
  - exposure generation
  - expected unit lift
  - gross revenue
  - ROI
- Show comparison table for multiple scenarios.
- Store scenarios only as editor analysis records, clearly separate from shipping data unless promoted through validation.

### Acceptance Criteria

- Preview outputs are deterministic.
- Inputs are real editor/game data references.
- Invalid scenarios fail with validation messages.
- Scenario results are not exported as gameplay data unless explicitly promoted.
- `current.md` is updated.

### Primary Files

- editor simulation preview API
- release/marketing balance frontend screen
- formula/rule modules
- `current.md`

## Slice 11 - Chart Formula Lab

### Prompt

Implement a chart formula lab that previews weekly chart rankings from real or imported sales history and editable chart formulas.

### User-Facing Outcome

Designers can tune chart formulas and immediately see effects on ranks, movement, peaks, and player-owned highlights.

### Implementation Scope

- Add inputs:
  - chart definition
  - week/date
  - sales history source
  - formula profile
  - region/genre/format filters
- Add outputs:
  - ranked chart table
  - rank movement
  - points breakdown
  - peak/weeks-on-chart projection
  - warnings for missing sales data
- Reuse or mirror `UChartManagerSubsystem` formulas as closely as possible.
- Support importing save snapshots or fixture sales history for analysis.

### Acceptance Criteria

- Chart previews use real sales/formula inputs.
- No fake chart rows are generated.
- Formula changes produce deterministic rank changes.
- Missing inputs show explicit empty/error states.
- `current.md` is updated.

### Primary Files

- editor chart formula API
- chart lab frontend screen
- chart formula schemas
- `current.md`

## Slice 12 - Save Slot Inspector And Validation Report GUI

### Prompt

Implement a read-only save slot inspector with validation reports for MusicManager save files.

### User-Facing Outcome

Developers can inspect campaign saves, understand validation failures, and verify persisted subsystem snapshots without loading the game.

### Implementation Scope

- Add save slot listing using real save files/registry where accessible.
- Add save summary view:
  - slot name
  - label name
  - save version
  - current date
  - created/last saved
  - validation status
- Add snapshot sections:
  - player label
  - time
  - artists/contracts
  - songs
  - records/sales
  - finance
  - market
  - marketing
  - charts
  - news
  - future-system buckets
- Add validation report UI with grouped errors/warnings.
- Keep writes disabled unless a later validated save-edit slice is explicitly implemented.

### Acceptance Criteria

- Save slots are read from real save metadata/files.
- Broken saves produce structured validation reports.
- Inspector never mutates save files.
- Unsupported save versions are displayed clearly.
- `current.md` is updated.

### Primary Files

- editor save inspector API
- save inspector frontend screen
- save schema adapters
- `current.md`

## Slice 13 - GUI Reference And Production Screen Spec Manager

### Prompt

Implement a GUI reference manager for tracking OpenAI Image 2 references, screen specs, implementation status, and pixel-match notes.

### User-Facing Outcome

The team can manage production GUI targets for MusicManager screens and see which screens have reference images, backing classes, UMG assets, and comparison status.

### Implementation Scope

- Add GUI reference schema:
  - screen id
  - display name
  - reference image path
  - prompt used
  - linked source widget class
  - linked UMG asset path
  - implementation status
  - comparison notes
  - last reviewed timestamp
- Import existing reference workflow notes for:
  - release planner
  - marketing planner
  - charts
- Add manager UI:
  - reference gallery
  - screen status table
  - missing-reference warnings
  - notes editor
- Do not generate images in this slice unless implementing a specific screen's reference workflow.

### Acceptance Criteria

- Existing reference images are indexed from real repo paths.
- Missing images or UMG assets are reported.
- GUI statuses persist in editor data.
- No placeholder reference records are marked complete.
- `current.md` is updated.

### Primary Files

- editor GUI reference schema/store/API
- GUI reference manager frontend screen
- `docs/design/references/...`
- `current.md`

## Slice 14 - Data Change Review, Diff, And Audit Trail

### Prompt

Implement editor-side change review, diffs, and audit records for authored data changes.

### User-Facing Outcome

Designers can review what changed before saving/exporting and diagnose when balancing data changed.

### Implementation Scope

- Add dirty-state tracking per record.
- Add structured diffs for:
  - scalar fields
  - arrays
  - maps/matrices
  - asset references
- Add save confirmation showing validation status and diff summary.
- Add project-local audit records:
  - timestamp
  - user/machine identifier if available
  - changed entity ids
  - source path
  - validation result
- Keep audit logs local and privacy-conscious.

### Acceptance Criteria

- Users can see changed fields before save/export.
- Invalid changes cannot be committed.
- Audit records are written for successful saves/exports.
- Audit records do not contain secrets or personal data beyond configured local identifier.
- `current.md` is updated.

### Primary Files

- editor diff/audit modules
- frontend diff components
- editor data store
- `current.md`

## Slice 15 - End-To-End Editor Tests, Packaging, And Run Script

### Prompt

Add automated tests, packaging, and terminal run scripts for the local web editor.

### User-Facing Outcome

The editor can be run and verified consistently from terminal by developers and content creators.

### Implementation Scope

- Add backend tests for:
  - schema validation
  - broken-reference detection
  - import/export determinism
  - safe workspace writes
  - save inspector read-only behavior
- Add frontend tests for:
  - dashboard loading
  - validation report rendering
  - editor form validation
  - empty/error states
- Add an end-to-end smoke test for:
  - launch server
  - open dashboard
  - load validation summary
  - edit a record in a temporary editor data workspace
  - validate and export
- Add `run-editor.ps1` or equivalent to start the editor locally.
- Document ports, configuration, test commands, and output folders.

### Acceptance Criteria

- Tests run from terminal.
- The editor can be launched with one script.
- The script does not require network access for core functionality.
- Test data lives outside the shipping production data path.
- `current.md` is updated.

### Primary Files

- editor test files
- `run-editor.ps1`
- editor documentation
- `current.md`

## Suggested Execution Order

1. Slice 1 - Editor Architecture And Workspace Contract
2. Slice 2 - Editor Domain Schema Foundation
3. Slice 3 - Validation Engine And Reference Graph
4. Slice 4 - Editor Data Store And Import/Export Pipeline
5. Slice 5 - Web Shell, Navigation, And Project Dashboard
6. Slice 6 - Artist Template Editor
7. Slice 7 - Song Catalog Editor
8. Slice 8 - Market, Region, And Segment Editor
9. Slice 9 - Era, Format, Marketing Channel, And Chart Formula Editor
10. Slice 10 - Release And Marketing Balance Lab
11. Slice 11 - Chart Formula Lab
12. Slice 12 - Save Slot Inspector And Validation Report GUI
13. Slice 13 - GUI Reference And Production Screen Spec Manager
14. Slice 14 - Data Change Review, Diff, And Audit Trail
15. Slice 15 - End-To-End Editor Tests, Packaging, And Run Script

## Definition Of Done

- The editor launches locally from terminal.
- The editor reads and writes only configured project/editor paths.
- Static authoring data is versioned, validated, and exportable to Unreal-friendly files.
- The editor validates stable IDs and cross-references before saving.
- The dashboard and implemented editor screens use real project data.
- Save inspection is real and read-only unless a future validated write path is implemented.
- Balance labs use deterministic production-aligned formulas.
- GUI reference tracking uses real files and real implementation statuses.
- Tests cover validation, import/export, write safety, and key GUI flows.
- `current.md` accurately documents implemented editor capabilities as slices are completed.
