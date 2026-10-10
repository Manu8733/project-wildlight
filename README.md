# Project Wildlight

A single-player fantasy RTS in which a connected light network powers a growing forest kingdom. The first playable milestone focuses on building, resource logistics, beacon sabotage and magic that changes routes. All player-facing text is English.

**This repository is an implementation workbench, not a verified Windows game build.** The portable C++ simulation has been compiled and tested on Windows with MinGW. Unreal compilation, Windows packaging, integrated visuals and GPU playtesting remain unverified. Steam work is deferred, as requested.

Read [the current handoff](Docs/SESSION_HANDOFF.md) first. This is a partial recovery of the original cloud sources: 200 original binary assets were not transferred. Historical manifests and reports must not be treated as current proof that those files exist. The user now requests one refined map supporting up to six participants and an Epic/Fab-library-first asset workflow.

## Start here

1. Read [current evidence and remaining work](Docs/PROGRESS.md). The two requested lists contain **72 + 33 = 105 requirements**. Source implementation, cloud verification, engine verification and user acceptance are tracked separately.
2. For the portable simulation, follow [Build and verification](Docs/BUILD_AND_VERIFY.md).
3. On the Windows laptop, install **Unreal Engine 5.8.1**, Visual Studio 2022 with the Unreal C++ workload, Git and Git LFS. Follow [the laptop checklist](Docs/LAPTOP_QUICKSTART.md), then run `Tools/Build-Windows.ps1` from PowerShell.
4. Read [the game design](Docs/GDD.md), [art direction](Docs/ArtBible.md) and [QA acceptance plan](Docs/QA_PLAN.md) before changing balance or adding content.

## Repository layout

| Path | Purpose |
| --- | --- |
| `Core/` | Engine-independent simulation, catalogs and tests |
| `Source/Wildlight/` | Unreal C++ presentation, input and game bootstrap |
| `Config/` | Locked defaults and laptop graphics configuration |
| `Art/Source/`, `Art/Exports/`, `Art/Renders/` | Original Blender source, exports and clearly labeled asset previews |
| `Tools/` | Import, verification, Windows build and independent backup tools |
| `Docs/` | Original brief, design, traceability, handoff and test plans |

## What the initial integration is designed to support

Three scenario definitions, nineteen building definitions, twelve unit definitions, nine spell definitions and three AI personalities exist as simulation catalogs. Catalog entries are not equivalent to finished, animated, balanced game content. The engine adapter presents these systems through a strategic camera, selection, construction previews and an English HUD; it has not been compiled against Unreal in this environment.

The original visual kit is authored with explicit geometry and materials in Blender scripts. The project does not depend on generated stock images. Rendered kit previews are art previews, not proof of Unreal gameplay or final graphical quality.

View the [day and evening asset previews](Docs/VISUAL_PREVIEW.md) to inspect the current original style.

The repository is [Manu8733/project-wildlight](https://github.com/Manu8733/project-wildlight). It was verified public on 10 October 2026; the user explicitly approved pushing to this public repository. The original private-repository request is preserved in the brief, with this later override recorded in the handoff. Push and hosted-check evidence are tracked separately from game acceptance. The source archive remains usable independently of GitHub.

## Scope and ownership

The starting project name is internal. Single-player comes first; multiplayer is a documented future architecture only. No paid asset purchase, workstation rental, Steam account setup or publication has been performed. The user's [original brief](Docs/Original_Romanian_Brief.txt) remains the source for the requested scope.

Use [asset provenance](Docs/ASSET_PROVENANCE.md) when bringing in models, textures, audio or fonts. Keep credentials outside the repository. See [backup and recovery](Docs/BACKUP_AND_RECOVERY.md) for an independent restore procedure.
