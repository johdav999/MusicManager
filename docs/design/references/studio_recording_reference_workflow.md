# Studio Recording Reference Workflow

- Reference image: `docs/design/references/studio_recording_reference.png`
- Intended screen: Studio recording GUI / UMG-backed record creation panel.
- Workflow source: generated with OpenAI Image 2 via built-in image generation.

## Visual Target

The screen should present a premium black-and-gold studio dashboard for choosing a record type, previewing genre-compatible songs, selecting tracks, and confirming a recording session.

Core components:

- Header: `STUDIO RECORDING`, selected artist, artist genre.
- Record type segmented control: `SINGLE`, `EP`, `LP`.
- Available songs list: dynamic list rows, each with preview/play action, song name, genre, year, quality/hit metrics, and select action.
- Selected tracks list: dynamic list rows with remove action and track order.
- Production summary: recording cost, duration, completion date, selected track count, and validation warnings.
- Actions: `CONFIRM RECORDING` and `CANCEL`.

## Implementation Notes

- Do not hardcode songs into the background. Song rows must come from real `USongManagerSubsystem` data.
- The GUI must use `URecordWidget` and `URecordSongListItemWidget` bindings for runtime data and interactions.
- Record type must drive validation and projection through `URecordManagerSubsystem::BuildRecordingProjection`.
- Preview playback must use the existing `UMusicPlayerComponent` route.
- Confirm must dispatch `UCommandDispatcherSubsystem::ExecuteStartRecording`.

## Acceptance Check

- Blueprint layout should match the reference in broad proportions, dark studio material treatment, gold highlights, row density, and bottom production-summary placement.
- Missing data must render as a clear empty/error state, not placeholder rows.
