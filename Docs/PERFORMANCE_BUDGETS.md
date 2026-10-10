# Performance targets and measurement contract

These numbers are development budgets. They are not benchmark results or published minimum specifications. A portable CPU simulation timing cannot establish Unreal GPU performance.

## Named machines

Available user laptop: Intel i7 12th generation, RTX 3050 Laptop **4 GB VRAM**, **16 GB RAM**. Exact CPU model, GPU power limit, cooling, driver version and Windows build are not yet measured. Use plugged-in power and report them before comparing runs.

A future Windows workstation target is an RTX 4070-class or stronger GPU, at least 12 GB VRAM, 32–64 GB RAM and sufficient NVMe free space for Unreal, shader cache and backups. It is a selection guideline, not an acquired or rented machine. There is no approved rental spend and no remote GPU session in the current cloud.

| Budget | Laptop Low target | Workstation High target |
| --- | --- | --- |
| Test resolution | 1280×720, then 1920×1080 comparison | 1920×1080 |
| Sustained frame rate | 30 FPS at 720p during the accepted slice | 60 FPS during representative gameplay |
| 95th-percentile frame time | ≤33.3 ms in the controlled slice | ≤16.7 ms in the controlled slice |
| VRAM budget | ≤3.2 GB observed dedicated usage | ≤80% of measured VRAM |
| System RAM budget | ≤10 GB game plus OS headroom, measure both | Define after first actual profile |
| Simulation update | 10 Hz; target ≤4 ms per update at 100 units | Measure 100/200/500 supported entities |
| Path/AI time | Report per phase and per replan burst | Same scene, same seed and commands |
| Draw calls | Initial planning ceiling 600 at Low | Revise after integrated scene profiling |

The original 60 FPS ambition is a future measured reference-PC goal; it is not guaranteed for a 4 GB laptop. A target failure triggers density/graphics tuning and a repeated capture, not a relabeled result.

## Initial content budgets

Buildings: approximately 1–5k triangles for ordinary buildings, up to 12k for the command center at LOD0; smaller LODs must reduce silhouette-neutral detail. Units: 0.5–2k triangles at LOD0, shared role materials, low-cost distant representations. Repeated trees: approximately 300–1,200 triangles per near tree plus distant clusters. These guidelines are not measured counts of the generated kit.

Low uses conservative texture sizes, normally 512–1024 shared atlases; unique 4k textures are excluded from the slice baseline. Minimize independent material slots. Instance repeated static objects and cull decorative props outside view. Spell gameplay indicators remain visible when decorative particle density is zero.

## Graphics tiers to verify

| Feature | Low | Medium | High/Epic candidates |
| --- | --- | --- | --- |
| Dynamic GI | Off / simple ambient lighting | Compare affordable alternative | Lumen only after a measured comparison |
| Nanite | Off | Optional static scene experiment | Compare Nanite benefits and cost |
| Shadows | Short distance, modest resolution | Moderate | Larger distance; tested for cost |
| Vegetation | Sparse props and aggressive culling | Moderate | More density within CPU/GPU budget |
| Ambient particles | Reduced or disabled | Moderate | Capped density |
| Spell indicators | Full tactical information | Full tactical information | Full tactical information |

Default renderer choices should match actual `Config/` settings; these are acceptance experiments for additional tiers. "Epic" preset availability in Unreal is not proof that the game supports it well.

## Measurement procedure

Record build commit, engine version, map, seed, resolution, preset, power mode, drivers, average/median/95th-percentile frame time, minimum sustained FPS, CPU/game/render/GPU time, RAM/VRAM, active unit/building count and trace duration. Warm shaders first; keep shader compilation outside a steady-state FPS claim and report it separately.

Use Unreal Insights plus engine GPU/frame tools. Capture empty start, developed economy, network rebuild, dense gate movement, simultaneous bridge expiry, siege and 100/200/500-unit stress where supported. Separate total entities from combat units. Use the same scripted orders for comparisons. Keep raw traces outside Git when large, index their filenames and checksums in the QA evidence package.
