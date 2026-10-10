# Wildlight art bible

Working direction for the English-language RTS prototype. This document defines a consistent asset family; it does not certify final art, animation, Unreal integration or performance.

## Identity

A cosy forest kingdom built around living light. Rounded stone, curved turrets, cream plaster and dark timber give settlements a handmade character. Warm terracotta shingle roofs sit above cottage walls; sage and teal appear as restrained accents. Olive meadows, ivy, flowers and soft light make the landscape inviting. Small crystal hearts and warm fireflies mark the light network without flooding the frame with neon.

Tiny Glade is a reference for the requested feeling of comfortable miniature architecture and gentle landscape lighting. Wildlight uses its own geometry, proportions, palette, building silhouettes and magical language; no reference-game models, textures or UI are copied. Rounded shapes and careful composition take priority over intricate surface noise.

Geometry is authored specifically for Wildlight through Blender Python. Use explicit proportions, rounded architectural parts, controlled bevels, curved turrets and hand-arranged roof shingles. Avoid interchangeable primitive blocks, random ornament and stock fantasy silhouettes. No downloaded models or image-generated textures form this asset set. Procedural authorship is a construction method; refinement and playtesting remain necessary.

## Palette and materials

These are sRGB colour targets. Convert correctly when entering shader values; lighting and exposure will change their screen appearance.

| Role | Hex | Use |
| --- | --- | --- |
| Deep timber | `#493C32` | Beams, tool handles, roof structure |
| Warm timber | `#866542` | Boards, shutters, carts |
| Cream plaster | `#F2E5CA` | Inhabited walls and readable broad faces |
| Plaster shade | `#D7C8AA` | Recessed plaster faces |
| Terracotta roof | `#B96750` | Primary cottage roof mass |
| Roof light / shadow | `#D58B6C` / `#8B5145` | Shingles and soft broad roof variation |
| Foundation stone | `#A3AD98` | Basements, rounded fortifications, quarry |
| Stone light / shadow | `#C1CAB4` / `#73836F` | Rounded masonry relief |
| Meadow | `#9CAC77` | Ground cover; varied in grouped patches |
| Meadow light | `#B5BC83` | Soft olive landscape highlights |
| Forest canopy | `#77945B` | Trees; keep gaps around paths and silhouettes |
| Foliage light / dark | `#A3B576` / `#536F55` | Canopies, ivy and moss accents |
| Earth / paths | `#8A7156` / `#D7BD92` | Banks, soil and legible routes |
| Crystal | `#8CCFBC` | Subtle sage-teal magical accents |
| Rune | `#C5EBD4` | Restrained light-network detail |
| Hearth amber | `#FFD99A` | Windows, lanterns and warm fireflies |
| Metal | `#57717D` | Armour and functional machinery |
| Sage cloth | `#74958A` | Shared cloth accent; ownership overlay review pending |
| Gold banner | `#E2BC76` | Warm architectural banners |
| Rival cloth | `#A45043` | Complementary warm accent; in-game team treatment pending |

Use opaque materials with broad roughness variation. Timber and plaster remain matte; terracotta shingles catch soft edge light; rounded masonry has gentle value transitions. Metal has controlled highlights. Crystal emission is limited to a small heart; rune highlights and fireflies feel warm and quiet rather than neon. Keep emissive bloom restrained so disconnected nodes remain distinguishable. Material slots follow shared families rather than a new material for every small part. No translucent crystal shell is required for the first integration.

## Scale, export and naming

- Blender dimensions are metres: a standing human is approximately 1.8 m. Unreal dimensions are centimetres: the human should import at approximately 180 cm. Check a 1 m reference cube before batch import; do not repair a scale error through per-instance scaling.
- Place the asset origin at its footprint centre on the ground plane, with its lowest structural face at Z = 0. Units and siege engines use the same ground pivot. Buildings face Blender -Y; verify that export/import conversion consistently yields the intended Unreal forward axis.
- Apply object transforms before export. Asset objects use scale `(1, 1, 1)`; isolated meshes export without the showcase camera, lighting or terrain.
- FBX is the static-mesh integration route. Current source/export identifiers are `SM_Castle`, `SM_Beacon` and `SM_Worker`, with the exact enum name after `SM_`. Keep these stable during import. Reserve `SK_...` for later rigged replacements; a static unit mesh is not a skeletal asset.
- Export glTF only for inspection/interchange if available. FBX and glTF material interpretation differ; neither format proves Unreal material fidelity.
- Keep collision separate from visible trim. Use simple footprint boxes/convex volumes for buildings; verify walkable gateways and navigation clearances in Unreal. Do not use render triangles as the default collision mesh.

## Building silhouettes

Dimensions below are design targets in metres, including trim. The final export manifest must record measured bounds. Footprints may be tuned to the build grid without losing the silhouette.

| Building | Target footprint / height | Recognition cue |
| --- | --- | --- |
| Castle | approximately 8 × 8 / 12 | Rounded corner turrets, warm roofs and a clear arched front gate |
| House | approximately 4.3 × 4 / 5 | Terracotta shingle roof, cream gable, soft chimney and cottage porch |
| Farm | 8 × 7 / 4 | Low shelter beside aligned crop rows |
| Mill | 6 × 5 / 8 | Large four-blade wind rotor above roof line |
| Depot | 6 × 5 / 5 | Broad open loading frontage and stacked goods |
| Lumber camp | 6 × 5 / 4 | Exposed timber frame, log rack and saw station |
| Quarry | 7 × 6 / 4 | Layered stone cut, hoist and rubble apron |
| Gold mine | 6 × 5 / 5 | Braced dark portal, mine cart and gold seams |
| Barracks | 7 × 6 / 6 | Long pitched roof, fenced drill frontage, team banner |
| Archer range | 7 × 6 / 4 | Open shooting lanes and bold round targets |
| Forge | 6 × 5 / 6 | Heavy chimney, covered work bay and amber furnace |
| Market | 7 × 6 / 5 | Colour-blocked awnings around a central stall |
| Wall | 4 × 1.5 / 4 | Rounded masonry, broad crenellated top and modular end faces |
| Gate | 6 × 2 / 6 | Two stone piers and a wide traversable arch |
| Watchtower | 4 × 4 / 10 | Curved narrow shaft and projecting lookout |
| Beacon | 3 × 3 / 7 | Rounded stone pedestal, small crystal heart and restrained warm glow |
| Sanctuary | 8 × 8 / 10 | Ancient branching tree within a stone circular base |
| Mage tower | 5 × 5 / 12 | Tall tapered body, offset platforms and crystal spire |
| Siege workshop | 8 × 7 / 6 | Open large bay with heavy wheels and lifting frame |

Avoid tiny mesh details that disappear from the strategic camera. A beacon must remain distinct from a watchtower without relying on emission alone. Sanctuary branches form a recognisable canopy rather than a dense tangle. Wall segments must connect cleanly and gate openings must match navigation dimensions.

## Units

The initial set contains static mesh prototypes. They establish proportions, equipment and class recognition; they are not finished animated characters.

| Unit | Profile and equipment |
| --- | --- |
| Worker | Plain short tunic, light cap, visible work tool |
| Militia | Compact padded body, round shield and short weapon |
| Spearman | Upright long spear above the group silhouette, narrow shield |
| Swordsman | Broad metal shoulders, long blade and solid shield |
| Archer | Lean body, strongly curved bow and visible quiver |
| Cavalry | Horse-length profile, raised rider and long weapon |
| Engineer | Satchel, work apron and bulky tool; distinct from Worker |
| Arcane Guard | Heavy shoulder shape, faceted teal crest, substantial shield |
| Forest Ally | Branching limbs, moss crown and broad non-human stance |
| Ram | Low wheeled chassis, long beam and covered horizontal roof |
| Catapult | Wide wheelbase with a raised throwing arm and counterweight |
| Wizard | Long robe, tall staff crystal and recognisable hood |

Enlarge hands, weapon ends and shields enough to survive the RTS camera. Do not use team colour as the sole class cue. Preserve usable body separation around shoulders, hips and elbows for later rigging; the current geometry will need topology and skinning review before animation.

## Strategic-camera readability

Review at 1920 × 1080, 1280 × 720 and 100/150% UI scale. Test near, normal and maximum zoom at noon and night, with fog, selection rings and a 100-unit mixed group. The normal camera must show class identity through silhouette; the maximum zoom must at least show ownership, occupied footprint and threat type.

Use rounded bevels to catch soft light on roofs, plinths and shields. Shingles form broad, slightly varied rows rather than a sharp repetitive grid. Ivy hugs selected walls; flowers cluster beside cottages and paths rather than obscuring commands. Avoid high-frequency colour patches. Trees frame settlements and leave command paths legible. Glow links should communicate connectivity without covering units; disconnected and sabotaged nodes need shape/state indicators in addition to colour. Night scenes retain ground and unit readability, with warm lights confined to small focal areas and sparse fireflies.

## Initial budgets

These are provisional targets for the RTX 3050 Laptop 4 GB VRAM system in the brief, not benchmark results.

| Asset class | LOD0 triangle target | Material slots | Later LOD target |
| --- | ---: | ---: | --- |
| Small building | 1,000–6,000 | ≤ 8 | LOD1 ≤ 50%, LOD2 ≤ 25% |
| Castle / sanctuary | 4,000–15,000 | ≤ 8 | LOD1 ≤ 50%, LOD2 ≤ 25% |
| Foot unit | 500–3,000 | ≤ 6 | LOD1 ≤ 50%, LOD2 ≤ 25% |
| Cavalry / siege | 1,500–6,000 | ≤ 8 | LOD1 ≤ 50%, LOD2 ≤ 25% |
| Tree / rock | 200–2,000 | ≤ 3 | Instanced; LODs before dense map use |

Shared opaque materials and untextured colour geometry keep the first assets light. Final UVs, atlases, LODs, merged material sections, instancing and texture budgets require measurement in Unreal. A low polygon count alone does not establish good performance: draw calls, shadows, AI and visibility all contribute.

## Review gate

The Blender showcase demonstrates asset cohesion under composed lighting. Label its images as **Blender asset showcase — not Unreal gameplay**. Import into a real test map before approving scale, pivots, collision, lighting or camera readability. Final models, rigs, movement/attack/work/spell animations, destruction states, UV/baking passes, Niagara VFX and laptop performance validation remain work to complete. See `ASSET_REVIEW.md` for the acceptance checklist.
