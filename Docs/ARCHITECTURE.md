# Architecture and implementation standards

The portable `Core/` simulation owns game rules. Unreal owns input, presentation, import and platform integration. This separation allows Linux verification of logic while making the missing Windows/Unreal checks explicit.

## Command and state flow

Input → player-controller intent → validated `wildlight::Command` → `Simulation::issue` → authoritative state → presentation/event update. AI submits equivalent commands. Presentation never subtracts resources, grants damage or decides ownership independently.

`State` owns buildings, units, resource nodes, effects, projectiles, faction state, visibility grids and scenario information. Each entity uses a nonzero `uint32_t` `Id`; `next_id` advances monotonically for a match. A UI selection stores IDs and resolves them against current state, dropping entities removed since selection. Arrays or actor pointers are not save-file identifiers.

`Simulation::step` advances a 0.1-second fixed step using an accumulator. Keep rendering interpolation separate. Validate negative/nonfinite delta times and cap real-time catch-up explicitly where integrated. Seeded randomness supports repeatable regression scenarios; deterministic claims are limited to the measured toolchain and order of operations until cross-platform tests exist.

The Unreal adapter uses **200 cm per simulation grid cell**. Simulation coordinates refer to cell centers. Building dimensions and movement speed remain grid values; translate through one shared scale constant. Blender asset export uses metric dimensions and documented import scale. Do not silently scale gameplay differently from its imported collision.

## Module responsibilities

| Layer | Owns | Verification |
| --- | --- | --- |
| Core catalogs | Costs, footprints, unit/spell rules, map definitions | Catalog validation and boundary tests |
| Core simulation | Economy, commands, paths, combat, AI, network, outcomes | Headless tests, seeded runs, sanitizer checks |
| Core persistence | Versioned state serialization, checksum, validation | Round-trip, corruption, unsupported-version and atomic-failure tests |
| Unreal game mode | Match boot and lifetime | Editor startup and packaged startup |
| Unreal controller/camera | Cursor hit tests, selection, command intent, strategic view | Manual laptop control matrix |
| Unreal presentation | Meshes, material state, effects, English HUD | Import, visual clarity and frame-time profiling |
| Tools | Repeatable export/import, build, reports, backup | Cloud script checks and Windows execution |

## C++ conventions

Use C++20 for portable code unless toolchain compatibility requires a documented fallback. Core types use `wildlight` namespace and explicit enum classes. Use `.hpp` declarations, `.cpp` implementations, descriptive identifiers and bounded collections. Include invariants next to the state they protect. Handle invalid input through a rejected result with a useful reason; do not mutate half a command before returning failure.

Unreal names follow engine conventions: `A` actor, `U` object, `F` struct, `E` enum, `b` boolean. Keep reflection-visible fields and lifetime semantics appropriate to engine ownership. Do not use a raw UObject pointer as a long-lived ownership mechanism; prefer engine-managed references. Enable exceptions only where required and isolate portable code's error policy from Unreal build settings.

Treat core definitions as the current balance source. A future Data Asset import/export path must be versioned and validated against the same schema; parallel unsynchronized values in C++ and Blueprint are prohibited.

## Blueprint and content conventions

Native C++ handles authoritative systems and frequently updated logic. Blueprint may assemble view components, route animation events, choose material variants and author scenarios. A Blueprint must not bypass command validation or mutate resources directly. Data Assets hold content definitions after a tested loading path exists; that path is planned, not claimed from folder naming alone.

| Prefix | Use | Example |
| --- | --- | --- |
| `BP_` | Blueprint presentation actor | `BP_BeaconView` |
| `DA_` | Validated content Data Asset | `DA_Scenario_Fernwake` |
| `SM_` | Static mesh | `SM_Beacon` |
| `SK_` | Skeletal mesh | `SK_Wizard` |
| `M_`, `MI_` | Master material, instance | `M_Kingdom`, `MI_Kingdom_Player` |
| `T_` | Texture | `T_Kingdom_BaseColor` |
| `WBP_` | UMG widget if introduced | `WBP_ResourceBar` |
| `NS_` | Niagara system | `NS_BeaconPulse` |
| `S_`, `SC_` | Sound wave, cue | `S_NetworkBreak` |
| `AN_`, `ABP_` | Animation, animation Blueprint | `AN_Worker_Carry` |

Folders separate `Core`, `Gameplay`, `Presentation`, `Input`, `UI`, `Art/Buildings`, `Art/Units`, `Art/Environment`, `VFX`, `Audio` and `Scenarios`. Runtime imported static meshes use `/Game/Wildlight/Meshes/SM_<CatalogIdentifier>`. One asset's mesh name should correspond to one explicit catalog identifier, not a translated display name.

## Save compatibility and failure model

Current portable schema is version 1. Its serialized payload must include map/state seed, next ID, time, orders, cargo, queues, cooldowns, resources, technologies and active effects. A checksum detects damaged bytes; it is not cryptographic authentication. Deserialization constructs a candidate state, validates all constraints, then replaces live state. Rejecting a bad save must leave the current match unchanged.

Schema migration is **not implemented simply by a version field**. Unsupported versions are rejected with an explanation until a migration is deliberately authored and tested. The platform adapter should save to the user profile through a temporary file and atomic rename when supported, retaining one prior known-good autosave. Test actual platform autosave separately from core round-trip tests.

## Future multiplayer plan

See `MULTIPLAYER_PLAN.md`. No network transport, lobby, matchmaking or multiplayer build is part of the current work. Keep authoritative state independent of camera and use serializable validated commands so this boundary remains useful later.
