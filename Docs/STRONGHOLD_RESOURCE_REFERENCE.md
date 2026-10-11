# Stronghold Resource Reference

Reviewed: 11 October 2026. Scope: Stronghold Crusader classic/HD as a reference for readable harvesting and logistics, not an asset source or a specification to copy unchanged. The user subsequently requested completing Wildlight's map population and map Blueprints/mechanics. This research does not establish that those systems are implemented, verified or accepted.

## Officially Documented Economy

| Activity | Classic/HD manual evidence |
| --- | --- |
| Wood | A hut assigns an available peasant to woodcutting. |
| Stone | Sandstone supports a three-worker quarry; a one-worker ox tether carries stone to the stockpile. |
| Iron | Two-worker mines use red hill rocks; miners extract, smelt and deliver iron. |
| Pitch | Rigs extract from marshes. |
| Food | Farms require oasis ground. Apples reach the granary; dairy starts milking after three cows. Wheat becomes flour, then bread; hops become ale for inns. |
| Storage/placement | Food and raw goods use different stores. Capacity limits production; inaccessible footprints cannot be built on. |

These are process and placement rules, not measured animation durations, replenishment timers or a castle-distance formula. See the [publisher-supplied Crusader HD manual](https://shared.steamstatic.com/store_item_assets/steam/apps/40970/manuals/Stronghold_Crusader_HD_Manual_-_English.pdf?t=1738143111), printed pages 20, 28-31, 49-50 and 60-61.

Firefly's Extreme economy diary recommends additional ox tethers for distant stone workings, confirming that transporting output matters. It provides no universal distance in metres or exact travel-time formula. This is an Extreme-era developer statement, not a measured HD timer. [Firefly developer diary, republished by GameWatcher](https://www.gamewatcher.com/2008-29-05-stronghold-crusader-extreme-developer-diary-for-economy-and-resources).

## Reported Tree Experiments

These are **albriradios's firsthand HD 1.41 observations**, not independently reproduced or official rules:

- Mature parents produce sprouts; stump replacement has no guaranteed timer. Four growth stages.
- Approximate maturity: cherry palm 3 years, olive/coconut 3.5, date 4; in-game years, not minutes.
- Palms use green farmland; olives other suitable soil. Water, rock, marsh, ruins and farm footprints exclude sprouts.
- Eight reported spots: cardinal spacing four cells, diagonal six; separation three, barriers six. These are not Wildlight metres.
- Mature harvesting: five falling impacts plus 36 processing impacts; nine logs become 36 boards over three delivery trips.
- Younger trees yield less. Hut-nearest targeting is not necessarily route-nearest; wind appears cosmetic.

Source: [Trees: All That Can Be Told](https://steamcommunity.com/sharedfiles/filedetails/?id=2261818426). Reproduce timings before balancing. Inference: preserving mature seed sources may sustain regrowth.

## Version Boundaries

- The classic/HD reference is the default for this request. Do not silently mix it with Crusader 2 or Definitive Edition.
- Crusader 2 explicitly uses ox transport for both iron and stone. That is a different rule, not evidence that classic/HD iron requires an ox tether. [Official Crusader 2 manual](https://cdn.akamai.steamstatic.com/steam/apps/232890/manuals/Stronghold_Crusader_2_Manual_EN.pdf), printed page 9.
- The Definitive Edition manual was located but not read in this research. Its mechanics are not confirmed here.
- No copied Stronghold models, textures, sprites, animation clips or map geometry belong in Wildlight.

## Unconfirmed Details

| Question | Current evidence limit |
| --- | --- |
| Exact worker animation clips, frame counts and impact timestamps | No frame-by-frame native reference capture was performed. |
| Exact mining, sowing, growth, harvesting and unloading cycle times | Not established at a recorded game-speed setting. |
| Mineral depletion or replenishment | Do not infer an infinite deposit, disappearing rock or respawn timer from the extraction description. |
| Distance from a castle or kingdom boundary | No universal radius or metres-based castle separation was verified. Transport and reachable terrain must be measured. |
| Growth after eliminating a rival | No applicable verified reference rule; Wildlight's confirmed neutral-resource preservation remains independent. |
| Numerical growth converted to real time | Requires the reference edition/build, game speed, pauses and calendar-rate measurement. |

This is a bounded research checkpoint, not a claim that every Stronghold mechanic or animation has been verified.

## Original Wildlight Application

The following are implementation guidance and pending balance decisions for the existing fixed-step architecture. They are **not claims about Stronghold**, new approved numerical rules, or completed native Blueprints.

| System | Presentation sequence to implement | Authoritative data |
| --- | --- | --- |
| Trees | Approach, chop standing tree, show partial damage, fall safely, process remaining timber, carry, unload | Stable resource ID, remaining yield, progress, worker/cargo state |
| Stone/ore | Approach work point, tool impact, show diminishing material/chips, collect, carry, unload | Finite node amount, extraction progress, cargo, destination |
| Crops | Prepare plot, sow, show growth stages, harvest visibly, carry, unload, start next legal cycle | Plot stage, simulation ticks, production amount, stored output |
| Regrowth | Show a small plant progressing through distinct stages; preserve root placement and wind weights | Deterministic candidate, species/stage, growth ticks, exclusion masks |

Animation events may synchronize sound, chips and foliage movement, but must not create resources. Simulation events and stable IDs control extraction, cargo and deposit credit exactly once. Blending, culling, reloads, canceled jobs or changing frame rate must not duplicate a harvest or advance an authoritative timer twice.

Tree regeneration must not obstruct building footprints, reserved base space, river water, crossing decks, required access or other occupied nodes. Save the deterministic regrowth state; neutral resource behavior must not become automatic territory annexation. Growth cadence, seed radius and per-species yield remain balance parameters to select and test, not borrowed constants from an isometric reference.

Judge distance by actual reachable paths and measured trip duration at Wildlight's own worker speeds. For all six irregular starts, compare first usable wood/stone/food access, round-trip time, usable build area and risk of blocked return routes. Visual roads must not become exclusive movement corridors.

## Current Native Binding Gap

The private `WoodedValley1600x1200` checkpoint contains decorative StyleHex trees with `NoCollision`, not harvestable resource actors. The native planner protects a 110 m base clearing plus a 22 m blend and scaled crown margin. An authored logical wood offset of `[-35,-25]` m lies inside that reserved space. These two placements do not yet represent the same usable forest. Deliberately bind resource positions and visual harvest instances before claiming wood access or regrowth; do not silently shrink the protected clearings to hide the mismatch.

Saved/reloaded foliage counts, a source plan and a still image do not prove harvesting, motion, collision, navigation, complete environment Blueprints or economy integration. Three StyleHex wind comparisons remain failed as recorded in [SESSION_HANDOFF.md](SESSION_HANDOFF.md) and [StyleHexVegetation.json](AssetSources/StyleHexVegetation.json).

## Verification Before Acceptance

1. Record the exact reference-game edition/build and game speed, then capture multiple full harvest/delivery and growth cycles without speeding up unrecorded intervals.
2. Separate impact counts, calendar time, simulation time and wall-clock seconds. Observe partial resource changes and canceled/interrupted jobs, not just final inventory totals.
3. Test the Wildlight authoritative state with depleted nodes, full stores, blocked destinations, job changes, elimination and save/load during every stage.
4. Verify native paths off roads, all crossing approaches, six-base resource access and legal build space. Keep permanent crossings and neutral nodes independent of defeated owners.
5. Compile and play the actual Blueprints/animation bindings; capture near/RTS wind and harvesting, then profile the laptop Low preset. Source tests and published research alone do not satisfy these gates.
