"""Deterministic original tree transforms; no engine, asset import or file writes."""

import hashlib
import math
import random
import re

import author_valley_terrain as terrain


MAX_INSTANCES = 10000
MAX_CANDIDATES = 200000
MIN_SCALE = 0.8
MAX_SCALE = 1.2


def _integer(value, label, minimum, maximum):
    terrain._number(value, label, minimum, maximum)
    if not isinstance(value, int):
        raise ValueError(f"{label} must be an integer")
    return value


def _bindings(mesh_bindings):
    if mesh_bindings is None:
        return []
    if not isinstance(mesh_bindings, dict) or not 1 <= len(mesh_bindings) <= 8:
        raise ValueError("Mesh bindings must contain one to eight actual Unreal asset paths")
    for name, path in mesh_bindings.items():
        if not isinstance(name, str) or not 1 <= len(name) <= 64:
            raise ValueError("Mesh binding IDs must be nonempty bounded strings")
        if (not isinstance(path, str) or len(path) > 256 or
                re.fullmatch(r"/Game/[A-Za-z0-9_]+(?:/[A-Za-z0-9_]+)*(?:\.[A-Za-z0-9_]+)?", path) is None):
            raise ValueError("Mesh bindings require /Game package or object paths")
    return sorted(mesh_bindings.items())


def _reserved(layout, x, y, canopy):
    for clearing in layout["clearings"]:
        if math.dist((x, y), clearing["center_m"]) <= clearing["radius_m"] + clearing["blend_margin_m"] + canopy:
            return True
    for route in layout["routes"]:
        if terrain._route_distance(route, x, y) <= route["width_m"] / 2 + route["blend_margin_m"] + canopy:
            return True
    river = layout["terrain"]["river"]
    # Smoothstep's maximum derivative is 1.5. Its Lipschitz bound protects
    # the entire circular crown against bank bends, not only its centre row.
    bend = max(1.5 * abs(b[0] - a[0]) / (b[1] - a[1])
               for a, b in zip(river["centerline_m"], river["centerline_m"][1:]))
    if abs(x - terrain._river_x(river, y)) <= river["bank_width_m"] / 2 + canopy * math.hypot(1, bend):
        return True
    return any(abs(x - deck["center_m"][0]) <= deck["span_m"] / 2 + canopy and
               abs(y - deck["center_m"][1]) <= deck["width_m"] / 2 + canopy
               for deck in layout["crossings"])


def plan_trees(layout, *, seed=0, density=0.65, min_spacing_m=8, canopy_radius_m=4,
               max_slope_degrees=25, max_instances=2400, mesh_bindings=None):
    """Return metre-space transforms for later native foliage/HISM binding.

    canopy_radius_m must bound every chosen mesh's unscaled horizontal crown.
    Binding paths are metadata, not proof that the assets exist or have usable
    licenses/wind. The planner neither changes terrain nor verifies navigation.
    """
    terrain._validate_layout(layout)
    if layout["name"] != "WoodedValley1600x1200":
        raise ValueError("Tree placement supports only the approved rectangular profile")
    _integer(seed, "seed", 0, 2 ** 31 - 1)
    terrain._number(density, "density", 0, 1)
    terrain._number(min_spacing_m, "minimum spacing", 4, 80)
    terrain._number(canopy_radius_m, "canopy radius", 0.1, 20)
    terrain._number(max_slope_degrees, "slope limit", 0, 35)
    _integer(max_instances, "instance budget", 0, MAX_INSTANCES)
    bindings = _bindings(mesh_bindings)
    extent_x, extent_y = layout["extent_m"]
    columns = math.ceil(extent_x / min_spacing_m)
    rows = math.ceil(extent_y / min_spacing_m)
    if columns * rows > MAX_CANDIDATES:
        raise ValueError("Candidate work exceeds the bounded placement budget")
    plan = {
        "schema_version": 1,
        "layout_name": layout["name"],
        "layout_sha256": hashlib.sha256(terrain._canonical(layout)).hexdigest(),
        "extent_m": list(layout["extent_m"]),
        "parameters": {
            "seed": seed, "density": density, "min_spacing_m": min_spacing_m,
            "canopy_radius_m": canopy_radius_m, "max_slope_degrees": max_slope_degrees,
            "max_instances": max_instances, "scale_range": [MIN_SCALE, MAX_SCALE],
        },
        "mesh_bindings": dict(bindings),
        "instances": [],
    }
    if density == 0 or max_instances == 0:
        return plan
    rng = random.Random(seed)
    phase = rng.uniform(0, math.tau)
    candidates = list(range(columns * rows))
    rng.shuffle(candidates)
    canopy = canopy_radius_m * MAX_SCALE
    boundary = max(canopy, 0.5)
    cell_x, cell_y = extent_x / columns, extent_y / rows
    slope_limit = math.tan(math.radians(max_slope_degrees))
    buckets = {}
    for candidate in candidates:
        x = (candidate % columns + rng.uniform(0.1, 0.9)) * cell_x
        y = (candidate // columns + rng.uniform(0.1, 0.9)) * cell_y
        if not (boundary <= x <= extent_x - boundary and boundary <= y <= extent_y - boundary):
            continue
        # A low scattered baseline plus broad irregular patches avoids a solid grid forest.
        patch = 0.5 + (math.sin(x / 90 + phase) + math.sin(y / 73 - phase) +
                       math.sin((x + y) / 140 + phase)) / 6
        if rng.random() >= density * (0.12 + 0.88 * patch * patch) or _reserved(layout, x, y, canopy):
            continue
        bx, by = math.floor(x / min_spacing_m), math.floor(y / min_spacing_m)
        nearby = (point for ix in range(bx - 1, bx + 2) for iy in range(by - 1, by + 2)
                  for point in buckets.get((ix, iy), ()))
        if any((x - px) ** 2 + (y - py) ** 2 < min_spacing_m ** 2 for px, py in nearby):
            continue
        gx = terrain.height_metres(layout, x + 0.5, y) - terrain.height_metres(layout, x - 0.5, y)
        gy = terrain.height_metres(layout, x, y + 0.5) - terrain.height_metres(layout, x, y - 0.5)
        if math.hypot(gx, gy) > slope_limit:
            continue
        binding_id, mesh_path = bindings[rng.randrange(len(bindings))] if bindings else (None, None)
        plan["instances"].append({
            "position_m": [x, y, terrain.height_metres(layout, x, y)],
            "yaw_degrees": rng.uniform(0, 360), "scale": rng.uniform(MIN_SCALE, MAX_SCALE),
            "role": "tree", "binding_id": binding_id, "mesh_path": mesh_path,
        })
        buckets.setdefault((bx, by), []).append((x, y))
        if len(plan["instances"]) == max_instances:
            break
    return plan
