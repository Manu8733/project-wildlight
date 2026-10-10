# Automatic Growth Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the approved automatic House/Farm level transitions, compatible saves and authoritative presentation controls.

**Architecture:** Extend the existing fixed-step Core, not frame-driven Blueprint rules. Reuse a single upgrade-price/application path; store bounded production and policy metadata in a version-2 save extension. Unreal reads authoritative state, refreshes tier meshes and submits validated policy commands.

**Tech Stack:** C++20 portable Core, existing Unreal 5.8.1 C++ adapter, Python unittest and GitHub Ubuntu workers. The existing Codex Cloud verification chat is currently unreachable.

## Global Constraints

- Approved specification: `Docs/superpowers/specs/2026-10-10-automatic-growth-design.md`.
- House/Farm levels 1-3; reserve 100 wood/50 stone; demand 80% for 100 ticks; food milestones 120/240; cooldown 150 ticks; evaluation `tick % 10 == 0`.
- The reserve applies only to automatic upgrades. Manual commands remain available and bypass automatic cooldown.
- Keep two current factions; do not implement six participants, crop cycles, tree harvesting, final art, foliage wind or Niagara in this increment.
- No local Unreal, Blender render or GPU workload. Compile and execute Core tests on cloud workers; preserve expected RED evidence separately from passing milestones.
- No new Meshy spending, subscription or publication of unverified licensed assets. The oak study is excluded from final art.
- Normal Git only; push reviewed milestones promptly; no changes to the original brief or acceptance flags.

## Shared Interfaces And File Ownership

Core implementer owns `Core/include/wildlight/Simulation.hpp`, `Core/src/Simulation.cpp` and `Core/tests/simulation_tests.cpp` including persistence tests. Adapter implementer owns `Source/Wildlight/Public/WildlightWorld.h`, `Source/Wildlight/Private/WildlightWorld.cpp`, `Source/Wildlight/Public/WildlightHUD.h`, `Source/Wildlight/Private/WildlightHUD.cpp` and focused adapter contract tests. Root owns documentation, evidence, Git and cloud verification dispatch. Never stage or revert another worker's files.

Exact public contract:

```cpp
struct AutomaticGrowthPolicy {
    bool enabled = true;
    double protected_wood = 100;
    double protected_stone = 50;
};
// FactionState additions:
AutomaticGrowthPolicy automatic_growth;
std::uint32_t housing_demand_ticks = 0;
std::uint32_t automatic_growth_cooldown_ticks = 0;
// Building additions:
double harvested_food = 0;
std::uint64_t last_damage_tick = 0;
// Append CommandType::SetAutomaticGrowth without renumbering old commands.
// Command addition:
AutomaticGrowthPolicy automatic_growth;
// Simulation public query (truthful English reason, or "Ready"):
std::string automatic_growth_status(Id building_id) const;
// Pure shared public price function:
Resources building_upgrade_cost(const Building& building);
```

## Task 1: Deterministic Growth And Save Compatibility

**Files:** Modify the three Core files above. Preserve true v1 fixtures; do not merely relabel a v2 payload as v1.

**Consumes:** Existing `issue`, `step`, `serialize`, `deserialize`, resource affordability, farming and manual upgrade behavior.

**Produces:** The exact shared public interface above and SaveVersion 2; existing commands remain compatible.

- [ ] Add a regression using only existing APIs before adding production code. Set up a completed player House with sufficient deposited resources and genuine population pressure; at tick 99 it must remain level 1, at tick 100 it must become level 2. In test-only setup state may be adjusted through the existing public const state view; never add mutable production APIs solely for tests.

```cpp
advance(sim, 99);
CHECK(sim.building(house_id)->level == 1);
advance(sim, 1);
CHECK(sim.building(house_id)->level == 2);
```

- [ ] Push only this failing test on the isolated feature branch. Root runs the existing workflow/Cloud verifier; capture the expected behavioral failure before allowing implementation. Do not compile on the laptop.
- [ ] Implement centralized validated policy commands, price/application reuse, damage and actual harvested-food accounting, fixed-tick decay/dwell and eligible lowest-ID selection after outcome resolution. Add behavioral boundary tests for each decision before its corresponding code.

```cpp
Command policy;
policy.type = CommandType::SetAutomaticGrowth;
policy.owner = Owner::Player;
policy.automatic_growth = {true, 100, 50};
CHECK(sim.issue(policy));
CHECK(sim.state().factions[0].automatic_growth.protected_wood == 100);
policy.automatic_growth.protected_wood = -1;
CHECK(reject_without_mutation(sim, policy));
```

- [ ] Add checked v2 extension write/read and explicit v1 migration. Validate faction count, policy encodings, finite reserves, zero disabled dwell, timers, exact production bounds, building ID coverage and damage tick. Keep candidate state atomic on failure.

```cpp
const auto before = sim.serialize();
CHECK(!sim.deserialize(correctly_checksummed_invalid_extension));
CHECK(sim.serialize() == before);
Simulation restored;
CHECK(restored.deserialize(before));
CHECK(restored.serialize() == before);
advance(sim, 150);
advance(restored, 150);
CHECK(restored.serialize() == sim.serialize());
```

- [ ] Exercise both factions, queued recruit pressure, disabled history/reenable, cargo exclusion, affordability tolerance, manual/automatic resets, max/damaged/unfinished buildings, absolute tick phase, no spending after terminal outcomes, deterministic IDs and split steps. Run all normal Python/Core regressions in cloud.
- [ ] Review the complete Core diff and persistence compatibility. Commit only after passing execution evidence; fix any actionable findings with covering regression tests.

## Task 2: Controls, Tier Refresh And Event Feedback

**Files:** Modify the four adapter files above; add `Tests/test_growth_adapter.py` only for static wiring checks, explicitly not an Unreal behavior test.

**Consumes:** `FactionState::automatic_growth`, `Simulation::automatic_growth_status`, `CommandType::SetAutomaticGrowth`, existing events and stable IDs.

**Produces:** Wired checkbox/numeric reserve controls, level/status readout, correct live/load tier selection, deduplicated upgrade cue.

- [ ] Write focused failing contract checks against the real adapter source for policy-command wiring, level-aware refresh and load event cursor initialization. These establish source wiring only; engine behavior stays unverified.

```python
self.assertIn('CommandType::SetAutomaticGrowth', world_source)
self.assertIn('automatic_growth_status', hud_source)
self.assertIn('SM_House_L', world_source)
self.assertIn('SM_Farm_L', world_source)
```

- [ ] Implement controls with Slate, matching the existing Canvas HUD and already-enabled Slate dependencies, accessible labels and finite numeric reserve bounds. Do not let the HUD make eligibility decisions or mutate Core state directly.
- [ ] Cache authoritative building level per actor; look for the exact tier asset and refresh on changes or direct loaded tier. Fallback to an available prototype with one diagnostic per missing tier; do not describe fallback as completed visual growth.
- [ ] Preserve transform, selection, fog and actor identity. Initialize event deduplication at save/load so events on the saved tick cannot replay upgrade cues.
- [ ] Run contract checks and all regressions on the cloud worker. Review source for engine API plausibility and preserve `ImplementedNotEngineVerified`: no compatible cloud engine exists yet.
- [ ] Commit the adapter only after source checks pass. Actual compile, interaction, tier renders and performance remain explicit release gates.

## Task 3: Integration Evidence And Delivery

**Files:** `Docs/SESSION_HANDOFF.md`, `Docs/CHANGELOG.md`, dated evidence; user-facing checkpoint summary/archive under chat outputs.

- [ ] Fetch the remote and verify no unrelated upstream commits are overwritten. Run `git diff --check` and scan changed files for credentials and licensed binaries.
- [ ] Run `python3 -m unittest discover -s Tests -p 'test_*.py'`, `python3 Tools/verify.py --core-only`, and the existing CMake/CTest suite in cloud for the exact code commit. Report actual counts, commands and workflow URL. Preserve the known LeakSanitizer limitation; no default sanitizer success may be claimed.
- [ ] Obtain an independent whole-change review covering reserve underflow, deterministic scheduling, v1/v2 persistence and presentation feedback. Resolve important findings, rerun affected tests, then merge normally into main and push.
- [ ] Record exact remote SHA and verification. Export a source-only Git archive and verify ZIP entries/CRC and original brief hash. Do not include missing assets or licensed Fab files.
- [ ] Tell the user what works in Core, what remains engine/art-unverified, and leave the Fab login tab for inventory. Show real render progress only when there is an actual new rendered artifact; no local rendering is allowed by the latest instruction.
