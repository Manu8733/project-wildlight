import copy
import math
from pathlib import Path
import random
import sys
import unittest

REPO = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO / "Tools"))
import author_valley_terrain as terrain
import author_valley_surface as surface


class ValleySurfaceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.layout = terrain.load_layout(REPO / "Art/Maps/WoodedValley1600x1200.layout.json")

    def test_surface_outputs_are_bounded_and_normalized(self):
        for x, y in [(250, 250), (1458, 630), (0, 0), (800, 600), (1500, 1100)]:
            for normal_z in (0, .8, 1):
                value = surface.sample_surface(self.layout, x, y, normal_z=normal_z)
                self.assertTrue(all(0 <= channel <= 1 for channel in value["base_color"]))
                self.assertTrue(.6 <= value["roughness"] <= 1)
                self.assertAlmostEqual(math.sqrt(sum(n*n for n in value["normal"])), 1)
                self.assertGreater(value["normal"][2], .98)

    def test_nearby_meadow_points_have_restrained_detail(self):
        a = surface.sample_surface(self.layout, 1458, 630, normal_z=1)
        b = surface.sample_surface(self.layout, 1458.5, 630.5, normal_z=1)
        self.assertNotEqual(a["base_color"], b["base_color"])
        self.assertLess(max(abs(x-y) for x,y in zip(a["base_color"], b["base_color"])), .06)
        self.assertNotEqual(a["normal"], b["normal"])

    def test_route_treatment_does_not_restrict_walkability(self):
        a, b = self.layout["routes"][0]["polyline_m"][:2]
        p = [(a[i]+b[i])/2 for i in range(2)]
        value = surface.sample_surface(self.layout, *p, normal_z=1)
        self.assertGreater(value["path_weight"], .6)
        self.assertNotIn("walkable", value)
        self.assertNotIn("collision", value)

    def test_water_band_is_narrow_and_responds_to_height(self):
        self.assertEqual(surface.shore_weight(5.6, 5.5), 1)
        self.assertEqual(surface.shore_weight(6.5, 5.5), 0)
        self.assertTrue(0 < surface.shore_weight(6.0, 5.5) < 1)

    def test_steeper_ground_has_more_stone_treatment(self):
        flat = surface.sample_surface(self.layout, 1458, 630, normal_z=1)
        cliff = surface.sample_surface(self.layout, 1458, 630, normal_z=.6)
        self.assertLess(flat["rock_weight"], cliff["rock_weight"])

    def test_generators_preserve_layout_and_rng(self):
        before, rng = copy.deepcopy(self.layout), random.getstate()
        first = surface.build_surface_shaders(self.layout)
        self.assertEqual(first, surface.build_surface_shaders(self.layout))
        self.assertEqual(before, self.layout)
        self.assertEqual(rng, random.getstate())
        self.assertEqual(set(first), {"color", "normal", "roughness"})

    def test_color_shader_has_all_route_segments_and_no_time_or_density_edits(self):
        shaders = surface.build_surface_shaders(self.layout)
        self.assertEqual(shaders["color"].count("c.segment(p,"), sum(len(r["polyline_m"])-1 for r in self.layout["routes"]))
        self.assertNotIn("Time", "".join(shaders.values()))
        self.assertNotIn("GrassMask", "".join(shaders.values()))
        self.assertIn("P.xy*.01", shaders["normal"])

    def test_invalid_coordinate_or_normal_is_rejected(self):
        for value in (True, math.nan, math.inf, -1, 1.01):
            with self.subTest(normal=value), self.assertRaises(ValueError):
                surface.sample_surface(self.layout, 1458, 630, normal_z=value)
        with self.assertRaises(ValueError):
            surface.sample_surface(self.layout, -1, 0, normal_z=1)

    def test_old_map_cannot_be_reinterpreted(self):
        old = terrain.load_layout(REPO / "Art/Maps/WoodedValley768.layout.json")
        with self.assertRaises(ValueError):
            surface.build_surface_shaders(old)


if __name__ == "__main__":
    unittest.main()
