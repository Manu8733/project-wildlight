# Wildlight implementation evidence

**The requested work is not fully complete.** The first two original lists contain **72 and 33 tasks (105 total)**; the declared totals match the parsed checkboxes. List 3 / Steam is deferred. All 105 user-acceptance flags remain false until explicit acceptance is recorded.

Read [the current recovery and continuation handoff](SESSION_HANDOFF.md) first. The original cloud's evidence is historical; 200 original binary assets were not transferred. Fresh Windows MinGW portable-core checks passed, but Unreal APIs, Editor/Shipping builds, integrated visuals, GPU performance and independent playtests remain unverified.

The [GitHub repository](https://github.com/Manu8733/project-wildlight) was verified public on 10 October 2026. The user explicitly approved public push, overriding the original private-repository request. The first checkpoint push is being prepared; hosted workflow execution and remote LFS restoration are not yet verified.

## Status totals

| Evidence state | List 1 | List 2 | Total |
| --- | ---: | ---: | ---: |
| `NotStarted` | 1 | 0 | 1 |
| `ImplementedNotEngineVerified` | 51 | 0 | 51 |
| `VerifiedCloud` | 13 | 0 | 13 |
| `BlockedHardware` | 5 | 33 | 38 |
| `NeedsPlaytest` | 2 | 0 | 2 |
| `BlockedCredentials` | 0 | 0 | 0 |

A `VerifiedCloud` row covers only the recorded document, configuration, asset or portable-logic scope. It does not imply full composite-task completion, engine verification or user acceptance. `scope_coverage` and `remaining` in [Requirements.json](Requirements.json) make those limits explicit.

## Verification record

Fresh baseline: Windows GCC 15.1.0 / CMake 3.29.2 / GNU Make 4.4.1, Release C++20. Configuration, build and CTest passed: 26 scenarios, 452 assertions and CTest 2/2. Evidence: `Evidence/2026-10-10/core-baseline.json` and companion logs. This does not verify Unreal, sanitizers, six participants or current missing asset exports.

## Outstanding gates

- Compile Unreal 5.8.1 on Windows; run importer, editor match and actual Shipping package.
- Finish and review unit rigs/animation, final UV/texture/LOD/collision, VFX/audio, complete tutorial/remapping/accessibility and all polished scenarios.
- Measure 100/200/500-unit scenes where supported and long sessions on an actual GPU; regress RTX 3050 Low.
- Complete fair-AI and economy/class tuning, independent playtest and explicit acceptance.
- Confirm public-repository push/hosted checks and copy verified backup to independent owner-controlled storage.

## Full two-list traceability

Every original item is retained below; detailed remaining work, original Romanian wording, source line, commands and evidence paths are in the JSON ledger. No original checkbox has been auto-checked.

### List 1

| ID | Requirement | State | Coverage |
| --- | --- | --- | --- |
| `CLOUD-A01` | Define audience, comparisons and light-network differentiation | `VerifiedCloud` | document |
| `CLOUD-A02` | Finalize game design, pillars, loop, objectives and victory conditions | `VerifiedCloud` | document |
| `CLOUD-A03` | Define milestones, sprints, tasks and done criteria | `VerifiedCloud` | document |
| `CLOUD-A04` | Choose and lock a stable Unreal Engine version | `VerifiedCloud` | configuration |
| `CLOUD-A05` | Install local Unreal, C++ toolchain, Git and Git LFS | `BlockedHardware` | none |
| `CLOUD-A06` | Create Unreal C++ project and a correctly starting minimal level | `ImplementedNotEngineVerified` | partial |
| `CLOUD-A07` | Create private GitHub repository and sensitive-file safeguards | `VerifiedCloud` | partial |
| `CLOUD-A08` | Connect repository workflow, agent rules, PR templates and checks | `ImplementedNotEngineVerified` | partial |
| `CLOUD-A09` | Define C++, Blueprint, Data Asset, naming and folder standards | `VerifiedCloud` | document |
| `CLOUD-A10` | Design command, simulation, presentation, AI, save and content architecture | `VerifiedCloud` | document |
| `CLOUD-A11` | Design stable IDs, serialization and controlled simulation steps | `ImplementedNotEngineVerified` | partial |
| `CLOUD-A12` | Inventory required assets, licenses, authors, sources and costs | `ImplementedNotEngineVerified` | partial |
| `CLOUD-A13` | Configure backups, asset versioning and verified restoration | `ImplementedNotEngineVerified` | partial |
| `CLOUD-B01` | Implement strategic camera, pan, zoom, rotation, edge scroll and limits | `ImplementedNotEngineVerified` | partial |
| `CLOUD-B02` | Implement single/box selection, unit groups and indicators | `ImplementedNotEngineVerified` | partial |
| `CLOUD-B03` | Implement remappable input and right-click orders | `ImplementedNotEngineVerified` | partial |
| `CLOUD-B04` | Implement Build, Move, Attack, Gather and CastSpell command infrastructure | `ImplementedNotEngineVerified` | partial |
| `CLOUD-B05` | Implement valid/invalid building preview, rotation, collision and costs | `ImplementedNotEngineVerified` | partial |
| `CLOUD-B06` | Implement progressive construction, repair, upgrade, demolition and cancellation | `ImplementedNotEngineVerified` | partial |
| `CLOUD-B07` | Implement connected wall, gate and tower rules | `ImplementedNotEngineVerified` | partial |
| `CLOUD-B08` | Create resource/building/unit/alert/minimap/tooltip HUD | `ImplementedNotEngineVerified` | partial |
| `CLOUD-B09` | Create main menu, pause, settings and victory/defeat screens | `ImplementedNotEngineVerified` | partial |
| `CLOUD-C01` | Define wood, stone, food, gold and magic energy | `ImplementedNotEngineVerified` | partial |
| `CLOUD-C02` | Implement storage/capacity, production, consumption and costs | `ImplementedNotEngineVerified` | partial |
| `CLOUD-C03` | Implement workers, jobs, assignment and production priorities | `ImplementedNotEngineVerified` | partial |
| `CLOUD-C04` | Implement gathering, mining, farms, short chains and cargo logistics | `ImplementedNotEngineVerified` | partial |
| `CLOUD-C05` | Implement housing, population, morale and shortage consequences | `ImplementedNotEngineVerified` | partial |
| `CLOUD-C06` | Implement economic research/upgrades with explicit prerequisites | `ImplementedNotEngineVerified` | partial |
| `CLOUD-C07` | Automatically evaluate balance and progression deadlocks | `NeedsPlaytest` | partial |
| `CLOUD-D01` | Implement recruitment, upkeep, classes and unit statistics | `ImplementedNotEngineVerified` | partial |
| `CLOUD-D02` | Implement group movement, formations, avoidance and collision | `ImplementedNotEngineVerified` | partial |
| `CLOUD-D03` | Implement paths through walls, temporary bridges and gates | `ImplementedNotEngineVerified` | partial |
| `CLOUD-D04` | Implement melee, ranged projectiles, armor, damage and cooldowns | `ImplementedNotEngineVerified` | partial |
| `CLOUD-D05` | Implement defense, patrol, retreat, assistance and target priorities | `ImplementedNotEngineVerified` | partial |
| `CLOUD-D06` | Implement rams, catapults, defense towers and destruction | `ImplementedNotEngineVerified` | partial |
| `CLOUD-D07` | Balance unit classes and readable strategic controls | `NeedsPlaytest` | partial |
| `CLOUD-E01` | Implement beacons and verifiable node connections | `ImplementedNotEngineVerified` | partial |
| `CLOUD-E02` | Compute network influence, consumption and supply | `ImplementedNotEngineVerified` | partial |
| `CLOUD-E03` | Connect production and territory effects to the light network | `ImplementedNotEngineVerified` | partial |
| `CLOUD-E04` | Implement sabotage/restoration with clear player feedback | `ImplementedNotEngineVerified` | partial |
| `CLOUD-E05` | Implement first wizard, Light Bridge and Root Barrier | `ImplementedNotEngineVerified` | partial |
| `CLOUD-E06` | Validate duration, mana, cooldown, counterplay and terrain | `ImplementedNotEngineVerified` | partial |
| `CLOUD-E07` | Extend to three magic schools and approved abilities | `ImplementedNotEngineVerified` | partial |
| `CLOUD-E08` | Implement magic ruins, sites, side objectives and exploration | `ImplementedNotEngineVerified` | partial |
| `CLOUD-E09` | Integrate day/night atmosphere while preserving RTS readability | `ImplementedNotEngineVerified` | partial |
| `CLOUD-F01` | Implement AI economy, building plan, production and resources | `ImplementedNotEngineVerified` | partial |
| `CLOUD-F02` | Implement AI recruitment, scouting, attack, defense and siege | `ImplementedNotEngineVerified` | partial |
| `CLOUD-F03` | Implement AI beacon defense, link attacks and spell use | `ImplementedNotEngineVerified` | partial |
| `CLOUD-F04` | Implement three AI personalities and fair testable difficulties | `ImplementedNotEngineVerified` | partial |
| `CLOUD-F05` | Implement outcomes, surrender, restart and match statistics | `ImplementedNotEngineVerified` | partial |
| `CLOUD-F06` | Implement scenario/start-position/map/objective infrastructure | `ImplementedNotEngineVerified` | partial |
| `CLOUD-F07` | Implement guided tutorial, challenges and manageable onboarding | `ImplementedNotEngineVerified` | partial |
| `CLOUD-F08` | Implement saves, autosave, loading, migration and corruption tests | `ImplementedNotEngineVerified` | partial |
| `CLOUD-G01` | Create silhouette, palette, proportion, material and readability art bible | `VerifiedCloud` | document |
| `CLOUD-G02` | Create Blender Python building/environment generator scripts | `VerifiedCloud` | export_automation |
| `CLOUD-G03` | Create low-poly 3D prototypes with pivots and collision shapes | `ImplementedNotEngineVerified` | partial |
| `CLOUD-G04` | Automate FBX/glTF exports and validate names | `VerifiedCloud` | export_automation |
| `CLOUD-G05` | Create basic stylized materials and procedural texture recipes | `ImplementedNotEngineVerified` | partial |
| `CLOUD-G06` | Prepare rigs and simple animations within hardware limits | `NotStarted` | none |
| `CLOUD-G07` | Define Niagara/firefly scripts and parameters | `VerifiedCloud` | document |
| `CLOUD-G08` | Create UI icons and sound/music specifications | `ImplementedNotEngineVerified` | partial |
| `CLOUD-G09` | Integrate lightweight assets and verify laptop readability | `BlockedHardware` | partial |
| `CLOUD-H01` | Automate resource, command, building, magic and combat tests | `ImplementedNotEngineVerified` | partial |
| `CLOUD-H02` | Run feasible headless economy and AI-confrontation simulations | `ImplementedNotEngineVerified` | partial |
| `CLOUD-H03` | Verify local builds and available laptop tests | `BlockedHardware` | none |
| `CLOUD-H04` | Profile logic and create reproducible logs/diagnostics | `ImplementedNotEngineVerified` | partial |
| `CLOUD-H05` | Implement UI scale, accessibility, key remapping and reduced effects | `ImplementedNotEngineVerified` | partial |
| `CLOUD-H06` | Verify resolutions and Low settings | `BlockedHardware` | partial |
| `CLOUD-H07` | Verify provenance/license of every model, texture, sound and font | `ImplementedNotEngineVerified` | partial |
| `CLOUD-H08` | Document build, tests, debugging, versions and changelog | `VerifiedCloud` | document |
| `CLOUD-H09` | Plan future multiplayer architecture without implementation | `VerifiedCloud` | document |
| `CLOUD-H10` | Prepare playable demo and playtest feedback collection | `BlockedHardware` | partial |

### List 2

| ID | Requirement | State | Coverage |
| --- | --- | --- | --- |
| `GPU-A01` | Choose appropriate Windows GPU machine and verify budget | `BlockedHardware` | preparation_only |
| `GPU-A02` | Install Unreal, C++ toolchain, Blender, drivers and profiling utilities | `BlockedHardware` | preparation_only |
| `GPU-A03` | Configure secure remote access, Git/LFS, repo and asset sync | `BlockedHardware` | preparation_only |
| `GPU-A04` | Verify VRAM, RAM, storage, bandwidth and session cost | `BlockedHardware` | preparation_only |
| `GPU-A05` | Prepare precise session tasks and before/after backup | `BlockedHardware` | preparation_only |
| `GPU-B01` | Import all assets and verify scale, pivots and collision | `BlockedHardware` | preparation_only |
| `GPU-B02` | Finish building, castle, fortification, wizard, troop and creature models | `BlockedHardware` | preparation_only |
| `GPU-B03` | Finish UVs, baking, premium textures/materials and optimized atlases | `BlockedHardware` | preparation_only |
| `GPU-B04` | Finish rigging, skinning and move/attack/work/spell animation | `BlockedHardware` | preparation_only |
| `GPU-B05` | Finish LOD, instancing, culling, texture streaming and VRAM budgets | `BlockedHardware` | preparation_only |
| `GPU-B06` | Polish maps, vegetation, water, terrain, ruins and magic sites | `BlockedHardware` | preparation_only |
| `GPU-C01` | Finalize lighting, shadows, exposure and reflections | `BlockedHardware` | preparation_only |
| `GPU-C02` | Compare Lumen/Nanite and affordable low-tier alternatives | `BlockedHardware` | preparation_only |
| `GPU-C03` | Integrate Niagara fireflies, ambient particles, magic dust and animated vegetation | `BlockedHardware` | preparation_only |
| `GPU-C04` | Finalize spell/projectile/explosion/destruction VFX | `BlockedHardware` | preparation_only |
| `GPU-C05` | Integrate fog, weather and day/night with RTS readability | `BlockedHardware` | preparation_only |
| `GPU-C06` | Integrate animation/audio with actual gameplay events | `BlockedHardware` | preparation_only |
| `GPU-C07` | Polish interface across resolutions and UI scales | `BlockedHardware` | preparation_only |
| `GPU-C08` | Capture representative gameplay and gameplay trailer | `BlockedHardware` | preparation_only |
| `GPU-D01` | Run full compilation, shader compilation and integrated project tests | `BlockedHardware` | preparation_only |
| `GPU-D02` | Run supported 100/200/500+ unit stress tests | `BlockedHardware` | preparation_only |
| `GPU-D03` | Profile CPU/GPU with Unreal Insights and engine tools | `BlockedHardware` | preparation_only |
| `GPU-D04` | Optimize pathfinding, AI, materials, VFX and memory | `BlockedHardware` | preparation_only |
| `GPU-D05` | Test dense maps and long sessions with developed economy | `BlockedHardware` | preparation_only |
| `GPU-D06` | Detect stalls, leaks, errors and crashes | `BlockedHardware` | preparation_only |
| `GPU-D07` | Define and verify Low/Medium/High/Epic graphics tiers | `BlockedHardware` | preparation_only |
| `GPU-D08` | Validate strong GPU and regress RTX 3050 laptop locally | `BlockedHardware` | preparation_only |
| `GPU-E01` | Playtest integrated final systems and maps | `BlockedHardware` | preparation_only |
| `GPU-E02` | Fix animation, collision, camera, UI and navigation defects | `BlockedHardware` | preparation_only |
| `GPU-E03` | Compile/cook/package Windows Shipping and test clean installation | `BlockedHardware` | preparation_only |
| `GPU-E04` | Validate actual system requirements and stability | `BlockedHardware` | preparation_only |
| `GPU-E05` | Prepare candidate build, QA reports and asset package | `BlockedHardware` | preparation_only |
| `GPU-E06` | Sync repo/backups and stop paid compute after session | `BlockedHardware` | preparation_only |
