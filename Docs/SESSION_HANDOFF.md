# Wildlight Session Handoff

Updated: 10 October 2026. This file is the durable continuation context for either account. Read it with `AGENTS.md`, the original brief, and `Requirements.json`.

## Recovery And Repository

- Original cloud task: `01a124bb-f708-7253-bbc6-964469454061`.
- Recovered project: 93 original files, 1,060,391 bytes. The original recovery snapshot remains unchanged outside this checkout.
- Original brief SHA-256: `68f3aeb67af1a6b56260d556b1b8381b5d6b828e4b1d6c05fd98f5b89df3868c`.
- `Recovery/RECOVERY_MANIFEST.json` records the original snapshot, not the later edited checkout. `Recovery/MISSING_ASSETS.json` records 200 missing original binary assets.
- Missing: 82 static exports, four rigged exports, two Blender masters, three renders, eight WAVs, 50 SVGs, 50 PNGs and one icon contact sheet. No original `.uasset` or `.umap` had been produced.
- Public repository: `https://github.com/Manu8733/project-wildlight`. Public visibility and the initially empty remote were verified on 10 October. The user explicitly approved public push after being told about the visibility.
- First source checkpoint: `e0731fc1d7295c75342eae985c6077bd378d00a8`, confirmed by `git ls-remote origin refs/heads/main`. Its [GitHub core workflow](https://github.com/Manu8733/project-wildlight/actions/runs/38064225293) completed successfully. Binary assets were not included.
- The initial recovery archive was CRC- and per-file-SHA-256-verified. It is not a complete cloud backup or restored Git history.

## Current User Requirements

1. One large, carefully authored forest map, supporting up to six participants. Opponent count must be selectable. The existing single-player-first brief suggests one human plus one to five AI, but FFA/teams and final dimensions are not yet approved.
2. Refined medium-or-higher geometry. Tiny Glade is a visual direction reference, not a source of copied assets. Do not promote procedural low-poly prototypes to final art.
3. Review both original implementation lists: 72 plus 33 requirements. The 50-item third list and Steam publication remain deferred.
4. Use the owned Epic/Fab library first; Meshy only fills confirmed gaps. New Meshy generation uses 7.1, or 7 if required. Do not use a cheaper old model silently.
5. Show the actual rendered asset with its materials/textures. Foliage needs wind, not just a static model.
6. Push tested milestones and context before usage exhaustion. Never force-push over another account's commits.
7. The requested cloud is Codex Cloud, not a rented GPU/SSH server. No server rental or subscription purchase is authorized.
8. Two to three tree species/types, each in different sizes. Harvesting trees, deposits and crops must visibly change them during the operation, not only after depletion. Buildable houses need automatic levels with genuinely larger/different structures; extend suitable progression to farms and other production buildings. The user approved the [written house/farm growth design](superpowers/specs/2026-10-10-automatic-growth-design.md), including the concrete 100 wood/50 stone reserve, 80% population pressure for 10 seconds, 120/240 food milestones and 15-second automatic cooldown, and requested implementation. This is not feature acceptance or completed code.
9. The user rejected the Meshy oak study as a final tree. Preserve it as historical work only; do not import or use it in the final map. Search owned/downloaded Epic/Fab environments for the necessary trees, grass and flowers first. All selected vegetation must visibly respond to wind, using its suitable material/vertex-weight wind system or a skeletal rig where warranted; a static mesh alone is not wind acceptance.
10. Niagara VFX must also follow the Epic/Fab-first policy. Create only the missing suitable effects, with actual Unreal compilation and visual/performance validation required. Existing VFX source specifications are not functioning Niagara systems.
11. The user wants cloud-heavy development and to test on the laptop only at the end. Do not start local Unreal sessions, Blender renders or GPU loads. Source editing and browser library inventory remain lightweight local coordination; use cloud/GitHub workers for Core compilation and tests. The current Codex Cloud worker has no verified Unreal/Blender/GPU toolchain; do not claim engine or render work there.

## Verified Baseline

Windows portable-core verification used GCC 15.1.0 (MSYS2 MinGW64), CMake 3.29.2 and GNU Make 4.4.1. Release C++20 with warnings-as-errors, no exceptions and no RTTI passed configuration, build and CTest. Results: 26 scenarios, 452 assertions; CTest 2/2, including a 600-tick headless match with seed 101. See `Evidence/2026-10-10/core-baseline.json` and companion logs. No sanitizers, six-participant match or Unreal build were run by this baseline.

The follow-up cloud chat independently cloned the first checkpoint in detached HEAD and passed `python3 Tools/verify.py --core-only`: all six checks, 26 scenarios and 452 assertions, plus the 600-tick headless match. It found GCC 13.3.0, Python 3.12.14, Git 2.51.1, Git LFS 3.4.1 and Make; CMake, Blender, UnrealEditor and `nvidia-smi` were not found in PATH. This is a portable-core check, not a GPU or Unreal session.

The later local regression checkpoint fixes three confirmed defects: effect-order-dependent paths through RootBarrier/LightBridge overlaps; acceptance of active research on an already-maxed technology; and the Windows verifier silently skipping runtime checks when the compiler produced `.exe` files. Each defect was reproduced before its fix. Fresh Windows verification now runs all six checks and passes 28 scenarios/769 assertions plus 600 headless ticks. The isolated Python suite passes 14/14 tests, covering visibility/acceptance reporting and verifier execution on Windows/Linux. CMake/CTest also passed 2/2. See `Evidence/2026-10-10/core-regressions-windows.json`. The independent review reported no remaining actionable findings in this limited scope. These fixes do not implement six participants, staged harvesting or automatic building growth.

Published regression source checkpoint: `1f7423945de61d40f04bcf6e27833cb4732dabf7`. It matched `origin/main` and its [GitHub workflow](https://github.com/Manu8733/project-wildlight/actions/runs/38065197895) passed both Python and core checks. The cloud independently fetched this exact commit and passed 14/14 Python tests plus all six normal core checks (28/28 scenarios, 769 assertions and 600 ticks). The default sanitized command **failed** because LeakSanitizer cannot read `/proc/.../task` in that cloud. An additional run of the same sanitized binaries with `ASAN_OPTIONS=detect_leaks=0` passed without ASan/UBSan diagnostics. Preserve the default FAIL: memory-leak detection remains unverified. No source changes, paid generation or GPU/Unreal checks were performed by that cloud verification.

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

The bounded local downloaded-library inventory found Ribbon Grass, Medieval Modular Wall Pack and Day by the Beach. C: and D: plus 18 standard alternate paths were checked, but this is not a full account-ownership inventory. The online Fab library requires Epic login in Chrome before absence can be confirmed. No licensed Epic files were copied to GitHub. See `ASSET_SOURCE_POLICY.md`.

## Cloud And Cross-Account Continuation

A separate current-account cloud chat was created: `6aca4e5e-be78-83eb-aa4c-46fca7061ac8` (title returned: `Verificare atașament Wildlight`; Codex executor `01a12642-643a-7713-9ec3-53468169d05c`, host `durable`). It received the full original text brief, but not the local source ZIP or oak files. After the public push, it cloned the exact first checkpoint, read all three lists, verified the brief hash and passed portable-core tests. Its checkout remained clean. It is not a published reusable Codex Cloud environment, and Blender/Unreal/GPU availability is not established there.

Use the GitHub repository as the common versioned source. Each account must connect its own authorized GitHub access for writes. Personal vault values and cloud task files are account/task-specific. Existing task files and conversations do not become a live shared filesystem through a repository or a shared link.

At each continuation: fetch the remote, inspect status and divergence, read this handoff, run the relevant tests, commit a focused milestone and push normally. Reconcile concurrent commits; never reset or overwrite work from the other account. Write outcomes and remaining gates into the repository before ending.

## Next Work

Implement the approved automatic house/farm design with compatibility tests and a cloud-verified checkpoint. Review the real Fab library before selecting vegetation, building tiers or VFX; the rejected oak is not a final-asset candidate. Resolve the six-participant map design before expanding the faction/map/save contract. Full engine import, gameplay, performance, package and user acceptance remain required; Unreal/Blender/GPU work needs a verified compatible cloud toolchain, not a source-only claim.
