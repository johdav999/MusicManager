# Top Status Bar Reference Workflow

## Output

- Parent reference image: `docs/design/references/main_gameplay_hud_aaa_reference_v2.png`
- Dedicated widget reference image: `docs/design/references/top_status_bar_reference.png`
- Widget Blueprint asset: `/Game/GUI/HUD/TopStatusBarBP`
- C++ class: `UTopStatusBarWidget`

## Intent

Implement the top HUD status strip from the dedicated top status bar AAA reference. The widget presents the MusicManager brand mark, current date, player label name, cash, reputation, time controls, and menu entry in the project's dark vinyl-and-gold style.

## Data Sources

- `UGameTimeSubsystem` for current date and pause/play state.
- `UPlayerLabelSubsystem` for label display name and reputation.
- `UFinanceManagerSubsystem` for player label cash balance.
- `UCommandDispatcherSubsystem` for command-driven fast-forward.

## Implementation Target

The widget should stay compact and screen-edge aligned:

- Matte black full-width top strip.
- Gold MusicManager identity at the left.
- Date, label, cash, and reputation in compact status pills.
- Pause, play, fast-forward, and menu buttons on the right.
- Warm gold highlights and off-white text.

## Pixel-Match Expectations

The widget is implemented as a C++ behavior contract with a Blueprint-owned designer hierarchy. `TopStatusBarBP` must contain the bound components named in `UTopStatusBarWidget`, including the root panels, generated background image, status icons, text blocks, and command buttons.

The Blueprint hierarchy uses layout containers, not absolute x-coordinate placement:

- `RootCanvas`
- `StatusBarRoot`
- `BackgroundImage`
- `TopStatusMainRow`
- `BrandGroupRow`
- `TopStatusFlexibleSpacer`
- status pill size boxes/borders/content rows
- `TopStatusControlRow`

Generated image assets are imported under `/Game/GUI/HUD` and assigned to Blueprint image fields:

- `TopStatusBarSurface`
- `TopStatusBarFrame`
- `TopStatusIcon_BrandRecord`
- `TopStatusIcon_DateCalendar`
- `TopStatusIcon_LabelPerson`
- `TopStatusIcon_CashDollar`
- `TopStatusIcon_ReputationStar`
- `TopStatusIcon_Pause`
- `TopStatusIcon_Play`
- `TopStatusIcon_FastForward`
- `TopStatusIcon_Menu`

The current Blueprint hierarchy is positioned to follow `top_status_bar_reference.png`. Further polish should be judged by direct Blueprint-vs-reference comparison inside the Unreal Designer viewport; if the layout drifts materially, rebuild the widget hierarchy and generated assets from the reference breakdown.
