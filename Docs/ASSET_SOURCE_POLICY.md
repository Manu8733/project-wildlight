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

A bounded follow-up checked C: and D:, 18 standard alternate cache paths and selected shallow project/download directories. No additional downloaded tree, crop/farm or house-level kit was located. This local search did not establish an account-library gap.

The authenticated Fab library was subsequently inspected through rendered product cards: 59 of 59 listed products were observed. Relevant owned candidates include three Megascans tree species, grass and flowers, modular houses/castles, the Stylized Harvest Farm environment and a Melee Weapon Aura VFX pack. The full account inventory stays in local outputs, not in the public repository. The shortlist and verification limits are recorded in `FAB_ASSET_SHORTLIST.md`; owning a listing does not prove its files are downloaded, compatible with 5.8.1 or final-quality for Wildlight.

## Vegetation And VFX Acceptance

- The user rejected the previous Meshy oak as a final tree. Preserve its historical files; do not use it as the final map tree or keep refining it in place of a suitable library candidate.
- Trees, grass and flowers must visibly react to gentle wind and gusts. A skeleton alone is insufficient evidence; material-driven WPO/Pivot Painter or a suitable skeletal wind system must preserve anchored roots/stem bases and appropriate branch/leaf motion.
- The three owned Megascans tree listings declare simple and Pivot Painter wind plus LOD/quality tiers. This is publisher evidence, not a verified Wildlight import. Preserve their existing wind metadata if adapting geometry in Blender; regenerated vertex colors/UVs can destroy masks and pivots.
- Retain source textures/UVs, opacity/translucency, normals, scale and LOD data as appropriate. The recovered palette-only importer disables material/texture import and cannot silently be reused for these PBR assets. Its validator also rejects textured GLBs; extend the contract with tests before adopting a textured pipeline.
- Check rooted deformation, per-instance variation, culling bounds, shadows, Low-preset performance and visual motion in a compatible real Unreal session. No final wind/rig/performance acceptance is claimed from a product description.
- Niagara effects follow the same library-first order. The owned melee aura pack may supply suitable weapon glows, but its listing's links to other packs do not establish ownership of their trails, impact or projectile systems. Search environment packages for included effects before declaring a specific gameplay VFX missing.
- The later user instruction authorizes optimized local Unreal via MCP, superseding the earlier cloud-only restriction for that work. The user is installing 5.8 and will announce readiness; no local editor/shader/render workload starts before that message. Do not interpret this as authorization to install an engine, rent a cloud GPU, perform expensive Blender renders or publish licensed content. Cloud Core execution remains separate from editor/renderer verification.

## Public Repository Boundary

Do not commit Epic/Fab raw assets, exports, textures or derived redistributable copies until their license and public redistribution permissions are verified. Public source code, import instructions and non-sensitive provenance records are separate from licensed content. No Epic library files have been published in this session.

The original Meshy oak was a separately approved free-plan test, subsequently rejected for final art. Its CC BY 4.0 attribution and retained source must accompany any public distribution. It is not mislabeled as Meshy 7/7.1 or as a privately licensed asset.

## Model Access Check

Meshy documents 7.1 with higher geometry detail and the same credit cost as 7. API preview geometry is estimated at 20 credits; higher geometry resolution adds five; 2K/4K texturing adds ten. These are estimates, not approval to spend.

The free-plan documentation limits new free downloads to Meshy 6 Lite and states that API access needs a paid plan. Verify the active account before any 7/7.1 submission, so credits are not spent on an asset that cannot be downloaded. The installed CLI 0.4.0 has no authenticated profile at this checkpoint.

Sources checked 10 October 2026:

- https://docs.meshy.ai/en/api/changelog
- https://docs.meshy.ai/en/api/pricing
- https://help.meshy.ai/en/articles/15696428-what-is-included-on-the-free-plan
