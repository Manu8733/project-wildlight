# Working rules for Wildlight

User instructions and the original brief in `Docs/Original_Romanian_Brief.txt` determine project scope. Work on Lists 1 and 2; defer List 3 and any Steam publication. The initial game and project documentation are English. The user's Romanian handoff may remain Romanian.

## Later user instructions, 10 October 2026

- Read `Docs/SESSION_HANDOFF.md` before continuing. The recovered binary asset inventory is incomplete; historical reports are not fresh verification.
- Build one large, refined map with room for up to six participants and selectable opponent count. The participant/network mode and exact map design must be resolved before replacing the two-faction implementation. Single-player-first still applies.
- Final visuals require medium-or-higher refined geometry and the original art direction inspired by Tiny Glade; the old low-poly kit is prototype-only.
- Search the user's Epic/Fab library first. Use Meshy only for a missing, suitable asset, using Meshy 7.1 (or 7 where necessary) for new generation. Read `Docs/ASSET_SOURCE_POLICY.md`; check usable download rights and an approved credit budget before submitting.
- The user explicitly approved a push to the public repository. Do not publish credentials or third-party asset files whose redistribution rights have not been verified. Keep Git LFS enabled for binary asset types.
- Commit and push verified milestones promptly, including a truthful progress handoff. Do not wait until the usage window is nearly exhausted. Git is version synchronization, not shared live task memory.

## Evidence and acceptance

- Do not describe generated source as a compiled or playtested Unreal game. Record the toolchain, exact command, result and evidence path for verification.
- Use `Docs/Requirements.json` as the requirement ledger and `Docs/PROGRESS.md` as the readable summary. All original requirement IDs are stable. Do not mark a requirement accepted without explicit user acceptance.
- Preserve `Docs/Original_Romanian_Brief.txt`; do not rewrite the requested checklist or silently remove difficult tasks.
- An untested Unreal feature is `ImplementedNotEngineVerified`. GPU and Windows tasks remain `BlockedHardware` when no suitable machine is available. A catalog entry alone does not complete final content, polish, balance or accessibility.

## Implementation

- Lock the project to Unreal Engine 5.8.1 until a deliberate, documented migration is tested. Keep `Core/` free of Unreal headers and platform-only APIs.
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
