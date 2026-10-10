# Asset inventory and provenance policy

Actual asset evidence lives in [Art/LICENSES.md](../Art/LICENSES.md), the generated `Art/manifest.json`, the source generator and export review report. This document records the required inventory boundary. It is not legal clearance or a claim that every planned asset exists.

The original procedural kit is made from explicitly authored Blender geometry/material source for Wildlight. No image-generation service is used for the project art kit. Coding-assistant assistance is recorded honestly. External tooling has its own licenses; original exported artwork is not automatically licensed under Blender's GPL merely because Blender produced it. Release rights remain subject to the owner's terms and applicable rules.

## Current versus required content

| Category | Current evidence | Remaining acceptance |
| --- | --- | --- |
| Building geometry | 19 original prototype definitions/export evidence as generated | In-engine scale/pivot/collision, LOD, final material/UV review |
| Unit geometry | 12 original static prototype definitions/export evidence as generated | Rigging, skinning, animation and in-engine role readability |
| Environment | Original showcase trees/rocks/ground as generated | Final playable terrain, water, vegetation, ruins and light sites |
| Materials | Original palette/source material recipe | Final texture/atlas import, roughness/emission review, streaming budget |
| UI icons | Record vector/generated source if present | A reviewed consistent complete set and HUD integration |
| VFX | Specification or source scaffolding only where present | Niagara systems and integrated timing/clarity evidence |
| Audio/music | Original deterministic synthesis studies in `Art/Audio/` from `Tools/author_audio.py`, when generated | Listening review, mix, final composition and gameplay-event integration |
| Fonts | No bundled font cleared by this document | Exact license before bundling; engine default font is engine content |

## Entry schema for every addition

Record: stable asset ID, category, local path, editable-source path, author, method, exact source URL when external, license identifier and retained license text, attribution obligations, modification notes, cost/currency/date, intended Unreal path and SHA-256 of the output. Include import settings and verification result separately. Purchased files require receipt retention outside public history where it contains personal information.

Do not invent a license for assistant-authored project files or assign third-party rights without evidence. Do not import content taken from Tiny Glade, Stronghold or any reference title. A visual reference does not grant source-asset rights. Before a candidate build, enumerate the actual packaged files and reconcile them with the manifest; a design list is not a packaged inventory.

The initial audio studies use original oscillator/envelope source, with no downloaded samples or voice/model service. Eight planned outputs include a 32-second woodland bell-loop study and order, building, magic, alert, impact, victory and defeat cues. These are prototype synthesis studies; file creation alone does not establish a finished soundtrack or good mix. Retain the source and generated manifest, and perform listening/in-engine review before acceptance.
