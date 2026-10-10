# Wildlight asset review

**Stage:** original Blender-authored static prototype art. **Acceptance:** pending Unreal visual review and gameplay testing. This document does not certify a finished art pass, a functioning GPU workstation, an art director's approval, a Windows build or any FPS result.

The rendered showcase is a composed **Blender asset showcase — not Unreal gameplay**. Its lighting, camera, placement and material response cannot be used as evidence that the game already looks or performs the same way.

## Inventory to reconcile against exports

- Buildings (19): Castle, House, Farm, Mill, Depot, Lumber Camp, Quarry, Gold Mine, Barracks, Archer Range, Forge, Market, Wall, Gate, Watchtower, Beacon, Sanctuary, Mage Tower, Siege Workshop.
- Units (12): Worker, Militia, Spearman, Swordsman, Archer, Cavalry, Engineer, Arcane Guard, Forest Ally, Ram, Catapult, Wizard.
- Environment/props (10 planned exports, 41 assets in the complete intended set): source-authored trees, rocks, flowers and other props. Reconcile exact filenames against the measured export manifest. Showcase terrain and composed lighting are separate presentation objects, not evidence of gameplay assets.

## Cloud structural checks

Record results from the export/validation tools in their reports. Only mark a check passed after inspecting that report; a planned check is not a result.

- [ ] Every required building/unit has one matching stable asset identifier.
- [ ] Export paths use consistent naming and contain no showcase-only light or camera objects.
- [ ] Blender source loads and the generation script runs without errors.
- [ ] Mesh coordinates, dimensions and transforms are finite; meshes contain non-zero geometry.
- [ ] Lowest structural point and footprint-centred pivot follow the art bible.
- [ ] Triangle/material counts are measured against the provisional budget.
- [ ] FBX files reopen; exported dimensions match source dimensions after unit conversion.
- [ ] Shared material identities survive export; missing material slots are reported.
- [ ] Provenance register covers every exported art source.
- [ ] Showcase images are labelled correctly wherever they are presented.

A structural pass catches malformed exports. It cannot accept silhouettes, animation quality, texture seams or in-game performance.

## Unreal import and visual review

Run this gate on the laptop or an accessible Windows GPU machine, with the project's pinned Unreal version. Save the engine version, machine specification, import settings, map name, screenshots and findings with the review.

- [ ] A 1 m reference cube imports as 100 cm, and a human imports at the intended height.
- [ ] Ground-centred pivots place assets without sinking, hovering or per-instance scale fixes.
- [ ] Buildings face consistently; rotated placement previews match final meshes.
- [ ] Rounded masonry, curved turrets, shingle roofs, ivy and flowers create a coherent original cosy kingdom; no copied reference-game assets are used.
- [ ] Normals, rounded bevel shading, materials and emissive values render without visible defects.
- [ ] Collisions cover intended structural volumes; gates and paths remain traversable.
- [ ] Mesh bounds support selection, placement footprints and culling correctly.
- [ ] All 19 buildings are recognisable at the normal strategic camera distance.
- [ ] All 12 unit classes differ through silhouette/equipment in mixed groups.
- [ ] Player and rival accents remain distinguishable under lighting changes and colour-vision simulation.
- [ ] Selection rings, health bars, orders and spell effects stay readable against the art.
- [ ] Connected, disconnected and sabotaged beacons remain distinguishable without colour alone.
- [ ] Soft noon/night lighting, sparse warm fireflies and fog preserve terrain, threats and navigable routes; crystal emission never washes out nearby art.
- [ ] 720p and 1080p checks pass at Low settings with 100% and 150% UI scale.

## Art work still required

- [ ] Art-directed refinement of prototypes after in-game review; remove weak silhouettes and repeated decorative motifs.
- [ ] Production topology, UVs, baking/atlases and final material variants where they improve readability.
- [ ] Character skeletons, skinning and animation-ready meshes; static prototypes do not satisfy this item.
- [ ] Idle, walk, attack, work, gather, cast, hit and death animation sets, plus cavalry/engine mechanisms.
- [ ] Animation timing integrated with actual combat, gathering and spell events.
- [ ] Construction, damage, upgrade and destruction states for buildings.
- [ ] LODs, collision meshes, material section consolidation and environment instancing.
- [ ] Final terrain, water, vegetation and magical landmarks fitted to playable maps.
- [ ] Gameplay-driven Niagara spells, network links, projectiles and ambient effects.

## Performance and acceptance evidence

- [ ] Measure a representative settlement and 100-unit scene on the RTX 3050 Laptop 4 GB VRAM / 16 GB RAM reference laptop; record CPU/GPU frame times, VRAM, draw calls and settings.
- [ ] Test 200/500-unit scenes only after those populations are supported by the gameplay systems; record any failures.
- [ ] Check camera motion, shadow cost, transparency cost, texture residency and culling under Unreal profiling tools.
- [ ] Capture representative gameplay screenshots from Unreal after integration, clearly separate from the Blender showcase.
- [ ] Ask independent players to identify building purposes and unit threats at normal zoom; record confusion before revising art.
- [ ] Resolve visual defects and repeat affected checks; keep a dated reviewer record before marking accepted.

The brief's 60 FPS goal remains a measured acceptance target. No cloud-rendered screenshot or polygon report proves it has been reached.
