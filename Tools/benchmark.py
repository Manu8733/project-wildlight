#!/usr/bin/env python3
"""Measure portable simulation CPU time across map/AI/difficulty combinations.

This is a deterministic idle-player soak, not a graphics benchmark or balance
playtest. Each run has its own process/time limit and reports its actual state.
"""
import argparse
from datetime import datetime, timezone
import itertools
import json
from pathlib import Path
import platform
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, default=ROOT / ".artifacts/wildlight_headless")
    parser.add_argument("--ticks", type=int, default=3000)
    parser.add_argument("--output", type=Path, default=ROOT / ".artifacts/soak.json")
    args = parser.parse_args()
    if not 1 <= args.ticks <= 360000:
        parser.error("--ticks must be in [1, 360000]")
    records = []
    for map_index, ai, difficulty in itertools.product(range(3), ("builder", "conqueror", "arcanist"), ("easy", "normal", "hard")):
        command = [str(args.binary.resolve()), "--map", str(map_index), "--ai", ai, "--difficulty", difficulty, "--seed", "57118", "--ticks", str(args.ticks)]
        start = time.perf_counter()
        try:
            result = subprocess.run(command, capture_output=True, text=True, timeout=60, cwd=ROOT)
            state = json.loads(result.stdout) if result.returncode == 0 else None
            records.append({"map": map_index, "ai": ai, "difficulty": difficulty,
                            "command": command, "exit_code": result.returncode,
                            "wall_seconds": time.perf_counter() - start,
                            "state": state, "stderr": result.stderr[-2000:]})
        except (subprocess.TimeoutExpired, json.JSONDecodeError) as error:
            records.append({"map": map_index, "ai": ai, "difficulty": difficulty,
                            "command": command, "exit_code": -1, "error": str(error)})
        print(f"{len(records)}/27 map={map_index} ai={ai} difficulty={difficulty}: {'PASS' if records[-1]['exit_code'] == 0 else 'FAIL'}", flush=True)
    report = {"timestamp_utc": datetime.now(timezone.utc).isoformat(), "platform": platform.platform(),
              "requested_ticks_per_case": args.ticks, "simulation_step_seconds": .1,
              "scope": "Portable simulation only; player takes no orders; AI acts. Not human playtesting or verified balance.",
              "unreal_compiled": False, "graphics_measured": False, "records": records,
              "success": all(item["exit_code"] == 0 for item in records)}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("Report: " + str(args.output))
    return 0 if report["success"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
