"""Bounded, clustered groundcover metadata without native assets or imports."""

import copy
import math
from pathlib import Path
import random
import sys
import unittest


REPO = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO / "Tools"))
import author_valley_terrain as terrain
import plan_valley_vegetation as vegetation

try:
    import plan_valley_groundcover as groundcover
except ModuleNotFoundError:
    groundcover = None


class ValleyGroundcoverTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(groundcover, "The full-map groundcover planner is missing")
        self.layout = terrain.load_layout(REPO / "Art/Maps/WoodedValley1600x1200.layout.json")

    def test_repeated_calls_preserve_layout_global_rng_and_bindings(self):
        bindings = {"grass": {"mesh_path": "/Game/Owned/Grass", "category": "grass",
                               "footprint_radius_m": 0.4, "material_path": "/Game/Owned/GrassMIC"}}
        before = copy.deepcopy((self.layout, bindings))
        state = random.getstate()
        first = groundcover.plan_groundcover(self.layout, seed=19, max_instances=100,
                                             mesh_bindings=bindings)
        second = groundcover.plan_groundcover(self.layout, seed=19, max_instances=100,
                                              mesh_bindings=bindings)
        self.assertEqual(first, second)
        self.assertEqual((self.layout, bindings), before)
        self.assertEqual(random.getstate(), state)
        self.assertNotEqual(first["instances"], groundcover.plan_groundcover(
            self.layout, seed=20, max_instances=100, mesh_bindings=bindings)["instances"])

    def test_transforms_use_legal_heights_and_original_scale_ranges(self):
        plan = groundcover.plan_groundcover(self.layout, max_instances=180)
        self.assertEqual(plan["extent_m"], [1600, 1200])
        self.assertEqual(plan["layout_name"], "WoodedValley1600x1200")
        self.assertEqual(len(plan["instances"]), 180)
        self.assertEqual(len(plan["coverage"]), 6)
        for record in plan["instances"]:
            x, y, z = record["position_m"]
            self.assertTrue(all(math.isfinite(v) for v in (x, y, z)))
            self.assertTrue(0.5 <= x <= 1599.5 and 0.5 <= y <= 1199.5)
            self.assertEqual(z, terrain.height_metres(self.layout, x, y))
            self.assertTrue(0 <= record["yaw_degrees"] < 360)
            self.assertTrue(0.8 <= record["scale"] <= 1.15)
            self.assertIn(record["category"], ("grass", "flowers", "shrubs"))
            self.assertEqual(record["role"], "groundcover")
            self.assertIsNone(record["mesh_path"])
            self.assertIsNone(record["binding_id"])
            self.assertIsNone(record["material_path"])
            self.assertNotIn("species", record)

    def test_scaled_footprints_do_not_overlap_reserved_features_or_existing_sample(self):
        bindings = {"bush": {"mesh_path": "/Game/Owned/Bush", "category": "shrubs",
                              "footprint_radius_m": 3}}
        plan = groundcover.plan_groundcover(self.layout, max_instances=180, mesh_bindings=bindings)
        for record in plan["instances"]:
            x, y, _ = record["position_m"]
            radius = record["footprint_radius_m"] * record["scale"]
            self.assertFalse(vegetation._reserved(self.layout, x, y, radius))
            dx = max(394 - x, 0, x - 460)
            dy = max(328 - y, 0, y - 392)
            self.assertGreater(math.hypot(dx, dy), radius)

    def test_bent_river_bank_and_sample_corner_are_protected(self):
        self.assertTrue(groundcover._reserved(self.layout, 876, 230, 20))
        self.assertTrue(groundcover._reserved(self.layout, 393.5, 327.5, 0.8))

    def test_slope_limit_samples_both_axes(self):
        plan = groundcover.plan_groundcover(self.layout, max_instances=90, max_slope_degrees=8)
        self.assertEqual(len(plan["instances"]), 90)
        for record in plan["instances"]:
            x, y, _ = record["position_m"]
            gx = terrain.height_metres(self.layout, x + 0.5, y) - terrain.height_metres(self.layout, x - 0.5, y)
            gy = terrain.height_metres(self.layout, x, y + 0.5) - terrain.height_metres(self.layout, x, y - 0.5)
            self.assertLessEqual(math.degrees(math.atan(math.hypot(gx, gy))), 8)

    def test_all_six_sectors_receive_clustered_groundcover(self):
        plan = groundcover.plan_groundcover(self.layout, max_instances=600)
        self.assertTrue(all(sector["count"] > 0 for sector in plan["coverage"]))
        self.assertEqual(sum(sector["count"] for sector in plan["coverage"]), 600)
        patch_by_id = {patch["id"]: patch for patch in plan["patches"]}
        for record in plan["instances"]:
            center = patch_by_id[record["patch_id"]]["center_m"]
            self.assertLessEqual(math.dist(record["position_m"][:2], center), 15)
        # The map remains meadow openings plus dense local patches, not a grid carpet.
        self.assertLessEqual(len(plan["patches"]), 400)
        self.assertLess(len(plan["patches"]) * math.pi * 15 ** 2, 1600 * 1200 * 0.16)

    def test_actual_binding_order_is_irrelevant_and_no_unbound_category_is_added(self):
        bindings = {
            "grass2": {"mesh_path": "/Game/Owned/Grass2.Grass2", "category": "grass", "footprint_radius_m": 0.5},
            "flower": {"mesh_path": "/Game/Owned/Flower", "category": "flowers", "footprint_radius_m": 0.3},
            "grass1": {"mesh_path": "/Game/Owned/Grass1", "category": "grass", "footprint_radius_m": 0.4},
        }
        first = groundcover.plan_groundcover(self.layout, max_instances=140, mesh_bindings=bindings)
        second = groundcover.plan_groundcover(self.layout, max_instances=140,
                                              mesh_bindings=dict(reversed(list(bindings.items()))))
        self.assertEqual(first, second)
        self.assertEqual({r["binding_id"] for r in first["instances"]}, set(bindings))
        self.assertNotIn("shrubs", {r["category"] for r in first["instances"]})
        for record in first["instances"]:
            self.assertEqual(record["mesh_path"], bindings[record["binding_id"]]["mesh_path"])

    def test_zero_and_small_budgets_are_exactly_bounded(self):
        for budget in (0, 1, 6, 17):
            plan = groundcover.plan_groundcover(self.layout, max_instances=budget)
            self.assertEqual(len(plan["instances"]), budget)
            self.assertLessEqual(plan["statistics"]["candidate_count"], 200000)
        self.assertEqual(groundcover.plan_groundcover(self.layout, max_instances=0)["patches"], [])

    def test_category_caps_and_candidate_cap_hold_for_the_largest_budget(self):
        plan = groundcover.plan_groundcover(self.layout, max_instances=120000)
        counts = plan["statistics"]["category_counts"]
        self.assertLessEqual(len(plan["instances"]), 71000)
        self.assertLessEqual(counts["grass"], 60000)
        self.assertLessEqual(counts["flowers"], 10000)
        self.assertLessEqual(counts["shrubs"], 1000)
        self.assertLessEqual(plan["statistics"]["candidate_count"], 200000)

    def test_invalid_parameters_are_rejected_before_sampling(self):
        for name, values in {
            "seed": (True, 0, -1, 1.5, 2 ** 32),
            "max_instances": (True, -1, 120001, 2.5),
            "max_slope_degrees": (True, -1, 36, math.nan, math.inf),
            "mesh_bindings": ([], {}, {"bad": "/Game/Owned/Grass"},
                              {1: {"mesh_path": "/Game/Owned/Grass", "category": "grass", "footprint_radius_m": 1},
                               "valid": {"mesh_path": "/Game/Owned/Grass", "category": "grass", "footprint_radius_m": 1}},
                              {"": {"mesh_path": "/Game/Owned/Grass", "category": "grass", "footprint_radius_m": 1}},
                              {"bad": {"mesh_path": "/Game/Bad Path", "category": "grass", "footprint_radius_m": 1}},
                              {"bad": {"mesh_path": "C:/secret.uasset", "category": "grass", "footprint_radius_m": 1}},
                              {"bad": {"mesh_path": "/Game/Owned/Grass", "category": "tree", "footprint_radius_m": 1}},
                              {"bad": {"mesh_path": "/Game/Owned/Grass", "category": [], "footprint_radius_m": 1}},
                              {"bad": {"mesh_path": "/Game/Owned/Grass", "category": "grass", "footprint_radius_m": math.nan}},
                              {"bad": {"mesh_path": "/Game/Owned/Grass", "category": "grass", "footprint_radius_m": 0}},
                              {"bad": {"mesh_path": "/Game/Owned/Grass", "category": "grass", "footprint_radius_m": 9}},
                              {"bad": {"mesh_path": "/Game/Owned/Grass", "category": "grass", "footprint_radius_m": 1,
                                       "material_path": "/Engine/Unapproved"}}),
        }.items():
            for value in values:
                with self.subTest(name=name, value=value), self.assertRaises(ValueError):
                    groundcover.plan_groundcover(self.layout, **{name: value})

    def test_binding_count_is_bounded_at_32(self):
        records = {f"g{index}": {"mesh_path": f"/Game/Owned/G{index}", "category": "grass",
                                  "footprint_radius_m": 0.4} for index in range(32)}
        self.assertLessEqual(len(groundcover.plan_groundcover(
            self.layout, max_instances=32, mesh_bindings=records)["instances"]), 32)
        records["extra"] = records["g0"]
        with self.assertRaises(ValueError):
            groundcover.plan_groundcover(self.layout, mesh_bindings=records)

    def test_historical_layout_is_not_reinterpreted_or_changed(self):
        old = terrain.load_layout(REPO / "Art/Maps/WoodedValley768.layout.json")
        before = copy.deepcopy(old)
        with self.assertRaises(ValueError):
            groundcover.plan_groundcover(old)
        self.assertEqual(old, before)


if __name__ == "__main__":
    unittest.main()
