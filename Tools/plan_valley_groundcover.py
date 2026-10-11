"""Bounded original meadow patches; metadata only, no native asset writes."""

import hashlib
import math
import random
import re

import author_valley_terrain as terrain
import plan_valley_vegetation as vegetation


CAPS = {"grass": 60000, "flowers": 10000, "shrubs": 1000}
SAMPLE = (394, 328, 460, 392)
MAX_CANDIDATES = 200000


def _reserved(layout, x, y, radius):
    dx = max(SAMPLE[0] - x, 0, x - SAMPLE[2])
    dy = max(SAMPLE[1] - y, 0, y - SAMPLE[3])
    return math.hypot(dx, dy) <= radius or vegetation._reserved(layout, x, y, radius)


def _path(value):
    return isinstance(value, str) and len(value) <= 256 and re.fullmatch(
        r"/Game/[A-Za-z0-9_]+(?:/[A-Za-z0-9_]+)*(?:\.[A-Za-z0-9_]+)?", value) is not None


def _bindings(source):
    if source is None:
        return {category: [(None, {"mesh_path": None, "category": category,
                                  "footprint_radius_m": 0.5, "material_path": None})]
                for category in CAPS}
    if not isinstance(source, dict) or not 1 <= len(source) <= 32:
        raise ValueError("Bindings require one to 32 actual mesh records")
    for name, value in source.items():
        if not isinstance(name, str) or not 1 <= len(name) <= 64 or not isinstance(value, dict):
            raise ValueError("Invalid bounded binding ID or record")
    result = {}
    for name, value in sorted(source.items()):
        if not isinstance(value.get("category"), str) or value["category"] not in CAPS or not _path(value.get("mesh_path")):
            raise ValueError("Binding requires actual /Game mesh path and known category")
        radius = terrain._number(value.get("footprint_radius_m"), "footprint radius", 0.05, 8)
        material = value.get("material_path")
        if material is not None and not _path(material):
            raise ValueError("Material binding requires a /Game path")
        result.setdefault(value["category"], []).append((name, {
            "mesh_path": value["mesh_path"], "category": value["category"],
            "footprint_radius_m": radius, "material_path": material}))
    return result


def plan_groundcover(layout, *, seed=1, max_instances=60000,
                     max_slope_degrees=25, mesh_bindings=None):
    """Return clustered metre-space transforms without modifying the layout.

    Binding footprints must bound the actual pivot-relative mesh radius. Counts
    are hard authoring budgets, not measured graphics performance or coverage.
    """
    terrain._validate_layout(layout)
    if layout["name"] != "WoodedValley1600x1200":
        raise ValueError("Groundcover supports only the approved rectangle")
    vegetation._integer(seed, "seed", 1, 2 ** 31 - 1)
    vegetation._integer(max_instances, "instance budget", 0, 120000)
    terrain._number(max_slope_degrees, "slope limit", 0, 35)
    bindings = _bindings(mesh_bindings)
    total = min(max_instances, sum(CAPS[category] for category in bindings))
    categories = list(bindings)
    caps_total = sum(CAPS[category] for category in categories)
    targets = {category: min(CAPS[category], total * CAPS[category] // caps_total)
               for category in categories}
    for category in categories:
        targets[category] += min(CAPS[category] - targets[category], total - sum(targets.values()))
    plan = {"schema_version": 1, "layout_name": layout["name"],
            "layout_sha256": hashlib.sha256(terrain._canonical(layout)).hexdigest(),
            "extent_m": list(layout["extent_m"]),
            "parameters": {"seed": seed, "max_instances": max_instances,
                           "max_slope_degrees": max_slope_degrees, "patch_radius_m": 15,
                           "scale_range": [0.8, 1.15], "excluded_sample_m": list(SAMPLE)},
            "patches": [], "instances": [],
            "coverage": [{"sector": [x, y], "count": 0} for y in range(2) for x in range(3)],
            "statistics": {"candidate_count": 0, "category_counts": {c: 0 for c in CAPS}}}
    if total == 0:
        return plan
    rng = random.Random(seed)
    cells = list(range(40 * 30))
    rng.shuffle(cells)
    patch_target = min(400, max(24, math.ceil(total / 400)))
    for cell in cells:
        x = (cell % 40 + rng.uniform(0.3, 0.7)) * 40
        y = (cell // 40 + rng.uniform(0.3, 0.7)) * 40
        if _reserved(layout, x, y, 15):
            continue
        plan["patches"].append({"id": len(plan["patches"]), "center_m": [x, y]})
        if len(plan["patches"]) == patch_target:
            break
    if not plan["patches"]:
        return plan
    slope_limit = math.tan(math.radians(max_slope_degrees))
    counts = plan["statistics"]["category_counts"]
    buckets = {}
    for candidate in range(MAX_CANDIDATES):
        plan["statistics"]["candidate_count"] = candidate + 1
        remaining = [c for c in categories if counts[c] < targets[c]]
        if not remaining:
            break
        weights = [targets[c] - counts[c] for c in remaining]
        category = rng.choices(remaining, weights=weights, k=1)[0]
        binding_id, binding = rng.choice(bindings[category])
        patch = plan["patches"][candidate % len(plan["patches"])]
        angle = rng.uniform(0, math.tau)
        distance = 15 * math.sqrt(rng.random())
        x = patch["center_m"][0] + math.cos(angle) * distance
        y = patch["center_m"][1] + math.sin(angle) * distance
        size = rng.uniform(0.8, 1.15)
        radius = binding["footprint_radius_m"] * size
        margin = max(0.5, radius)
        if not (margin <= x <= 1600 - margin and margin <= y <= 1200 - margin):
            continue
        if _reserved(layout, x, y, radius):
            continue
        gx = terrain.height_metres(layout, x + 0.5, y) - terrain.height_metres(layout, x - 0.5, y)
        gy = terrain.height_metres(layout, x, y + 0.5) - terrain.height_metres(layout, x, y - 0.5)
        if math.hypot(gx, gy) > slope_limit:
            continue
        # Avoid repeated blades on the exact same location; crowns may overlap
        # naturally within a patch without creating physical movement blockers.
        bx, by = math.floor(x / 0.65), math.floor(y / 0.65)
        if any((x - px) ** 2 + (y - py) ** 2 < 0.65 ** 2
               for ix in range(bx - 1, bx + 2) for iy in range(by - 1, by + 2)
               for px, py in buckets.get((ix, iy), ())):
            continue
        plan["instances"].append({"position_m": [x, y, terrain.height_metres(layout, x, y)],
            "yaw_degrees": rng.uniform(0, 360), "scale": size, "role": "groundcover",
            "category": category, "binding_id": binding_id, "mesh_path": binding["mesh_path"],
            "material_path": binding["material_path"], "footprint_radius_m": binding["footprint_radius_m"],
            "patch_id": patch["id"]})
        buckets.setdefault((bx, by), []).append((x, y))
        counts[category] += 1
        plan["coverage"][min(1, int(y / 600)) * 3 + min(2, int(x / (1600 / 3)))]["count"] += 1
        if len(plan["instances"]) == total:
            break
    return plan
