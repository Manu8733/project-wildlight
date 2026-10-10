# Initial balance baseline

Values extracted from the current portable catalogs on 10 October 2026. They are implementation defaults, **not playtested final balance**. Costs use W / S / F / G / M (wood, stone, food, gold, magic energy). Distances are grid cells; the presentation scale is 200 cm/cell. The authoritative source remains `Core/src/Simulation.cpp`.

## Buildings

| ID / English name | Cost W/S/F/G/M | Footprint | Health | Build seconds | Population / storage | Worker slots | Light radius / required |
| --- | --- | --- | ---: | ---: | --- | ---: | --- |
| `Castle` / Dawnkeep | 300 / 250 / 0 / 100 / 0 | 4×4 | 1800 | 45 | 18 / 1200 | 0 | 15 / false |
| `House` / Hearth Cottage | 45 / 10 / 0 / 0 / 0 | 2×2 | 260 | 8 | 8 / 0 | 0 | 0 / false |
| `Farm` / Sungrain Farm | 70 / 10 / 0 / 0 / 0 | 3×3 | 320 | 12 | 0 / 0 | 3 | 0 / false |
| `Mill` / Windweave Mill | 90 / 30 / 0 / 15 / 0 | 2×2 | 360 | 14 | 0 / 0 | 0 | 0 / false |
| `Depot` / Wayfarer Depot | 65 / 25 / 0 / 0 / 0 | 2×2 | 420 | 10 | 0 / 600 | 0 | 0 / false |
| `LumberCamp` / Timber Lodge | 55 / 10 / 0 / 0 / 0 | 2×2 | 300 | 9 | 0 / 0 | 3 | 0 / false |
| `Quarry` / Stonecutters' Yard | 70 / 15 / 0 / 0 / 0 | 2×2 | 350 | 12 | 0 / 0 | 3 | 0 / false |
| `GoldMine` / Gilded Hollow | 80 / 35 / 0 / 0 / 0 | 2×2 | 350 | 14 | 0 / 0 | 3 | 0 / false |
| `Barracks` / Sentinel Barracks | 110 / 55 / 0 / 20 / 0 | 3×3 | 650 | 18 | 0 / 0 | 0 | 0 / false |
| `ArcherRange` / Willow Range | 100 / 30 / 0 / 25 / 0 | 3×2 | 480 | 16 | 0 / 0 | 0 | 0 / false |
| `Forge` / Ember Forge | 85 / 65 / 0 / 40 / 0 | 2×2 | 580 | 18 | 0 / 0 | 0 | 0 / false |
| `Market` / Lantern Market | 100 / 40 / 0 / 45 / 0 | 3×2 | 460 | 18 | 0 / 0 | 0 | 0 / false |
| `Wall` / Runestone Wall | 0 / 12 / 0 / 0 / 0 | 1×1 | 520 | 4 | 0 / 0 | 0 | 0 / false |
| `Gate` / Oath Gate | 20 / 35 / 0 / 0 / 0 | 1×3 | 700 | 9 | 0 / 0 | 0 | 0 / false |
| `Watchtower` / Thornwatch Tower | 55 / 85 / 0 / 25 / 0 | 2×2 | 850 | 18 | 0 / 0 | 0 | 0 / false |
| `Beacon` / Threadlight Beacon | 65 / 50 / 0 / 30 / 15 | 1×1 | 430 | 12 | 0 / 0 | 0 | 13 / false |
| `Sanctuary` / Grove Sanctuary | 90 / 90 / 0 / 65 / 30 | 3×3 | 650 | 22 | 0 / 0 | 0 | 14 / true |
| `MageTower` / Starweaver Spire | 100 / 110 / 0 / 90 / 35 | 2×2 | 580 | 24 | 0 / 0 | 0 | 0 / true |
| `SiegeWorkshop` / Ironroot Workshop | 140 / 80 / 0 / 70 / 0 | 3×3 | 700 | 24 | 0 / 0 | 0 | 0 / false |

## Units

| ID / English name | Cost W/S/F/G/M | Recruited at | HP / damage / armor | Range / speed | Cooldown / train s | Population | Siege |
| --- | --- | --- | --- | --- | --- | ---: | --- |
| `Worker` / Hearthfolk | 0 / 0 / 25 / 0 / 0 | Castle | 65 / 4 / 0 | 0.9 / 2.2 | 1.2 / 6 | 1 | false |
| `Militia` / Lantern Militia | 12 / 0 / 30 / 8 / 0 | Barracks | 100 / 12 / 1 | 1.1 / 2.4 | 1.0 / 7 | 1 | false |
| `Spearman` / Briar Spearman | 20 / 0 / 35 / 15 / 0 | Barracks | 125 / 16 / 2 | 1.5 / 2.25 | 1.1 / 9 | 1 | false |
| `Swordsman` / Oathblade | 10 / 12 / 40 / 30 / 0 | Barracks | 155 / 22 / 5 | 1.0 / 2.1 | 1.0 / 11 | 1 | false |
| `Archer` / Willow Archer | 25 / 0 / 35 / 20 / 0 | ArcherRange | 85 / 15 / 1 | 6.5 / 2.3 | 1.4 / 10 | 1 | false |
| `Cavalry` / Dawnrider | 20 / 0 / 65 / 45 / 0 | Barracks | 220 / 26 / 4 | 1.2 / 3.8 | 1.2 / 16 | 2 | false |
| `Engineer` / Runesmith | 30 / 10 / 40 / 25 / 0 | SiegeWorkshop | 95 / 7 / 2 | 1.0 / 2.1 | 1.5 / 12 | 1 | false |
| `ArcaneGuard` / Threadbound Guard | 0 / 25 / 40 / 45 / 25 | MageTower | 190 / 24 / 6 | 1.4 / 2.2 | 1.2 / 16 | 2 | false |
| `ForestAlly` / Grove Warden | 20 / 0 / 50 / 20 / 30 | Sanctuary | 210 / 20 / 3 | 1.4 / 2.0 | 1.4 / 15 | 2 | false |
| `Ram` / Ironroot Ram | 90 / 30 / 0 / 45 / 0 | SiegeWorkshop | 380 / 32 / 8 | 1.2 / 1.4 | 2.2 / 22 | 3 | true |
| `Catapult` / Moonstone Catapult | 110 / 60 / 0 / 65 / 0 | SiegeWorkshop | 220 / 42 / 3 | 9.5 / 1.1 | 3.8 / 26 | 3 | true |
| `Wizard` / Wildlight Weaver | 0 / 15 / 35 / 65 / 35 | MageTower | 90 / 18 / 1 | 5.8 / 2.1 | 1.6 / 18 | 2 | false |

## Spells

| ID / English name | School | Mana | Duration s | Cooldown s | Radius | Range |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `RootBarrier` / Briar Rampart | Roots | 25 | 18 | 26 | 1.8 | 13 |
| `Heal` / Kindred Renewal | Roots | 20 | 2 | 18 | 4 | 12 |
| `HarvestBloom` / Sungrain Bloom | Roots | 25 | 40 | 48 | 6 | 14 |
| `LightBridge` / Threadway | Light | 30 | 30 | 38 | 3.7 | 15 |
| `Amplify` / Beacon Chorus | Light | 25 | 35 | 42 | 6 | 14 |
| `Shield` / Oathlight Aegis | Light | 30 | 22 | 30 | 4.5 | 12 |
| `Veil` / Mothveil | Shadows | 25 | 20 | 28 | 5 | 12 |
| `Disrupt` / Unravel | Shadows | 35 | 24 | 35 | 4.5 | 14 |
| `Illusion` / False Dawn | Shadows | 30 | 20 | 30 | 3 | 12 |

## Scenario baseline

| Scenario | Grid | Player / enemy start | Objective |
| --- | --- | --- | --- |
| The Threadwater Vale | 64×48 | (9.5, 24.5) / (54.5, 24.5) | Destroy rival Dawnkeep |
| Emberfall Pass | 64×48 | (9.5, 12.5) / (54.5, 35.5) | Destroy rival Dawnkeep |
| The Lantern Mere | 64×48 | (9.5, 24.5) / (54.5, 24.5) | Connect any two objective beacons for 90 s, or destroy rival Dawnkeep |

These are generated simulation grids; final authored map art, routing and fairness remain unaccepted.

## Current economy and difficulty defaults

- Every 5 seconds upkeep consumes 0.35 food per population. Sufficient food recovers 2 morale; shortage subtracts 6 morale, with floor 20. Overpopulation subtracts another 3. Exact morale influences production, training and movement; tune against measured match behavior.
- Farms require workers who carry food back to storage. Connected farm yield gains 20%. A working mill grants 40% yield while consuming wood; verify the actual consumption interval before final tuning. Harvest Bloom doubles eligible yield temporarily.
- Completed connected nodes produce magic energy per second: Dawnkeep 0.35, Beacon 0.2, Sanctuary 1.2, modified by level and Arcana.
- Automatic jobs prioritize nearby unfinished construction, then available production slots with farm preference. There is no completed player priority-management interface.
- Easy / Normal / Hard AI planning cadence: 5 / 2.8 / 1.5 seconds. Baseline attack triggers: Conqueror 110 s, Builder 190 s, Arcanist 150 s; Easy scales by 1.5 and Hard by 0.8. Personalities use shared paid commands; integrated behavior still needs testing.
- Paid building upgrades are currently immediate and limited to three levels. This is prototype behavior, not final animated upgrade presentation.

## Tuning protocol

Keep a seed-controlled before/after record. Change one major economic lever at a time; evaluate 10 representative openings, full-match outcomes, shortages, idle workers, army composition and beacon decisions. Examine at least one deliberately damaged economy and one broken network. Match duration and enjoyment require independent playtest, not just an AI win count.
