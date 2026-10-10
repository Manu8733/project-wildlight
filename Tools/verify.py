#!/usr/bin/env python3
"""Compile/test the actual portable core; never imply an Unreal build ran."""
from __future__ import annotations
import argparse
import ast
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import struct
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
ARTIFACTS = ROOT / ".artifacts"


def validate_art():
    path = ROOT / "Art/manifest.json"
    if not path.exists():
        raise RuntimeError("Art/manifest.json is not present")
    data = json.loads(path.read_text(encoding="utf-8"))
    assets = data["assets"]
    identifiers = set()
    for item in assets:
        name = item["id"]
        if name in identifiers or not name.replace("_", "").isalnum():
            raise RuntimeError("Duplicate or invalid asset identifier: " + name)
        identifiers.add(name)
        if item.get("mesh_name") != "SM_" + name:
            raise RuntimeError("Mesh convention mismatch: " + name)
        for field in ("fbx", "glb"):
            if not item.get(field):
                continue
            file = (ROOT / "Art" / item[field]).resolve()
            if (ROOT / "Art").resolve() not in file.parents or not file.is_file():
                raise RuntimeError("Missing/unsafe asset: " + str(file))
            if field == "glb":
                content = file.read_bytes()
                if len(content) < 12 or content[:4] != b"glTF":
                    raise RuntimeError("Invalid GLB header: " + name)
                version, length = struct.unpack_from("<II", content, 4)
                if version != 2 or length != len(content):
                    raise RuntimeError("Invalid GLB length/version: " + name)
        if int(item.get("triangles", 0)) <= 0:
            raise RuntimeError("No triangle evidence for: " + name)
        for field in ("bounds_min", "bounds_max"):
            if len(item.get(field, [])) != 3 or not all(isinstance(value, (float, int)) and math.isfinite(value) for value in item[field]):
                raise RuntimeError("Missing bounds: " + name)
        if any(lower > upper for lower, upper in zip(item["bounds_min"], item["bounds_max"])):
            raise RuntimeError("Inverted bounds: " + name)
    return {"assets": len(assets), "units": data.get("units"), "ids": sorted(identifiers)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--core-only", action="store_true")
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    ARTIFACTS.mkdir(exist_ok=True)
    report = {"format_version": 1, "compiler": os.environ.get("CXX", "c++"),
              "checks": [], "unreal_compiled": False, "windows_playtested": False,
              "gpu_performance_verified": False}
    failures = []

    def check(name, action):
        started = time.perf_counter()
        try:
            details = action()
            report["checks"].append({"name": name, "status": "passed", "seconds": time.perf_counter()-started, "details": details})
            print("PASS " + name)
        except (RuntimeError, OSError, ValueError, KeyError, SyntaxError, subprocess.SubprocessError) as error:
            report["checks"].append({"name": name, "status": "failed", "seconds": time.perf_counter()-started, "details": str(error)})
            failures.append(name)
            print("FAIL " + name + ": " + str(error))

    def run(name, command, timeout=60):
        result = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, timeout=timeout)
        log = ARTIFACTS / (name + ".log")
        log.write_text(result.stdout, encoding="utf-8")
        if result.returncode:
            raise RuntimeError(f"exit {result.returncode}; see {log.relative_to(ROOT)}\n" + result.stdout[-4000:])
        return {"command": command, "log": str(log.relative_to(ROOT)), "output": result.stdout[-3000:]}

    def syntax():
        scripts = sorted((ROOT / "Tools").glob("*.py")) + sorted((ROOT / "Art/Source").glob("*.py")) + sorted((ROOT / "Docs").glob("*.py"))
        for path in scripts:
            ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
        return {"python_files": len(scripts), "note": "Syntax only; Unreal Python execution is pending."}

    check("python_syntax", syntax)
    if (ROOT / "Wildlight.uproject").exists():
        def project():
            data = json.loads((ROOT / "Wildlight.uproject").read_text())
            lock = json.loads((ROOT / "engine-lock.json").read_text())
            if data["EngineAssociation"] != lock["association"]:
                raise RuntimeError("Engine association differs from engine lock")
            return {"target": lock["version"], "compiled": False}
        check("project_metadata", project)

    compiler = shutil.which(report["compiler"])
    if not compiler:
        failures.append("compiler")
        report["checks"].append({"name": "compiler", "status": "failed", "details": "C++ compiler not found"})
    else:
        base = [compiler, "-std=c++20", "-fno-exceptions", "-fno-rtti", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-I", str(ROOT / "Core/include")]
        base += ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"] if args.sanitize else ["-O2"]
        implementation = str(ROOT / "Core/src/Simulation.cpp")
        tests = sorted(str(path) for path in (ROOT / "Core/tests").glob("*.cpp"))
        suffix = "_sanitized" if args.sanitize else ""
        test_binary = ARTIFACTS / ("core_tests" + suffix)
        cli_binary = ARTIFACTS / ("wildlight_headless" + suffix)
        check("compile_tests", lambda: run("compile_tests" + suffix, base + [implementation, *tests, "-o", str(test_binary)]))
        if tests and test_binary.exists() and "compile_tests" not in failures:
            check("simulation_tests", lambda: run("simulation_tests" + suffix, [str(test_binary)]))
        check("compile_headless", lambda: run("compile_headless" + suffix, base + [implementation, str(ROOT / "Core/tools/headless.cpp"), "-o", str(cli_binary)]))
        if cli_binary.exists() and "compile_headless" not in failures:
            check("headless_match", lambda: run("headless_match" + suffix, [str(cli_binary), "--ticks", "600", "--seed", "101"]))
    if not args.core_only:
        check("asset_exports", validate_art)
    report["success"] = not failures
    target = ARTIFACTS / ("verification_sanitized.json" if args.sanitize else "verification.json")
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("Report: " + str(target.relative_to(ROOT)))
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
