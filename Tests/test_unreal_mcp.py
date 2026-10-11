"""Restricted project-probe boundaries; no installed editor or MCP SDK required."""

import contextlib
import importlib
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock


REPO = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO / "Tools"))
try:
    probe = importlib.import_module("unreal_mcp_probe")
except ModuleNotFoundError:
    probe = None


class ProbeTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(probe, "Restricted probe implementation is missing")
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.project = self.root / "Wildlight" / "Wildlight.uproject"
        self.project.parent.mkdir()
        self.project.write_text('{"EngineAssociation":"5.8"}', encoding="utf-8")
        self.engine = self.root / "UE_5.8"
        build = self.engine / "Engine" / "Build" / "Build.version"
        build.parent.mkdir(parents=True)
        build.write_text('{"MajorVersion":5,"MinorVersion":8,"PatchVersion":3}', encoding="utf-8")
        self.config = probe.ProbeConfig(self.engine, self.project)

    def node(self, **changes):
        value = {"node_id": "editor-a", "project_root": str(self.project.parent),
                 "project_name": "Wildlight", "engine_version": "5.8.3-58210709+++UE5+Release-5.8",
                 "engine_root": str(self.engine), "user": "test", "machine": "test-host"}
        value.update(changes)
        return value

    def response(self, **changes):
        value = {"project_file": str(self.project), "engine_version": "5.8.3-58210709+++UE5+Release-5.8"}
        value.update(changes)
        return {"success": True, "command": probe.FIXED_EDITOR_PROBE, "result": "", "output": [{"type": "Info", "output":
            "WILDLIGHT_PROJECT_PROBE=" + json.dumps(value)}]}

    def test_exact_project_selected_among_other_editors(self):
        nodes = [self.node(node_id="other", project_root=str(self.root / "Other")), self.node()]
        self.assertEqual(probe.select_project_node(nodes, self.project.parent), "editor-a")

    def test_path_normalization_preserves_exact_identity(self):
        for root in (str(self.project.parent) + "/../Wildlight/", str(self.project.parent) + "/"):
            with self.subTest(root=root):
                self.assertEqual(probe.select_project_node([self.node(project_root=root)],
                                self.project.parent), "editor-a")

    def test_missing_or_multiple_matches_fail(self):
        for nodes in ([], [self.node(), self.node(node_id="editor-b")]):
            with self.subTest(nodes=nodes), self.assertRaises(probe.ProbeError):
                probe.select_project_node(nodes, self.project.parent)

    def test_malformed_node_identity_fails_closed(self):
        for changes in ({"node_id": ""}, {"node_id": None}, {"project_root": ""},
                        {"project_root": "relative/path"}, {"project_name": ""}, {"engine_version": None}):
            with self.subTest(changes=changes), self.assertRaises(probe.ProbeError):
                probe.select_project_node([self.node(**changes)], self.project.parent)
        for nodes in ([None], "nodes", [{"node_id": "x"}]):
            with self.subTest(nodes=nodes), self.assertRaises(probe.ProbeError):
                probe.select_project_node(nodes, self.project.parent)

    def test_success_reports_only_connection_acceptance(self):
        result = probe.parse_probe_response(self.response(), self.config)
        self.assertTrue(result["connected"])
        self.assertEqual(result["engine_version"], "5.8.3")
        self.assertEqual(result["project_file"], str(self.project.resolve()))
        for key in ("engine_render_verified", "performance_verified", "content_accepted"):
            self.assertIs(result[key], False)

    def test_wrong_project_and_version_fail(self):
        for changes in ({"project_file": str(self.root / "Wrong.uproject")},
                        {"engine_version": "5.6.1"}, {"engine_version": "5.8.1"}, {"engine_version": "5.8.10"}):
            with self.subTest(changes=changes), self.assertRaises(probe.ProbeError):
                probe.parse_probe_response(self.response(**changes), self.config)

    def test_remote_command_failure_and_malformed_output_fail(self):
        responses = [None]
        for changes in ({"success": False}, {"success": 1}, {"output": "text"},
                        {"output": [{}]}, {"output": [{"type": "Info", "output": "WILDLIGHT_PROJECT_PROBE={"}]}):
            response = self.response()
            response.update(changes)
            responses.append(response)
        for response in responses:
            with self.subTest(response=response), self.assertRaises(probe.ProbeError):
                probe.parse_probe_response(response, self.config)

    def test_missing_duplicate_or_incomplete_marker_fails(self):
        for output in ([{"type": "Info", "output": "ordinary log"}],
                       [{"type": "Info", "output": "WILDLIGHT_PROJECT_PROBE={}"}],
                       self.response()["output"] * 2):
            response = self.response()
            response["output"] = output
            with self.subTest(output=output), self.assertRaises(probe.ProbeError):
                probe.parse_probe_response(response, self.config)

    def test_engine_and_project_checked_before_execution(self):
        probe.validate_config(self.config)
        with mock.patch.object(probe, "_run_child", side_effect=AssertionError("must not execute")):
            build = self.engine / "Engine" / "Build" / "Build.version"
            build.write_text('{"MajorVersion":5,"MinorVersion":6,"PatchVersion":1}', encoding="utf-8")
            self.assertFalse(probe.run_bounded_probe(self.config)["connected"])

    def test_wrong_project_association_and_malformed_build_fail(self):
        self.project.write_text('{"EngineAssociation":"5.6"}', encoding="utf-8")
        with self.assertRaises(probe.ProbeError):
            probe.validate_config(self.config)
        self.project.write_text('{"EngineAssociation":"5.8"}', encoding="utf-8")
        (self.engine / "Engine" / "Build" / "Build.version").write_text('{}', encoding="utf-8")
        with self.assertRaises(probe.ProbeError):
            probe.validate_config(self.config)

    def test_child_timeout_is_failure(self):
        with mock.patch.object(probe, "_run_child", side_effect=subprocess.TimeoutExpired("probe", 45)):
            result = probe.run_bounded_probe(self.config)
        self.assertFalse(result["connected"])
        self.assertIn("timeout", result["error"].lower())

    def test_child_nonzero_exit_and_malformed_json_are_failure(self):
        for result in ((2, b'{"connected":true}'), (0, b'not-json'), (0, b'{}'),
                       (0, b'{"connected":true,"engine_version":"5.6.1"}')):
            with self.subTest(result=result), mock.patch.object(probe, "_run_child", return_value=result):
                self.assertFalse(probe.run_bounded_probe(self.config)["connected"])

    def test_missing_engine_is_truthful_failure(self):
        result = probe.run_bounded_probe(probe.ProbeConfig(self.root / "missing", self.project))
        self.assertFalse(result["connected"])
        self.assertIn("error", result)

    def test_loading_installed_backend_does_not_write_engine_cache(self):
        source = self.engine / "Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python/remote_execution.py"
        source.parent.mkdir(parents=True)
        source.write_text("INSTALLED_ADAPTER_SENTINEL = 42\n", encoding="utf-8")
        module = probe._load_backend(self.config)
        self.assertEqual(module.INSTALLED_ADAPTER_SENTINEL, 42)
        self.assertEqual(list(source.parent.iterdir()), [source])

    def test_no_editor_stops_backend(self):
        backend = mock.Mock()
        backend.remote_nodes = []
        backend_config = SimpleNamespace(multicast_bind_address="0.0.0.0",
            multicast_group_endpoint=("239.0.0.1", 6766), multicast_ttl=1, command_endpoint=("0.0.0.0", 6776))
        module = mock.Mock()
        module.RemoteExecutionConfig.return_value = backend_config
        module.RemoteExecution.return_value = backend
        with mock.patch.object(probe, "_load_backend", return_value=module), \
                mock.patch.object(probe, "DISCOVERY_SECONDS", 0):
            with self.assertRaises(probe.ProbeError):
                probe.run_editor_probe(self.config)
        backend.stop.assert_called_once()
        backend.open_command_connection.assert_not_called()

    def test_backend_configuration_and_fixed_command(self):
        backend = mock.Mock()
        backend.remote_nodes = [self.node()]
        backend.run_command.return_value = self.response()
        config = SimpleNamespace(multicast_bind_address="0.0.0.0",
                                 multicast_group_endpoint=("239.0.0.1", 6766),
                                 multicast_ttl=1, command_endpoint=("0.0.0.0", 6776))
        module = mock.Mock()
        module.RemoteExecutionConfig.return_value = config
        module.RemoteExecution.return_value = backend
        with mock.patch.object(probe, "_load_backend", return_value=module), \
                mock.patch.object(probe, "DISCOVERY_SECONDS", 0):
            result = probe.run_editor_probe(self.config)
        self.assertTrue(result["connected"])
        self.assertEqual(config.multicast_bind_address, "127.0.0.1")
        self.assertEqual(config.multicast_group_endpoint, ("239.0.0.1", 6766))
        self.assertEqual(config.multicast_ttl, 0)
        self.assertEqual(config.command_endpoint, ("127.0.0.1", 6776))
        backend.open_command_connection.assert_called_once_with("editor-a")
        backend.run_command.assert_called_once_with(probe.FIXED_EDITOR_PROBE, unattended=True,
                exec_mode="ExecuteFile", raise_on_failure=False)
        backend.stop.assert_called_once()

    def test_missing_loopback_config_field_cannot_start_backend(self):
        module = mock.Mock()
        module.RemoteExecutionConfig.return_value = SimpleNamespace()
        with mock.patch.object(probe, "_load_backend", return_value=module), \
                self.assertRaises(probe.ProbeError):
            probe.run_editor_probe(self.config)
        module.RemoteExecution.assert_not_called()

    def test_missing_or_wrong_native_response_fields_fail_closed(self):
        for field, value in (("command", "print('other command')"), ("command", None), ("result", None)):
            response = self.response()
            response[field] = value
            with self.subTest(field=field, value=value), self.assertRaises(probe.ProbeError):
                probe.parse_probe_response(response, self.config)
        response = self.response()
        response["output"][0]["type"] = "unknown"
        with self.assertRaises(probe.ProbeError):
            probe.parse_probe_response(response, self.config)

    def test_duplicate_node_ids_fail_closed(self):
        with self.assertRaises(probe.ProbeError):
            probe.select_project_node([self.node(project_root=str(self.root / "Other")), self.node()],
                                      self.project.parent)

    def test_child_error_is_preserved_without_success(self):
        output = b'{"connected":false,"error":"No exact project editor found"}'
        with mock.patch.object(probe, "_run_child", return_value=(1, output)):
            result = probe.run_bounded_probe(self.config)
        self.assertFalse(result["connected"])
        self.assertIn("No exact project editor found", result["error"])

    def test_cli_failed_probe_exits_nonzero(self):
        server = importlib.import_module("unreal_mcp")

        async def unavailable(config):
            return {"connected": False, "error": "Editor unavailable"}

        output = io.StringIO()
        with mock.patch.object(server, "probe_via_mcp", side_effect=unavailable), contextlib.redirect_stdout(output):
            code = server.main(["probe", "--engine-root", str(self.engine), "--project-file", str(self.project)])
        self.assertEqual(code, 1)
        self.assertIs(json.loads(output.getvalue())["connected"], False)

    def test_wrong_discovery_version_or_name_cannot_execute(self):
        for changes in ({"engine_version": "5.6.1"}, {"project_name": "Wrong"}):
            backend = mock.Mock()
            backend.remote_nodes = [self.node(**changes)]
            module = mock.Mock()
            module.RemoteExecutionConfig.return_value = SimpleNamespace(multicast_bind_address="0.0.0.0",
                multicast_group_endpoint=("239.0.0.1", 6766), multicast_ttl=1, command_endpoint=("0.0.0.0", 6776))
            module.RemoteExecution.return_value = backend
            with self.subTest(changes=changes), mock.patch.object(probe, "_load_backend", return_value=module), \
                    mock.patch.object(probe, "DISCOVERY_SECONDS", 0), self.assertRaises(probe.ProbeError):
                probe.run_editor_probe(self.config)
            backend.run_command.assert_not_called()
            backend.stop.assert_called_once()

    def test_backend_stops_after_startup_failure(self):
        backend = mock.Mock()
        backend.start.side_effect = OSError("Unavailable loopback port")
        module = mock.Mock()
        module.RemoteExecutionConfig.return_value = SimpleNamespace(multicast_bind_address="127.0.0.1",
            multicast_group_endpoint=("239.0.0.1", 6766), multicast_ttl=0, command_endpoint=("127.0.0.1", 6776))
        module.RemoteExecution.return_value = backend
        with mock.patch.object(probe, "_load_backend", return_value=module), self.assertRaises(OSError):
            probe.run_editor_probe(self.config)
        backend.stop.assert_called_once()

    def test_child_success_revalidates_identity_and_cannot_grant_acceptance(self):
        payload = {"connected": True, "project_file": str(self.project), "engine_version": "5.8.3",
                   "engine_render_verified": True, "performance_verified": True, "content_accepted": True}
        with mock.patch.object(probe, "_run_child", return_value=(0, json.dumps(payload).encode())):
            result = probe.run_bounded_probe(self.config)
        self.assertTrue(result["connected"])
        for field in ("engine_render_verified", "performance_verified", "content_accepted"):
            self.assertIs(result[field], False)
        payload["project_file"] = str(self.root / "Wrong.uproject")
        with mock.patch.object(probe, "_run_child", return_value=(0, json.dumps(payload).encode())):
            self.assertFalse(probe.run_bounded_probe(self.config)["connected"])

    def test_child_retained_output_is_bounded(self):
        code, output = probe._run_child([sys.executable, "-c", "import sys;sys.stdout.write('x'*200000)"])
        self.assertEqual(code, 0)
        self.assertEqual(len(output), 128 * 1024)

    def test_child_process_timeout_kills_and_returns(self):
        with self.assertRaises(subprocess.TimeoutExpired):
            probe._run_child([sys.executable, "-c", "import time;time.sleep(10)"], timeout=0.1)


if __name__ == "__main__":
    unittest.main()
