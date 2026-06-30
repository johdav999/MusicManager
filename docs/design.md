# MusicManager Design Style

## Visual Direction

MusicManager should feel like a premium music industry dashboard built from dark studio materials, polished vinyl, and warm gold light. The style is elegant, tactile, and slightly dramatic without becoming flashy. It should suggest late-night record label work: contracts, artists, releases, money, and reputation all handled inside a refined control room.

The reference image centers on a black vinyl record set into a rounded square black surface with golden rings and glossy highlights. This should guide the game's overall interface language: dark, minimal, high-contrast, circular music motifs, and precise metallic accents.

## Core Mood

- Premium record label management
- Dark studio atmosphere
- Warm golden success signals
- Clean, focused business UI
- Tactile materials rather than flat panels
- Elegant music culture, not nightclub neon

## Color Palette

Use a mostly dark palette with restrained gold accents.

- Background black: `#070807`
- Soft panel black: `#11110F`
- Charcoal surface: `#1A1915`
- Muted border gray: `#34322B`
- Deep gold: `#B47A20`
- Bright gold highlight: `#FFD35A`
- Warm amber glow: `#F2A93B`
- Soft text: `#E8E1D2`
- Muted text: `#A79D8C`
- Warning/red accent, used sparingly: `#B84A3A`

Gold should be used for important affordances, selected states, progress, revenue, reputation, and premium emphasis. Avoid covering large areas in gold.

## Materials

Surfaces should feel physical and layered.

- Matte black panels with subtle grain or brushed texture
- Vinyl-style circular grooves for music-related widgets
- Soft bevels and edge highlights
- Thin golden strokes around important elements
- Glossy radial highlights on records, discs, charts, and player controls
- Gentle shadows that make panels feel heavy and grounded

Avoid bright plastic, candy colors, heavy gradients, and sci-fi blue hologram styling.

## Shapes

The dominant shape language should combine rounded square containers with circular record motifs.

- Main cards: rounded rectangles with 8-16px equivalent radius
- Important music objects: circles, rings, discs, grooves
- Buttons: compact, clean, slightly rounded
- Dividers: thin gold or dark gray lines
- Icons: simple line icons, preferably gold or muted ivory

The UI should feel precise and intentional. Rounded corners should be soft, not bubbly.

## Typography

Typography should be clean, modern, and readable.

- Use strong but restrained headings
- Prefer compact dashboard typography over large marketing-style text
- Use uppercase labels for small metadata when useful
- Keep body text in warm off-white or muted beige
- Use gold text only for emphasis, values, or selected state

Suggested font direction: modern sans-serif with a refined editorial feel.

## UI Patterns

The game should present information like a music executive's operating desk.

- Artist cards can resemble premium catalog entries
- Records and releases can use circular vinyl thumbnails
- Revenue, hype, and reputation can use gold progress rings or meters
- Contract views can use dark paper-like panels with gold section rules
- Notifications can feel like label memos or ticker updates
- Active selections should glow subtly in amber/gold

Screens should be dense enough for management gameplay, but still polished and calm.

## Lighting And Effects

Effects should be subtle and cinematic.

- Warm gold rim light on selected objects
- Soft inner glow for active controls
- Radial sheen on record graphics
- Low-opacity shadows behind panels
- Slight vignette around main screens
- Micro animations that feel smooth and expensive

Avoid constant pulsing, neon bloom, particle overload, or arcade-style motion unless used for a rare celebratory moment.

## Game Identity Notes

MusicManager should look like it belongs to the world of albums, contracts, charts, and prestige. The player should feel like they are running a serious label with taste and ambition. Every major screen should quietly reinforce the idea of music as both art and business.

## Image And GUI Production Rules

All image generation for the game must be done using OpenAI Image 2. This includes icons, pictures, illustrations, portraits, textures, UI imagery, album-style graphics, decorative assets, and any other generated visual material.

All images and GUI elements must be production ready. Do not create placeholder art, mock-up GUI, temporary interface layouts, or rough visual drafts for in-game use. Every delivered asset and screen should be suitable for direct integration into the game or clearly marked as a non-shipping exploration outside the production asset path.

Before implementing any GUI, first generate a production-quality reference image for the intended screen, widget, panel, or control. Use that reference image as the visual target during implementation.

The implemented GUI must match the reference image as closely as practical at the pixel level, including layout, spacing, proportions, color, material treatment, lighting, typography, and interaction states. After the widget is implemented, compare it against the reference image. If it does not match closely enough, revise the implementation and repeat the comparison until the result aligns with the reference.
