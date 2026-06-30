# UserWidget Implementation Workflow

## Purpose

Use this workflow whenever a new production GUI widget is requested for MusicManager.

The goal is to deliver an end-to-end Unreal UMG widget:

- AAA-grade GUI reference image created before implementation
- C++ `UUserWidget` backing class
- Blueprint Widget asset inheriting from the C++ class
- Blueprint-owned widget tree with all required components and inherited/bound variables
- Production-ready generated image assets
- Icons, backgrounds, panels, button imagery, and texture assets as needed
- Widget tree assembled and positioned to match the approved reference image
- Runtime data binding and interaction logic
- Build verification
- `current.md` update

No mock-up GUI, placeholder art, fake data, or unfinished scaffolding is allowed.

## Required Reading

Before implementation, read:

- `AGENTS.md`
- `docs/design.md`
- `docs/architecture.md` if the widget affects data flow, subsystem responsibilities, commands, save/load, or screen routing
- The relevant reference image workflow under `docs/design/references/`

## Required Inputs

Each widget task must define:

- Widget name
- Player-facing purpose
- Parent/screen where it appears
- Reference image path
- Data source subsystem(s)
- Commands/interactions
- Required child widgets
- Required image assets
- Blueprint asset path
- C++ class name and file path

Example:

```md
Widget: Bottom Command Dock
C++ class: UBottomCommandDockWidget
Blueprint asset: /Game/GUI/HUD/WBP_BottomCommandDock
Reference image: docs/design/references/main_gameplay_hud_aaa_reference_v2.png
Parent: LayoutBP1 / ULayout
Data sources: UCommandDispatcherSubsystem, UUIManagerSubsystem
Commands: Audition, Market, Contracts, Studio, Charts
```

## Vertical Slice Requirement

If the prompt is vague, first convert it into a vertical slice.

A valid slice must include:

- User-facing outcome
- Real gameplay data used
- Exact widget states
- Exact commands or events wired
- Assets needed
- Acceptance criteria
- Files/assets changed

Do not implement broad or ambiguous GUI work until the slice is clear.

## AAA Reference Image Rule

Every widget generation starts by creating or identifying a AAA-grade GUI reference image for the exact widget being implemented.

Before implementing any widget:

1. Generate or identify a production-quality AAA reference image for the widget.
2. Save it under:

```text
docs/design/references/
```

3. Add a workflow note beside it:

```text
docs/design/references/<widget_name>_reference_workflow.md
```

4. Break the reference down into C++ bindings, Blueprint components, generated image assets, layout slots, text blocks, buttons, and child widgets.
5. Implement the C++ widget class and Blueprint widget asset to match the reference as closely as practical.
6. Compare the finished Blueprint widget against the reference image.
7. If the Blueprint widget does not match the reference closely at a pixel-like level, restart the widget generation from scratch rather than layering fixes onto a weak foundation.

Pixel-like matching means the Blueprint widget should align with the reference in proportions, position, spacing, anchors, material treatment, color, typography, icon scale, border weights, glow intensity, and interaction state visuals.

## Asset Generation Rule

All generated images must use OpenAI Image 2 through the imagegen workflow.

This includes:

- Panel backgrounds
- Button backgrounds
- Icons
- Textures
- Portrait frames
- Decorative discs
- News card imagery
- Any GUI-specific bitmap asset

Assets must be production ready. Do not use temporary images.

For GUI work, always generate any needed icons, graphics, backgrounds, button surfaces, texture strips, status frames, dividers, or decorative imagery with imagegen so the implemented Blueprint can be matched pixel-like to the reference image. Do not substitute generic engine widgets, rough vector shapes, text-only placeholders, or temporary materials for final visual assets.

Save generated assets under an appropriate project path, for example:

```text
Content/GUI/HUD/
Content/GUI/Icons/
Content/GUI/Textures/
docs/design/references/
```

Source PNGs should be kept in the repo when useful for traceability.

## C++ UserWidget Requirements

Each C++ widget class must:

- Inherit from `UUserWidget`
- Declare `UPROPERTY` variables for every Blueprint component the class needs to read, refresh, bind events for, or update
- Use `BindWidget` or `BindWidgetOptional` for Blueprint-owned child widgets
- Expose clean `BlueprintCallable` refresh/setup functions
- Use view-model structs where helpful
- Avoid direct formatting duplication across widgets
- Avoid hardcoded fake data
- Fail safely when bindings are missing
- Log useful diagnostics for missing critical bindings
- Keep rendering/local interaction logic inside the widget
- Keep cross-widget orchestration in `ULayout` or `UUIManagerSubsystem`

The C++ class defines the contract and behavior. It must not replace the production Blueprint layout by constructing the visible widget tree as the primary path at runtime. Runtime-created widgets are only acceptable for dynamic repeated children inside an explicitly bound Blueprint container, such as generated list items.

Example structure:

```cpp
UCLASS(BlueprintType, Blueprintable)
class MUSICMANAGER_API UBottomCommandDockWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category="Commands")
    void RefreshCommands();

protected:
    UPROPERTY(meta=(BindWidget))
    UHorizontalBox* CommandContainer;

    UPROPERTY(EditDefaultsOnly, Category="Commands")
    TSubclassOf<UCommandItemWidget> CommandItemClass;
};
```

## Blueprint Widget Requirements

For each widget, create a Blueprint Widget asset that inherits from the C++ class.

The Blueprint must:

- Use the C++ class as parent
- Contain correctly named child widgets for all `BindWidget` and required `BindWidgetOptional` properties
- Mark inherited/bound components as variables where the C++ class needs to access them
- Add every required visual component to the Blueprint designer hierarchy, including root panels, background images, borders, buttons, text blocks, icon images, slots, spacers, overlays, and child widget containers
- Use generated production assets
- Position all components, graphics, text, icons, panels, and child widgets to match the reference image at a pixel-like level
- Have responsive anchors and alignment
- Use consistent padding, sizing, and safe margins
- Avoid designer-only placeholder children in runtime containers
- Set default visibility correctly
- Use real child widget classes where needed

An empty Widget Blueprint that only inherits the C++ class is not complete. A Blueprint that relies on C++ runtime construction for its main visual hierarchy is not complete. The Widget Blueprint must visibly contain the production components in the designer so artists and designers can inspect, tune, and inherit the GUI.

Blueprint asset paths should follow this pattern:

```text
/Game/GUI/HUD/WBP_<WidgetName>
/Game/GUI/HUD/Items/WBP_<ChildWidgetName>
```

## Unreal Asset Creation

When creating or modifying Blueprint assets, prefer Unreal Editor automation:

- Unreal Python
- Editor Utility scripts
- C++ defaults where appropriate

Do not binary-edit `.uasset` files directly.

If a Blueprint asset cannot be fully assembled through automation, create the C++ class, generated assets, and an editor checklist describing the exact Blueprint assembly steps.

The preferred delivery is a fully assembled Blueprint asset, not a checklist. Use a checklist only when Unreal automation cannot create a required designer structure or assign a required asset safely.

## Layout Integration

If the widget belongs to the main HUD, wire it through `ULayout`.

`ULayout` should own composition and routing.

Child widgets should own their own visuals.

Correct responsibility split:

```text
ULayout:
- show/hide screens
- route commands
- route selected artist changes
- route news events
- coordinate layers

Child UserWidget:
- display data
- local hover/pressed state
- local animations
- local formatting
- visual refresh
```

## Required States

Each production widget should support relevant states:

- Default
- Hovered
- Pressed
- Selected
- Disabled
- Empty
- Loading, only if real async loading exists
- Error/failure
- No-data state

Do not create fake loading or fake data states.

## Verification

Before completion:

1. Build the project.
2. Open or run the relevant screen if possible.
3. Verify the widget appears.
4. Verify all critical bindings are valid.
5. Verify real data appears.
6. Verify commands work.
7. Verify missing data fails cleanly.
8. Compare the Blueprint widget visually against the reference image.
9. Update `current.md`.

If the comparison shows that the Blueprint widget is not pixel-like to the reference image, restart the widget generation from the reference breakdown and rebuild the Blueprint hierarchy, generated assets, and layout. Do not ship a visually divergent widget as "close enough."

## Acceptance Criteria

The task is complete only when:

- C++ widget class exists and builds
- Blueprint Widget asset exists and inherits the C++ class
- Blueprint Widget asset contains the inherited/bound variables and all required designer components
- Required generated assets exist in the repo
- Blueprint uses the generated assets
- Widget is positioned and styled pixel-like to the reference
- Widget is wired into the real screen or parent widget
- Real data/commands are connected
- No placeholder/fake data is used
- Build succeeds
- Blueprint-vs-reference comparison has passed, or the widget generation has been restarted and corrected until it passes
- `current.md` is updated
