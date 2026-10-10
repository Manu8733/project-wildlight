#!/usr/bin/env python3
"""Render evidence status from the manually reviewed requirement ledger.

This never grants acceptance or infers implementation from file existence.
"""
from collections import Counter
from pathlib import Path
import json

DOCS = Path(__file__).resolve().parent
data = json.loads((DOCS / "Requirements.json").read_text(encoding="utf-8"))
records = data["requirements"]
assert len(records) == 105
assert len({r["id"] for r in records}) == 105
assert sum(r["list"] == 1 for r in records) == 72
assert sum(r["list"] == 2 for r in records) == 33
assert all(isinstance(r["accepted"], bool) for r in records)
assert all(r["status"] in data["status_definitions"] for r in records)
lines = [
    "# Wildlight implementation evidence",
    "",
    "**The requested work is not fully complete.** The first two original lists contain **72 and 33 tasks (105 total)**; the declared totals match the parsed checkboxes. List 3 / Steam is deferred. All 105 user-acceptance flags remain false until explicit acceptance is recorded.",
    "",
    "This cloud can compile the portable C++ simulation, validate scripts/assets and produce original Blender previews. It cannot verify Unreal APIs, Windows Editor/Shipping builds, integrated GPU visuals, laptop performance or independent playtests. Source/catal og entries are not proof of completed final content.",
    "",
    (
        "The [private GitHub repository](" + data["repository"]["url"] + ") exists and its private visibility was verified. "
        + ("The initial source/asset push has been verified. " if data["repository"].get("pushed") else "Local Git/LFS setup is ready; first push remains pending. ")
        + ("Hosted checks passed as recorded below." if data["repository"].get("hosted_checks_verified") else "Hosted workflow execution is not yet verified.")
        if data.get("repository", {}).get("private_verified")
        else "Private remote creation/push is unverified. Earlier integration creation attempts were rejected with 403 — Resource not accessible by integration; local Git/LFS setup is prepared."
    ),
    "",
    "## Status totals",
    "",
    "| Evidence state | List 1 | List 2 | Total |",
    "| --- | ---: | ---: | ---: |",
]
for status in data["status_definitions"]:
    a = sum(r["list"] == 1 and r["status"] == status for r in records)
    b = sum(r["list"] == 2 and r["status"] == status for r in records)
    lines.append(f"| `{status}` | {a} | {b} | {a+b} |")
lines += [
    "",
    "A `VerifiedCloud` row covers only the recorded document, configuration, asset or portable-logic scope. It does not imply full composite-task completion, engine verification or user acceptance. `scope_coverage` and `remaining` in [Requirements.json](Requirements.json) make those limits explicit.",
    "",
    "## Verification record",
    "",
]
for entry in data.get("verification_runs", []):
    lines.append(f"- {entry['summary']} Evidence: `{entry['evidence_path']}`. Command: `{entry['command']}`.")
if not data.get("verification_runs"):
    lines.append("Core/art/backup checks are being assembled. No final passing report is claimed until the exact command and output are recorded here and in the ledger.")
lines += [
    "",
    "## Outstanding gates",
    "",
    "- Compile Unreal 5.8.1 on Windows; run importer, editor match and actual Shipping package.",
    "- Finish and review unit rigs/animation, final UV/texture/LOD/collision, VFX/audio, complete tutorial/remapping/accessibility and all polished scenarios.",
    "- Measure 100/200/500-unit scenes where supported and long sessions on an actual GPU; regress RTX 3050 Low.",
    "- Complete fair-AI and economy/class tuning, independent playtest and explicit acceptance.",
    "- Confirm private-repository push/hosted checks and copy verified backup to independent owner-controlled storage.",
    "",
    "## Full two-list traceability",
    "",
    "Every original item is retained below; detailed remaining work, original Romanian wording, source line, commands and evidence paths are in the JSON ledger. No original checkbox has been auto-checked.",
    "",
]
for list_number in (1, 2):
    lines += [f"### List {list_number}", "", "| ID | Requirement | State | Coverage |", "| --- | --- | --- | --- |"]
    for r in records:
        if r["list"] == list_number:
            lines.append(f"| `{r['id']}` | {r['title_en']} | `{r['status']}` | {r['scope_coverage']} |")
    lines.append("")
(DOCS / "PROGRESS.md").write_text("\n".join(lines).replace("catal og", "catalog") + "\n", encoding="utf-8")
print(f"Rendered {len(records)} tracked tasks; accepted {sum(r['accepted'] for r in records)}.")
