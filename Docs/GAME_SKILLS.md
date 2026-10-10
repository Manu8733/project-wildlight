# Wildlight Game Development Skills

Installed at the user's explicit request on 10 October 2026, using the bundled Codex skill installer and pinned public revisions. These are selected community guides, not a claim that they are universally the best or official Epic/Blender/OpenAI integrations. The current official curated OpenAI catalog did not offer a directly relevant Unreal or game-design skill.

| Skill | Source Revision | License | Wildlight Use |
| --- | --- | --- | --- |
| `game-designer` | [edhahn/agent-skills](https://github.com/edhahn/agent-skills/tree/ffc6fc4bf1408bc3cac522916bb58e603f39ab45/game-designer) | MIT | Player-facing design, scope, trade-offs and explicit acceptance |
| `level-design` | [gamedev-skills](https://github.com/gamedev-skills/awesome-gamedev-agent-skills/tree/0a70cfc64672d512c5b5b6006b5dcb5cf90dbdbd/skills/disciplines/level-design) | Apache-2.0 | Whole-map blockout, routes, reachability and readable landmarks |
| `performance-optimization` | [gamedev-skills](https://github.com/gamedev-skills/awesome-gamedev-agent-skills/tree/0a70cfc64672d512c5b5b6006b5dcb5cf90dbdbd/skills/disciplines/performance-optimization) | Apache-2.0 | Repeatable CPU/GPU profiling before optimization |
| `unreal-cpp-gameplay` | [gamedev-skills](https://github.com/gamedev-skills/awesome-gamedev-agent-skills/tree/0a70cfc64672d512c5b5b6006b5dcb5cf90dbdbd/skills/unreal/unreal-cpp-gameplay) | Apache-2.0 | UE reflection, component ownership, GC and module boundaries |
| `unreal-niagara` | [gamedev-skills](https://github.com/gamedev-skills/awesome-gamedev-agent-skills/tree/0a70cfc64672d512c5b5b6006b5dcb5cf90dbdbd/skills/unreal/unreal-niagara) | Apache-2.0 | Missing environmental effects, exposed parameters and effect lifetime |
| `blender-workbench` | [1Pio/blender-workbench](https://github.com/1Pio/blender-workbench/tree/540a658915cd345748a7eb7cece6a57bc0e670cb) | MIT | Versioned editable scenes, native render evidence and export/geometry diagnostics |

## Installation And Limits

The six folders were installed under the desktop account's `$CODEX_HOME/skills`. All entry files were present and hashed after installation. They can be discovered automatically on the next turn; their instructions were also read directly for this continuation. Installation here does not copy them to another account or to the disconnected cloud executor. This repository records source pins and application rules, not copied skill packages.

The first five selected folders contain Markdown guidance/references and code examples, not executable support scripts. Blender Workbench includes two optional inspection scripts and tests. Its optional comparator may download pinned NumPy/OpenCV/Pillow dependencies through `uv`; no comparator, Blender probe, render, dependency installation or MCP reconfiguration was executed. Upstream Blender testing was on Apple Silicon, not a Wildlight Windows/Pivot Painter acceptance test. No skills provide Unreal, Blender, a GPU, assets, paid credits or a reconnection mechanism.

## Apply To This Map

- Translate level-design metrics to RTS unit collision, formation width, building footprints, depot exits and camera reach. Do not import platformer jumps, linear boss routes or keyed gates into the approved free-for-all map.
- Block out and validate all six clearings and permanent crossings before detailed dressing. Check each start's two usable exits, resource access and room for expansion; visual symmetry alone is not balance evidence.
- Use terrain and resource records for future logical integration. Decorative castle/start markers are not functional opponents. Keep gameplay in `Core/`; do not move economy/AI ownership into visual components.
- Preserve licensed foliage UVs, pivots, vertex colors and wind/material dependencies through adaptation. A generic geometry probe does not verify Pivot Painter, animation, render-only modifiers or Unreal import.
- Keep Niagara cosmetic in this weather increment, bound its lifetime and culling, and retain tactical readability. GPU particles do not decide gameplay damage or resource extraction.
- Profile the same warmed, reproducible scene and camera at each weather/day state. Record actual CPU/GPU/frame time and memory; examples and generic asset budgets in skills are not measured Wildlight results.
- Preserve editable masters and inspect actual exported files and native renders. Tiny Glade remains a mood reference, not a geometry-copy or pixel-matching target. Never upload NoAI assets to generative services.

The project instructions, source policy, approved design and verified tool availability override any incompatible generic example in these guides.
