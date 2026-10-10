#!/usr/bin/env python3
"""Render evidence status from the manually reviewed requirement ledger.

This never grants acceptance or infers implementation from file existence.
"""
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
accepted_count = sum(r["accepted"] for r in records)
repository = data.get("repository") or {}
visibility = repository.get("visibility")
if visibility not in ("public", "private") and repository.get("private_verified"):
    visibility = "private"
repository_label = f"{visibility} GitHub repository" if visibility in ("public", "private") else "GitHub repository"
repository_url = repository.get("url")
repository_reference = f"[{repository_label}]({repository_url})" if repository_url else repository_label
repository_notes = [f"The {repository_reference} is recorded."] if repository else []
if visibility == "private" and repository.get("private_verified"):
    repository_notes.append("Its private visibility was verified.")
elif visibility in ("public", "private") and repository.get("visibility_verified_date"):
    repository_notes.append(f"Its {visibility} visibility was verified on {repository['visibility_verified_date']}.")
else:
    repository_notes.append("Repository visibility is unverified.")
if visibility == "public":
    repository_notes.append(
        "Public push is explicitly approved by the user."
        if repository.get("public_push_approved") else "Public push approval is not recorded."
    )
repository_notes.append(
    "The source push has been verified."
    if repository.get("pushed") else "The first push remains pending or unverified."
)
repository_notes.append(
    "Hosted checks passed as recorded in the ledger."
    if repository.get("hosted_checks_verified") else "Hosted workflow execution is not yet verified."
)
lines = [
    "# Wildlight implementation evidence",
    "",
    "**The requested work is not fully complete.** The first two original lists contain **72 and 33 tasks (105 total)**; the declared totals match the parsed checkboxes. List 3 / Steam is deferred. "
    f"Explicit user acceptance is recorded for {accepted_count} of {len(records)} requirements; rendering never changes acceptance flags.",
    "",
    "Read the current [SESSION_HANDOFF.md](SESSION_HANDOFF.md) for the continuation context and environment limits. "
    "[Recovery/RECOVERY_MANIFEST.json](Recovery/RECOVERY_MANIFEST.json) describes the original recovered snapshot; "
    "[Recovery/MISSING_ASSETS.json](Recovery/MISSING_ASSETS.json) records its missing assets, not completed content. "
    "Historical reports are not fresh verification. Current verification is recorded below from the ledger. "
    "Source/catalog entries are not proof of completed final content or engine validation.",
    "",
    " ".join(repository_notes),
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
    lines.append("No current verification run is recorded in the ledger. No passing baseline is claimed without its exact command and evidence path.")
lines += [
    "",
    "## Outstanding gates",
    "",
    "- Compile Unreal 5.8.1 on Windows; run importer, editor match and actual Shipping package.",
    "- Finish and review unit rigs/animation, final UV/texture/LOD/collision, VFX/audio, complete tutorial/remapping/accessibility and all polished scenarios.",
    "- Measure 100/200/500-unit scenes where supported and long sessions on an actual GPU; regress RTX 3050 Low.",
    "- Complete fair-AI and economy/class tuning, independent playtest and explicit acceptance.",
    "- Keep repository visibility, push and hosted-check evidence current; copy a verified backup to independent owner-controlled storage.",
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
(DOCS / "PROGRESS.md").write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")
print(f"Rendered {len(records)} tracked tasks; accepted {sum(r['accepted'] for r in records)}.")
