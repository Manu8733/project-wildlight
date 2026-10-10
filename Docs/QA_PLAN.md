# QA and acceptance matrix

No requirement is user-accepted automatically. Store each result with commit, machine, engine/compiler version, exact test steps, actual outcome, evidence path and defect ID. A skipped or hardware-blocked check is neither a pass nor a failure. `Requirements.json` records coverage for each of the 105 original tasks.

## Portable simulation checks

| Area | Required behavior and boundaries |
| --- | --- |
| Resource conservation | Accepted build/train/research/trade costs are paid once; rejected orders leave stock unchanged; capacity and shortage remain finite/nonnegative |
| Ownership and IDs | Enemy/neutral/stale IDs cannot be commanded by player; deleted selection/target IDs are handled; IDs remain unique through load |
| Placement | Bounds, rotated footprints, occupied cells, water and required light are validated; invalid/nonfinite coordinates are rejected |
| Construction | Progress completes once; interruption, repair, upgrade, demolition and cancellation follow documented cost/refund rules |
| Logistics | Worker gathers, carries and returns cargo; exhausted source and lost/full depot recover without duplication or a permanent deadlock |
| Navigation | Obstacles and friendly/enemy gate rules; legal bridges; roots; bridge expiration while moving; no forbidden diagonal corner cut |
| Combat | Armor/damage/cooldown; projectile target destroyed mid-flight; siege scaling; stationary defense; no damage from an invalid owner |
| Light | Connected chain, disconnected island, cyclic graph, both owners, sabotage duration, reconnect and no double production gain |
| Spells | Caster, mana, range, terrain, cooldown and ownership; expiry; failed cast spends nothing; illusion does not grant population/resources |
| AI | Pays fair costs; builds economy and recruits; variations measurable; cannot issue invalid hidden-information attacks |
| Outcome | Destroyed rival command center; surrender; scenario objective threshold; no new gameplay mutation after match end |
| Saves | Round-trip every supported field; unsupported version; truncation/checksum damage; oversized payload; invalid enum/ID/order; rejected load preserves live state |

Coverage must be demonstrated by tests, not assumed because an API exists. For source mutation bugs, add a small meaningful regression. Use sanitizer or equivalent memory diagnostics when supported. Archive logs of seeded headless runs; report outcomes even when the simulation ends in a stalemate.

## Unreal interaction checks

| Test | Pass criterion |
| --- | --- |
| Clean editor bootstrap | Native targets build; Valley loads; no missing-class/mesh dialogs; match starts from intended scenario |
| Strategic camera | Pan/zoom/rotation/edge scroll obey limits; camera stays usable after pause and UI interaction |
| Selection | Click/box/Shift/group recall; no selection through HUD; dead units removed; indicators match actual selected IDs |
| Context orders | Right-click ground/enemy/resource yields intended command and valid rejection feedback |
| Building preview | Footprint and orientation match actual placed mesh; valid/invalid state is readable; no resource cost before confirmation |
| Menus | Start scenario/personality/difficulty; pause blocks gameplay; settings persist; restart/surrender/victory/defeat behave consistently |
| HUD and minimap | Values match simulation; minimap clicks map to correct world position; tooltips fit viewport; warnings are actionable |
| Remapping | All required actions can be rebound; conflict/duplicate key handling; reset; save/load of bindings |
| Tutorial | Each step tests actual player state; repeat/retry works; no skipped progression due to unrelated AI activity |
| Save/autosave | F5/F9 and platform autosave perform actual file I/O; interruption retains previous good save; failed load shows useful message |

The initial native Canvas HUD and controller source require this entire check on an actual engine installation. Configurable key declarations alone do not prove a complete remapping interface.

## Content acceptance

For all 19 buildings and 12 unit definitions: source/provenance, matching catalog ID, scale and pivot, collision, correct owner cue, selection, health/state feedback, role statistics and save/load. For animated units: rig, skinning, ground contact, walk/attack/work/cast/death transitions and animation events synchronized to simulation. For every spell: valid/invalid cast, gameplay effect, clear footprint, expiry warning, counterplay and audio/VFX.

Three prototype scenario grids must not be counted as polished maps. Final maps require authored traversal, balanced start access, legal crossing anchors, readable objectives, map-specific AI/path regression and demonstrated full match completion. Final lighting, UI, VFX and audio are accepted in the integrated scene.

## Performance matrix

On each actual target machine, run the same commit, map, seed and scripted command sequence. Begin with 100 units; expand to 200 and 500 only when spawning, pathing and entity limits support those scenes. Count workers/combat/siege/buildings separately. If 500 is unsupported or unusable, record the limit and defect; do not fake the result by rendering static copies.

| Scene | Duration | Required evidence |
| --- | --- | --- |
| Opening village | 5 min steady-state | Frame-time trace, RAM/VRAM, shader warm-up separated |
| 100 units + developed economy | 10 min | CPU/game/render/GPU phase times; command responsiveness |
| 200-unit gate/crossing | 10 min if supported | Path/replan spikes, no stuck groups, frame percentiles |
| 500-unit stress | 10 min if supported | Actual active units, memory, failure threshold and trace |
| Multiple bridge/root expirations | 2 min repeatable burst | Path recovery and simulation/graphics spike |
| Full economy and siege | One completed match | Match duration, result, resource/network behavior |
| Long session | 60 min minimum | Memory samples every 5 min, autosaves, no crash or accumulating growth |
| Soak | 120 min where feasible | Same samples; source of any growth or leak diagnosed |

Portable timings, static Blender renders and strong-GPU results cannot stand in for the laptop regression. Compare targets to `PERFORMANCE_BUDGETS.md`; report unmet targets plainly.

## Accessibility and resolution

Check 1280×720, 1920×1080 and 2560×1440 where a suitable display is available. Test 100%, 125% and 150% UI scale, fullscreen/windowed transitions and long English tooltips. Inspect army/selection/network information in monochrome and common color-vision simulations. Test reduced particles, disabled flashes/shake and sound-muted play. Confirm gameplay indicators survive each reduced-effects option.

## Packaging and clean installation

Build Windows Shipping, inspect cooked content, verify startup from a clean location and user account, and play a complete saved/reloaded match. Check no developer engine installation is required, packaged resources are present, user saves/settings are writable, and quit leaves no orphan process. Record binary hashes and exact build log. Repeat Low laptop regression against that exact package before a release-candidate claim.

## Independent playtest

Recruit at least five players who did not implement the project, with consent and no personal data beyond necessary feedback. Give a brief objective and observe without explaining every action. Record whether they understand light connections, find the bridge, recover from a broken link, finish onboarding, and can explain their loss/victory. Ask: what was confusing, what decision mattered, what felt satisfying, what would you change? Do not extrapolate sales from five testers.

Exit target: at least four of five independently explain the light-network consequence, no blocker bug, and at least three report one meaningful economy-versus-tactics decision. These targets are provisional acceptance criteria; no playtest has yet established them.

## Defect severity and release gates

Blocker: crash, corrupt accepted save, unfinishable match/tutorial, startup/import failure. Major: resource exploit, command/ownership breach, repeatably stuck army, unreadable vital cue, severe target-machine performance failure. Minor: nonblocking visual/audio mismatch or wording. A candidate cannot pass with an unresolved blocker; exceptions to major/minor defects require recorded owner acceptance. All final hardware and release evidence remains pending in this cloud.
