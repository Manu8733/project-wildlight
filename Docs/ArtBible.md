# Wildlight art direction

Working style: **Fernwake kingdom**. An original forest civilization binds pale stone and bent timber around living light lenses. Buildings look built and inhabited: foundation, joinery, doorway, roof, functional props and one strong identifying feature. The user's latest direction is an anime fairy-tale atmosphere with the cozy feel of Tiny Glade: rounded welcoming forms, soft warm daylight, rich vegetation, evening fireflies, subtle mist and luminous threads. This applies to the entire scene, including terrain, water, rocks, deposits, vegetation, structures and effects. This is a feeling reference only; geometry, textures, compositions and interface remain Wildlight's own. The goal is an authored RTS scene, with a repeatable visual language and enough selective detail to reward closer inspection.

This is a style specification. The current original kit and its honest acceptance state are documented in [Art/ART_BIBLE.md](../Art/ART_BIBLE.md), [asset review](../Art/ASSET_REVIEW.md) and [provenance](../Art/LICENSES.md). Those procedural prototypes are not final game art. Blender showcase renders are labeled as art previews.

## Shape language

Kingdom roofs use a low, unequal curved profile: one long soft plane shelters the work area, a shorter upturned edge catches light, and thick rounded eaves suggest hand-built shelter. Structural timber bends gently near living roots; it never becomes an arbitrary collection of cylinders. Pale masonry uses broad courses, softened corners and squat buttresses. Doorways and windows use generous arches. Magical architecture uses a three-petal lens cradle and hanging cable/branch connections, distinguishable at strategic zoom without relying on particles.

Castle: a low inhabited stone ring anchored around a high living crown, with entrance facing its economic courtyard. Beacon: a narrow tripod with three arching petals framing a floating lens, visibly separate from an ordinary watchtower. Wizard: tall asymmetric mantle, one luminous staff head and readable casting arm. Worker: short bonnet/hood, broad carry basket and simple work tool. Soldier roles have distinct weapon length and head silhouettes. Siege is wider and lower than infantry with visible pulling or rolling structure.

Avoid identical roof profiles on every building, random spikes, excessive glowing crystals, unrelated saturated hues and decoration that obscures a unit's allegiance. Architecture must explain its role before it explains its fantasy details.

## Palette and materials

| Function | Color | Use |
| --- | --- | --- |
| Forest shadow | `#536F55` | Vegetation depth and frame edges |
| Leaf midtone | `#77945B` | Main foliage, broken into controlled clusters |
| Pale leaf highlight | `#A3B576` | Sparse sun-facing accents |
| Limestone | `#A3AD98` | Walls and foundations |
| Cream plaster | `#F2E5CA` | Warm broad wall faces |
| Warm timber | `#866542` | Frames, decking and roof braces |
| Terracotta roof | `#B96750` | Soft roof mass with `#D58B6C` highlights |
| Light amber | `#FFD99A` | Warm windows and household lights |
| Arcane teal | `#8CCFBC` | Magical lenses, used sparingly |
| Enemy cloth | `#A45043` | Visible enemy markers and banners |
| Hearth banner | `#E2BC76` | Original warm banners |

Material breakup is purpose-driven: broad roughness changes for stone, restrained longitudinal marks for timber, a seam in roof panels and hand-painted-looking value variation derived from explicit masks. There is no noisy photographic detail or all-over emissive surface. Base color and roughness must look correct without bloom. Light lenses have a contained bright center and a darker supporting frame.

Owned photorealistic Epic/Fab assets are source material, not an exception to this direction. Preserve usable medium-or-higher silhouette detail, UVs, material dependencies and wind data, then adapt shape, broad color/value grouping, normal strength and roughness to the coherent fairy-tale scene. Avoid both untouched photographic noise and a flat recolor presented as finished art. Review the rendered result at close and RTS distances before accepting it. Any adaptation must respect the asset's license and NoAI restrictions.

These values match the latest original generator palette. The generated manifest retains actual palette values; material conversion and the in-engine lit result still require review. Geometry convergence to the cozy style is a tracked art task, not inferred from palette alone.

## Readability at RTS scale

At far zoom, keep a building's functional silhouette and owner cue. At mid zoom, show entrance, activity and damage. At close zoom, show joinery and worker interaction. Unit selection rings have clean ground contact and do not require high bloom. Health and network state must remain readable against both dark forests and pale paving.

View a 1280×720 capture at 100%: select 20 infantry beside two buildings and two spell effects; at least nine out of ten testers should correctly identify ownership, selected units and the blocked path without instruction. This is an acceptance target, not a measured result. A monochrome review catches cues that depend only on color.

## Environment and day/night

Use clustered canopies with clear trunks and navigable ground gaps. Three foliage density bands frame the map; important crossings and depot exits stay open. A river is a broad legible navigation barrier, with legal bridge anchors visibly marked by carved stone and dormant lenses. Ruins reuse the kingdom's original material grammar in broken, overgrown forms.

The approved map is a 1600 by 1200 m rectangle with six irregular authored starts. Stronghold Crusader is a reference for the organization of playable relief only: broad buildable plains, grouped rises rather than noisy hills everywhere, clear crossings and open approaches. Do not copy its map layouts or visual treatment. Mountains, banks and rock clusters still use Wildlight's soft, refined anime fairy-tale shape/material language; the reference does not authorize rescaling props or moving approved starts without review.

Authored roads help read the landscape but do not restrict NPC movement. Navigation must cover all practicable terrain. Decorative foliage and small rocks should not create accidental blockers; real cliffs, water and structures have deliberate navigation boundaries, with crossings validated in the native scene. An attractive road network is not proof of functioning pathfinding.

Daytime is cool green ambient light with warm directional highlights. Night preserves building and troop contrast through restrained cool fill; beacon light is a readable tactical landmark. Exposure must not wash out the HUD or change army ownership cues. Fireflies occupy limited low-contrast pockets outside battle lanes. Weather and fog never hide command targets merely for atmosphere.

## Effects and animation briefs

Beacon connection: a narrow thread and slow directional pulse; network interruption: one brief pulse retreat and a persistent inspectable state. Root Barrier: roots grow along a clearly projected footprint, then settle; expiration retracts them without a flash. Light Bridge: anchor lenses connect, translucent span resolves, and the final seconds show a visible warning. Projectile effects should follow actual projectile state, never imply a hit before damage resolves.

Worker actions need idle, walk, carry, gather, build, repair and death. Troops need idle, walk, attack and death; wizard additionally needs spell wind-up and recovery. The source geometry must support joint clearance. Static meshes, bobbing props and a standalone rig scaffold do not satisfy final skeletal animation acceptance.

## Budgets and art acceptance

Use budgets in `PERFORMANCE_BUDGETS.md`. Spend geometry on silhouette, hide small joinery at distance, share material atlases and use instancing for repeated trees and wall segments. The Low tier must retain the network's tactical information even when ambient effects are disabled.

A final asset is accepted only with original/source evidence, editable master, correct scale/pivot, tested collision, texture/UV review, appropriate LOD, no material import errors and an in-engine readability review. Quality is demonstrated in the integrated scene, not inferred from a render or polygon count.
