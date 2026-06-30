# Release And Marketing Planner Reference Images

These production reference images were generated with OpenAI Image 2 as required by `docs/design.md`.

## Release Planner

- Reference: `docs/design/references/release_planner_reference.png`
- Intended widget backing class: `UReleasePlannerWidget`
- Visual target: black/gold premium label dashboard with recorded release list, release options, projected reach, warnings, and schedule actions.

## Marketing Planner

- Reference: `docs/design/references/marketing_planner_reference.png`
- Intended widget backing class: `UMarketingPlannerWidget`
- Visual target: black/gold premium label dashboard with campaign builder, channel targeting, budget controls, ROI forecast, and active campaign timeline.

## Pixel-Match Workflow

1. Assemble UMG Blueprint widgets against the C++ backing classes.
2. Use the reference images as the visual target for layout, spacing, materials, typography, and interaction states.
3. Capture the implemented widgets in Unreal.
4. Compare against the reference images.
5. Revise until the implemented screens match as closely as practical.
