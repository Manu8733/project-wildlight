"""Exercise verifier control flow without compiling or touching checkout evidence."""

import contextlib
import importlib.util
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
SPEC = importlib.util.spec_from_file_location("wildlight_verify", REPO / "Tools" / "verify.py")
VERIFY = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(VERIFY)
CORE_CHECKS = ["python_syntax", "project_metadata", "compile_tests", "simulation_tests",
               "compile_headless", "headless_match"]


class VerifyTests(unittest.TestCase):
    def run_verifier(self, platform, sanitize=False, missing=(), compile_failure=None):
        suffix = "_sanitized" if sanitize else ""
        extension = ".exe" if platform == "nt" else ""
        calls = []
        with tempfile.TemporaryDirectory(prefix="verify-fixture-", dir=REPO.parent) as temporary:
            root = Path(temporary)
            fixtures = {
                "Tools/example.py": "value = 1\n",
                "Core/src/Simulation.cpp": "// Mock compiler fixture.\n",
                "Core/tests/simulation_tests.cpp": "// Mock compiler fixture.\n",
                "Core/tools/headless.cpp": "// Mock compiler fixture.\n",
                "Wildlight.uproject": json.dumps({"EngineAssociation": "5.8"}),
                "engine-lock.json": json.dumps({"association": "5.8", "version": "5.8.1"}),
            }
            for name, content in fixtures.items():
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(content, encoding="utf-8")
            artifacts = root / ".artifacts"

            def fake_run(command, **kwargs):
                calls.append(list(command))
                self.assertEqual(kwargs["cwd"], root)
                if "-o" in command:
                    binary = Path(command[command.index("-o") + 1])
                    stem = binary.name.removesuffix(".exe").removesuffix("_sanitized")
                    if stem == compile_failure:
                        return subprocess.CompletedProcess(command, 1, "mock compile failure")
                    if stem not in missing:
                        # MinGW appends .exe when an extensionless output is requested.
                        if platform == "nt" and binary.suffix != ".exe":
                            binary = binary.with_name(binary.name + ".exe")
                        binary.write_bytes(b"mock executable; never run")
                elif not Path(command[0]).is_file():
                    raise FileNotFoundError(command[0])
                return subprocess.CompletedProcess(command, 0, "mock successful output")

            argv = ["verify.py", "--core-only"] + (["--sanitize"] if sanitize else [])
            output = io.StringIO()
            with mock.patch.object(VERIFY, "ROOT", root), \
                    mock.patch.object(VERIFY, "ARTIFACTS", artifacts), \
                    mock.patch.object(VERIFY, "os", SimpleNamespace(name=platform, environ={"CXX": "mock-c++"})), \
                    mock.patch.object(VERIFY.shutil, "which", return_value="mock-c++"), \
                    mock.patch.object(VERIFY.subprocess, "run", side_effect=fake_run), \
                    mock.patch.object(sys, "argv", argv), \
                    contextlib.redirect_stdout(output):
                result = VERIFY.main()
            target = artifacts / ("verification_sanitized.json" if sanitize else "verification.json")
            report = json.loads(target.read_text(encoding="utf-8"))
            return result, report, calls, output.getvalue(), suffix, extension

    def assert_complete_core_run(self, platform, sanitize=False):
        result, report, calls, output, suffix, extension = self.run_verifier(platform, sanitize)
        self.assertEqual(result, 0, output)
        self.assertEqual([check["name"] for check in report["checks"]], CORE_CHECKS)
        self.assertTrue(report["success"])
        self.assertTrue(all(check["status"] == "passed" for check in report["checks"]))
        self.assertEqual(len(calls), 4, "Compile and execute both Core programs")
        self.assertEqual(Path(calls[0][-1]).name, "core_tests" + suffix + extension)
        self.assertEqual(Path(calls[1][0]).name, "core_tests" + suffix + extension)
        self.assertEqual(Path(calls[2][-1]).name, "wildlight_headless" + suffix + extension)
        self.assertEqual(Path(calls[3][0]).name, "wildlight_headless" + suffix + extension)
        self.assertEqual(calls[3][1:], ["--ticks", "600", "--seed", "101"])
        if sanitize:
            self.assertIn("-fsanitize=address,undefined", calls[0])
            self.assertIn("-fsanitize=address,undefined", calls[2])
        runtime = {check["name"]: check for check in report["checks"]}
        self.assertEqual(Path(runtime["simulation_tests"]["details"]["log"]).as_posix(),
                         ".artifacts/simulation_tests" + suffix + ".log")
        self.assertEqual(Path(runtime["headless_match"]["details"]["log"]).as_posix(),
                         ".artifacts/headless_match" + suffix + ".log")
        self.assertNotIn("asset_exports", runtime)
        self.assertFalse(report["unreal_compiled"])
        self.assertFalse(report["windows_playtested"])
        self.assertFalse(report["gpu_performance_verified"])

    def test_windows_core_only_compiles_and_executes_all_six_checks(self):
        self.assert_complete_core_run("nt")

    def test_linux_core_only_compiles_and_executes_all_six_checks(self):
        self.assert_complete_core_run("posix")

    def test_sanitized_binary_and_log_suffixes_are_preserved_on_both_platforms(self):
        for platform in ("nt", "posix"):
            with self.subTest(platform=platform):
                self.assert_complete_core_run(platform, sanitize=True)

    def test_missing_required_binaries_after_successful_compile_fail_closed(self):
        for platform in ("nt", "posix"):
            for sanitize in (False, True):
                for binary, check_name in (("core_tests", "simulation_tests"),
                                           ("wildlight_headless", "headless_match")):
                    with self.subTest(platform=platform, sanitize=sanitize, missing=binary):
                        result, report, _, output, _, _ = self.run_verifier(
                            platform, sanitize, missing=(binary,))
                        self.assertEqual(result, 1, output)
                        self.assertFalse(report["success"])
                        self.assertEqual([check["name"] for check in report["checks"]], CORE_CHECKS)
                        checks = {check["name"]: check for check in report["checks"]}
                        self.assertEqual(checks[check_name]["status"], "failed")
                        self.assertIn("executable", checks[check_name]["details"].lower())
                        compile_name = "compile_tests" if binary == "core_tests" else "compile_headless"
                        self.assertEqual(checks[compile_name]["status"], "passed")

    def test_failed_compilation_cannot_be_reported_as_success(self):
        for platform in ("nt", "posix"):
            with self.subTest(platform=platform):
                result, report, calls, output, _, _ = self.run_verifier(
                    platform, compile_failure="core_tests")
                self.assertEqual(result, 1, output)
                self.assertFalse(report["success"])
                checks = {check["name"]: check for check in report["checks"]}
                self.assertEqual(checks["compile_tests"]["status"], "failed")
                self.assertNotIn("simulation_tests", checks)
                self.assertIn("headless_match", checks)
                self.assertEqual(checks["headless_match"]["status"], "passed")
                self.assertEqual(len(calls), 3)


if __name__ == "__main__":
    unittest.main(verbosity=2)
