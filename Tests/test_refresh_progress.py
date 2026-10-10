"""Run the progress renderer on temporary ledgers, never the checkout ledger."""

import copy
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest


REPO = Path(__file__).resolve().parents[1]
RENDERER = REPO / "Docs" / "refresh_progress.py"
LEDGER = REPO / "Docs" / "Requirements.json"


class RefreshProgressTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.base_ledger = json.loads(LEDGER.read_text(encoding="utf-8"))

    def render(self, repository, verification_runs=None, accepted=False, verify_recovery_targets=False):
        ledger = copy.deepcopy(self.base_ledger)
        ledger["repository"] = repository
        ledger["verification_runs"] = verification_runs or []
        if accepted:
            ledger["requirements"][0]["accepted"] = True
        with tempfile.TemporaryDirectory(prefix="refresh-progress-test-", dir=REPO.parent) as temporary:
            docs = Path(temporary) / "Docs"
            docs.mkdir()
            recovery = docs / "Recovery"
            recovery.mkdir()
            recovery_names = {"RECOVERY_MANIFEST.json", "MISSING_ASSETS.json"}
            for name in recovery_names:
                shutil.copy2(REPO / "Docs" / "Recovery" / name, recovery / name)
            shutil.copy2(RENDERER, docs / RENDERER.name)
            ledger_path = docs / "Requirements.json"
            ledger_path.write_text(json.dumps(ledger, ensure_ascii=False), encoding="utf-8")
            before = ledger_path.read_bytes()
            process = subprocess.run(
                [sys.executable, str(docs / RENDERER.name)],
                cwd=temporary,
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(process.returncode, 0, process.stdout + process.stderr)
            self.assertEqual(ledger_path.read_bytes(), before, "Rendering must not change the requirement ledger")
            progress = (docs / "PROGRESS.md").read_text(encoding="utf-8")
            if verify_recovery_targets:
                targets = [target for target in re.findall(r"\[[^\]]*\]\(([^)]+)\)", progress)
                           if Path(target).name in recovery_names]
                self.assertEqual(len(targets), 2, "Both recovery manifests must have generated links")
                for target in targets:
                    self.assertFalse(Path(target).is_absolute(), target)
                    self.assertTrue((docs / target).resolve().is_file(), f"Broken recovery link: {target}")
            return progress, ledger

    def test_verified_public_repository_does_not_claim_private(self):
        progress, _ = self.render({
            "url": "https://github.com/example/wildlight",
            "visibility": "public",
            "visibility_verified_date": "2026-10-10",
            "private_verified": False,
            "public_push_approved": True,
            "pushed": True,
            "hosted_checks_verified": False,
        })
        self.assertIn("[public GitHub repository](https://github.com/example/wildlight)", progress)
        self.assertIn("Public push is explicitly approved", progress)
        self.assertIn("source push has been verified", progress)
        self.assertNotIn("source/asset push", progress)
        self.assertNotIn("private-repository", progress)
        self.assertNotIn("Private remote creation/push", progress)
        self.assertNotIn("403", progress)

    def test_verified_private_repository_retains_private_status(self):
        progress, _ = self.render({
            "url": "https://github.com/example/private-wildlight",
            "private_verified": True,
            "pushed": False,
            "hosted_checks_verified": True,
        })
        self.assertIn("[private GitHub repository](https://github.com/example/private-wildlight)", progress)
        self.assertIn("private visibility was verified", progress)
        self.assertIn("first push remains pending", progress)
        self.assertIn("Hosted checks passed", progress)
        self.assertNotIn("Public push is explicitly approved", progress)

    def test_missing_repository_does_not_invent_visibility_or_permission_error(self):
        progress, _ = self.render({})
        self.assertIn("Repository visibility is unverified", progress)
        self.assertNotIn("[private GitHub repository]", progress)
        self.assertNotIn("[public GitHub repository]", progress)
        self.assertNotIn("403", progress)

    def test_reported_private_repository_does_not_claim_verified_visibility(self):
        progress, _ = self.render({
            "url": "https://github.com/example/reported-private",
            "visibility": "private",
            "private_verified": False,
        })
        self.assertIn("private GitHub repository", progress)
        self.assertIn("visibility is unverified", progress)
        self.assertNotIn("private visibility was verified", progress)

    def test_current_verification_is_ledger_driven_not_cloud_capability_claim(self):
        progress, _ = self.render({}, [{
            "summary": "Fresh Windows baseline: 26 scenarios, 452 assertions; CTest 2/2",
            "evidence_path": "Evidence/2026-10-10/core-baseline.json",
            "command": "ctest --test-dir build --output-on-failure",
        }])
        self.assertIn("26 scenarios, 452 assertions; CTest 2/2", progress)
        self.assertIn("Evidence/2026-10-10/core-baseline.json", progress)
        self.assertIn("[SESSION_HANDOFF.md](SESSION_HANDOFF.md)", progress)
        self.assertIn("Recovery/MISSING_ASSETS.json", progress)
        self.assertNotIn("This cloud can", progress)
        self.assertNotIn("checks are being assembled", progress)

    def test_missing_current_verification_does_not_relabel_historical_reports(self):
        progress, _ = self.render({})
        self.assertIn("No current verification run is recorded", progress)
        self.assertIn("Historical reports are not fresh verification", progress)
        self.assertNotIn("This cloud can", progress)

    def test_recovery_links_resolve_relative_to_generated_progress(self):
        self.render({}, verify_recovery_targets=True)

    def test_all_ids_states_and_acceptance_flags_are_preserved(self):
        progress, ledger = self.render({}, accepted=True)
        rows = re.findall(r"^\| `([^`]+)` \| .*? \| `([^`]+)` \|", progress, re.MULTILINE)
        expected = [(entry["id"], entry["status"]) for entry in ledger["requirements"]]
        self.assertEqual(rows, expected)
        self.assertEqual(len(rows), 105)
        self.assertIn(f"{sum(entry['accepted'] for entry in ledger['requirements'])} of 105 requirements", progress)
        self.assertNotIn("All 105 user-acceptance flags remain false", progress)

    def test_existing_acceptance_is_not_reset_by_synthetic_approval(self):
        self.base_ledger = copy.deepcopy(self.base_ledger)
        for entry in self.base_ledger["requirements"]:
            entry["accepted"] = False
        self.base_ledger["requirements"][1]["accepted"] = True
        progress, ledger = self.render({}, accepted=True)
        self.assertIn("2 of 105 requirements", progress)
        self.assertTrue(ledger["requirements"][1]["accepted"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
