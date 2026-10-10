# Verification scope

`Core/tests/simulation_tests.cpp` exercises the engine-independent C++20 simulation through its public command, query, step and persistence interfaces. It contains 28 independent scenarios covering:

- Stable entity IDs, seeded determinism, fixed-step partitioning and invalid elapsed time.
- Atomic rejection of mixed ownership, stale IDs, invalid enums, NaN/infinite/huge coordinates, unseen hostile targets and illegal spells.
- Upfront building/recruitment costs, progressive construction, cancellation refunds and reserved population.
- Movement, duplicate selections, permanent-ford paths, physical worker cargo/deposit delivery and non-negative resource bounds.
- Light extension/disconnection, enemy sabotage/restoration, root detours and temporary bridge routes with safe expiry. Overlapping roots always block bridge routes, regardless of casting order.
- Illusions and population, Veil concealment and nearby scout reveal, real combat damage and friendly-fire rejection.
- Research, economic upgrades and market trade.
- Save roundtrip, deterministic continuation, truncated/oversized/checksum-invalid files and correctly checksummed invalid state fields. Active research of an already-maxed technology is atomically rejected for either faction; valid version-1 research states remain compatible.
- Three AI personalities, an AI skirmish ending by castle defeat and a player victory through connected lantern objectives.

The checksummed corruption fixtures intentionally target schema version 1 at the file boundary. A save-format migration must update those fixtures and add compatibility tests rather than silently removing them.

## Run

From the repository root:

```sh
python3 -m unittest discover -s Tests -p 'test_*.py'
python3 Tools/verify.py --core-only
```

The Python suite checks verification-run execution and progress-ledger reporting in isolated temporary fixtures. Drop `--core-only` only when the asset exports have actually been restored: 200 original binary assets are currently missing, so a full art check is expected to fail rather than certify absent content.

For a standalone strict GCC build:

```sh
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror \
  -fno-exceptions -fno-rtti -ICore/include \
  Core/src/Simulation.cpp Core/tests/simulation_tests.cpp \
  -o /tmp/wildlight-simulation-tests
/tmp/wildlight-simulation-tests
```

The core is portable; these Linux commands do not establish a successful Windows/MSVC build. Use the core CMake project for another toolchain and record the actual result.

## Evidence

The test executable prints each scenario result and the total assertions evaluated. Counts are a summary of that run, not a measure of overall game completeness. The repository's final verification process copies reports to `Tests/Verification.json`, `Tests/VerificationSanitized.json` and `Tests/Soak.json` when those checks have actually run. Use the recorded commands, timestamps, toolchain and output in the reports as evidence; a report that is absent has no implied pass.

Portable CLI timings measure simulation CPU work for the recorded map, seed, tick count, AI and difficulty. An idle-player match can end early, so use the **actual completed tick count** and resulting population when describing a run. They do not measure the Unreal adapter, rendering, VRAM, laptop performance or FPS, and are not a 100/200/500-unit graphics stress test.

## Remaining acceptance

Unreal compilation, editor launch, asset/material import, controls, HUD interaction, animation, Niagara, accessibility, Windows packaging, GPU profiling and independent playtests require their own evidence. C++ source or a passing portable simulation suite does not certify those tasks. Steam work remains deferred.
