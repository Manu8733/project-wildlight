# Working rules for Wildlight

User instructions and the original brief in `Docs/Original_Romanian_Brief.txt` determine project scope. Work on Lists 1 and 2; defer List 3 and any Steam publication. The initial game and project documentation are English. The user's Romanian handoff may remain Romanian.

## Later user instructions, 10 October 2026

- Read `Docs/SESSION_HANDOFF.md` before continuing. The recovered binary asset inventory is incomplete; historical reports are not fresh verification.
- Build one large, refined 1600 by 1200 metre rectangular map with selectable one to five AI opponents. The explicit 11 October size approval supersedes both the intermediate 1600 metre square and previous 768 metre map. At 200 cm per cell the future logical grid is 800 by 600; the legacy Core grid is not yet migrated. Six authored starts must be irregularly positioned, not aligned in two columns. Keep the actual physical scale of units, buildings and resources. One human plus one to five AI, free-for-all, each with its own base and no online multiplayer, remains confirmed. The wooded valley layout and clear/cloudy/light-rain visual weather are approved; surrender, victory and simultaneous-loss proposals still require approval before replacing the two-faction implementation. Single-player-first still applies.
- The new FFA has one starting castle per participant, no extra castles and no capture or ownership transfer. Losing that sole castle eliminates its owner and automatically destroys all owned buildings/foundations, units, queues, stockpiles, research, projectiles, effects and active navigation blockers. Nothing is awarded to the victor, and saves must not resurrect eliminated assets. Neutral terrain and natural resource nodes remain. Freed space may be built on under normal placement, access, visibility and light-network rules; do not invent automatic annexation or zone ownership. These confirmed rules do not constitute approval of the remaining design or FFA implementation. Preserve the separate contract and readability of legacy saves/scenarios.
- Final visuals require medium-or-higher refined geometry and the original art direction inspired by Tiny Glade; the old low-poly kit is prototype-only.
- Latest priority: build the complete 1600 by 1200 m environment first, before further gameplay/FFA or building-tier expansion. The user approved a wooded valley with mountains at the edges, six clearings, a river with crossings, three tree species and clear/cloudy/light-rain weather with no economy/combat effects. Include natural deposits, shared wind, full day/night lighting and real progress captures. Use an oblique top-down RTS camera like Stronghold/LoL with bounded zoom; the existing source defaults are pitch -55 degrees, FOV 45, initial arm 60 m, zoom 18-110 m. Keep environment completion, working six-participant gameplay and final user acceptance separate.
- The later user instruction explicitly authorizes optimized local Unreal and asks for an MCP connection. On 11 October the user announced installation completion and explicitly approved the installed 5.8.3 patch with a backup before opening. The 5.8.1 source checkpoint is backed up independently; use 5.8.3 for environment authoring and keep gameplay compatibility unverified until tested. Do not create the declined 5.6 environment. Earlier cloud-only prohibitions are superseded for this authorized Unreal work; no paid replacement server is approved. Avoid a second editor instance and preserve other applications.
- Search the user's Epic/Fab library first. Use Meshy only for a missing, suitable asset, using Meshy 7.1 (or 7 where necessary) for new generation. Read `Docs/ASSET_SOURCE_POLICY.md`; check usable download rights and an approved credit budget before submitting.
- The user explicitly approved a push to the public repository. Do not publish credentials or third-party asset files whose redistribution rights have not been verified. Keep Git LFS enabled for binary asset types.
- Commit and push verified milestones promptly, including a truthful progress handoff. Do not wait until the usage window is nearly exhausted. Git is version synchronization, not shared live task memory.

## Evidence and acceptance

- Do not describe generated source as a compiled or playtested Unreal game. Record the toolchain, exact command, result and evidence path for verification.
- Use `Docs/Requirements.json` as the requirement ledger and `Docs/PROGRESS.md` as the readable summary. All original requirement IDs are stable. Do not mark a requirement accepted without explicit user acceptance.
- Preserve `Docs/Original_Romanian_Brief.txt`; do not rewrite the requested checklist or silently remove difficult tasks.
- An untested Unreal feature is `ImplementedNotEngineVerified`. GPU and Windows tasks remain `BlockedHardware` when no suitable machine is available. A catalog entry alone does not complete final content, polish, balance or accessibility.

## Implementation

- The current user-approved target is Unreal Engine 5.8.3; record native verification separately from the approved target change. Keep `Core/` free of Unreal headers and platform-only APIs.
- Keep game rules in the fixed-step simulation. Input requests commands; presentation reads state and events. Use stable entity IDs, validate commands centrally, and preserve atomic load failure.
- Use the actual adapter scale consistently; current integration uses 200 Unreal centimeters per grid cell. Update documentation and placement/import settings together when scale changes.
- Follow `Docs/ARCHITECTURE.md` for names, folders, serialization and Blueprint boundaries. Changes to save format require compatibility tests and an explicit version policy.
- Reject resource underflow, invalid coordinates, unauthorized ownership and stale IDs before mutation. Respect the same resource and visibility rules for player and AI.
- Use small patches with meaningful verification. Tests should exercise behavior and boundary conditions rather than mirror implementation.

## Art and content

- Build an original forest-kingdom identity following `Docs/ArtBible.md`. Do not copy protected geometry, textures, names or UI from reference games.
- New external assets need an exact source, author, license, modification notes and import path in the asset registry. Record synthesized/original source honestly; do not relabel generated previews as screenshots.
- Use source Blender files/scripts as editable masters and maintain repeatable export conventions. Do not overwrite another contributor's authored files without coordinating.
- Default to the RTX 3050 laptop's Low preset. Graphics targets are budgets until measured; do not announce minimum system requirements from speculation.

## Repository hygiene

- Use Git LFS for the configured binary asset types. Do not commit passwords, tokens, downloaded engine installations, `Saved/`, `Intermediate/`, build trees or caches.
- Keep an independent backup with restore verification; a GitHub push is not an independent backup.
- Do not add multiplayer implementation or publishing integrations until the single-player milestone is accepted.
