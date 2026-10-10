# Wildlight Session Handoff

Updated: 10 October 2026. This file is the durable continuation context for either account. Read it with `AGENTS.md`, the original brief, and `Requirements.json`.

## Recovery And Repository

- Original cloud task: `01a124bb-f708-7253-bbc6-964469454061`.
- Recovered project: 93 original files, 1,060,391 bytes. The original recovery snapshot remains unchanged outside this checkout.
- Original brief SHA-256: `68f3aeb67af1a6b56260d556b1b8381b5d6b828e4b1d6c05fd98f5b89df3868c`.
- `Recovery/RECOVERY_MANIFEST.json` records the original snapshot, not the later edited checkout. `Recovery/MISSING_ASSETS.json` records 200 missing original binary assets.
- Missing: 82 static exports, four rigged exports, two Blender masters, three renders, eight WAVs, 50 SVGs, 50 PNGs and one icon contact sheet. No original `.uasset` or `.umap` had been produced.
- Public repository: `https://github.com/Manu8733/project-wildlight`. Public visibility and the initially empty remote were verified on 10 October. The user explicitly approved public push after being told about the visibility.
- The initial recovery archive was CRC- and per-file-SHA-256-verified. It is not a complete cloud backup or restored Git history.

## Current User Requirements

1. One large, carefully authored forest map, supporting up to six participants. Opponent count must be selectable. The existing single-player-first brief suggests one human plus one to five AI, but FFA/teams and final dimensions are not yet approved.
2. Refined medium-or-higher geometry. Tiny Glade is a visual direction reference, not a source of copied assets. Do not promote procedural low-poly prototypes to final art.
3. Review both original implementation lists: 72 plus 33 requirements. The 50-item third list and Steam publication remain deferred.
4. Use the owned Epic/Fab library first; Meshy only fills confirmed gaps. New Meshy generation uses 7.1, or 7 if required. Do not use a cheaper old model silently.
5. Show the actual rendered asset with its materials/textures. Foliage needs wind, not just a static model.
6. Push tested milestones and context before usage exhaustion. Never force-push over another account's commits.
7. The requested cloud is Codex Cloud, not a rented GPU/SSH server. No server rental or subscription purchase is authorized.
8. Two to three tree species/types, each in different sizes. Harvesting trees, deposits and crops must visibly change them during the operation, not only after depletion. Buildable houses need automatic levels with genuinely larger/different structures; extend suitable progression to farms and other production buildings. Automatic-level triggers, costs and progression rules still need a focused design review.

## Verified Baseline

Windows portable-core verification used GCC 15.1.0 (MSYS2 MinGW64), CMake 3.29.2 and GNU Make 4.4.1. Release C++20 with warnings-as-errors, no exceptions and no RTTI passed configuration, build and CTest. Results: 26 scenarios, 452 assertions; CTest 2/2, including a 600-tick headless match with seed 101. See `Evidence/2026-10-10/core-baseline.json` and companion logs. No sanitizers, six-participant match or Unreal build were run by this baseline.

Installed Unreal is 5.6.1; the recovered project lock is 5.8.1. Do not relabel the lock or claim engine compatibility without a deliberate tested migration. Blender 5.2.2 LTS is available and has produced native CPU renders. The previous generators recorded Blender 4.3.2.

## Existing Implementation Limits

- Simulation currently has Player, Enemy and Neutral, only two faction/fog states and two starts.
- Scenario grids are 64 by 48 cells at two metres per cell: 128 by 96 metres. Three prototype scenarios are not the requested single premium map.
- Only Enemy has AI; destruction of the first base ends the current two-faction match.
- Save loading assumes two factions, the old dimensions and three map indices. Expand with explicit format compatibility and atomic failure tests.
- HUD and minimap also assume the old map/opponent/faction contract. Adding extra base meshes alone would not create six participants.

## Art Status

One Meshy 6 Lite oak test was approved and generated before the later model preference: 10 geometry credits, free-plan CC BY 4.0. Original GLB SHA-256: `00abb0c19b27efc41f69696f7803a25b4a4ea1b89da3438b5f803ab0246f8bb7`. Do not regenerate it merely to change the model label.

Blender refinement v1 has a grounded 5.43 m pivot, smooth geometry, bark/foliage materials, UVs, vertex colors, LODs at 38,000/18,000/8,000 triangles and a simple trunk collision proxy. It is an art study pending user and engine acceptance, not final Tiny Glade-quality vegetation. PNG texture baking and wind integration were still outstanding at this checkpoint.

Independent QA passed 68/69 round-trip checks. The one strict normal comparison fails after Blender reimport. A separate raw-GLB diagnostic passes the same `1e-4` tolerance on all 38,000 triangles (max error `8.3736e-5`); the larger difference is introduced by Blender custom-normal reimport, not the exported normal accessor. Keep this warning visible and verify Unreal import/rendering. Do not loosen the global tolerance to hide it.

The local downloaded library inventory found Ribbon Grass, Medieval Modular Wall Pack and Day by the Beach. This is not a full account-ownership inventory. No licensed Epic files were copied to GitHub. See `ASSET_SOURCE_POLICY.md`.

## Cloud And Cross-Account Continuation

A separate current-account cloud chat was created: `6aca4e5e-be78-83eb-aa4c-46fca7061ac8` (title returned: `Verificare atasament Wildlight`). It received the full original text brief, but not the local source ZIP or oak files. Its initial response confirmed the missing attachment. It is not a published reusable Codex Cloud environment.

Use the GitHub repository as the common versioned source once the push is verified. Each account must connect its own authorized GitHub access. Personal vault values and cloud task files are account/task-specific. Existing task files and conversations do not become a live shared filesystem through a repository or a shared link.

At each continuation: fetch the remote, inspect status and divergence, read this handoff, run the relevant tests, commit a focused milestone and push normally. Reconcile concurrent commits; never reset or overwrite work from the other account. Write outcomes and remaining gates into the repository before ending.

## Next Work

Resolve the six-participant map design, then implement and test the faction/map/save contract before making a polished Unreal scene around it. Finish the oak texture/wind study without further Meshy spending; review the real library before deciding a new generation. Full engine import, gameplay, performance, package and user acceptance remain required.
