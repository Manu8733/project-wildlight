# Wildlight art provenance and licensing

This register covers the initial Blender-authored art set. It is an inventory of provenance, not an independent legal clearance or Steam submission record.

| Asset group | Source | Method | Third-party art licence / cost | Current status |
| --- | --- | --- | --- | --- |
| 19 building prototypes | Wildlight project Blender Python source | Original assembled geometry, bevels and colour materials | No third-party art imported; no asset purchase | Prototype; inspect exports before release |
| 12 unit prototypes | Wildlight project Blender Python source | Original static geometry and equipment | No third-party art imported; no asset purchase | Static prototypes; rigging/animation unfinished |
| 10 environment/prop prototypes | Wildlight project Blender Python source | Original procedural trees, rocks, flowers and other prop geometry/materials | No third-party art imported; no asset purchase | Presentation/environment prototypes |
| Showcase lighting, camera and renders | Wildlight project Blender scene | Composed and rendered in Blender | No stock image or image-generation service | Asset showcase, not gameplay evidence |

Buildings: Castle, House, Farm, Mill, Depot, Lumber Camp, Quarry, Gold Mine, Barracks, Archer Range, Forge, Market, Wall, Gate, Watchtower, Beacon, Sanctuary, Mage Tower, Siege Workshop.

Units: Worker, Militia, Spearman, Swordsman, Archer, Cavalry, Engineer, Arcane Guard, Forest Ally, Ram, Catapult, Wizard.

## Ownership and release policy

The files were authored for this project through coding-assistant work and Blender scripts. “Original” describes the source and geometry; it does not claim independent human authorship or resolve copyright eligibility in every jurisdiction. No downloadable art-pack licence is being granted by this document. Until the project owner chooses release terms, treat project art and source as private project material and do not redistribute them as a public asset pack.

Blender is an external tool distributed under its own GPL terms. Use of Blender does not by itself apply Blender's software licence to original exported artwork. Retain the source scripts and the Blender version/export report so provenance and outputs can be reproduced. Unreal Engine and its toolchain are governed by their own terms and are outside this art register.

## Boundaries of this register

- No external models, textures, stock art, fonts, music or sound effects are included in the Blender asset groups listed above. `Art/Audio` is a separate set of original oscillator-based audio studies created by `Tools/author_audio.py`; consult its registry for filenames and audio review status.
- System fonts used only by tooling or a review page do not establish redistribution rights for bundled fonts. Any later bundled font needs an explicit licence entry.
- No generated image is used as a texture or artwork. This does not remove the need to review applicable disclosures for AI-assisted code or other project content at release.
- Before adding purchased, downloaded or commissioned material, record the exact filename, creator, source URL, licence text/version, attribution requirement, redistribution limits, receipt and cost. Keep that record with the asset.
- Run a release inventory against actual packaged files; this table cannot clear files that are added later.

## Reproducibility record

Keep Blender Python source, export settings, a measured per-asset manifest and checksums in version control. Keep `.blend`, FBX and large rendered outputs in Git LFS where repository policy requires it. Derive checksums from generated files, not from this document. Re-exported or modified assets must retain their stable identifiers and provenance history.
