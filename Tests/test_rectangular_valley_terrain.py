"""Approved rectangular terrain, safe approaches and preserved historical input."""

import copy
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest


REPO = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO / "Tools"))
import author_valley_terrain as terrain


class RectangularTerrainTests(unittest.TestCase):
    def setUp(self):
        path = REPO / "Art/Maps/WoodedValley1600x1200.layout.json"
        self.assertTrue(path.exists(), "Approved rectangular authored layout is missing")
        self.layout = terrain.load_layout(path)
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)

    def validate(self, layout):
        path = self.root / "edited.json"
        path.write_text(json.dumps(layout), encoding="utf-8")
        return terrain.load_layout(path)

    def test_unequal_xy_extent_and_valid_native_topology(self):
        native = self.layout["landscape"]
        self.assertEqual(native["samples"], [1009, 1009])
        self.assertEqual(native["components"], [8, 8])
        self.assertEqual(native["quads_per_section"] * native["sections_per_component"] * 8, 1008)
        for axis, extent_cm in enumerate((160000, 120000)):
            self.assertAlmostEqual(native["xy_scale_cm"][axis] * 1008, extent_cm, places=6)
        self.assertNotEqual(native["xy_scale_cm"][0], native["xy_scale_cm"][1])
        self.assertGreater(terrain.height_metres(self.layout, 1600, 600), 60)
        self.assertGreater(terrain.height_metres(self.layout, 300, 1200), 60)
        for x, y in ((1600.01, 600), (600, 1200.01), (-1, 0)):
            with self.assertRaises(ValueError):
                terrain.height_metres(self.layout, x, y)

    def test_six_irregular_disjoint_flat_starts_keep_physical_resources(self):
        old = terrain.load_layout(REPO / "Art/Maps/WoodedValley768.layout.json")
        centers = [c["center_m"] for c in self.layout["clearings"]]
        self.assertEqual(centers, [[250, 250], [590, 190], [330, 930], [1200, 260], [1070, 750], [1370, 970]])
        self.assertEqual(len({c[0] for c in centers}), 6)
        self.assertEqual(len({c[1] for c in centers}), 6)
        self.assertEqual(self.layout["resource_templates"], old["resource_templates"])
        self.assertEqual(self.layout["source_footprints"], old["source_footprints"])
        for clearing in self.layout["clearings"]:
            x, y = clearing["center_m"]
            for dx, dy in ((0, 0), (109, 0), (-109, 0), (0, 109), (0, -109)):
                self.assertAlmostEqual(terrain.height_metres(self.layout, x + dx, y + dy), 8)
        invalid = copy.deepcopy(self.layout)
        invalid["clearings"][1]["center_m"] = [260, 260]
        with self.assertRaises(ValueError):
            self.validate(invalid)

    def test_connected_graph_has_two_exits_and_bent_safe_approaches(self):
        starts = {c["id"] for c in self.layout["clearings"]}
        graph = {start: set() for start in starts}
        exits = {start: set() for start in starts}
        for route in self.layout["routes"]:
            category = "transverse" if "crossing_id" in route else "bank"
            for start in route["start_ids"]:
                exits[start].add(category)
            for a, b in zip(route["start_ids"], route["start_ids"][1:]):
                graph[a].add(b)
                graph[b].add(a)
            if category == "transverse":
                self.assertGreater(len({p[1] for p in route["polyline_m"]}), 1)
        reached, pending = set(), [next(iter(starts))]
        while pending:
            node = pending.pop()
            if node not in reached:
                reached.add(node)
                pending.extend(graph[node] - reached)
        self.assertEqual(reached, starts)
        self.assertTrue(all(value == {"bank", "transverse"} for value in exits.values()))
        edited = copy.deepcopy(self.layout)
        edited["routes"][2]["polyline_m"].insert(1, [450, 280])
        self.validate(edited)
        for deck in self.layout["crossings"]:
            self.assertIs(deck["permanent"], True)
            self.assertIs(deck["terrain_fill"], False)
            self.assertLess(terrain.height_metres(self.layout, *deck["center_m"]), 0)

    def test_off_deck_channel_bends_and_route_width_are_rejected(self):
        for mutation in ("off-deck", "diagonal", "double-crossing", "bank-edge", "wide", "disconnected"):
            invalid = copy.deepcopy(self.layout)
            route = invalid["routes"][2]
            if mutation == "off-deck":
                route["polyline_m"][1:1] = [[720, 400], [900, 400]]
            elif mutation == "diagonal":
                route["polyline_m"][2][1] += 1
            elif mutation == "double-crossing":
                route["polyline_m"][3:3] = [[700, 300], [900, 300]]
            elif mutation == "bank-edge":
                invalid["routes"][0]["polyline_m"].insert(1, [720, 500])
            elif mutation == "wide":
                route["width_m"] += 1
            else:
                route["start_ids"] = ["west-one", "east-two"]
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                self.validate(invalid)

    def test_only_two_named_profiles_and_correct_scaling_are_supported(self):
        for key, value in (("name", "../unsafe"), ("extent_m", [1600, 1600]),
                           ("landscape", {**self.layout["landscape"], "xy_scale_cm": 150}),
                           ("landscape", {**self.layout["landscape"], "xy_scale_cm": [158.73, 119.04]}),
                           ("landscape", {**self.layout["landscape"], "sections_per_component": 4}),
                           ("landscape", {**self.layout["landscape"], "samples": [1009.0, 1009]})):
            invalid = copy.deepcopy(self.layout)
            invalid[key] = value
            with self.subTest(key=key, value=value), self.assertRaises(ValueError):
                self.validate(invalid)

    def test_transverse_routes_cannot_record_same_bank_pairs_with_dangling_bridges(self):
        invalid = copy.deepcopy(self.layout)
        invalid["routes"][2].update({
            "start_ids": ["west-one", "west-two"],
            "polyline_m": [[250, 250], [590, 190], [690, 300], [800, 300], [910, 300], [1200, 260]],
        })
        invalid["routes"][3].update({
            "start_ids": ["west-three", "east-three"],
            "polyline_m": [[330, 930], [690, 650], [800, 650], [910, 650], [1370, 970]],
        })
        invalid["routes"][4].update({
            "start_ids": ["east-one", "east-two"],
            "polyline_m": [[250, 250], [690, 950], [800, 950], [910, 950], [1200, 260], [1070, 750]],
        })
        with self.assertRaises(ValueError):
            self.validate(invalid)

    def test_transverse_recorded_starts_are_the_terminal_endpoints(self):
        for endpoint in (0, -1):
            invalid = copy.deepcopy(self.layout)
            route = invalid["routes"][2]
            if endpoint == 0:
                route["polyline_m"].insert(0, [200, 250])
            else:
                route["polyline_m"].append([1250, 260])
            with self.subTest(endpoint=endpoint), self.assertRaises(ValueError):
                self.validate(invalid)
        reversed_route = copy.deepcopy(self.layout)
        reversed_route["routes"][2]["polyline_m"].reverse()
        self.validate(reversed_route)

    def test_three_reserved_deck_footprints_must_be_disjoint(self):
        for separation, second_x in ((0, 800), (12, 799.35875), (24, 797.57)):
            invalid = copy.deepcopy(self.layout)
            for index, deck in enumerate(invalid["crossings"][:2]):
                y = 300 + index * separation
                x = second_x if index else 800
                deck["center_m"] = [x, y]
                route = invalid["routes"][index + 2]
                route["polyline_m"] = [route["polyline_m"][0], [690, y], [x, y], [910, y], route["polyline_m"][-1]]
            with self.subTest(separation=separation), self.assertRaises(ValueError):
                self.validate(invalid)

    def test_rectangular_export_full_png_r16_orientation_and_exclusive_paths(self):
        output = self.root / "export"
        manifest = terrain.export_terrain(self.layout, output)
        raw = (output / "WoodedValley1600x1200.r16").read_bytes()
        self.assertEqual(len(raw), 2036162)
        self.assertEqual(manifest["sample_count"], 1018081)
        self.assertEqual(manifest["world_to_grid"]["logical_cells"], [800, 600])
        self.assertEqual(manifest["extent_m"], [1600, 1200])
        self.assertEqual(manifest["orientation"]["row_0"], "Y=0")
        self.assertEqual(manifest["orientation"]["columns_increase"], "+X")
        for col, row in ((0, 0), (1008, 0), (0, 1008), (1008, 1008), (504, 546), (159, 210)):
            sample = struct.unpack_from("<H", raw, (row * 1009 + col) * 2)[0]
            height = terrain.height_metres(self.layout, col * 1600 / 1008, row * 1200 / 1008)
            self.assertLessEqual(abs(terrain.decode_height(sample, 100, 0) - height), 1 / 256)
        from PIL import Image
        png_path = output / "WoodedValley1600x1200.png"
        self.assertEqual(struct.unpack(">IIBB", png_path.read_bytes()[16:26]), (1009, 1009, 16, 0))
        with Image.open(png_path) as image:
            pixels = image.load()
            self.assertEqual([pixels[col, row] for row in range(1009) for col in range(1009)],
                             list(struct.unpack("<1018081H", raw)))
        for record in manifest["files"].values():
            data = (output / record["name"]).read_bytes()
            self.assertEqual(hashlib.sha256(data).hexdigest(), record["sha256"])
        self.assertTrue(all(value is False for key, value in manifest["verification"].items()
                            if key != "offline_encoding_verified"))
        with self.assertRaises(FileExistsError):
            terrain.export_terrain(self.layout, output)


if __name__ == "__main__":
    unittest.main()
