# River Environment Implementation Plan

> **For agentic workers:** Use subagent-driven-development for source authoring and independent review; the root agent alone controls the native editor.

**Goal:** Continue the already-approved wooded valley with continuous native river water, improved ground material and owned environment assets, preserving terrain and unrestricted practical-terrain movement.

**Architecture:** The validated 1600 by 1200 metre layout remains authoritative. A small original source utility samples the river center and terrain shoreline; a guarded private Unreal 5.8.3 action creates native water without deforming Landscape. Licensed sources remain in private staging.

**Tech Stack:** Python 3.12, standard library, existing terrain sampler, Unreal 5.8.3 Python remote execution.

## Global Constraints

- Keep the historical 768 metre source/map and immutable private backups intact.
- Six authored irregular starts; actual units/buildings/plants are not scaled up.
- The user clarified NPC movement is across all practicable terrain, not limited to roads. Roads have no exclusive navigation privilege. Grass and decorative foliage must not create accidental blockers; real cliffs, water and physical structures require a later complete navigation test.
- Map-first priority: do not expand FFA, save format, economy or gameplay in this checkpoint.
- No Meshy spending, purchases, unauthorized third-party uploads or public licensed binaries.
- Do not claim complete map, working pathfinding, full day cycle or laptop performance from source or still captures.

## Task 1: Original River Sampling

Files: `Tools/author_valley_water.py`, `Tests/test_valley_water.py`.

- [ ] Write failing unittest cases for endpoints, original smoothstep bends, finite meter values, deterministic output, no input mutation and invalid step/layout inputs.
- [ ] Implement `sample_river(layout, step_m=10.0)` returning ordered center/left/right points in metres. Derive each shoreline from the existing `height_metres` sampler using bounded bisection between bed and bank radii; verify the river floor stays below water and terrain outside banks stays above it. Keep left/right asymmetric terrain intersection rather than assuming constant width. Include authored centerline knots in the samples and document the remaining piecewise-linear native approximation.
- [ ] Test all cross-section edge heights near the requested water level, positive width, map bounds and explicit no road-only navigation rules. Use only the validated existing layout; historical terrain exports must remain unchanged.
- [ ] Run focused and full tests, independently review source behavior, then commit.

## Task 2: Native Map Dressing

Files: private `work/unreal-authoring/native_environment_actions.py` and native private project only; safe public evidence in `Evidence/2026-10-11`.

- [ ] Inspect installed native water APIs and source, then create one native river actor with Landscape deformation disabled and bounded water zone. Set centerline points, depth/width and stock Water material after verifying their actual properties.
- [ ] Re-export native Landscape and compare all heights with the immutable rectangular source. Read back river spline/bounds, compile diagnostics and water settings; inspect real screenshot, not a script description.
- [ ] Replace periodic high-frequency ground grain with coherent nonperiodic color variation; preserve physical terrain height. Capture wide and near views to assess aliasing and color.
- [ ] Inspect owned local packs for suitable rocks/terrain/dependencies and only migrate a conservative private subset. Do not invent unverified tree availability; download/connection limitations stay explicit.
- [ ] Save a new private checkpoint with original archive unchanged, truthful handoff and real captures. Complete source checks, normal main integration and public push without licensed assets or credentials.

## Acceptance Boundaries

Native water, map terrain and a still image are separate from full walkable-terrain navigation, real bridge meshes/collision, three species with shared wind, flowers/deposits, changing weather/day cycle, RTS playtesting and measured performance. Preserve all original user acceptance flags.
