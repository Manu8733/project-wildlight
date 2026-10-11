# Valley Authoring Preparation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prepare a restricted local Unreal MCP probe and original 768 m terrain data while the user installs Unreal 5.8; do not launch an editor before their readiness message.

**Architecture:** An official MCP SDK stdio server exposes one fixed read-only project probe through Epic's installed Python Remote Execution module. Separately, an engine-independent authored layout and exporter produce reproducible 16-bit terrain input and import metadata. Editor execution, native Landscape creation, licensed asset intake and final visual acceptance remain subsequent gates.

**Tech Stack:** Python 3.12 standard library, optional pinned `mcp==1.28.1` for protocol transport, Pillow for the documented grayscale PNG export, Unreal Python Remote Execution after installation, existing unittest/CI.

## Global Constraints

- One 768 by 768 metre wooded-valley environment first; future 384 by 384 logical cells at 200 cm. No Core/gameplay/save changes.
- The user now authorizes local Unreal with prudent optimization, but explicitly asks to wait for their Unreal 5.8 readiness message before starting it. Do not open 5.6, migrate the game, install an engine, start render/shader workloads or close other applications.
- The user announced installation completion and approved the installed 5.8.3 patch with backup on 11 October. Build.version is verified; gameplay compatibility remains pending. Preserve the independent 5.8.1 backup and use 5.8.3 for these preparations. No paid server, premium plugin or Meshy spending.
- Epic/Fab content stays private and retains original material/wind data; no licensed binaries, secrets or engine source copies in public Git.
- Preparation tests, terrain data and MCP protocol success are not native Landscape, six-player gameplay, final art, GPU profiling or wind/weather acceptance.
- Use the original refined valley design and the scoped skills in `Docs/GAME_SKILLS.md`; do not substitute a low-poly prototype or generated concept image for a game capture.

## Task 1: Restricted MCP Project Probe

**Files:** Create `Tools/unreal_mcp.py`, `Tools/unreal_mcp_probe.py`, `Tools/requirements-unreal-mcp.txt`, `Tests/test_unreal_mcp.py`, `Docs/UNREAL_MCP.md`.

**Interfaces:** `ProbeConfig(engine_root: Path, project_file: Path, expected_version: str = "5.8.3")`; `select_project_node(nodes: list[dict], project_dir: Path) -> str`; `parse_probe_response(response: dict, config: ProbeConfig) -> dict`; `run_bounded_probe(config: ProbeConfig) -> dict`. The stdio server has one no-argument tool, `probe_project`; a CLI client supports `probe`, while `serve` is the server entry point. Trusted CLI paths select the engine/project; tool calls cannot pass arbitrary Python, shell commands, script paths or node overrides.

- [ ] Write unittest fixtures first: valid exact project; no/multiple matches; malformed/missing node identity; normalized path handling; wrong project/version; unsuccessful/malformed remote command; missing probe marker; child timeout/nonzero exit; no-editor failure. Keep tests independent of an installed engine or MCP package by testing the real policy/parsing boundaries and mocking only unavailable external execution.
- [ ] Run `python -m unittest discover -s Tests -p 'test_unreal_mcp.py' -v` and retain the expected failure before implementation.
- [ ] Implement exact project selection and fail-closed response parsing. Load Epic's `Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python/remote_execution.py` from the explicitly configured installation, without copying it into the repository. Read the engine's `Build.version` JSON and the configured `.uproject`, reject a version mismatch before connection, and verify project/version again in the fixed editor probe.
- [ ] Use loopback UDP bind `127.0.0.1`, group `239.0.0.1:6766`, TTL `0` and a loopback TCP callback. Discover for at most 3 seconds; never pick the first arbitrary editor. Always stop the backend in `finally`. Wrap backend execution in a child process with `shell=False`, a 45-second timeout and a 128 KiB retained-output limit; the client permits 55 seconds. If native 5.8 metadata/API differ, report failure rather than bypass identity checks.
- [ ] Use `FastMCP(...).run(transport="stdio")` and the official `ClientSession`/`stdio_client` protocol. Lazy-load the optional SDK; reserve stdout for protocol traffic in `serve`. A successful result reports connection/project/version only, with engine render/performance/content acceptance false. CLI failure must exit nonzero, not print a successful connection.
- [ ] Pin `mcp==1.28.1`. Document an isolated workspace venv and the exact project-only Python settings below. Do not modify global Codex config, firewall, other projects or their running editors. Remote Execution itself is unauthenticated local Python execution; loopback and project selection are safeguards, not authentication.

```ini
[/Script/PythonScriptPlugin.PythonScriptPluginSettings]
bRemoteExecution=True
RemoteExecutionMulticastGroupEndpoint=239.0.0.1:6766
RemoteExecutionMulticastBindAddress=127.0.0.1
RemoteExecutionMulticastTtl=0
```

- [ ] Run focused tests to green, review spec compliance and code quality, then verify an actual MCP initialize/list-tools exchange in the isolated venv without starting Unreal. A missing editor or unfinished installation must produce a truthful failed probe. Publish source/docs only after review.

## Task 2: Original Terrain Input And Layout

**Files:** Create `Art/Maps/WoodedValley768.layout.json`, `Tools/author_valley_terrain.py`, `Tests/test_valley_terrain.py`, `Docs/VALLEY_TERRAIN.md`. Generated terrain input goes outside public Git in the chat's `outputs/` or isolated private authoring workspace.

**Interfaces:** `load_layout(path: Path) -> dict`; `height_metres(layout: dict, x_m: float, y_m: float) -> float`; `encode_height(height_m: float, z_scale: float, actor_z_m: float) -> int`; `decode_height(sample: int, z_scale: float, actor_z_m: float) -> float`; `export_terrain(layout: dict, output_dir: Path) -> dict`. CLI: `python Tools/author_valley_terrain.py --layout Art/Maps/WoodedValley768.layout.json --output <new-directory>`.

- [ ] Write failing behavioral tests for invalid/nonfinite/oversized input; valid extent; UInt16 zero/positive/negative height literals and overflow rejection; deterministic terrain samples; flat, disjoint clearings; route/crossing topology; equal initial resource templates; exact R16 byte order/length; output orientation; decoded export round-trip. Tests must not compile C++, import Unreal or require a render.
- [ ] Record an editable initial blockout proposal: six centres at X `144/624` m and Y `144/384/624` m, clearing radius `60` m, route/crossing width `20` m, a fixed meandering river near X `384` m and three permanent crossings at the three row heights. Six starts connect through two bank routes and three transverse routes; endpoint starts have a bank exit and a transverse exit. These dimensions are proposals for native blockout validation, not approved balance or certified formation clearance.
- [ ] Use one explicit authored analytic height field, not random map generation: raised irregular edge mountains, a gentle central valley, carved continuous riverbed, flattened clearing cores with smooth margins and road corridors outside the bed. Reserve crossings as separate deck records; do not fill/dam the river beneath them. Record the source's current 8 m castle and 6 m farm footprints but do not invent physical unit radii.
- [ ] Default to native Landscape input `505 x 505`: `63` quads/section, `1` section/component, `8 x 8` components. Scale XY is `76800/504` cm, giving a measured `76800` cm extent. Logical 2 m cells are separate. ZScale `100`, actor Z `0`, sample `32768` at zero; encode `round(32768 + height_m * 100 * 128 / 100)` and reject out-of-range heights rather than silently clipping. Row 0 maps to Y=0; column 0 maps to X=0. Record the explicit world-to-grid transform.
- [ ] Export primary 16-bit single-channel grayscale PNG using Pillow only at export time; also export row-major little-endian UInt16 R16, with native byte-order verification still pending. Record hashes, byte sizes, min/max height, sample count, scale, layout hash and all unverified engine/visual gates in a JSON manifest. Refuse to overwrite existing files and reject invalid layouts before writing. Do not label the heightmap as an Unreal screenshot.
- [ ] Run focused tests to green plus all Python tests, verify generated PNG bit depth/dimensions and decoded R16 samples, review both spec compliance and code quality, then publish source/docs. Actual Landscape creation/import is deliberately not guessed from absent/unreflected Python APIs; inspect completed 5.8 before that step.

## Self-Review And Remaining Gates

This plan implements only preparation. It does not implement asset/wind/weather integration or native map authoring from the environment spec. Those depend on the user's completed engine installation and successful exact-project connection; the next plan must use the actual 5.8 Landscape creation API/editor workflow. The original cloud-only rule is superseded for local Unreal by the latest explicit user instruction, not by an agent workaround.

Primary references: [MCP client](https://py.sdk.modelcontextprotocol.io/v1/client/), [MCP server](https://py.sdk.modelcontextprotocol.io/v1/server/), [Landscape dimensions](https://dev.epicgames.com/documentation/en-us/unreal-engine/landscape-technical-guide-in-unreal-engine?application_version=5.6), [Heightmap formats](https://dev.epicgames.com/documentation/en-us/unreal-engine/importing-and-exporting-landscape-heightmaps-in-unreal-engine?application_version=5.6). The 5.6 documentation/source informed preparation; 5.8 compatibility remains a separate live check.
