# Build and verification

Run commands from the repository root. Build directories and logs are generated evidence; they do not belong in source commits. The authoritative latest results belong in `PROGRESS.md` and the verification report, not in assumptions in this guide.

## Cloud-compatible simulation

Requirements: a C++20 compiler and Python 3. The actual cloud toolchain is Linux with g++ 14; Unreal is absent. The canonical verifier invokes the compiler directly; it does not depend on CMake.

```bash
python3 Tools/verify.py
python3 Tools/verify.py --core-only
python3 Tools/verify.py --core-only --sanitize
```

The verifier builds `.artifacts/core_tests` and `.artifacts/wildlight_headless`, runs the assertions and a 600-tick seeded simulation, checks Python syntax, engine-lock metadata and—unless `--core-only`—the generated asset manifest/containers. It writes `.artifacts/verification.json` or `.artifacts/verification_sanitized.json` plus logs. A passing core test proves only its stated assertions. It does not prove Unreal compilation, rendering, imported collision, mouse hit-testing, packaged startup or laptop performance.

Normal compilation uses `-std=c++20 -fno-exceptions -fno-rtti -Wall -Wextra -Wpedantic -Werror -O2`. The sanitizer option uses AddressSanitizer and UndefinedBehaviorSanitizer with debug information and frame pointers. Capture exact flags, exit code and any finding in the QA report. Do not compare sanitized timings against release performance targets. `CXX` may name an installed compatible compiler.

An optional CMake route exists for Linux and portable Windows/MSVC logic checks:

```bash
cmake -S Core -B .artifacts/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .artifacts/cmake --parallel 2
ctest --test-dir .artifacts/cmake --output-on-failure
```

On a Windows multi-configuration generator, pass `--config Release` to the build and `-C Release` to CTest. For GCC/Clang memory checks, configure a separate tree with `-DWILDLIGHT_SANITIZE=ON`. This CMake sanitizer option explicitly rejects MSVC. Portable Windows compilation is separate from Unreal Editor/Shipping compilation.

For the 27-case idle-player simulation matrix after a verified build:

```bash
python3 Tools/benchmark.py --ticks 12000
```

This requests up to 20 simulated minutes per map/personality/difficulty combination, but each run stops when a match ends. Record actual ticks/duration from the output. The player issues no commands, so it is an AI activity/timing check, not a balanced two-player match or long GPU session.

## Windows Unreal

The build wrapper checks the installed engine's `Build.version` against `engine-lock.json`. A different version is a migration requiring separate evidence.

```powershell
./Tools/Build-Windows.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8' -Stage Verify
./Tools/Build-Windows.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8' -Stage Editor
./Tools/Build-Windows.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8' -Stage Import
```

`Verify` checks paths/toolchain only. `Editor` compiles the C++ editor target. `Import` compiles, runs `Tools/Import-Assets.py` in Unreal Python, and creates `/Game/Wildlight/Maps/Valley`. This has not run in this cloud. The engine's built-in Entry map is a bootstrap fallback; it is not a authored final map. Open Valley and use Play-in-Editor to exercise the runtime presentation.

```powershell
./Tools/Build-Windows.ps1 -EngineRoot 'C:\Program Files\Epic Games\UE_5.8' -Stage Shipping
```

Shipping uses Unreal Automation Tool `BuildCookRun` and archives under `Builds/Windows`. Confirm imported assets and the required map are included in cooking before calling a package complete. Test the actual archived executable in a clean Windows environment. No Windows Shipping binary is supplied by cloud-only C++ tests.

## Art reproduction

Use the generator and exact commands in `Art/ART_BIBLE.md` and `Art/ASSET_REVIEW.md`. Blender 4.3.2 is available in the cloud for CPU operations. Re-export and compare `Art/manifest.json` and export reports. FBX is imported as static meshes in the initial Unreal script. Unit skeletal import and final animations require an additional verified path; static unit meshes do not complete rigging.

## Debugging and evidence

For a bug record: commit, map/seed, selected AI/difficulty, exact steps, expected/actual outcome, stable entity IDs when useful, save before the failure, log and screenshot/trace. Use the smallest reproducible scene. Check core command rejection text before debugging a view symptom. Compare simulation state before and after a failed load to detect unintended mutation.

For Unreal crash/compile failure, retain `Saved/Logs` and crash context outside Git; attach sanitized excerpts to an issue. For imported geometry problems, capture Blender dimensions, FBX scale and Unreal mesh bounds together. Redact credentials and personal paths from shared diagnostic output.

Version baseline references:

- [Unreal Engine 5.8 release](https://www.unrealengine.com/news/unreal-engine-5-8-is-now-available)
- [Epic release announcements](https://forums.unrealengine.com/tag/released/11396)

The project lock targets 5.8.1. Engine installation and toolchain compatibility must still be checked on the user's actual machine.
