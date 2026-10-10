# Automatic House And Farm Growth

Date: 10 October 2026. Source baseline: `0cca2196e29175c4393d194e659a10a5d13c8ce6`.

Status: the user approved the written rules on 10 October 2026 and explicitly requested implementation. Implementation and verification remain pending; this is not gameplay or art acceptance.

## Scope And Approach

Implement automatic growth for House and Farm only, using the existing levels 1-3, upgrade prices, population/yield multipliers and health-ratio preservation. Military buildings, walls, towers, research and other production facilities retain manual upgrades in this first increment.

Recommended approach: deterministic automatic eligibility followed by the existing atomic level transition, plus level-aware presentation. An alternative is timed upgrade construction with interrupted jobs, refunds and partially upgraded structures; that needs a separate construction design and is deferred. Expanding a building's occupied footprint during growth also needs separate overlap/evacuation rules and is deferred.

This specification does not implement the six-participant map, tree species, resource-depletion visuals, crop growth cycles, wind or final asset production. Those are separate increments. In particular, farms currently produce continuously; no existing sowing/ripe/harvest cycle is being claimed.

## Automatic Policy And Reserve

- New matches enable automatic growth for both current factions under identical rules.
- Each faction owns its policy: enabled, protected wood and protected stone. Proposed defaults are 100 wood and 50 stone.
- Reserve inputs must be finite and between 0 and 1,000,000. Values above storage capacity are allowed and deliberately prevent an automatic purchase until the policy is lowered or storage changes.
- Only deposited faction stock is spendable. Worker cargo, expected yield and storage capacity are not money.
- An automatic purchase must leave stock at or above each protected amount, using the existing `1e-8` affordability tolerance. Evaluate the final subtraction, not just whether the full upgrade price is affordable.
- The reserve protects automatic upgrades only. Explicit construction, recruitment, research, upkeep, trade and manual Upgrade keep their current behavior; they may spend below this reserve. There is no new general economy lock.
- Changing policy is an owner-validated command. Reject invalid numbers, invalid owner or malformed input without mutating policy, resources or buildings.
- Turning growth off resets housing-demand dwell progress. It does not erase genuine farm production history or refund earlier completed upgrades. Re-enabling cannot bypass cooldown or other eligibility checks.

## Eligibility And Scheduling

The authoritative Core owns all decisions. Presentation reads state; it never initiates a level transition using frame time.

Common eligibility: completed live House/Farm, below level 3, at least 90% health, and no actual building damage within the previous 100 fixed ticks (10 seconds). Record actual health-reducing damage; a spell effect or merely nearby enemy is not automatically damage. A last-damage tick of zero means no recorded damage (including unknown v1 history), not a fabricated damage event at match start. Recorded damage occurs at ticks 1 or later; a building becomes eligible when `tick - last_damage_tick >= 100`, or when the sentinel is zero. Existing light requirements remain unchanged; do not add a light requirement to these two kinds.

House eligibility uses faction population pressure because per-house residents do not exist in the current simulation. Population includes the existing queued-recruit reservations. At least 80% of population capacity must be needed continuously for 100 fixed ticks. Reset the dwell counter if pressure falls below the threshold. Zero capacity is not a valid denominator or an automatic-growth trigger.

Farm eligibility uses food actually harvested into worker cargo since the last level transition. Proposed milestones are 120 food at level 1 and 240 at level 2. This is production, not delivered inventory: food can subsequently be lost with its worker. Credit only the positive amount actually added by this farm's Gather branch, never potential output, elapsed time or cargo capacity. Cap the counter at the current milestone, reset on any successful manual or automatic level transition, and stop accumulating at level 3. Production history continues accumulating while automatic growth is off: the switch disables purchasing, not farming. Once the milestone is reached, the farm need not have a worker currently assigned; it still must pass every common and economic eligibility check.

Evaluate candidates at absolute fixed ticks divisible by 10 (one second), after ordinary upkeep, worker production/delivery, combat, AI commands, dead-entity cleanup, faction recalculation and outcome resolution. Only a match still in `Playing` may purchase an automatic upgrade, so base destruction or surrender cannot cause spending on the terminal tick. Existing explicit commands therefore spend first. Use the same ordering for player and AI; do not grant AI resources or hidden thresholds.

Allow at most one automatic upgrade per faction per evaluation and apply a 150-tick (15-second) faction cooldown after any successful House/Farm upgrade, including a manual one. Manual commands bypass that automatic cooldown and the protected reserve; resetting the cooldown only prevents an immediate subsequent automatic tier change. Choose an eligible and affordable House before a Farm; within each kind choose the lowest stable entity ID. A blocked or unaffordable House does not prevent a qualifying Farm from being considered. Recalculate capacity immediately after a successful level change. Never charge twice or allow several houses to respond to stale housing capacity.

At the start of each live fixed tick, decrement an existing nonzero cooldown once, before any commands or automatic purchases occurring in that tick. Set a new cooldown after a successful transition, without decrementing it again that tick; the earliest automatic purchase after an upgrade at tick T is tick T+150 when that tick is also an evaluation tick, or the next evaluation tick otherwise. Cooldown decays while the policy is off but not while simulation time is paused. After ordinary work and outcome resolution, update housing dwell once per live tick from current totals: disabled, non-Playing or pressure below 80% means zero; otherwise increment up to 100. A manual command applied between ticks at saved tick T starts its cooldown at T, so the next tick supplies its first decrement.

Fixed-tick counters, farm history, damage history and policy must survive saves. Render FPS, RNG, vector insertion accidents or splitting `step()` calls must not change the result.

## Prices And Transition

Reuse the current manual formula: base wood/stone/gold cost multiplied by `0.65 * current_level`, with food and mana zero. House and Farm currently spend only wood and stone.

| Kind | Level 1 to 2 | Level 2 to 3 |
| --- | --- | --- |
| House | 29.25 wood, 6.5 stone | 58.5 wood, 13 stone |
| Farm | 45.5 wood, 6.5 stone | 91 wood, 13 stone |

Manual and automatic paths share one price/application helper. A successful transition deducts the price once, increments one level, preserves the existing health fraction, resets that building's production milestone and updates faction totals. It retains entity ID, owner, position, rotation, worker assignments and selection. A failed transition changes none of these values.

The first increment has no paid-in-advance upgrade job. The authoritative transition is atomic; the presentation refresh does not postpone population capacity or food productivity. Level-dependent population and farm-rate behavior remain the existing multipliers.

## Save Compatibility

Write save version 2 and explicitly read versions 1 and 2. Do not append new bytes while labeling the result version 1 or reinterpret the old unused production timer as genuine historical yield.

Preserve the existing v1 payload fields and add a checksummed v2 extension containing:

- Per-faction policy, housing-demand ticks and automatic cooldown ticks.
- One metadata entry per saved building, keyed by its stable ID, containing harvested-food progress and last-damage tick.

Validate the extension before committing candidate state: exact faction count for the current two-faction model, boolean encoding 0/1, finite reserves within 0-1,000,000, dwell 0-100 (exactly zero when disabled), cooldown 0-150, damage tick not after saved tick, and complete nonduplicated metadata coverage of known building IDs. Harvested-food progress must be finite within [0,120] for level-1 farms and [0,240] for level-2 farms, and exactly zero for non-farms and level-3 farms. Reject unknown versions, duplicate IDs, missing/truncated extension, invalid values and trailing bytes atomically.

For v1 migration, preserve existing gameplay fields; default automatic growth off, reserve 100/50, dwell/cooldown zero, farm history zero and unknown damage history zero. Enabling automatic growth is then an explicit player choice. Do not invent production history for an old save.

Version-specific corruption fixtures must retain real v1 coverage as well as v2 coverage. Round-trip and deterministic continuation tests must include eligibility and cooldown boundaries.

## Presentation And Art Contract

Refresh a building mesh whenever its authoritative level changes; the current actor-spawn-only mesh lookup is insufficient. Also select the correct model when loading a save directly at level 2 or 3. Preserve actor identity, transform, selection and existing visibility rules.

Use distinct `SM_House_L1/L2/L3` and `SM_Farm_L1/L2/L3` model variants. House progression should visibly add structure, such as an expanded wing and an upper floor; farm progression should develop the barn/storage and field detailing. Global scale alone is not an acceptable level variant.

The existing full gameplay footprint is reserved at initial placement: House 2x2 cells (4x4 m), Farm 3x3 cells (6x6 m). Smaller early-tier structures and larger later tiers remain inside that reserved area. Keep roof/structural clearance within the footprint; do not make cosmetic growth obstruct adjacent roads, units or buildings. Height and model silhouette may change.

Missing level variants may retain the available prototype mesh with a diagnostic warning, but must be recorded as missing art and must not be described as final visible growth. Final variants require Epic/Fab-first source selection, license/provenance checks and actual Unreal import/visual validation. No new Meshy credit spending or paid subscription is authorized by this specification.

## Controls And Feedback

Player-facing text remains English. The economy controls expose an accessible automatic-growth checkbox/toggle and numeric wood/stone reserve controls. The selected building displays its level and truthful eligibility status: disabled, maximum level, demand/production milestone, repair needed, recent damage, reserve or cooldown. Do not display these new controls as working until wired to authoritative commands.

The HUD can report a level change and play the existing build cue. Persistent state, rather than an animation event alone, drives the mesh after save/load. Loading a save on the exact tick of an upgrade must choose the saved tier without replaying that historical notification or sound; initialize the existing event deduplication cursor from saved events accordingly. Visual polish must not conceal missing level assets.

## Verification And Delivery Gates

Core tests cover: exact reserve boundary and insufficient-by-epsilon cases; worker cargo excluded; disabled/re-enabled policy including production accumulated while disabled; both factions; contested stock; deterministic ID priority; one upgrade per evaluation; absolute evaluation phase and cooldown boundaries, including manual transitions and disabled-policy decay; housing dwell/capacity recomputation; unfinished, damaged and max-level buildings; no spending on victory, defeat or surrender; actual farm production and capped/reset milestone; manual override behavior; v1 migration; invalid/truncated v2 extension including nonfinite, negative or over-milestone production and nonzero disabled-policy dwell; atomic load rejection; round-trip and fixed-step deterministic continuation.

Run all existing C++ and Python regressions, real normal verifier execution and CTest. Record sanitizer results honestly; the known cloud LeakSanitizer restriction is not a passing leak check.

Unreal requires a compatible verified toolchain, successful compilation, controls QA, save/load mesh refresh without replayed upgrade feedback and distinct tier renders without overlaps. Installed engine 5.6.1 does not establish compatibility with the recovered 5.8.1 lock. Final art and integrated gameplay acceptance remain separate from a passing Core implementation.

## Design Checklist

- Context inspected; approved automatic/reserve/off concept recorded.
- Reuse-versus-timed-construction alternatives considered; narrow reuse recommended.
- Rules, persistence, presentation boundaries and verification specified.
- Written specification review and approval required before implementation planning.
