# Bottom Command Dock Reference Workflow

## Output

- Reference image: `docs/design/references/bottom_command_dock_reference.png`
- Generated asset sheet source: `Content/GUI/HUD/CommandDock/BottomCommandDockAssetSheet_source.png`
- Runtime assets: `Content/GUI/HUD/CommandDock/BottomCommandDock_*.png`

## Intent

Create a production-quality bottom command dock for the main MusicManager HUD, matching the dark vinyl, matte black, and warm gold style in `docs/design.md` and the main gameplay HUD reference.

## Widget Breakdown

- `UCommandPanelWidget` owns the dock and creates repeated command item children from real command definitions.
- `UCommandItemWidget` owns the button visuals, icon, label, and hover/selected state.
- Blueprint asset `/Game/GUI/CommandPanelBP` contains the dock background and `CommandPanel` container.
- Blueprint asset `/Game/GUI/CommandItemWidgetBP` contains the button background, icon image, label text, and transparent button hit target.
- Commands: `Audition`, `Market`, `Contracts`, `Studio`, `Charts`.

## Pixel-Match Notes

The dock should be low, centered, and compact: a wide rounded black vinyl surface, five gold-bordered square command buttons, gold icons, ivory/gold labels, and subtle hover/selected tinting. Dynamic repeated command items are allowed because the `CommandPanel` container is explicitly bound in the Blueprint.
