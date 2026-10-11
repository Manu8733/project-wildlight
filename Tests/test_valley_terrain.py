"""Offline authoring boundaries and native Landscape input, without Unreal."""

import copy
import hashlib
import importlib
import json
import math
from pathlib import Path
import struct
import sys
import tempfile
import unittest


REPO = Path(__file__).resolve().parents[1]
LAYOUT = REPO / "Art" / "Maps" / "WoodedValley768.layout.json"
sys.path.insert(0, str(REPO / "Tools"))
try:
    terrain = importlib.import_module("author_valley_terrain")
except ModuleNotFoundError:
    terrain = None


class TerrainTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(terrain, "Offline terrain authoring implementation is missing")
        self.layout = terrain.load_layout(LAYOUT)
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)

    def write_layout(self, value):
        path = self.root / "layout.json"
        path.write_text(json.dumps(value), encoding="utf-8")
        return path

    def test_native_extent_and_logical_grid_remain_distinct(self):
        native = self.layout["landscape"]
        self.assertEqual(native["samples"], [505, 505])
        for samples, components in zip(native["samples"], native["components"]):
            self.assertEqual(samples - 1, native["quads_per_section"] *
                             native["sections_per_component"] * components)
            self.assertAlmostEqual((samples - 1) * native["xy_scale_cm"], 76800)
        self.assertEqual(self.layout["extent_m"], [768, 768])
        self.assertEqual(self.layout["logical_cell_m"], 2)

    def test_height_literals_and_vertical_scale_round_trip(self):
        for height, expected in ((0, 32768), (1, 32896), (-1, 32640), (255, 65408), (-256, 0)):
            with self.subTest(height=height):
                self.assertEqual(terrain.encode_height(height, 100, 0), expected)
                self.assertEqual(terrain.decode_height(expected, 100, 0), height)
        self.assertEqual(terrain.encode_height(10, 50, 10), 32768)
        self.assertEqual(terrain.encode_height(11, 50, 10), 33024)
        self.assertEqual(terrain.decode_height(33024, 50, 10), 11)

    def test_height_overflow_and_nonfinite_values_are_rejected(self):
        for height in (256, -256.01, 10 ** 1000, math.inf, -math.inf, math.nan, True, "1"):
            with self.subTest(height=height), self.assertRaises(ValueError):
                terrain.encode_height(height, 100, 0)
        for scale in (0, -1, math.inf, math.nan, True):
            with self.subTest(scale=scale), self.assertRaises(ValueError):
                terrain.encode_height(0, scale, 0)
        for sample in (-1, 65536, 32768.5, math.inf, True):
            with self.subTest(sample=sample), self.assertRaises(ValueError):
                terrain.decode_height(sample, 100, 0)

    def test_nonfinite_actor_transform_is_rejected(self):
        for actor_z in (math.inf, math.nan, True, "0"):
            with self.subTest(actor_z=actor_z):
                with self.assertRaises(ValueError):
                    terrain.encode_height(0, 100, actor_z)
                with self.assertRaises(ValueError):
                    terrain.decode_height(32768, 100, actor_z)

    def test_nonfinite_metadata_is_rejected_even_outside_height_settings(self):
        invalid = copy.deepcopy(self.layout)
        invalid["source_footprints"]["castle_m"] = [math.nan, 8]
        with self.assertRaises(ValueError):
            terrain.load_layout(self.write_layout(invalid))

    def test_existing_single_artifact_blocks_export_without_other_writes(self):
        output = self.root / "partial"
        output.mkdir()
        existing = output / "WoodedValley768.manifest.json"
        existing.write_bytes(b"preserve me")
        with self.assertRaises(FileExistsError):
            terrain.export_terrain(self.layout, output)
        self.assertEqual(list(output.iterdir()), [existing])
        self.assertEqual(existing.read_bytes(), b"preserve me")

    def test_invalid_and_oversized_layouts_fail_before_output_creation(self):
        changes = [
            ("extent_m", [769, 768]), ("extent_m", [math.inf, 768]),
            ("logical_cell_m", 0), ("schema_version", 2), ("clearings", []),
            ("landscape", {**self.layout["landscape"], "samples": [100000, 100000]}),
            ("landscape", {**self.layout["landscape"], "xy_scale_cm": 200}),
            ("terrain", {**self.layout["terrain"], "mountains": {"peak_height_m": math.nan}}),
        ]
        for key, value in changes:
            invalid = copy.deepcopy(self.layout)
            invalid[key] = value
            with self.subTest(key=key, value=value):
                with self.assertRaises(ValueError):
                    terrain.load_layout(self.write_layout(invalid))
                output = self.root / "invalid-export"
                with self.assertRaises(ValueError):
                    terrain.export_terrain(invalid, output)
                self.assertFalse(output.exists())
        path = self.root / "oversized.json"
        path.write_bytes(b" " * (256 * 1024 + 1))
        with self.assertRaises(ValueError):
            terrain.load_layout(path)
        for text in ("[]", "{", '{"schema_version": NaN}', '{"schema_version":1,"schema_version":1}'):
            path.write_text(text, encoding="utf-8")
            with self.subTest(text=text), self.assertRaises(ValueError):
                terrain.load_layout(path)

    def test_invalid_topology_geometry_and_resources_are_rejected(self):
        for mutation in ("overlap", "missing-exit", "dam", "duplicate-id", "river-gap", "unequal-resources", "short-deck", "remote-route", "outside"):
            invalid = copy.deepcopy(self.layout)
            if mutation == "overlap":
                invalid["clearings"][1]["center_m"] = [145, 144]
            elif mutation == "missing-exit":
                invalid["routes"].pop()
            elif mutation == "dam":
                invalid["crossings"][0]["terrain_fill"] = True
            elif mutation == "duplicate-id":
                invalid["clearings"][1]["id"] = invalid["clearings"][0]["id"]
            elif mutation == "river-gap":
                invalid["terrain"]["river"]["centerline_m"][0][1] = 1
            elif mutation == "unequal-resources":
                invalid["resource_templates"]["unfair"] = [{"kind": "Wood", "offset_m": [0, 0], "initial_quantity": 1}]
                invalid["clearings"][0]["resource_template_id"] = "unfair"
            elif mutation == "short-deck":
                invalid["crossings"][0]["span_m"] = 8
            elif mutation == "remote-route":
                invalid["routes"][0]["polyline_m"] = [[200, 144], [200, 624]]
            else:
                invalid["clearings"][0]["center_m"] = [-1, 144]
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                terrain.load_layout(self.write_layout(invalid))

    def test_edited_transverse_route_cannot_cross_river_outside_its_deck(self):
        invalid = copy.deepcopy(self.layout)
        invalid["routes"][2]["polyline_m"] = [[144, 144], [300, 200], [500, 200], [384, 144], [624, 144]]
        with self.assertRaises(ValueError):
            terrain.load_layout(self.write_layout(invalid))
        output = self.root / "unbridged"
        with self.assertRaises(ValueError):
            terrain.export_terrain(invalid, output)
        self.assertFalse(output.exists())

    def test_landscape_topology_counts_must_be_integers_before_export(self):
        for key, value in (("samples", [505.0, 505]), ("samples", [505, 505.0]),
                           ("components", [8.0, 8]), ("components", [8, 8.0]),
                           ("quads_per_section", 63.0), ("sections_per_component", 1.0),
                           ("samples", [True, 505]), ("sections_per_component", True)):
            invalid = copy.deepcopy(self.layout)
            invalid["landscape"][key] = value
            with self.subTest(key=key, value=value):
                with self.assertRaises(ValueError):
                    terrain.load_layout(self.write_layout(invalid))
                output = self.root / "float-count"
                with self.assertRaises(ValueError):
                    terrain.export_terrain(invalid, output)
                self.assertFalse(output.exists())

    def test_entire_transverse_channel_must_fit_deck_span(self):
        invalid = copy.deepcopy(self.layout)
        invalid["terrain"]["river"]["centerline_m"][2:2] = [[384, 134], [430, 140]]
        with self.assertRaises(ValueError):
            terrain.load_layout(self.write_layout(invalid))

    def test_straight_subdivided_transverse_route_remains_editable(self):
        edited = copy.deepcopy(self.layout)
        edited["routes"][2]["polyline_m"] = [[144, 144], [300, 144], [384, 144], [500, 144], [624, 144]]
        validated = terrain.load_layout(self.write_layout(edited))
        self.assertLess(terrain.height_metres(validated, 384, 144), 0)
        self.assertEqual(terrain.height_metres(validated, 300, 144), 8)

    def test_deterministic_samples_flat_clearings_and_raised_irregular_edges(self):
        another = terrain.load_layout(LAYOUT)
        for x, y in ((0, 0), (768, 768), (100, 400), (340, 220)):
            self.assertEqual(terrain.height_metres(self.layout, x, y), terrain.height_metres(another, x, y))
        for clearing in self.layout["clearings"]:
            x, y = clearing["center_m"]
            for dx, dy in ((0, 0), (59, 0), (-59, 0), (0, 59), (0, -59), (35, 35)):
                self.assertAlmostEqual(terrain.height_metres(self.layout, x + dx, y + dy), 8)
        self.assertGreater(terrain.height_metres(self.layout, 0, 200), 60)
        self.assertGreater(terrain.height_metres(self.layout, 768, 500), 60)
        self.assertNotAlmostEqual(terrain.height_metres(self.layout, 0, 200), terrain.height_metres(self.layout, 0, 500))
        for x, y in ((-1, 0), (769, 0), (0, math.inf), (math.nan, 0)):
            with self.subTest(x=x, y=y), self.assertRaises(ValueError):
                terrain.height_metres(self.layout, x, y)

    def test_flattened_margins_are_continuous(self):
        for x, y in ((204, 180), (220, 180), (180, 204), (155, 260), (162, 260)):
            h = terrain.height_metres(self.layout, x, y)
            self.assertLess(abs(terrain.height_metres(self.layout, x + 0.0001, y) - h), 0.01)
            self.assertLess(abs(terrain.height_metres(self.layout, x, y + 0.0001) - h), 0.01)

    def test_two_bank_and_three_transverse_routes_connect_all_six_starts(self):
        starts = {c["id"] for c in self.layout["clearings"]}
        graph = {start: set() for start in starts}
        incidence = {start: [] for start in starts}
        for route in self.layout["routes"]:
            self.assertEqual(route["width_m"], 20)
            for start in route["start_ids"]:
                incidence[start].append(route["id"])
            for a, b in zip(route["start_ids"], route["start_ids"][1:]):
                graph[a].add(b)
                graph[b].add(a)
        self.assertEqual(len(self.layout["routes"]), 5)
        reached = set()
        pending = ["west-south"]
        while pending:
            node = pending.pop()
            if node not in reached:
                reached.add(node)
                pending.extend(graph[node] - reached)
        self.assertEqual(reached, starts)
        for exits in incidence.values():
            self.assertEqual(len(exits), 2)
        for x, y in ((144, 260), (624, 260), (260, 144), (490, 384), (260, 624)):
            self.assertAlmostEqual(terrain.height_metres(self.layout, x, y), 8)

    def test_river_is_continuous_beneath_separate_permanent_decks(self):
        self.assertEqual([d["center_m"][1] for d in self.layout["crossings"]], [144, 384, 624])
        for deck in self.layout["crossings"]:
            self.assertIs(deck["permanent"], True)
            self.assertIs(deck["terrain_fill"], False)
            x, y = deck["center_m"]
            self.assertLess(terrain.height_metres(self.layout, x, y), 0)
            self.assertGreater(deck["deck_height_m"], terrain.height_metres(self.layout, x, y) + 8)
        self.assertAlmostEqual(terrain.height_metres(self.layout, 384, 384), -3.232)
        without_decks = copy.deepcopy(self.layout)
        without_decks["crossings"] = []
        for x, y in self.layout["terrain"]["river"]["centerline_m"]:
            self.assertLess(terrain.height_metres(self.layout, x, y), 0)
            self.assertEqual(terrain.height_metres(self.layout, x, y), terrain.height_metres(without_decks, x, y))
            if 0 < y < 768:
                self.assertLess(abs(terrain.height_metres(self.layout, x, y - 0.0001) -
                                    terrain.height_metres(self.layout, x, y + 0.0001)), 0.001)

    def test_initial_resources_have_equal_quantities_offsets_and_access(self):
        templates = self.layout["resource_templates"]
        expected = [
            {"kind": "Wood", "offset_m": [-35, -25], "initial_quantity": 1800},
            {"kind": "Stone", "offset_m": [-32, 28], "initial_quantity": 1800},
            {"kind": "Gold", "offset_m": [28, -32], "initial_quantity": 1800},
        ]
        for clearing in self.layout["clearings"]:
            self.assertEqual(templates[clearing["resource_template_id"]], expected)
            for resource in expected:
                x, y = clearing["center_m"]
                dx, dy = resource["offset_m"]
                self.assertLess(math.hypot(dx, dy), clearing["radius_m"])
                self.assertAlmostEqual(terrain.height_metres(self.layout, x + dx, y + dy), 8)

    def test_export_r16_byte_order_length_orientation_roundtrip_and_png_header(self):
        output = self.root / "export"
        manifest = terrain.export_terrain(self.layout, output)
        raw = (output / "WoodedValley768.r16").read_bytes()
        self.assertEqual(len(raw), 510050)
        self.assertEqual(manifest["sample_count"], 255025)
        self.assertEqual(manifest["orientation"], {"row_0": "Y=0", "column_0": "X=0", "rows_increase": "+Y", "columns_increase": "+X"})
        for col, row in ((0, 0), (504, 0), (0, 504), (252, 252), (94, 94), (203, 167)):
            index = row * 505 + col
            sample = struct.unpack_from("<H", raw, index * 2)[0]
            x, y = col * 768 / 504, row * 768 / 504
            height = terrain.height_metres(self.layout, x, y)
            self.assertLessEqual(abs(terrain.decode_height(sample, 100, 0) - height), 1 / 256)
            self.assertEqual(raw[index * 2:index * 2 + 2], bytes((sample & 255, sample >> 8)))
        self.assertEqual(raw[(252 * 505 + 252) * 2:(252 * 505 + 252) * 2 + 2], b"b~")
        self.assertNotEqual(raw[:2], raw[(504 * 505) * 2:(504 * 505) * 2 + 2])
        png = (output / "WoodedValley768.png").read_bytes()
        self.assertEqual(png[:8], b"\x89PNG\r\n\x1a\n")
        self.assertEqual(struct.unpack(">IIBB", png[16:26]), (505, 505, 16, 0))
        from PIL import Image
        with Image.open(output / "WoodedValley768.png") as image:
            self.assertEqual(image.size, (505, 505))
            pixels = image.load()
            self.assertEqual([pixels[col, row] for row in range(505) for col in range(505)],
                             list(struct.unpack("<255025H", raw)))
        for file in manifest["files"].values():
            data = (output / file["name"]).read_bytes()
            self.assertEqual(file["bytes"], len(data))
            self.assertEqual(file["sha256"], hashlib.sha256(data).hexdigest())
        self.assertEqual(json.loads((output / "WoodedValley768.manifest.json").read_text()), manifest)
        self.assertFalse(manifest["verification"]["native_import_verified"])
        self.assertFalse(manifest["verification"]["native_r16_byte_order_verified"])
        self.assertFalse(manifest["verification"]["engine_render_verified"])

    def test_export_is_deterministic_and_never_overwrites(self):
        first, second = self.root / "first", self.root / "second"
        self.assertEqual(terrain.export_terrain(self.layout, first), terrain.export_terrain(self.layout, second))
        for path in first.iterdir():
            self.assertEqual(path.read_bytes(), (second / path.name).read_bytes())
        original = {path.name: path.read_bytes() for path in first.iterdir()}
        with self.assertRaises(FileExistsError):
            terrain.export_terrain(self.layout, first)
        self.assertEqual(original, {path.name: path.read_bytes() for path in first.iterdir()})

    def test_export_overflow_leaves_no_partial_files(self):
        invalid = copy.deepcopy(self.layout)
        invalid["terrain"]["mountains"]["peak_height_m"] = 1000
        with self.assertRaises(ValueError):
            terrain.export_terrain(invalid, self.root / "overflow")
        self.assertFalse((self.root / "overflow").exists())


if __name__ == "__main__":
    unittest.main()
