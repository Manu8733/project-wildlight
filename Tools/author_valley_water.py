"""Original river samples derived from the existing rectangular valley height field."""

import math

import author_valley_terrain as terrain


MAX_RIVER_SAMPLES = 10000


def _shore_x(layout, center_x, y, direction, water_level):
    river = layout["terrain"]["river"]
    inner, outer = river["bed_width_m"] / 2, river["bank_width_m"] / 2
    if (terrain.height_metres(layout, center_x + direction * inner, y) >= water_level or
            terrain.height_metres(layout, center_x + direction * outer, y) <= water_level):
        raise ValueError("River requires a submerged bed and dry outer banks at every sampled row")
    for _ in range(48):
        middle = (inner + outer) / 2
        if terrain.height_metres(layout, center_x + direction * middle, y) < water_level:
            inner = middle
        else:
            outer = middle
    return center_x + direction * ((inner + outer) / 2)


def sample_river(layout: dict, step_m=10.0) -> list[dict]:
    """Return deterministic water center, shore, X-axis width and bed depth in metres.

    Include regular Y steps and all exact authored knots without changing the layout.
    Shore points independently intersect the analytic terrain at its declared water
    surface. Native piecewise-linear interpolation approximates the smooth bends;
    a native width centered on the authored spline also approximates asymmetric
    shores. These samples do not establish collision or navigation behavior.
    """
    terrain._validate_layout(layout)
    if layout["name"] != "WoodedValley1600x1200":
        raise ValueError("River sampling supports only the authored rectangular valley")
    step = terrain._number(step_m, "River sample step")
    if step <= 0:
        raise ValueError("River sample step must be positive")
    extent_y = layout["extent_m"][1]
    intervals = extent_y / step
    if not math.isfinite(intervals) or intervals >= MAX_RIVER_SAMPLES:
        raise ValueError("River sampling exceeds the 10,000 point limit")
    river = layout["terrain"]["river"]
    rows = {min(extent_y, index * step) for index in range(math.floor(intervals) + 1)}
    rows.update(point[1] for point in river["centerline_m"])
    if len(rows) > MAX_RIVER_SAMPLES:
        raise ValueError("River sampling exceeds the 10,000 point limit")

    level = river["water_surface_m"]
    samples = []
    for y in sorted(rows):
        center_x = terrain._river_x(river, y)
        depth = level - terrain.height_metres(layout, center_x, y)
        if depth <= 0:
            raise ValueError("River center bed must remain below the water surface")
        left_x = _shore_x(layout, center_x, y, -1, level)
        right_x = _shore_x(layout, center_x, y, 1, level)
        samples.append({
            "center_m": [center_x, y, level],
            "left_m": [left_x, y, level],
            "right_m": [right_x, y, level],
            "width_m": right_x - left_x,
            "depth_m": depth,
        })
    return samples
