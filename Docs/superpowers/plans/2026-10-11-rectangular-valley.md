# Rectangular Valley Revision

User-approved revision, 11 October 2026: one 1600 by 1200 metre wooded valley, six irregularly positioned starts, unchanged physical building/unit scale, oblique top-down RTS camera with bounded zoom. This supersedes the intermediate 1600 metre square and previous 768 metre map for current production. Preserve historical 768 metre source and private scene backups.

## Scope

Keep Unreal 5.8.3 and the private Blueprint-only native authoring project. Do not change the legacy Core/save/FFA contract, purchase/download unapproved assets, spend Meshy credits, publish licensed binaries or claim finished art/gameplay/performance. Terrain sample dimensions are independent of physical metres. The new original height source should use the existing native 1009 square sample topology and unequal XY spacing to cover the rectangle, not enlarge buildings or vegetation.

## Work

1. Extend original terrain authoring conservatively to a second explicit layout, preserving exact default 768 metre exports and old tests. Add the 1600 by 1200 layout with six unique X/Y start coordinates, equal resource templates, disjoint clearing margins and a connected two-bank/three-crossing route graph. Approaches may bend outside the river corridor; every crossing stays on its reserved straight deck and cannot silently cut the river elsewhere. Derive all bounds and sampling transforms from the approved layout. Test valid output, byte order, exact extent, invalid crossings/topology and default preservation. Generate no third-party asset geometry.
2. Save a separate native rectangular map in the private project, apply the new original height source, use all 64 native components, and verify both unequal XY bounds and exact native height readback. Keep the older native map unchanged. Position a persistent oblique RTS authoring camera matching the existing gameplay source defaults: pitch -55 degrees, FOV 45, initial arm distance 60 metres, source zoom bounds 18 to 110 metres. A native camera preview is not proof of interactive compiled-game controls.
3. Continue actual terrain material/owned-asset integration within private staging. Show real native captures, explicitly label the current production stage, and preserve a restore-tested private scene checkpoint. Update source handoff and current design dimensions without rewriting historical evidence.

## Verification

Independent source review and focused/full Python tests before normal main integration/push. GitHub CI owns Core compilation. Native evidence must distinguish actual creation, height/bounds verification and captures from still-pending water, foliage wind, weather/day-cycle integration, navigation, six-participant balance and laptop profiling. At two metres per logical cell, the future map is 800 by 600 cells, but the current legacy simulation remains unchanged.
