# Milestones and definition of done

These are dependency gates, not calendar promises. A sprint is a small reviewable batch with behavior, evidence and acceptance criteria. The 105 requirements in `Requirements.json` remain the scope ledger; completed preparatory work does not erase GPU requirements.

| Milestone | Sprint focus | Exit evidence |
| --- | --- | --- |
| M0 — Reproducible foundation | Repository, locked engine, portable core, standards, asset source | Clean clone; cloud tests; backup restore; Windows toolchain check pending until run |
| M1 — First complete skirmish | Camera, selection, placement, wood/food logistics, recruitment, AI, win/loss | One editor and packaged match with save/load and controls; no blocker defects |
| M2 — Distinctive network | Beacon reachability, visible economy consequence, sabotage, bridge and roots | Recorded network cut/reconnect test; legal and expired path tests; player understands the decision |
| M3 — Integrated visual slice | One authored valley, imported original models, animation, VFX, sound, HUD | In-engine captures; laptop frame-time trace; independent playtest; license inventory |
| M4 — Single-player breadth | Remaining maps and roster, three magic schools, personalities, tutorial and accessibility | Content acceptance matrix; seeded AI results; save compatibility policy; onboarding tests |
| M5 — Release candidate QA | Stress, long-session, clean install, Shipping, rollback and evidence package | Verified Windows Shipping package; measured system tiers; regression and defect closure |

Steam remains deferred until the user returns to List 3. No publishing milestone, storefront text or upload task is activated here.

## Sprint operating rules

Choose one player outcome and its necessary dependencies. Example: "A raid cuts the farm's light bonus; restoring the beacon restores that bonus." Include simulation validation, clear HUD feedback and one manual engine test in the same sprint. Keep a short issue record with requirement IDs, files, steps, expected behavior, evidence and unresolved risk.

Code review checks command ownership, resource conservation, stale IDs, predictable save behavior and presentation separation. Art review checks silhouette and original source before expensive polish. Only widen tests when new work or failure makes it useful.

## Definition of done

The user's original brief says a task is done only after verification, testing and acceptance. Therefore three independent fields are retained: implementation status, verification evidence, and user acceptance. A cloud-verified logic task may still need engine integration. No box in the original brief is checked automatically.

For a gameplay task: accepted command and rejected edge cases, behavior verified in available tests, English feedback, relevant save compatibility, documented limitations, and integrated engine evidence where applicable. For an art task: editable source, provenance, export/import validation, readable in-engine result and applicable animation/LOD/collision checks. For a hardware task: a measured run on the named actual machine, never a target specification.

An accepted milestone has no crash, resource duplication, unfinishable match, corrupt-load mutation or blocked tutorial. Remaining cosmetic defects are recorded with explicit acceptance, not silently treated as finished.
