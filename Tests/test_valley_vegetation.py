"""Original bounded tree placement metadata, without an engine or asset files."""

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


class ValleyVegetationTests(unittest.TestCase):
    def setUp(self):
        self.layout = terrain.load_layout(REPO / "Art/Maps/WoodedValley1600x1200.layout.json")

    def test_repeated_calls_are_deterministic_without_mutating_layout_or_global_rng(self):
        original = copy.deepcopy(self.layout)
        state = random.getstate()
        first = vegetation.plan_trees(self.layout, seed=19, max_instances=80)
        second = vegetation.plan_trees(self.layout, seed=19, max_instances=80)
        self.assertEqual(first, second)
        self.assertEqual(self.layout, original)
        self.assertEqual(random.getstate(), state)
        self.assertNotEqual(first["instances"], vegetation.plan_trees(
            self.layout, seed=20, max_instances=80)["instances"])

    def test_records_have_bounded_finite_world_transforms_and_no_invented_species(self):
        plan = vegetation.plan_trees(self.layout, max_instances=150)
        self.assertEqual(plan["extent_m"], [1600, 1200])
        self.assertEqual(plan["layout_name"], "WoodedValley1600x1200")
        self.assertEqual(len(plan["instances"]), 150)
        for record in plan["instances"]:
            x, y, z = record["position_m"]
            self.assertTrue(all(math.isfinite(n) for n in (x, y, z)))
            self.assertGreaterEqual(x, 4.8)
            self.assertLessEqual(x, 1595.2)
            self.assertGreaterEqual(y, 4.8)
            self.assertLessEqual(y, 1195.2)
            self.assertEqual(z, terrain.height_metres(self.layout, x, y))
            self.assertTrue(0 <= record["yaw_degrees"] < 360)
            self.assertTrue(0.8 <= record["scale"] <= 1.2)
            self.assertEqual(record["role"], "tree")
            self.assertIsNone(record["binding_id"])
            self.assertIsNone(record["mesh_path"])

    def test_clearings_roads_river_and_deck_footprints_include_scaled_canopy_clearance(self):
        plan = vegetation.plan_trees(self.layout, max_instances=250, canopy_radius_m=7)
        river = self.layout["terrain"]["river"]
        for record in plan["instances"]:
            x, y, _ = record["position_m"]
            canopy = 7 * 1.2
            for clearing in self.layout["clearings"]:
                self.assertGreater(math.dist([x, y], clearing["center_m"]),
                                   clearing["radius_m"] + clearing["blend_margin_m"] + canopy)
            for route in self.layout["routes"]:
                self.assertGreater(terrain._route_distance(route, x, y),
                                   route["width_m"] / 2 + route["blend_margin_m"] + canopy)
            self.assertGreater(abs(x - terrain._river_x(river, y)),
                               river["bank_width_m"] / 2 + canopy)
            for deck in self.layout["crossings"]:
                self.assertTrue(abs(x - deck["center_m"][0]) > deck["span_m"] / 2 + canopy or
                                abs(y - deck["center_m"][1]) > deck["width_m"] / 2 + canopy)

    def test_minimum_spacing_is_preserved_between_distinct_instances(self):
        instances = vegetation.plan_trees(self.layout, min_spacing_m=15,
                                          max_instances=300)["instances"]
        self.assertEqual(len(instances), 300)
        for index, record in enumerate(instances):
            for other in instances[index + 1:]:
                self.assertGreaterEqual(math.dist(record["position_m"][:2],
                                                 other["position_m"][:2]), 15)

    def test_canopy_cannot_overlap_a_bent_bank_at_an_adjacent_river_row(self):
        # At Y=230 the river centre is X=820. A crown at X=876 clears
        # that row's bank but overlaps the bank at Y=222, so it is reserved.
        self.assertTrue(vegetation._reserved(self.layout, 876, 230, 20))

    def test_slope_limit_uses_actual_height_field_in_both_axes(self):
        instances = vegetation.plan_trees(self.layout, max_slope_degrees=10,
                                          max_instances=100)["instances"]
        self.assertEqual(len(instances), 100)
        for record in instances:
            x, y, _ = record["position_m"]
            gx = terrain.height_metres(self.layout, x + 0.5, y) - terrain.height_metres(self.layout, x - 0.5, y)
            gy = terrain.height_metres(self.layout, x, y + 0.5) - terrain.height_metres(self.layout, x, y - 0.5)
            self.assertLessEqual(math.degrees(math.atan(math.hypot(gx, gy))), 10)

    def test_budgets_and_zero_density_do_not_emit_extra_instances(self):
        for budget in (0, 1, 12):
            self.assertEqual(len(vegetation.plan_trees(self.layout, max_instances=budget)["instances"]), budget)
        self.assertEqual(vegetation.plan_trees(self.layout, density=0)["instances"], [])

    def test_sampling_spreads_across_all_six_map_sectors_and_is_not_an_even_grid(self):
        points = [r["position_m"] for r in vegetation.plan_trees(
            self.layout, max_instances=300)["instances"]]
        sectors = {(min(2, int(x / (1600 / 3))), int(y / 600)) for x, y, _ in points}
        self.assertEqual(sectors, {(x, y) for x in range(3) for y in range(2)})
        self.assertGreater(len({round(x % 8, 3) for x, _, _ in points}), 30)

    def test_actual_mesh_bindings_are_optional_and_order_independent(self):
        bindings = {"large": "/Game/Owned/Trees/Large", "small": "/Game/Owned/Trees/Small.Small"}
        first = vegetation.plan_trees(self.layout, max_instances=40, mesh_bindings=bindings)
        second = vegetation.plan_trees(self.layout, max_instances=40,
                                        mesh_bindings=dict(reversed(list(bindings.items()))))
        self.assertEqual(first, second)
        self.assertEqual({r["binding_id"] for r in first["instances"]}, set(bindings))
        for record in first["instances"]:
            self.assertEqual(record["mesh_path"], bindings[record["binding_id"]])

    def test_invalid_parameters_and_nonapproved_extents_fail_before_generation(self):
        for name, values in {
            "seed": (True, -1, 1.5, 2 ** 32),
            "density": (True, -0.1, 1.01, math.nan, math.inf),
            "min_spacing_m": (True, 0, 3.99, 81, math.inf),
            "canopy_radius_m": (False, 0, 21, math.nan),
            "max_slope_degrees": (True, -1, 36, math.inf),
            "max_instances": (True, -1, 10001, 2.5),
            "mesh_bindings": ([], {}, {"tree": "C:/private/tree.uasset"},
                              {"tree": "/Game/Trees/Bad Path"}, {"": "/Game/Trees/Tree"}),
        }.items():
            for value in values:
                with self.subTest(name=name, value=value), self.assertRaises(ValueError):
                    vegetation.plan_trees(self.layout, **{name: value})
        invalid = copy.deepcopy(self.layout)
        invalid["extent_m"] = [1600, 1600]
        with self.assertRaises(ValueError):
            vegetation.plan_trees(invalid)

    def test_historical_profile_is_unchanged_and_not_silently_reinterpreted(self):
        historical = terrain.load_layout(REPO / "Art/Maps/WoodedValley768.layout.json")
        original = copy.deepcopy(historical)
        heights = [terrain.height_metres(historical, x, y) for x, y in ((0, 0), (200, 300), (768, 768))]
        with self.assertRaises(ValueError):
            vegetation.plan_trees(historical)
        self.assertEqual(historical, original)
        self.assertEqual(heights, [terrain.height_metres(historical, x, y)
                                  for x, y in ((0, 0), (200, 300), (768, 768))])


if __name__ == "__main__":
    unittest.main()
