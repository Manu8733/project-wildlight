import copy
import math
from pathlib import Path
import random
import sys
import unittest

REPO = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO / "Tools"))
import author_valley_terrain as terrain
import author_valley_blade_grass as grass


class BladeGrassTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.layout = terrain.load_layout(REPO / "Art/Maps/WoodedValley1600x1200.layout.json")

    def test_dry_meadow_has_blades_outside_existing_patches(self):
        self.assertGreater(grass.density_at(self.layout, 1458, 620, normal_z=1), .4)

    def test_water_is_empty_at_every_sampled_river_row(self):
        river = self.layout["terrain"]["river"]
        for y in range(0, 1201, 20):
            self.assertEqual(grass.density_at(self.layout, terrain._river_x(river, y), y, normal_z=1), 0)

    def test_all_road_center_segments_are_grass_free(self):
        for route in self.layout["routes"]:
            for a, b in zip(route["polyline_m"], route["polyline_m"][1:]):
                x, y = ((a[i] + b[i]) / 2 for i in range(2))
                self.assertEqual(grass.density_at(self.layout, x, y, normal_z=1), 0)

    def test_cliffs_are_empty_but_flat_terrain_is_not(self):
        self.assertEqual(grass.density_at(self.layout, 1458, 620, normal_z=math.cos(math.radians(35))), 0)
        self.assertGreater(grass.density_at(self.layout, 1458, 620, normal_z=1), 0)

    def test_bounds_and_crossing_footprints_remain_empty(self):
        self.assertEqual(grass.density_at(self.layout, 0, 600, normal_z=1), 0)
        self.assertEqual(grass.density_at(self.layout, 1600, 600, normal_z=1), 0)
        for deck in self.layout["crossings"]:
            self.assertEqual(grass.density_at(self.layout, *deck["center_m"], normal_z=1), 0)

    def test_broad_six_sector_coverage_and_bounded_weights(self):
        counts = [0] * 6
        for y in range(20, 1200, 40):
            for x in range(20, 1600, 40):
                weight = grass.density_at(self.layout, x, y, normal_z=1)
                self.assertTrue(0 <= weight <= 1)
                counts[(y // 600) * 3 + min(2, int(x / (1600 / 3)))] += weight > .1
        self.assertTrue(all(n > 50 for n in counts), counts)

    def test_shader_contains_all_route_segments_and_protected_crossings(self):
        code = grass.build_density_shader(self.layout)
        segments = sum(len(r["polyline_m"]) - 1 for r in self.layout["routes"])
        self.assertEqual(code.count("g.segment(p,"), segments)
        self.assertEqual(code.count("// crossing"), len(self.layout["crossings"]))
        self.assertIn("P.xy*.01", code)
        self.assertIn("N.z", code)
        self.assertIn("1600", code)
        self.assertIn("1200", code)

    def test_generation_is_deterministic_and_does_not_mutate_layout_or_rng(self):
        before = copy.deepcopy(self.layout)
        rng = random.getstate()
        self.assertEqual(grass.build_density_shader(self.layout), grass.build_density_shader(self.layout))
        self.assertEqual(self.layout, before)
        self.assertEqual(random.getstate(), rng)

    def test_invalid_normals_and_coordinates_are_rejected(self):
        for value in (True, math.nan, math.inf, -1.1, 1.1):
            with self.subTest(value=value), self.assertRaises(ValueError):
                grass.density_at(self.layout, 1458, 620, normal_z=value)
        with self.assertRaises(ValueError):
            grass.density_at(self.layout, -1, 620, normal_z=1)

    def test_old_layout_is_not_reinterpreted(self):
        old = terrain.load_layout(REPO / "Art/Maps/WoodedValley768.layout.json")
        with self.assertRaises(ValueError):
            grass.build_density_shader(old)


if __name__ == "__main__":
    unittest.main()
