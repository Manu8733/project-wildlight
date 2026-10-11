# Valley Environment Polish Implementation Plan

> Continue directly without subagents, as requested by the user. One owner of native editor actions.

**Goal:** Substantially improve the current real map's ground, vegetation integration and daylight within the already-approved ArtBible, then inspect close and oblique native captures.

**Architecture:** Original tested surface shaders extend a private clone of the existing ground material while preserving its native grass mask/output. Existing licensed meshes, wind functions and LODs stay private. Lighting adjustments use the existing sun/sky actors and do not replace the cosmetic runtime Blueprint.

**Tech Stack:** Python unittest, original HLSL, guarded Unreal 5.8.3 authoring, existing laptop DX12 software-Lumen profile.

## Global Constraints

- Preserve the 1600 by 1200 m layout, all terrain samples, six starts, river level and crossing reservations.
- Follow the approved anime fairy-tale palette: warm highlights, cool ambient fill, organic ground transitions and restrained detail.
- Keep the new blade field's density scaling/distance fading disabled. Do not shrink meshes or thin distant grass to fund this polish.
- Keep prior source/native material assets and independent private archives unchanged.
- No new purchase, generation, big-library download, local C++ build, weather/gameplay claim or acceptance change.
- "10 times nicer" is a visual improvement request, not a measured multiplier or permission to call unfinished systems complete.

## Task 1: Original Surface Detail

Files: `Tools/author_valley_surface.py`, `Tests/test_valley_surface.py`; private `native_polish_actions.py`.

- [x] Write failing tests for bounded colors/normals, organic spatial variation, route treatment, shore/slope response, invalid inputs and layout preservation.
- [x] Implement deterministic metre-space sampling and HLSL color/normal generation; no image/asset inputs.
- [x] Clone current native ground, preserve exact density/output bindings, connect new surface detail and compile.
- [x] Assign the compiled material on the actual persistent map; save and verify all 64 components.

## Task 2: Daylight And Integration

- [x] Read back sun/sky settings and source material parameters before changing them.
- [x] Add restrained warm sun/cool sky color, softer tree shadows, and adjust blade base/tip colors to the ground palette.
- [x] Preserve light actor/Blueprint references and existing wind; read back current renderer values without increasing quality.
- [x] Inspect same-camera close and oblique native captures; correct the yellow cast and the old overview tree-cull threshold. Final art is not accepted.

## Task 3: Verification And Checkpoint

- [x] Focused and full Python tests, native compiled material/readback, unchanged-height export and repeated grass wind comparison.
- [x] Separate private successor archive with CRC/member hash checks; no complete restore claim.
- [x] Truthful provenance, handoff and public source-only checkpoint preparation.

Publishing step: use a normal push and verify the exact remote SHA after committing this source snapshot. The commit cannot contain its own final SHA.

Results: 9 new focused tests and 139 full-suite tests pass; 1,018,081 unchanged height samples; 1,764 unchanged trees; exact blade mask retained; post-polish near-view wind passes. A bounded 120-frame editor CSV is advisory only. The private archive contains the source/plan snapshot taken before this completion annotation, not a full Git checkout.

Weather, mineral harvesting, full navigation, all-LOD root anchoring, six-player gameplay, measured release performance and final art acceptance remain separate work.
