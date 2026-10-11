# Grouped Relief And Raised River

11 October 2026. This source revision responds to the user's correction: research Stronghold Crusader terrain organization, remove the enclosed bowl-like mountain wall, soften the banks, raise the river and avoid a broad beach. Keep all graphics in Wildlight's original anime fairy-tale/Tiny Glade direction. Improved native reflections and a narrow non-sand shore material are separate engine tasks, not outputs of this height utility.

## Primary References

- [Firefly's official Stronghold Crusader HD Steam page](https://store.steampowered.com/app/40970/Stronghold_Crusader_HD/) and its [publisher-provided gameplay image](https://shared.fastly.steamstatic.com/store_item_assets/steam/apps/40970/ss_b2da292b97471df65e7db87ef15b84ae53799be0.1920x1080.jpg?t=1742481898): broad usable flats, localized raised/cliff sectors and readable low river land are visual organization references. This is an interpretation of the image, not a measured claim about that game's metres or slopes, and no layout or artwork is copied.
- [Firefly's map-editor guide](https://steamcommunity.com/sharedfiles/filedetails/?id=353757026) is a supplementary **Stronghold Crusader 2** guide, not documentation for the original game's editor. Its flat base/resource pads, grouped heights, open approaches and careful water/pathing checks inform authoring practice only.

## Fixed Source Parameters

The approved 1600 by 1200 metre rectangle, 1009 by 1009 height samples, unequal XY spacing, six irregular starts, physical resource/building scale, equal initial resource templates, five route reservations and three separate deck footprints are unchanged. Roads guide composition; they are not the only land NPCs may navigate.

The rectangular profile alone gains an optional `terrain.mountains.clusters` list. Each fixed ellipse has an ID, metre-space centre, two finite-support radii and a rise above the surrounding valley. Existing ridge fields are retained for the exact historical non-cluster branch; they do not generate a wall when clusters are present. The authoring code uses no random generator.

| Group | Centre X,Y (m) | Radii X,Y (m) | Rise (m) |
| --- | --- | --- | --- |
| Northwest massif | 30,30 | 160,145 | 42 |
| West-middle massif | 35,550 | 110,210 | 58 |
| Southwest massif | 40,1120 | 170,170 | 49 |
| Southwest ridge | 510,1170 | 200,80 | 37 |
| Northeast massif | 1115,15 | 180,100 | 60 |
| East-north massif | 1560,150 | 110,160 | 52 |
| East-middle massif | 1570,620 | 100,180 | 62 |
| Southeast ridge | 1150,1170 | 170,75 | 45 |
| West meadow hill | 270,570 | 105,100 | 10 |
| East meadow hill | 1330,600 | 105,90 | 12 |

The finite-support smoothstep shapes leave genuine low peripheral gaps, rather than adding a continuous four-sided ridge. General valley height stays 8 m; undulation is reduced from 0.75 to 0.35 m and lateral rise from 2.5 to 0.5 m. The six 110 m-radius clearing cores remain exactly 8 m high with their existing 22 m blend margins.

The channel retains its original authored centreline, 32 m bed and 70 m total bank corridor. Bed height changes from -4 to 2.5 m, and water surface from 0 to **5.5 m**. The unchanged 0.001 m/m downstream bed rise gives 3.0 m depth at Y=0 and 1.8 m at Y=1200. Deck reservations stay at 8 m, 2.5 m above water; they are still separate geometry, never terrain fill. Actual decks, collision, flood-free crossings and NPC navigation need native validation.

## Safety And Verification

Cluster validation bounds the list to ten entries, radii to 30-250 m and rise to 1-70 m. It rejects nonfinite/out-of-domain data, duplicate IDs, protected clearing/route/deck overlap and the entire reserved river/bank envelope. Clearings and route margins use a conservative normalized-space clearance; this may reject an otherwise close but safe edit rather than permit a hidden overlap. Historical `WoodedValley768` rejects this new optional field and retains its exact previous height path, layout and tests.

New regression tests first failed against the old source for the expected missing clustered relief, excessive bank slope, missing safety validation and unraised water. They then passed against the revision. Finite-difference sampling verifies slopes below 30 degrees across banks and the interior plains outside peripheral mountain supports, not every native navigation polygon. Tests also check low edge gaps, continuous cluster edges, six flat clearing cores, wet river under decks and historical layout bytes. Existing export tests separately retain the historical 768 metre R16 SHA-256 `196163c93d408dc77a1c649476c3608fbc86e565f1f8c0c104154edb14dc1247`.

Fresh focused verification used the bundled Python runtime: `python -m unittest discover -s Tests -p 'test_*valley*.py' -v`, **49 tests passed in 113.681 seconds**, exit code 0. This includes both complete terrain exporters, the new relief regressions and the river sampler. `git diff --check` also passed. `WoodedValley768.layout.json` and `test_valley_terrain.py` have no tracked changes.

Native application must preserve immutable earlier exports, generate a new dated input, load all 64 native components, read back the complete revised height field, update water/rocks to the new surface, inspect real captures and save a new private checkpoint. Source tests do not prove that these engine actions, final artwork, navigation, six-player capacity or laptop performance have passed.
