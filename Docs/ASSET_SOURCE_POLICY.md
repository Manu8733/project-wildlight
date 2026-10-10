# Asset Source Policy

User instruction, 10 October 2026: Epic/Fab library first; Meshy only for what cannot be found there.

## Decision Order

1. Search downloaded and owned library content for the required role. Verify actual files, model quality, style suitability, Unreal compatibility and license rights. A cache entry is not proof of ownership or permission to redistribute source assets.
2. Reuse or adapt a suitable licensed asset in Blender/Unreal without sending it to generative services where its metadata or license forbids that use.
3. Only when a suitable library asset is missing, propose a Meshy generation, with subject, model, cost estimate, usable download rights and credit cap. New jobs prefer explicit `meshy-7.1`, not an unpinned `latest`; use 7 only when necessary.
4. Submit once, retain task/resource IDs, inspect the preview and deliver usable files. No silent paid retry, texture stage or subscription purchase.
5. Maintain source, author, license, modification notes, scale, LOD, materials and import evidence. A rendered preview is not proof of game integration or final topology.

## Downloaded Inventory Observed

| Package | Files found | Candidate use | Unverified gates |
| --- | --- | --- | --- |
| Ribbon Grass / Quixel | Six FBX LOD0 variants; PBR, opacity, translucency and billboard textures | Grass ground layer | Ownership/license, wind, stylistic adaptation, additional LODs |
| Medieval Modular Wall Pack / Vampawn | Blender master; listing describes 48 modular models | Wall/castle structure study | Ownership/license, materials, original visual direction, engine import |
| Day by the Beach | 322 `.uasset`, two `.umap`; rocks, ground and a WPO function | Terrain/rock study | Ownership/license, engine compatibility, map suitability |

The first two cached listings declare `isAiForbidden: true`. They must not be Meshy or other generative-AI inputs. A WPO function name in the third package does not prove leaf animation. This cache inventory does not establish that only three packages are owned.

## Public Repository Boundary

Do not commit Epic/Fab raw assets, exports, textures or derived redistributable copies until their license and public redistribution permissions are verified. Public source code, import instructions and non-sensitive provenance records are separate from licensed content. No Epic library files have been published in this session.

The original Meshy oak was a separately approved free-plan test. Its CC BY 4.0 attribution and retained source must accompany public distribution. It is not mislabeled as Meshy 7/7.1 or as a privately licensed asset.

## Model Access Check

Meshy documents 7.1 with higher geometry detail and the same credit cost as 7. API preview geometry is estimated at 20 credits; higher geometry resolution adds five; 2K/4K texturing adds ten. These are estimates, not approval to spend.

The free-plan documentation limits new free downloads to Meshy 6 Lite and states that API access needs a paid plan. Verify the active account before any 7/7.1 submission, so credits are not spent on an asset that cannot be downloaded. The installed CLI 0.4.0 has no authenticated profile at this checkpoint.

Sources checked 10 October 2026:

- https://docs.meshy.ai/en/api/changelog
- https://docs.meshy.ai/en/api/pricing
- https://help.meshy.ai/en/articles/15696428-what-is-included-on-the-free-plan
