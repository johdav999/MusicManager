# MusicManager Web Editor Architecture

## Purpose

The MusicManager web editor is a local-first production tool for authoring static game data, validating references, inspecting save metadata, previewing balance formulas, and tracking GUI reference assets.

It does not replace the Unreal runtime. Unreal remains authoritative for gameplay simulation, command execution, save application, and cooked asset loading.

## Runtime Shape

```text
Browser UI
  |
  v
Local Editor Server
  |
  +-- EditorData JSON store
  +-- Content/Data import sources
  +-- EditorData/exports generated outputs
  +-- Saved read-only inspection
  +-- docs/design/references GUI references
```

The server binds to `127.0.0.1` by default. All write paths are guarded so the editor can only write under configured `allowedWriteRoots`, initially `EditorData`.

## Modules

- Static data authoring: artists, songs, regions, segments, eras, formats, marketing channels, chart formulas.
- Validation: duplicate IDs, required fields, cross references, numeric ranges, asset/reference paths, date ranges.
- Import/export: read existing CSV data and export deterministic JSON/CSV manifests.
- Save inspector: read-only file/descriptor inspection for Unreal save locations.
- Balance labs: deterministic previews for release/marketing and chart formula tuning using editor data.
- GUI reference manager: tracks OpenAI Image 2 references, implementation status, UMG asset links, and review notes.
- Audit trail: local audit records for successful writes and exports.

## API Versioning

All endpoints are rooted under `/api/v1`. Breaking response-shape changes must use a new API version.

## Data Ownership

- `EditorData/**`: web editor authored data and audit records.
- `EditorData/exports/**`: generated import/export artifacts.
- `Content/Data/**`: existing Unreal source data read by import adapters; not overwritten by default.
- `Saved/**`: read-only inspection only.
- `docs/design/references/**`: read-only reference discovery from the editor server unless a future explicit reference generation workflow is added.

## Production Rules

- No mock data is generated.
- Empty data sets are valid only for categories whose owning gameplay system is not yet authored.
- Every write validates before commit.
- Entity IDs are stable keys; display names are editable labels only.
- Save files remain read-only until a dedicated validated save-editing slice exists.
