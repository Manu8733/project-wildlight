"""Original river samples match the validated analytic rectangular terrain."""

import copy
import math
from pathlib import Path
import sys
import unittest


REPO = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO / "Tools"))
import author_valley_terrain as terrain
try:
    import author_valley_water as water
except ModuleNotFoundError as error:
    if error.name != "author_valley_water":
        raise
    water = None


class ValleyWaterTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(water, "Original river sampling utility is missing")
        self.layout = terrain.load_layout(REPO / "Art/Maps/WoodedValley1600x1200.layout.json")

    def test_endpoints_and_exact_authored_knots_survive_non_dividing_step(self):
        samples = water.sample_river(self.layout, step_m=73)
        rows = [sample["center_m"][1] for sample in samples]
        self.assertEqual(rows, sorted(set(rows)))
        self.assertEqual(samples[0]["center_m"], [800, 0, 5.5])
        self.assertEqual(samples[-1]["center_m"], [800, 1200, 5.5])
        self.assertTrue(set(range(0, 1201, 73)).issubset(rows))
        centers = {sample["center_m"][1]: sample["center_m"] for sample in samples}
        for x, y in self.layout["terrain"]["river"]["centerline_m"]:
            self.assertEqual(centers[y], [x, y, 5.5])

    def test_centerline_retains_smoothstep_bends_instead_of_linear_chords(self):
        centers = {sample["center_m"][1]: sample["center_m"][0]
                   for sample in water.sample_river(self.layout, step_m=40)}
        for y, expected_x in ((40, 806.25), (80, 820), (120, 833.75), (160, 840)):
            with self.subTest(y=y):
                self.assertAlmostEqual(centers[y], expected_x, places=10)

    def test_default_step_spans_the_entire_channel(self):
        samples = water.sample_river(self.layout)
        self.assertEqual([sample["center_m"][1] for sample in samples], list(range(0, 1201, 10)))

    def test_floating_step_roundoff_cannot_create_an_out_of_bounds_endpoint(self):
        try:
            samples = water.sample_river(self.layout, step_m=1200 / 1101)
        except ValueError as error:
            self.fail(f"A valid finite step must remain within the map: {error}")
        rows = [sample["center_m"][1] for sample in samples]
        self.assertTrue(all(0 <= y <= 1200 for y in rows))
        self.assertEqual(rows[-1], 1200)
        self.assertEqual(rows.count(1200), 1)

    def test_cross_sections_are_finite_in_bounds_and_have_actual_depth(self):
        river = self.layout["terrain"]["river"]
        for sample in water.sample_river(self.layout):
            with self.subTest(y=sample["center_m"][1]):
                self.assertEqual(set(sample), {"center_m", "left_m", "right_m", "width_m", "depth_m"})
                center, left, right = (sample[key] for key in ("center_m", "left_m", "right_m"))
                for point in (center, left, right):
                    self.assertEqual(len(point), 3)
                    self.assertTrue(all(math.isfinite(value) for value in point))
                    self.assertTrue(0 <= point[0] <= 1600 and 0 <= point[1] <= 1200)
                    self.assertEqual(point[1:], center[1:])
                self.assertLess(left[0], center[0])
                self.assertLess(center[0], right[0])
                self.assertTrue(math.isfinite(sample["width_m"]))
                self.assertTrue(math.isfinite(sample["depth_m"]))
                self.assertGreater(sample["width_m"], river["bed_width_m"])
                self.assertLess(sample["width_m"], river["bank_width_m"])
                self.assertAlmostEqual(sample["width_m"], right[0] - left[0], places=10)
                self.assertGreater(sample["depth_m"], 0)
                self.assertAlmostEqual(sample["depth_m"], 3 - 0.001 * center[1], places=10)
                self.assertAlmostEqual(sample["depth_m"], center[2] - terrain.height_metres(
                    self.layout, center[0], center[1]), places=10)

    def test_each_shore_intersects_terrain_at_the_water_surface(self):
        river = self.layout["terrain"]["river"]
        for sample in water.sample_river(self.layout):
            x, y, level = sample["center_m"]
            for key, sign in (("left_m", -1), ("right_m", 1)):
                with self.subTest(y=y, edge=key):
                    self.assertAlmostEqual(terrain.height_metres(self.layout, *sample[key][:2]),
                                           level, delta=0.000001)
                    self.assertLess(terrain.height_metres(self.layout, x + sign * river["bed_width_m"] / 2, y),
                                    level)
                    self.assertGreater(terrain.height_metres(self.layout, x + sign * river["bank_width_m"] / 2, y),
                                       level)

    def test_shores_use_independent_asymmetric_terrain_intersections(self):
        differences = []
        for sample in water.sample_river(self.layout):
            center_x = sample["center_m"][0]
            differences.append(abs((center_x - sample["left_m"][0]) -
                                   (sample["right_m"][0] - center_x)))
        self.assertGreater(max(differences), 0.0001)

    def test_sampling_is_deterministic_without_mutating_the_layout(self):
        original = copy.deepcopy(self.layout)
        first = water.sample_river(self.layout, step_m=17.25)
        self.assertEqual(first, water.sample_river(self.layout, step_m=17.25))
        self.assertEqual(self.layout, original)
        first[0]["center_m"][0] = -1
        first[0]["left_m"][0] = -2
        self.assertEqual(self.layout, original)
        self.assertEqual(water.sample_river(self.layout, step_m=17.25)[0]["center_m"], [800, 0, 5.5])

    def test_channel_continues_away_from_routes_and_under_separate_decks(self):
        samples = {sample["center_m"][1]: sample for sample in water.sample_river(self.layout)}
        self.assertEqual(samples[80]["center_m"], [820, 80, 5.5])
        for y in (300, 650, 950):
            self.assertEqual(samples[y]["center_m"], [800, y, 5.5])
            self.assertGreater(samples[y]["depth_m"], 0)
            self.assertLess(terrain.height_metres(self.layout, 800, y), 5.5)

    def test_validated_declared_water_level_sets_points_and_depth(self):
        self.layout["terrain"]["river"]["water_surface_m"] = 6.5
        for sample in water.sample_river(self.layout, step_m=100):
            x, y, z = sample["center_m"]
            self.assertEqual(z, 6.5)
            self.assertAlmostEqual(sample["depth_m"], 4 - 0.001 * y, places=10)
            for key in ("left_m", "right_m"):
                self.assertEqual(sample[key][2], 6.5)
                self.assertAlmostEqual(terrain.height_metres(self.layout, *sample[key][:2]),
                                       6.5, delta=0.000001)

    def test_invalid_steps_are_rejected(self):
        for step in (None, True, False, 0, -1, float("nan"), float("inf"), float("-inf"),
                     "10", [], {}, 10 ** 400):
            with self.subTest(step=step), self.assertRaises(ValueError):
                water.sample_river(self.layout, step_m=step)

    def test_extreme_sampling_requests_fail_before_unbounded_allocation(self):
        for step in (1e-300, 1e-9, 0.01):
            with self.subTest(step=step), self.assertRaises(ValueError):
                water.sample_river(self.layout, step_m=step)
        self.assertEqual(len(water.sample_river(self.layout, step_m=2000)), 9)

    def test_unsupported_profiles_and_malformed_layouts_are_rejected(self):
        historical = terrain.load_layout(REPO / "Art/Maps/WoodedValley768.layout.json")
        invalid = [None, [], {}, historical]
        for key, value in (("name", "UnknownValley"), ("extent_m", [1600, 1600]),
                           ("engine_target", "5.8.1"), ("schema_version", 2)):
            edited = copy.deepcopy(self.layout)
            edited[key] = value
            invalid.append(edited)
        for mutation in ("missing-water", "unordered-knots", "missing-route", "nonfinite", "dry-floor"):
            edited = copy.deepcopy(self.layout)
            river = edited["terrain"]["river"]
            if mutation == "missing-water":
                del river["water_surface_m"]
            elif mutation == "unordered-knots":
                river["centerline_m"][1][1] = 0
            elif mutation == "missing-route":
                edited["routes"].pop()
            elif mutation == "nonfinite":
                river["bed_height_m"] = float("nan")
            else:
                river["bed_height_m"] = river["water_surface_m"]
            invalid.append(edited)
        for index, layout in enumerate(invalid):
            with self.subTest(index=index), self.assertRaises(ValueError):
                water.sample_river(layout)

    def test_valid_terrain_without_two_dry_banks_cannot_produce_a_water_channel(self):
        self.layout["terrain"]["valley"]["height_m"] = -20
        self.layout["terrain"]["mountains"]["peak_height_m"] = 1
        terrain._validate_layout(self.layout)
        with self.assertRaises(ValueError):
            water.sample_river(self.layout)


if __name__ == "__main__":
    unittest.main()
