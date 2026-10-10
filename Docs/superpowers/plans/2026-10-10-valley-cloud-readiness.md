# Valley Cloud Readiness Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Establish a verified cloud authoring runtime and private asset intake before executing the approved 768 m environment design.

**Architecture:** Separate the runtime/rights gate from terrain authoring, vegetation/material production and environment-state integration. This first plan only proves the execution and source boundary; subsequent code/editor tasks require the actual engine APIs and inspected asset files. Failed connection checks remain failed and do not trigger local heavy work.

**Tech Stack:** Existing Codex Cloud executor, Unreal Engine 5.8.1, Blender, authorized Epic/Fab content, structured evidence, Git for public source only.

## Global Constraints

- One 768 by 768 metre wooded-valley environment first; future 384 by 384 logical cells at 200 cm.
- No laptop editor, rendering, shader compilation or GPU workloads; no unapproved rental, subscription, Meshy generation or premium plugin purchase.
- Preserve original source, legacy saves, licensed material/wind data and the rejected-oak exclusion.
- Public repository contains source/docs/provenance, not licensed asset binaries or secrets.
- A connected chat, installed skill or CPU test is not evidence of an Unreal/GPU renderer.

## Task 1: Prove Runtime Access

**Files:** Record `Evidence/2026-10-10/environment-cloud-readiness.json` after actual execution; preserve this plan and `SESSION_HANDOFF.md` on failure.

**Interfaces:** Consumes the existing executor identity from the handoff. Produces the verified host/tool availability, operating system, engine paths/versions, device availability and explicit pass/fail/unknown fields. No credential or environment-variable dump is permitted.

- [x] Dispatch the user's explicit lightweight retry and read the resulting turn: `01a12782-b202-7607-92d5-e6222c4dfb3a`. Result: no execution tools; no commands ran.
- [ ] Restore runtime access through the supported product/account flow and confirm terminal/filesystem tools are available. No exposed reconnection tool exists in this continuation; this is the current execution gate.
- [ ] In the connected cloud terminal only, run the read-only host probes individually and retain exit codes/output:

```bash
pwd
date -u +%Y-%m-%dT%H:%M:%SZ
command -v python3 blender UnrealEditor UnrealEditor-Cmd nvidia-smi
df -h .
free -h
nvidia-smi -L
```

- [ ] Inspect the actual engine's `Engine/Build/Build.version` as structured JSON and run the installed Blender's `--version`. An absent PATH entry is not an exhaustive installation search; inspect declared workspace installation roots without claiming unseen applications are missing.
- [ ] Run a tiny native render only after executable/version compatibility and resource availability are established; save new output, command, exit code and timing in isolated cloud scratch. If the probe fails or Unreal 5.8.1 is unavailable, record the precise limit and stop this gate. Do not migrate the engine lock or fabricate GPU evidence.
- [ ] Commit/push the factual source-only readiness report after review. Do not mark runtime passed when only metadata reads work.

## Task 2: Establish Authorized Private Asset Intake

**Files:** Read `Docs/ASSET_SOURCE_POLICY.md`, `Docs/FAB_ASSET_SHORTLIST.md`, `Docs/ArtBible.md`, `Tools/Import-Assets.py` and `Art/Source/validate_wildlight_art.py`. Record source-selection/provenance separately from private binaries.

**Interfaces:** Consumes a passed runtime gate and authorized Fab access. Produces inspected candidate files with exact author/source/license, hashes, engine compatibility, scale/pivot, material slots, textures, wind dependencies, LODs and collision state. Pending/failed inspections remain explicit; file ownership is not inferred from a product name.

- [ ] Establish authorized private storage outside the public checkout and verify that selected files cannot be staged into its public Git history. Do not upload assets to Meshy or publish them as CI artifacts.
- [ ] Retrieve only the selected owned tree/grass/flower/rock/deposit/water candidates using the account's permitted downloads. Keep the download/source/license evidence; a purchased product card is not permission to redistribute it.
- [ ] Inspect one representative tree and plant in compatible Unreal before bulk conversion. Preserve native wind/PBR dependencies; do not invoke the old palette importer or full art generator over the sources.
- [ ] Use an isolated Blender copy only for warranted adaptations. Keep original files and metadata; use actual export/native render evidence, not a geometry score alone, for acceptance.
- [ ] Classify each required category as accepted candidate, needs adaptation or confirmed missing. Only the last category may lead to a separate Meshy 7.1/7 proposal with an approved budget.
- [ ] Review and publish source-only provenance/intake results, then create the terrain/blockout implementation plan using the inspected engine APIs and measured unit/formation/building metrics. Do not resume FFA expansion before the environment milestone.

## Current Gate Result

Runtime remains unavailable: the cloud retry reported `exec_command: undefined` and `write_stdin: undefined`, without filesystem or reconnection tools. The previous `409 environment_offline` was not a newly rerun command response. Tasks requiring terminal/editor/asset transfer are not executed. No map, wind, weather or render completion is claimed by this plan.
