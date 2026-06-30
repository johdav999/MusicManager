# Agent Instructions

## Project Context

This repository is an Unreal Engine 5.6 project for MusicManager.

## Required References

- For any architecture-related task, read and follow `docs/architecture.md` before implementation.
- For any design-related task, read and follow `docs/design.md` before implementation.
- For any GUI-related task, read and follow `userwidget.md` before implementation.

Architecture-related tasks include changes to system structure, module boundaries, data flow, persistence, gameplay systems, subsystem responsibilities, integration patterns, and long-term technical direction.

Design-related tasks include UI, UX, visual style, icons, imagery, textures, layout, interaction patterns, menus, widgets, and any player-facing presentation.

GUI-related tasks include creating, modifying, wiring, styling, or verifying Unreal `UUserWidget` C++ classes, Widget Blueprint assets, child widgets, HUD panels, menus, buttons, icons, generated GUI assets, widget layout, widget states, and Blueprint widget inheritance.

## Production-Ready Standard

All tasks must be implemented as production-ready work.

Do not deliver:

- Mock-up data
- Placeholder functions
- Temporary scaffolding
- Stubbed behavior
- Fake integrations
- Throwaway UI
- Non-shipping assets

If temporary exploration is necessary, keep it outside the production path and clearly mark it as non-shipping.

## Current Inventory Maintenance

After completing any implementation task, update `current.md` with the status of any functionality and/or GUI that was implemented or materially changed.

The update should describe:

- The feature or GUI area changed
- What is now implemented
- Any important limitations that remain
- The primary files or assets involved

Do not leave `current.md` stale after adding production functionality, changing gameplay behavior, or adding/changing GUI.

## Handling Vague Prompts

If a prompt is too vague to implement safely, first turn it into a detailed, vertically sliced task before implementation.

A good vertical slice should define:

- The user-facing outcome
- The affected systems or screens
- The real data and behavior required
- The production-ready acceptance criteria
- The smallest complete implementation that can ship

Do not start broad or ambiguous implementation work until the task has been narrowed into a concrete vertical slice.
