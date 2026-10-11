# Restricted Unreal Project Probe

This preparation exposes one no-argument MCP tool, `probe_project`. It checks an
already-running editor's exact configured project and Unreal 5.8.3 release. The
5.8.3 default follows the user's approved installed-release migration; the
earlier task brief's 5.8.1 expectation is superseded. It
never starts Unreal or accepts caller-supplied Python, shell commands, script
paths or editor-node overrides. Render, performance and content acceptance remain
false even after a connection succeeds.

## Isolated Setup

From the workspace checkout, create a private workspace venv outside tracked
source. The optional official SDK is pinned to `mcp==1.28.1`; policy tests use
only the Python standard library.

```powershell
python -m venv ..\unreal-mcp-venv
& ..\unreal-mcp-venv\Scripts\python.exe -m pip install -r Tools\requirements-unreal-mcp.txt
python -m unittest discover -s Tests -p 'test_unreal_mcp.py' -v
```

Verify the installed engine's `Engine/Build/Build.version` is 5.8.3 and enable
Python Editor Script Plugin for the exact authorized authoring project through
its project settings. Its `.uproject` must have `EngineAssociation: "5.8"`.
The probe validates both JSON files before discovery. Do not change another project's settings
or global Codex MCP configuration. Add these settings to this project's
`Config/DefaultEngine.ini` only when preparing its authorized editor session:

```ini
[/Script/PythonScriptPlugin.PythonScriptPluginSettings]
bRemoteExecution=True
RemoteExecutionMulticastGroupEndpoint=239.0.0.1:6766
RemoteExecutionMulticastBindAddress=127.0.0.1
RemoteExecutionMulticastTtl=0
```

The probe never applies these settings or enables plugins itself. Opening an
editor and preparing a project are separate authorized operations. Do not change
firewall, engine files, global settings or other running editors.

## Protocol And CLI

Supply trusted absolute installation/project paths on the CLI. Replace the sample
paths with the verified installation and this checkout's `Wildlight.uproject`:

```powershell
& ..\unreal-mcp-venv\Scripts\python.exe Tools\unreal_mcp.py probe --engine-root 'C:\Program Files\Epic Games\UE_5.8' --project-file 'C:\Projects\Wildlight\Wildlight.uproject'
& ..\unreal-mcp-venv\Scripts\python.exe Tools\unreal_mcp.py serve --engine-root 'C:\Program Files\Epic Games\UE_5.8' --project-file 'C:\Projects\Wildlight\Wildlight.uproject'
```

`probe` uses the official `ClientSession` and `stdio_client`, initializes the
server, verifies its sole no-argument tool and invokes it. A failed connection
prints `connected: false` and exits nonzero. `serve` runs `FastMCP` with stdio
transport; stdout is reserved for protocol traffic. SDK imports are lazy.

The worker loads Epic's installed
`Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python/remote_execution.py`
in place, compiling source bytes in memory without writing engine bytecode
caches. No engine source is copied into Git. Discovery lasts at most three
seconds. Node metadata must provide `node_id`, an absolute `project_root`,
`project_name` and `engine_version`; node IDs must be unique and exactly one
project directory must match.
The node's project name/release are checked before connecting. A fixed read-only
command queries `unreal.Paths.get_project_file_path()`, converts it to an absolute
path and queries `unreal.SystemLibrary.get_engine_version()`; its unique JSON
marker must independently match the configured `.uproject` and release. The
native reply must echo that fixed command and provide a string result and valid
Info/Warning/Error output entries.

The UDP bind is `127.0.0.1`, multicast group `239.0.0.1:6766`, TTL zero, and TCP
callback `127.0.0.1:6776`. Port conflicts fail; the probe does not close other
editors or pick an arbitrary fallback. The backend always stops in `finally`.
External execution uses `shell=False`, a 45-second child limit and at most 128
KiB retained stdout; the protocol client permits 55 seconds. Failed subprocesses,
missing editor, missing module, ambiguous discovery, malformed responses and
version mismatches cannot become a successful result.

## Verified Transport And Remaining Runtime Gate

The installed UE 5.8.3 `remote_execution.py` and native protocol declarations were
read on 11 October 2026. The backend uses `multicast_bind_address`,
`multicast_group_endpoint`, `multicast_ttl` and `command_endpoint`; the probe
requires those fields before creating a backend. Native protocol declarations
confirm the project/version metadata and command-result fields. The reflected
Paths and SystemLibrary declarations support the fixed query, but actual editor
execution remains a separate runtime gate. Unsupported metadata or API
differences fail closed; do not remove identity checks to obtain a connection.
The isolated `mcp==1.28.1` venv completed initialize/list-tools/call-tool with
protocol `2025-11-25`, the sole empty-argument `probe_project` tool, and a truthful
`connected: false` result for a temporary project without an editor. The CLI
returned exit 1 for that fixture. The SDK emitted a nonfatal pydantic-settings
lifespan warning to stderr; stdout remained protocol traffic. See the task report
for focused test and verification details.
An MCP initialize/list-tools exchange alone is transport verification, not an
Unreal connection, compilation, Landscape import or gameplay/visual acceptance.

Epic Remote Execution is unauthenticated local Python execution. Loopback binding
and exact project selection are safeguards, not authentication. Enable it only
for the intended trusted local session and turn it off when that session ends.

Official protocol references: [MCP Python client](https://py.sdk.modelcontextprotocol.io/v1/client/)
and [MCP Python server](https://py.sdk.modelcontextprotocol.io/v1/server/).
