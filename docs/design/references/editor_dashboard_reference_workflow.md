# Editor Dashboard Reference Workflow

## Reference Asset

- Reference image: `docs/design/references/editor_dashboard_reference.png`
- Generation mode: OpenAI Image 2 via the built-in image generation workflow
- Intended screen: Local web editor dashboard

## Prompt Used

Use case: ui-mockup
Asset type: production reference image for local web editor dashboard, MusicManager Editor
Primary request: Create a polished production-ready reference image for a local web based/server editor dashboard for MusicManager, a premium music label management game.
Scene/backdrop: full browser-like app viewport without browser chrome, dense data editing dashboard.
Subject: left navigation with Dashboard, Artists, Songs, Market, Rules, Release Lab, Chart Lab, Saves, GUI References, Audit; main area with project health, validation summary, data store counts, recent audit activity, import/export status, and server workspace safety status.
Style/medium: high-fidelity production web app UI, admin/editor tool, black vinyl and warm gold premium studio style, compact and professional.
Composition/framing: 16:9 landscape, no landing page, real tool dashboard as first screen, dense but readable panels and tables.
Lighting/mood: matte black studio surfaces, restrained warm gold highlights, subtle vinyl groove motif, serious production tooling.
Color palette: #070807 background, #11110F panels, #1A1915 surfaces, #34322B borders, #B47A20 and #FFD35A accents, #E8E1D2 text, #A79D8C muted text, #B84A3A errors.
Materials/textures: subtle brushed black panels, thin gold dividers, minimal glow, refined management sim editor.
Text (verbatim): "MUSICMANAGER EDITOR", "VALIDATION", "DATA STORE", "SAVE INSPECTOR", "GUI REFERENCES", "EXPORTS", "LOCALHOST".
Constraints: production reference only, no placeholder/lorem ipsum, no fake brand logos, no watermark, no neon blue sci-fi styling, no device frame.

## Implementation Notes

- The local web editor dashboard should use live API responses for validation, data counts, save inspection, references, exports, and audit activity.
- Empty states are acceptable only when backed by real absence of project data.
- The dashboard style should follow `docs/design.md`: matte black surfaces, restrained gold accents, compact production-tool density, and clear error states.
