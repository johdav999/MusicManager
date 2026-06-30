# Charts GUI Reference Workflow

## Reference Asset

- Reference image: `docs/design/references/charts_reference.png`
- Generation mode: OpenAI Image 2 via the built-in image generation workflow
- Intended screen: Charts GUI / UMG-backed chart dashboard

## Prompt Used

Use case: ui-mockup
Asset type: production reference image for Unreal Engine UMG screen, MusicManager Charts GUI
Primary request: Create a polished production-ready reference image for the Charts screen of a premium music label management game.
Scene/backdrop: dark executive music-industry dashboard, no visible browser chrome, no device frame.
Subject: a full-screen charts dashboard showing weekly music charts. Include a left chart selector with Global Records, Singles, Albums, Vinyl, Streaming, Genre Charts; a main ranked chart table with columns for rank, movement, record, artist, units, points, peak, weeks; a top-right status panel with current #1, top player-owned release, biggest mover, and recent milestone; and a small vinyl/gold circular motif behind or beside the current #1.
Style/medium: high-fidelity game UI concept, production-quality, Unreal UMG-feasible, premium black vinyl and warm gold style.
Composition/framing: 16:9 landscape, dense management-sim dashboard layout, clear spacing, compact readable controls, no oversized marketing hero.
Lighting/mood: matte black studio surfaces, warm gold rim lighting, subtle glow on selected chart row, restrained cinematic highlights.
Color palette: background #070807, panels #11110F and #1A1915, borders #34322B, gold accents #B47A20 #FFD35A, text #E8E1D2 and #A79D8C.
Materials/textures: subtle vinyl grooves, brushed dark panels, thin gold rules, polished but not flashy.
Text (verbatim): "CHARTS", "GLOBAL RECORDS", "SINGLES", "ALBUMS", "#1", "UNITS", "POINTS", "PEAK", "WEEKS".
Constraints: production reference only, no placeholder labels like lorem ipsum, no fake brand logos, no watermark, no neon blue sci-fi styling, no mock browser/window frame.

## Implementation Notes

- `UChartsWidget` exposes `GetReferenceImagePath()` so the UMG asset can locate this target.
- The widget must bind to real chart view models from `UChartManagerSubsystem`; do not create fake chart rows in the UMG asset.
- Final UMG assembly should compare against the reference for layout, black/gold palette, compact density, table hierarchy, highlight treatment, and status panels before the GUI is considered done.
