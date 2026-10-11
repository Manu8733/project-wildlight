"""Fail-closed Unreal project identity probe; transport-independent standard library."""

import argparse
import contextlib
from dataclasses import dataclass
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import threading
import time


DISCOVERY_SECONDS = 3.0
CHILD_TIMEOUT_SECONDS = 45.0
OUTPUT_LIMIT = 128 * 1024
MARKER = "WILDLIGHT_PROJECT_PROBE="
FIXED_EDITOR_PROBE = '''import json
import unreal
project_file = unreal.Paths.convert_relative_path_to_full(unreal.Paths.get_project_file_path())
engine_version = unreal.SystemLibrary.get_engine_version()
if not project_file or not engine_version:
    raise RuntimeError("Project identity API returned an empty value")
print("WILDLIGHT_PROJECT_PROBE=" + json.dumps({"project_file": project_file, "engine_version": engine_version}))
'''


class ProbeError(RuntimeError):
    """Identity, protocol or engine-adapter validation failed."""


@dataclass(frozen=True)
class ProbeConfig:
    engine_root: Path
    project_file: Path
    expected_version: str = "5.8.3"


def _path_identity(value):
    if not isinstance(value, (str, Path)) or not str(value).strip():
        raise ProbeError("Missing or malformed project path")
    path = Path(value)
    if not path.is_absolute():
        raise ProbeError("Project identity must be an absolute path")
    return os.path.normcase(str(path.resolve()))


def _version(value):
    if not isinstance(value, str) or not re.fullmatch(r"\d+\.\d+\.\d+(?:-[^\r\n]+)?", value):
        raise ProbeError("Missing or unsupported engine version metadata")
    return value.split("-", 1)[0]


def _nonempty_string(value, label):
    if not isinstance(value, str) or not value.strip():
        raise ProbeError("Missing or malformed " + label)
    return value


def validate_config(config: ProbeConfig):
    expected = _version(config.expected_version)
    if expected != config.expected_version:
        raise ProbeError("Expected engine version must be an exact release")
    try:
        build = json.loads((config.engine_root / "Engine/Build/Build.version").read_text(encoding="utf-8-sig"))
        parts = [build[key] for key in ("MajorVersion", "MinorVersion", "PatchVersion")]
        if any(type(part) is not int or part < 0 for part in parts):
            raise ProbeError("Malformed engine Build.version")
        actual = ".".join(str(part) for part in parts)
        project = json.loads(config.project_file.read_text(encoding="utf-8-sig"))
        if config.project_file.suffix.lower() != ".uproject" or not isinstance(project, dict):
            raise ProbeError("Configured project must be a .uproject JSON object")
        if project.get("EngineAssociation") != ".".join(expected.split(".")[:2]):
            raise ProbeError("Project EngineAssociation does not match the engine lock")
        if actual != expected:
            raise ProbeError("Engine Build.version mismatch: expected " + expected + ", found " + actual)
    except (OSError, ValueError, KeyError, TypeError) as error:
        raise ProbeError("Cannot validate engine/project installation: " + str(error)) from error


def select_project_node(nodes: list[dict], project_dir: Path) -> str:
    if not isinstance(nodes, list):
        raise ProbeError("Unsupported remote_nodes metadata")
    expected = _path_identity(project_dir)
    matches = []
    node_ids = set()
    for node in nodes:
        if not isinstance(node, dict):
            raise ProbeError("Malformed editor discovery node")
        node_id = _nonempty_string(node.get("node_id"), "node_id")
        if node_id in node_ids:
            raise ProbeError("Duplicate editor discovery node_id")
        node_ids.add(node_id)
        root = _path_identity(node.get("project_root"))
        _nonempty_string(node.get("project_name"), "project_name")
        _version(node.get("engine_version"))
        if root == expected:
            matches.append(node_id)
    if len(matches) != 1:
        raise ProbeError("Expected exactly one editor for the configured project; found " + str(len(matches)))
    return matches[0]


def _verified_identity(payload, config):
    if not isinstance(payload, dict):
        raise ProbeError("Malformed project probe payload")
    if _path_identity(payload.get("project_file")) != _path_identity(config.project_file.resolve()):
        raise ProbeError("Editor returned a different project")
    if _version(payload.get("engine_version")) != config.expected_version:
        raise ProbeError("Editor engine version does not match the engine lock")
    return {"connected": True, "project_file": str(config.project_file.resolve()),
            "engine_version": config.expected_version, "engine_render_verified": False,
            "performance_verified": False, "content_accepted": False}


def parse_probe_response(response: dict, config: ProbeConfig) -> dict:
    if not isinstance(response, dict) or response.get("success") is not True:
        raise ProbeError("Remote command did not succeed")
    if response.get("command") != FIXED_EDITOR_PROBE or not isinstance(response.get("result"), str):
        raise ProbeError("Missing or mismatched native command result fields")
    output = response.get("output")
    if not isinstance(output, list):
        raise ProbeError("Malformed remote command output")
    markers = []
    for entry in output:
        if (not isinstance(entry, dict) or not isinstance(entry.get("output"), str)
                or entry.get("type") not in ("Info", "Warning", "Error")):
            raise ProbeError("Malformed remote output entry")
        for line in entry["output"].splitlines():
            if line.startswith(MARKER):
                markers.append(line[len(MARKER):])
    if len(markers) != 1:
        raise ProbeError("Expected one fixed project probe marker")
    try:
        payload = json.loads(markers[0])
    except ValueError as error:
        raise ProbeError("Malformed project probe JSON") from error
    return _verified_identity(payload, config)


def _load_backend(config):
    source = config.engine_root / "Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python/remote_execution.py"
    if not source.is_file():
        raise ProbeError("Installed Epic remote_execution.py is unavailable")
    spec = importlib.util.spec_from_file_location("wildlight_epic_remote_execution", source)
    if spec is None or spec.loader is None:
        raise ProbeError("Cannot load the installed Epic remote execution adapter")
    module = importlib.util.module_from_spec(spec)
    # Compile in memory so a read-only engine probe cannot create engine bytecode caches.
    exec(compile(source.read_bytes(), str(source), "exec"), module.__dict__)
    return module


def run_editor_probe(config: ProbeConfig) -> dict:
    validate_config(config)
    module = _load_backend(config)
    remote_config = module.RemoteExecutionConfig()
    settings = {"multicast_bind_address": "127.0.0.1",
                "multicast_group_endpoint": ("239.0.0.1", 6766),
                "multicast_ttl": 0, "command_endpoint": ("127.0.0.1", 6776)}
    for field, value in settings.items():
        if not hasattr(remote_config, field):
            raise ProbeError("Unsupported Epic remote configuration field: " + field)
        setattr(remote_config, field, value)
        if getattr(remote_config, field) != value:
            raise ProbeError("Epic remote configuration did not retain " + field)
    backend = module.RemoteExecution(remote_config)
    try:
        backend.start()
        # Observe the whole discovery window so an early reply cannot hide a second match.
        deadline = time.monotonic() + DISCOVERY_SECONDS
        while time.monotonic() < deadline:
            time.sleep(min(0.05, max(0, deadline - time.monotonic())))
        nodes = backend.remote_nodes
        node_id = select_project_node(nodes, config.project_file.parent.resolve())
        node = next(node for node in nodes if node["node_id"] == node_id)
        if node["project_name"] != config.project_file.stem:
            raise ProbeError("Discovered project_name does not match the configured project")
        if _version(node["engine_version"]) != config.expected_version:
            raise ProbeError("Discovered editor version does not match the engine lock")
        backend.open_command_connection(node_id)
        return parse_probe_response(backend.run_command(FIXED_EDITOR_PROBE, unattended=True,
                exec_mode="ExecuteFile", raise_on_failure=False), config)
    finally:
        backend.stop()


def _run_child(command, timeout=CHILD_TIMEOUT_SECONDS):
    retained = bytearray()
    with subprocess.Popen(command, shell=False, stdin=subprocess.DEVNULL,
                          stdout=subprocess.PIPE, stderr=subprocess.DEVNULL) as child:
        def drain():
            while chunk := child.stdout.read(4096):
                remaining = OUTPUT_LIMIT - len(retained)
                if remaining > 0:
                    retained.extend(chunk[:remaining])

        reader = threading.Thread(target=drain, daemon=True)
        reader.start()
        try:
            code = child.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            child.kill()
            child.wait()
            reader.join(timeout=1)
            raise
        reader.join(timeout=1)
        if reader.is_alive():
            raise ProbeError("Child output stream did not close")
    return code, bytes(retained)


def failed_probe(error):
    return {"connected": False, "error": str(error)[:2048], "engine_render_verified": False,
            "performance_verified": False, "content_accepted": False}


def run_bounded_probe(config: ProbeConfig) -> dict:
    try:
        validate_config(config)
        command = [sys.executable, str(Path(__file__).resolve()), "--child",
                   "--engine-root", str(config.engine_root.resolve()),
                   "--project-file", str(config.project_file.resolve()),
                   "--expected-version", config.expected_version]
        code, output = _run_child(command)
        if code != 0:
            try:
                failure = json.loads(output)
            except (ValueError, UnicodeError):
                failure = None
            if (isinstance(failure, dict) and failure.get("connected") is False
                    and isinstance(failure.get("error"), str) and failure["error"].strip()):
                raise ProbeError(failure["error"])
            raise ProbeError("Editor probe child failed with exit " + str(code))
        payload = json.loads(output)
        if not isinstance(payload, dict) or payload.get("connected") is not True:
            raise ProbeError("Editor probe child did not report a connection")
        return _verified_identity(payload, config)
    except subprocess.TimeoutExpired:
        return failed_probe("Editor probe timeout after 45 seconds")
    except Exception as error:
        return failed_probe(error)


def main():
    parser = argparse.ArgumentParser(description="Private bounded probe worker; use unreal_mcp.py probe")
    parser.add_argument("--child", action="store_true", required=True)
    parser.add_argument("--engine-root", required=True, type=Path)
    parser.add_argument("--project-file", required=True, type=Path)
    parser.add_argument("--expected-version", default="5.8.3")
    args = parser.parse_args()
    try:
        with contextlib.redirect_stdout(sys.stderr):
            result = run_editor_probe(ProbeConfig(args.engine_root, args.project_file, args.expected_version))
        code = 0
    except Exception as error:
        result = failed_probe(error)
        code = 1
    print(json.dumps(result))
    return code


if __name__ == "__main__":
    raise SystemExit(main())
