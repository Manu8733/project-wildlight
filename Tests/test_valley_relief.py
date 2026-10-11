"""Grouped original relief and gentle raised-water banks, without Unreal."""

import copy
import hashlib
import math
from pathlib import Path
import sys
import unittest


REPO = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO / "Tools"))
import author_valley_terrain as terrain
import author_valley_water as water


class ValleyReliefTests(unittest.TestCase):
    def setUp(self):
        self.layout = terrain.load_layout(REPO / "Art/Maps/WoodedValley1600x1200.layout.json")

    def test_fixed_grouped_massifs_and_two_small_hills_replace_the_closed_rim(self):
        clusters = self.layout["terrain"]["mountains"].get("clusters", [])
        self.assertEqual(len(clusters), 10, "Rectangular relief needs ten authored groups, not a closed rim")
        self.assertEqual(len([c for c in clusters if c["height_m"] > 15]), 8)
        self.assertEqual(len([c for c in clusters if c["height_m"] <= 15]), 2)
        self.assertEqual(len({tuple(c["center_m"]) for c in clusters}), 10)
        for cluster in clusters:
            center = terrain.height_metres(self.layout, *cluster["center_m"])
            self.assertGreater(center, 8 + cluster["height_m"] - 1)
        for point in ((0, 300), (0, 830), (300, 0), (650, 0),
                      (1600, 920), (800, 1200), (900, 1200)):
            with self.subTest(point=point):
                self.assertLess(terrain.height_metres(self.layout, *point), 11,
                                "Peripheral gaps must remain genuinely low")

    def test_water_is_raised_without_flooding_bases_or_separate_decks(self):
        river = self.layout["terrain"]["river"]
        self.assertEqual(river["bed_height_m"], 2.5)
        self.assertEqual(river["water_surface_m"], 5.5)
        self.assertEqual((river["bed_width_m"], river["bank_width_m"]), (32, 70))
        samples = water.sample_river(self.layout)
        self.assertAlmostEqual(samples[0]["depth_m"], 3)
        self.assertAlmostEqual(samples[-1]["depth_m"], 1.8)
        for deck in self.layout["crossings"]:
            x, y = deck["center_m"]
            self.assertEqual(deck["deck_height_m"], 8)
            self.assertLess(terrain.height_metres(self.layout, x, y), 5.5)
            self.assertGreater(deck["deck_height_m"] - river["water_surface_m"], 2)
        for clearing in self.layout["clearings"]:
            x, y = clearing["center_m"]
            for dx in range(-100, 101, 20):
                for dy in range(-100, 101, 20):
                    if math.hypot(dx, dy) <= 110:
                        self.assertEqual(terrain.height_metres(self.layout, x + dx, y + dy), 8)

    def test_bank_and_open_plain_gradients_stay_under_thirty_degrees(self):
        limit = math.tan(math.radians(30))
        river = self.layout["terrain"]["river"]
        delta = 0.1
        for y in range(1, 1200, 20):
            center = terrain._river_x(river, y)
            for offset in range(-40, 41):
                x = center + offset
                gx = (terrain.height_metres(self.layout, x + delta, y) -
                      terrain.height_metres(self.layout, x - delta, y)) / (2 * delta)
                gy = (terrain.height_metres(self.layout, x, y + delta) -
                      terrain.height_metres(self.layout, x, y - delta)) / (2 * delta)
                self.assertLess(math.hypot(gx, gy), limit, (x, y))
        clusters = self.layout["terrain"]["mountains"].get("clusters", [])
        for y in range(20, 1200, 40):
            for x in range(20, 1600, 40):
                if any(c["height_m"] > 15 and
                       math.hypot((x - c["center_m"][0]) / c["radii_m"][0],
                                  (y - c["center_m"][1]) / c["radii_m"][1]) < 1
                       for c in clusters):
                    continue
                gx = (terrain.height_metres(self.layout, x + delta, y) -
                      terrain.height_metres(self.layout, x - delta, y)) / (2 * delta)
                gy = (terrain.height_metres(self.layout, x, y + delta) -
                      terrain.height_metres(self.layout, x, y - delta)) / (2 * delta)
                self.assertLess(math.hypot(gx, gy), limit, (x, y))

    def test_group_boundaries_are_continuous_without_periodic_edge_walls(self):
        clusters = self.layout["terrain"]["mountains"].get("clusters", [])
        self.assertTrue(clusters, "Authored finite-support massifs are missing")
        for cluster in clusters:
            cx, cy = cluster["center_m"]
            rx, ry = cluster["radii_m"]
            for point in ((cx + rx, cy), (cx - rx, cy), (cx, cy + ry), (cx, cy - ry)):
                x, y = point
                if 0.001 < x < 1599.999 and 0.001 < y < 1199.999:
                    h = terrain.height_metres(self.layout, x, y)
                    self.assertLess(abs(terrain.height_metres(self.layout, x + 0.0001, y) - h), 0.001)
                    self.assertLess(abs(terrain.height_metres(self.layout, x, y + 0.0001) - h), 0.001)

    def test_cluster_edits_reject_unbounded_values_and_protected_geometry(self):
        valid = copy.deepcopy(self.layout)
        valid["terrain"]["mountains"]["clusters"] = [
            {"id": "safe", "center_m": [35, 550], "radii_m": [110, 210], "height_m": 58}
        ]
        terrain._validate_layout(valid)
        for mutation in ("nonfinite", "outside", "tiny-radius", "huge-radius", "huge-height",
                         "duplicate", "too-many", "clearing", "route", "river"):
            edited = copy.deepcopy(valid)
            cluster = edited["terrain"]["mountains"]["clusters"][0]
            if mutation == "nonfinite":
                cluster["height_m"] = math.inf
            elif mutation == "outside":
                cluster["center_m"] = [-1, 550]
            elif mutation == "tiny-radius":
                cluster["radii_m"][0] = 0
            elif mutation == "huge-radius":
                cluster["radii_m"][0] = 10000
            elif mutation == "huge-height":
                cluster["height_m"] = 5000
            elif mutation == "duplicate":
                edited["terrain"]["mountains"]["clusters"].append(copy.deepcopy(cluster))
            elif mutation == "too-many":
                edited["terrain"]["mountains"]["clusters"] = [
                    {**cluster, "id": str(i)} for i in range(11)]
            elif mutation == "clearing":
                cluster["center_m"] = [250, 250]
            elif mutation == "route":
                cluster["center_m"] = [520, 520]
            else:
                cluster["center_m"] = [800, 460]
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                terrain._validate_layout(edited)

    def test_historical_profile_keeps_exact_layout_and_height_bytes(self):
        path = REPO / "Art/Maps/WoodedValley768.layout.json"
        self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(),
                         "6b37d0abb9e506adb677f8f7558ea9877cd13f19eafbcee8a1589d84007e4944")
        historical = terrain.load_layout(path)
        self.assertNotIn("clusters", historical["terrain"]["mountains"])
        edited = copy.deepcopy(historical)
        edited["terrain"]["mountains"]["clusters"] = [
            {"id": "safe", "center_m": [35, 550], "radii_m": [110, 210], "height_m": 58}
        ]
        with self.assertRaises(ValueError):
            terrain._validate_layout(edited)


if __name__ == "__main__":
    unittest.main()
