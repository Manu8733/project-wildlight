"""Original offline valley height field; Pillow is imported only when exporting."""

import argparse
import hashlib
import io
import json
import math
from pathlib import Path
import platform
import struct


MAX_LAYOUT_BYTES = 256 * 1024
PNG_NAME = "WoodedValley768.png"
R16_NAME = "WoodedValley768.r16"
MANIFEST_NAME = "WoodedValley768.manifest.json"


def _number(value, label, minimum=None, maximum=None):
    try:
        finite = isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value)
    except OverflowError:
        finite = False
    if not finite:
        raise ValueError(f"{label} must be finite numeric data")
    if (minimum is not None and value < minimum) or (maximum is not None and value > maximum):
        raise ValueError(f"{label} is outside its supported range")
    return value


def _pair(value, label, minimum=0, maximum=768):
    if not isinstance(value, list) or len(value) != 2:
        raise ValueError(f"{label} must contain two coordinates")
    return [_number(v, label, minimum, maximum) for v in value]


def _canonical(layout):
    return json.dumps(layout, sort_keys=True, separators=(",", ":"), allow_nan=False).encode("utf-8")


def _unique(records, label, maximum):
    if not isinstance(records, list) or not 1 <= len(records) <= maximum:
        raise ValueError(f"{label} has an invalid record count")
    result = {}
    for record in records:
        if not isinstance(record, dict):
            raise ValueError(f"{label} records must be objects")
        name = record.get("id")
        if not isinstance(name, str) or not name or len(name) > 64 or name in result:
            raise ValueError(f"{label} IDs must be unique nonempty strings")
        result[name] = record
    return result


def _smooth(t):
    t = min(1.0, max(0.0, t))
    return t * t * (3.0 - 2.0 * t)


def _river_x(river, y):
    points = river["centerline_m"]
    for (x0, y0), (x1, y1) in zip(points, points[1:]):
        if y <= y1:
            return x0 + (x1 - x0) * _smooth((y - y0) / (y1 - y0))
    return points[-1][0]


def _segment_distance(x, y, a, b):
    dx, dy = b[0] - a[0], b[1] - a[1]
    length_squared = dx * dx + dy * dy
    if length_squared == 0:
        return math.hypot(x - a[0], y - a[1])
    t = min(1.0, max(0.0, ((x - a[0]) * dx + (y - a[1]) * dy) / length_squared))
    return math.hypot(x - a[0] - t * dx, y - a[1] - t * dy)


def _route_distance(route, x, y):
    points = route["polyline_m"]
    return min(_segment_distance(x, y, a, b) for a, b in zip(points, points[1:]))


def _validate_layout(layout):
    try:
        if not isinstance(layout, dict) or len(_canonical(layout)) > MAX_LAYOUT_BYTES:
            raise ValueError("Layout must be a bounded JSON object")
        if layout["schema_version"] != 1 or isinstance(layout["schema_version"], bool):
            raise ValueError("Unsupported layout schema")
        if layout["name"] != "WoodedValley768" or layout["engine_target"] != "5.8.3":
            raise ValueError("This authoring input targets WoodedValley768 on Unreal 5.8.3")
        if _pair(layout["extent_m"], "extent") != [768, 768]:
            raise ValueError("Playable extent must be 768 by 768 metres")
        if _pair(layout["origin_m"], "origin") != [0, 0] or layout["logical_cell_m"] != 2:
            raise ValueError("Logical transform requires origin zero and 2 metre cells")
        _number(layout["logical_cell_m"], "logical cell", 2, 2)
        native = layout["landscape"]
        counts = native["samples"] + native["components"] + [native["quads_per_section"], native["sections_per_component"]]
        if any(isinstance(count, bool) or not isinstance(count, int) for count in counts):
            raise ValueError("Landscape samples, components, quads and section counts must be integers")
        if (native["samples"] != [505, 505] or native["components"] != [8, 8] or
                native["quads_per_section"] != 63 or native["sections_per_component"] != 1):
            raise ValueError("Native Landscape requires 505 samples, 63 quads, 1 section, 8 components")
        _pair(native["samples"], "samples", 505, 505)
        _pair(native["components"], "components", 8, 8)
        _number(native["quads_per_section"], "quads", 63, 63)
        _number(native["sections_per_component"], "sections", 1, 1)
        scale = _number(native["xy_scale_cm"], "XY scale", 0.001, 1000)
        if not math.isclose(scale * 504, 76800, rel_tol=0, abs_tol=0.000001):
            raise ValueError("Native measured extent does not match 76800 cm")
        _number(native["z_scale"], "Z scale", 0.001, 10000)
        _number(native["actor_z_m"], "actor Z", -10000, 10000)
        settings = layout["terrain"]
        valley, mountains, river = settings["valley"], settings["mountains"], settings["river"]
        _number(valley["height_m"], "valley height", -200, 200)
        _number(valley["undulation_m"], "valley undulation", 0, 10)
        _number(valley["side_rise_m"], "valley side rise", 0, 20)
        _number(mountains["ridge_width_m"], "ridge width", 20, 140)
        _number(mountains["peak_height_m"], "mountain peak", 1, 2000)
        points = river["centerline_m"]
        if not isinstance(points, list) or not 2 <= len(points) <= 64:
            raise ValueError("River requires a bounded continuous centerline")
        for point in points:
            _pair(point, "river point")
        if points[0][1] != 0 or points[-1][1] != 768 or any(a[1] >= b[1] for a, b in zip(points, points[1:])):
            raise ValueError("River must span Y=0..768 with strictly increasing control points")
        bed_width = _number(river["bed_width_m"], "river bed width", 4, 60)
        bank_width = _number(river["bank_width_m"], "river bank width", bed_width + 1, 100)
        _number(river["bed_height_m"], "river bed height", -200, 200)
        rise = _number(river["downstream_rise_per_m"], "river grade", -0.02, 0.02)
        water = _number(river["water_surface_m"], "water surface", -200, 200)
        if max(river["bed_height_m"], river["bed_height_m"] + rise * 768) >= water:
            raise ValueError("Continuous river floor must remain below the proposed water surface")
        river_min, river_max = min(p[0] for p in points), max(p[0] for p in points)
        if river_min <= bank_width / 2 or river_max >= 768 - bank_width / 2:
            raise ValueError("River must have both banks within the extent")

        clearings = _unique(layout["clearings"], "clearings", 6)
        routes = _unique(layout["routes"], "routes", 5)
        decks = _unique(layout["crossings"], "crossings", 3)
        if len(clearings) != 6 or len(routes) != 5 or len(decks) != 3:
            raise ValueError("Six starts, two bank routes and three crossings are required")
        templates = layout["resource_templates"]
        if not isinstance(templates, dict) or not 1 <= len(templates) <= 6:
            raise ValueError("Resource templates must be bounded objects")
        reference = None
        flat_height = None
        for clearing in clearings.values():
            x, y = _pair(clearing["center_m"], "clearing center")
            radius = _number(clearing["radius_m"], "clearing radius", 20, 100)
            margin = _number(clearing["blend_margin_m"], "clearing margin", 1, 40)
            if min(x, y, 768 - x, 768 - y) < radius + margin:
                raise ValueError("Clearing and margin must stay within terrain")
            if not (x + radius + margin < river_min - bank_width / 2 or
                    x - radius - margin > river_max + bank_width / 2):
                raise ValueError("Clearing must not flatten the river or its banks")
            height = _number(clearing["flat_height_m"], "clearing height", -200, 200)
            if flat_height is not None and height != flat_height:
                raise ValueError("Starts require equal initial flat heights")
            flat_height = height
            template = templates[clearing["resource_template_id"]]
            if not isinstance(template, list) or len(template) != 3:
                raise ValueError("Initial resources require Wood, Stone and Gold anchors")
            kinds = set()
            for resource in template:
                kind = resource["kind"]
                if kind not in ("Wood", "Stone", "Gold") or kind in kinds:
                    raise ValueError("Resource kinds must be unique Wood, Stone and Gold")
                kinds.add(kind)
                dx, dy = _pair(resource["offset_m"], "resource offset", -100, 100)
                if math.hypot(dx, dy) >= radius:
                    raise ValueError("Initial resource anchors must remain in their clearing")
                _number(resource["initial_quantity"], "resource quantity", 1, 10000000)
            if reference is not None and template != reference:
                raise ValueError("All six starts must use equal initial resource templates")
            reference = template
        values = list(clearings.values())
        for i, a in enumerate(values):
            for b in values[i + 1:]:
                if math.dist(a["center_m"], b["center_m"]) <= a["radius_m"] + b["radius_m"] + a["blend_margin_m"] + b["blend_margin_m"]:
                    raise ValueError("Clearing cores and smooth margins must be disjoint")

        for deck in decks.values():
            x, y = _pair(deck["center_m"], "deck center")
            span = _number(deck["span_m"], "deck span", bank_width + 2, 160)
            width = _number(deck["width_m"], "deck width", 4, 80)
            _number(deck["deck_height_m"], "deck height", water + 1, 200)
            if deck["permanent"] is not True or deck["terrain_fill"] is not False:
                raise ValueError("Crossings must be permanent separate decks, never terrain fill")
            if abs(x - _river_x(river, y)) > 0.000001 or y < width / 2 or y > 768 - width / 2:
                raise ValueError("Deck must be centered over the river within the extent")
            # Smoothstep segments are monotone, so endpoints and interior control points bound the whole channel.
            test_rows = [y - width / 2, y, y + width / 2]
            test_rows.extend(point[1] for point in points if y - width / 2 < point[1] < y + width / 2)
            for test_y in test_rows:
                if abs(x - _river_x(river, test_y)) + bank_width / 2 >= span / 2:
                    raise ValueError("Deck must span the complete channel over its entire width")

        graph = {name: set() for name in clearings}
        exits = {name: set() for name in clearings}
        used_decks = set()
        for route in routes.values():
            width = _number(route["width_m"], "route width", 4, 80)
            _number(route["blend_margin_m"], "route margin", 1, 40)
            if _number(route["height_m"], "route height", -200, 200) != flat_height:
                raise ValueError("Routes must meet the flat start heights")
            polyline = route["polyline_m"]
            if not isinstance(polyline, list) or not 2 <= len(polyline) <= 64:
                raise ValueError("Routes need a bounded polyline")
            for point in polyline:
                _pair(point, "route point")
            if any(a == b for a, b in zip(polyline, polyline[1:])):
                raise ValueError("Route segments cannot have zero length")
            ids = route["start_ids"]
            if not isinstance(ids, list) or len(ids) not in (2, 3) or len(set(ids)) != len(ids):
                raise ValueError("Routes must connect distinct starts")
            category = "transverse" if "crossing_id" in route else "bank"
            for name in ids:
                if name not in clearings or _route_distance(route, *clearings[name]["center_m"]) > 0.000001:
                    raise ValueError("Every recorded start must meet its route centerline")
                exits[name].add(category)
            for a, b in zip(ids, ids[1:]):
                graph[a].add(b)
                graph[b].add(a)
            if category == "transverse":
                deck_id = route["crossing_id"]
                deck = decks[deck_id]
                if deck_id in used_decks or _route_distance(route, *deck["center_m"]) > 0.000001:
                    raise ValueError("Each transverse route must use its own matching deck")
                if deck["width_m"] != width or deck["deck_height_m"] != route["height_m"]:
                    raise ValueError("Deck must match route width and elevation")
                if any(point[1] != deck["center_m"][1] for point in polyline):
                    raise ValueError("Transverse routes must stay straight along their reserved deck axis")
                steps = [b[0] - a[0] for a, b in zip(polyline, polyline[1:])]
                if not (all(step > 0 for step in steps) or all(step < 0 for step in steps)):
                    raise ValueError("Transverse routes must cross their deck without doubling back")
                used_decks.add(deck_id)
            elif not (all(p[0] < river_min - bank_width / 2 for p in polyline) or
                      all(p[0] > river_max + bank_width / 2 for p in polyline)):
                raise ValueError("Bank routes must remain outside the river")
        if used_decks != set(decks) or any(e != {"bank", "transverse"} for e in exits.values()):
            raise ValueError("Every start needs a bank exit and transverse exit")
        visited, pending = set(), [next(iter(clearings))]
        while pending:
            name = pending.pop()
            if name not in visited:
                visited.add(name)
                pending.extend(graph[name] - visited)
        if visited != set(clearings):
            raise ValueError("All six starts must be connected")
    except (KeyError, TypeError, OverflowError, RecursionError) as error:
        raise ValueError("Malformed or unsupported valley layout") from error


def _json_object(pairs):
    value = {}
    for key, item in pairs:
        if key in value:
            raise ValueError(f"Duplicate JSON key: {key}")
        value[key] = item
    return value


def load_layout(path: Path) -> dict:
    """Load and validate the editable proposal without importing an engine or Pillow."""
    with Path(path).open("rb") as source:
        data = source.read(MAX_LAYOUT_BYTES + 1)
    if len(data) > MAX_LAYOUT_BYTES:
        raise ValueError("Layout exceeds 256 KiB limit")
    try:
        layout = json.loads(data, object_pairs_hook=_json_object)
    except (json.JSONDecodeError, UnicodeDecodeError, RecursionError) as error:
        raise ValueError("Layout is not valid JSON") from error
    _validate_layout(layout)
    return layout


def height_metres(layout: dict, x_m: float, y_m: float) -> float:
    """Sample a validated layout in metres; deck geometry never changes the riverbed."""
    x = _number(x_m, "X", 0, 768)
    y = _number(y_m, "Y", 0, 768)
    settings = layout["terrain"]
    valley, mountain, river = settings["valley"], settings["mountains"], settings["river"]
    height = (valley["height_m"] + valley["undulation_m"] * math.sin(x / 80) * math.sin(y / 96) +
              valley["side_rise_m"] * ((x - 384) / 384) ** 2)
    ridges = []
    for distance, along, phase in ((x, y, 0.4), (768 - x, y, 1.7), (y, x, 2.5), (768 - y, x, 3.8)):
        width = mountain["ridge_width_m"] * (0.86 + 0.14 * math.sin(along / 97 + phase))
        taper = _smooth(1 - distance / width)
        profile = 0.78 + 0.14 * math.sin(along / 62 + phase) + 0.08 * math.cos(along / 31 - phase)
        ridges.append(taper * taper * profile)
    height += mountain["peak_height_m"] * math.sqrt(sum(ridge * ridge for ridge in ridges))

    for clearing in layout["clearings"]:
        distance = math.hypot(x - clearing["center_m"][0], y - clearing["center_m"][1])
        blend = 1 - _smooth((distance - clearing["radius_m"]) / clearing["blend_margin_m"])
        height += (clearing["flat_height_m"] - height) * blend
    for route in layout["routes"]:
        blend = 1 - _smooth((_route_distance(route, x, y) - route["width_m"] / 2) / route["blend_margin_m"])
        height += (route["height_m"] - height) * blend

    # Carve last so transverse road flattening never dams a crossing.
    distance = abs(x - _river_x(river, y))
    blend = 1 - _smooth((distance - river["bed_width_m"] / 2) /
                        ((river["bank_width_m"] - river["bed_width_m"]) / 2))
    bed = river["bed_height_m"] + river["downstream_rise_per_m"] * y
    return height + (bed - height) * blend


def encode_height(height_m: float, z_scale: float, actor_z_m: float) -> int:
    """Convert metres to Unreal UInt16 height without clipping or wrapping."""
    height = _number(height_m, "height")
    scale = _number(z_scale, "Z scale", 0.000001)
    actor_z = _number(actor_z_m, "actor Z")
    encoded = 32768 + (height - actor_z) * 100 * 128 / scale
    if not math.isfinite(encoded) or not 0 <= encoded <= 65535:
        raise ValueError("Height is outside the UInt16 Landscape range")
    return round(encoded)


def decode_height(sample: int, z_scale: float, actor_z_m: float) -> float:
    """Decode a UInt16 Landscape sample into world metres."""
    if isinstance(sample, bool) or not isinstance(sample, int) or not 0 <= sample <= 65535:
        raise ValueError("Sample must be a UInt16 integer")
    scale = _number(z_scale, "Z scale", 0.000001)
    actor_z = _number(actor_z_m, "actor Z")
    height = actor_z + (sample - 32768) * scale / (100 * 128)
    return _number(height, "decoded height")


def export_terrain(layout: dict, output_dir: Path) -> dict:
    """Validate and prepare complete deterministic buffers before creating output files."""
    _validate_layout(layout)
    output_dir = Path(output_dir)
    targets = [output_dir / name for name in (PNG_NAME, R16_NAME, MANIFEST_NAME)]
    if any(path.exists() or path.is_symlink() for path in targets):
        raise FileExistsError("Refusing to overwrite existing terrain output")
    native = layout["landscape"]
    columns, rows = native["samples"]
    heights, samples = [], []
    for row in range(rows):
        y = row * layout["extent_m"][1] / (rows - 1)
        for column in range(columns):
            x = column * layout["extent_m"][0] / (columns - 1)
            height = height_metres(layout, x, y)
            heights.append(height)
            samples.append(encode_height(height, native["z_scale"], native["actor_z_m"]))
    raw = struct.pack(f"<{len(samples)}H", *samples)
    from PIL import Image, __version__ as pillow_version

    image = Image.frombytes("I;16", (columns, rows), raw, "raw", "I;16")
    png_buffer = io.BytesIO()
    image.save(png_buffer, format="PNG", optimize=False, compress_level=9)
    png = png_buffer.getvalue()
    manifest = {
        "schema_version": 1, "name": layout["name"], "engine_target": layout["engine_target"],
        "exporter": {"python_version": platform.python_version(), "pillow_version": pillow_version},
        "artifact_type": "Original offline Landscape input, not an Unreal screenshot",
        "layout_sha256": hashlib.sha256(_canonical(layout)).hexdigest(),
        "layout_hash_encoding": "UTF-8 sorted JSON keys, compact separators, finite numbers",
        "extent_m": layout["extent_m"], "sample_count": len(samples),
        "min_height_m": min(heights), "max_height_m": max(heights),
        "min_encoded_sample": min(samples), "max_encoded_sample": max(samples),
        "max_roundtrip_error_m": max(abs(decode_height(s, native["z_scale"], native["actor_z_m"]) - h)
                                     for s, h in zip(samples, heights)),
        "landscape": native,
        "orientation": {"row_0": "Y=0", "column_0": "X=0", "rows_increase": "+Y", "columns_increase": "+X"},
        "world_to_grid": {
            "origin_world_cm": [0, 0, native["actor_z_m"] * 100],
            "logical_cell_cm": 200, "logical_cells": [384, 384],
            "logical_formula": "cell_x=floor(world_x_cm/200), cell_y=floor(world_y_cm/200)",
            "logical_domain_cm": "0 <= X,Y < 76800; upper edge is boundary, not cell 384",
            "landscape_formula": "column=world_x_cm/xy_scale_cm, row=world_y_cm/xy_scale_cm",
        },
        "files": {
            "png": {"name": PNG_NAME, "bytes": len(png), "sha256": hashlib.sha256(png).hexdigest(),
                    "format": "PNG 16-bit single-channel grayscale"},
            "r16": {"name": R16_NAME, "bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(),
                    "format": "Row-major little-endian UInt16"},
        },
        "verification": {
            "offline_encoding_verified": True, "native_import_verified": False,
            "native_r16_byte_order_verified": False, "native_extent_verified": False,
            "engine_render_verified": False, "routes_collision_navigation_verified": False,
            "formation_clearance_verified": False, "bridge_water_verified": False,
            "resource_balance_verified": False, "wind_weather_lighting_verified": False,
            "performance_verified": False, "six_participant_gameplay_verified": False,
            "user_accepted": False,
        },
    }
    buffers = [png, raw, (json.dumps(manifest, indent=2, sort_keys=True, allow_nan=False) + "\n").encode("utf-8")]
    output_dir.mkdir(parents=True, exist_ok=True)
    created = []
    try:
        for path, data in zip(targets, buffers):
            with path.open("xb") as destination:
                created.append(path)
                destination.write(data)
    except BaseException:
        for path in created:
            path.unlink()
        raise
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--layout", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    try:
        manifest = export_terrain(load_layout(args.layout), args.output)
    except (ValueError, OSError, ImportError) as error:
        parser.exit(1, f"Terrain export failed: {error}\n")
    print(json.dumps(manifest, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
