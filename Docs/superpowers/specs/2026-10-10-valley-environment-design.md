# Wildlight Wooded Valley Environment

Approved direction, 10 October 2026. Design approval is not implemented content or final user acceptance. The latest size is **768 by 768 metres**, superseding the intermediate 1 km proposal. Environment production is the first milestone; further FFA/gameplay and building-tier expansion are deferred.

## Experience And Scope

One authored wooded valley with mountains at the edges, six generous base clearings, forests between regions and a river with permanent crossings. The intended future match is one human plus one to five AI, free-for-all, without online multiplayer. Unused starts remain explorable space, not hidden opponents. Natural expansion space extends beyond each initial clearing.

Each active start needs equivalent initial resources and comparable access, at least two usable exits, room for economic buildings and routes wide enough for the supported units/formations. Validate a whole-map blockout before detailed dressing. Derive route/clearance measurements from actual unit collision, building footprints and depot access; do not use platformer metrics or claim balance from visual symmetry.

Use original refined medium-or-higher art with the cozy forest-kingdom direction in `ArtBible.md`. Tiny Glade is a feeling reference, not copied geometry. Author actual mountain terrain, coherent natural deposits, forest strata and water rather than enlarging prototype props. All playable regions need finished terrain and integrated materials; the old procedural kit remains prototype-only.

## Coordinates And Logical Boundary

The existing logical scale is 200 Unreal centimetres per cell. A 768 m square corresponds to 384 by 384 logical cells. Landscape vertices/components must use valid Unreal dimensions independently of that grid and cover the measured 768 m playable extent. Record the world-to-logical transform with terrain data; do not create a ground object per logical cell.

The current two-faction 64 by 48 simulation remains unchanged in this environment milestone. Start markers and an art map do not establish AI, six economies, fog, saves or working gameplay. Later FFA integration requires its own approved end-state rules and compatibility tests. The confirmed single-castle/no-capture/full-destruction rule remains recorded in `SESSION_HANDOFF.md`; surrender, victory and simultaneous-loss proposals are not approved here.

## Asset Intake And Vegetation

Inspect the owned Epic/Fab library first in authorized private staging. The observed candidates cover Black Alder, Hazel and Hornbeam in multiple sizes, grasses/flowers, rocks, mine/quarry and bridge/environment elements. Product ownership and publisher-declared wind are not proof of downloaded binaries, a compatible import or accepted final art.

Preserve PBR, opacity/translucency maps, material dependencies, UVs, vertex colors, pivots and wind metadata. Native Unreal foliage packages should retain their native wind/material functions; FBX import is not evidence that Pivot Painter survived. The current palette importer is unsuitable for selected PBR assets without a tested preservation path. Never run the old full generator over licensed staging. Do not use the rejected Meshy oak, publish licensed binaries or send NoAI inputs to generative services. Use Meshy 7.1/7 only for a confirmed gap with an approved budget.

Three tree species and multiple sizes share a coherent style. Trees, grass and flowers visibly react to gentle wind and gusts with anchored bases and per-instance variation. Verify deformation, all used LODs, culling, bounds and shadows in Unreal. Skeleton presence or still images alone do not establish wind. Natural resource stages must later respond to actual extraction; art-only variants are not integrated harvest acceptance.

## Shared Environment State

Approved weather: clear, cloudy and light rain, with gradual transitions, restrained wetness and consistent wind. This increment does not alter economy, combat or tactical visibility rules. Seasons and snow are excluded. Inspect owned effects before creating missing Niagara systems; control cosmetic effect lifetime and bounds independently of authoritative gameplay.

Lighting includes sunrise, midday, sunset and readable night, with the sun moving across the sky and coordinated atmosphere, water and ambient light. Preserve owner/selection/resource readability and the original light-network information. Use the simulation clock as the reference, not an independent day cycle that drifts across pause/load. Do not implicitly change the legacy 240-second period. Current source only adjusts sun intensity/color; a complete environment controller is not implemented.

Native Unreal Landscape, Water and Sky Atmosphere are the selected implementation direction, subject to actual compatibility and cost validation. Preserve existing Pivot Painter paths when appropriate. No plugin purchase or mandatory premium sky/weather package is approved. [Landscape](https://dev.epicgames.com/documentation/en-us/unreal-engine/landscape-overview), [Water](https://dev.epicgames.com/documentation/unreal-engine/water-system-in-unreal-engine?lang=en-US), [Sky Atmosphere](https://dev.epicgames.com/documentation/en-us/unreal-engine/sky-atmosphere-component-in-unreal-engine), [Pivot Painter](https://dev.epicgames.com/documentation/unreal-engine/pivot-painter-tool-2.0-in-unreal-engine?lang=en-US).

## Verification And Delivery

1. Prove compatible Unreal and a usable authoring connection before engine work. The later user instruction permits optimized local Unreal through MCP; wait for their announced completion of the 5.8 installation before launching it. The game remains locked to 5.8.1 unless a deliberate migration is approved/tested. No unapproved server rental, subscription or credit spending.
2. Verify private asset intake and repeatable material/wind-preserving import before adapting geometry. Use isolated versioned Blender masters and inspect exports as well as scene previews.
3. Validate the full blockout and measured extent, then terrain, routes, crossings, resource access, vegetation and water. No unfinished regions, holes, incoherent overlaps or permanently sealed starts.
4. Capture actual overview/detail renders at day, dusk, night and rain, plus a short wind/transition sequence. Index commit, engine version, preset, camera and artifact hashes. Label Blender asset previews separately from Unreal environment/gameplay captures.
5. Profile reproducible Low/High views with warm shaders, CPU/GPU/frame time and RAM/VRAM. Refine LOD/culling/density without silently downgrading the requested near-art quality. Preserve both raw evidence and unresolved defects; final acceptance requires the user's test.

Use the scoped guides in `GAME_SKILLS.md` for design, engine boundaries, Niagara, Blender evidence and profiling. They do not supply a runtime. The latest cloud retry received messages but exposed no terminal/filesystem tools; no new engine session, import or render is established.
