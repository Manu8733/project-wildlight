"""Original streamed grass density; no licensed content or native asset writes."""

import math

import author_valley_terrain as terrain


def _layout(layout):
    terrain._validate_layout(layout)
    if layout["name"] != "WoodedValley1600x1200":
        raise ValueError("Blade grass supports only the approved rectangular map")


def _smooth(low, high, value):
    return terrain._smooth((value - low) / (high - low))


def density_at(layout, x_m, y_m, *, normal_z):
    """Sample the same metre-space mask used by the native Grass Output shader."""
    _layout(layout)
    normal = terrain._number(normal_z, "normal Z", -1, 1)
    height = terrain.height_metres(layout, x_m, y_m)
    x, y = x_m, y_m
    water = layout["terrain"]["river"]["water_surface_m"]
    result = _smooth(water + .45, water + 1.5, height)
    result *= _smooth(math.cos(math.radians(30)), math.cos(math.radians(18)), normal)
    for distance in (x, y, layout["extent_m"][0] - x, layout["extent_m"][1] - y):
        result *= _smooth(0, 2, distance)
    for route in layout["routes"]:
        half = route["width_m"] / 2
        result *= _smooth(half, half + 2, terrain._route_distance(route, x, y))
    for deck in layout["crossings"]:
        dx = abs(x - deck["center_m"][0]) - deck["span_m"] / 2
        dy = abs(y - deck["center_m"][1]) - deck["width_m"] / 2
        result *= _smooth(0, 1, math.hypot(max(dx, 0), max(dy, 0)) + min(max(dx, dy), 0))
    for clearing in layout["clearings"]:
        distance = math.dist((x, y), clearing["center_m"])
        result *= .6 + .4 * _smooth(clearing["radius_m"] * .65, clearing["radius_m"], distance)
    return result * (.8 + .2 * math.sin(x * .025) * math.sin(y * .033))


def build_density_shader(layout):
    """HLSL takes absolute world-centimetre P and vertex world-normal N inputs."""
    _layout(layout)
    f = lambda value: format(value, ".10g")
    water = layout["terrain"]["river"]["water_surface_m"]
    lines = ["""struct GrassMask {
 float segment(float2 p, float2 a, float2 b) {
  float2 v=b-a; return length(p-a-v*saturate(dot(p-a,v)/dot(v,v)));
 }
 float box(float2 p, float2 center, float2 halfSize) {
  float2 q=abs(p-center)-halfSize; return length(max(q,0))+min(max(q.x,q.y),0);
 }
}; GrassMask g;
float2 p=P.xy*.01;""",
        f"float density=smoothstep({f(water+.45)},{f(water+1.5)},P.z*.01);",
        f"density*=smoothstep({f(math.cos(math.radians(30)))},{f(math.cos(math.radians(18)))},N.z);",
        f"density*=smoothstep(0,2,p.x)*smoothstep(0,2,p.y)*smoothstep(0,2,{f(layout['extent_m'][0])}-p.x)*smoothstep(0,2,{f(layout['extent_m'][1])}-p.y);"]
    for route in layout["routes"]:
        distances = [f"g.segment(p,float2({f(a[0])},{f(a[1])}),float2({f(b[0])},{f(b[1])}))"
                     for a, b in zip(route["polyline_m"], route["polyline_m"][1:])]
        expression = distances[0]
        for distance in distances[1:]:
            expression = f"min({expression},{distance})"
        half = route["width_m"] / 2
        lines.append(f"density*=smoothstep({f(half)},{f(half+2)},{expression});")
    for deck in layout["crossings"]:
        x, y = deck["center_m"]
        lines.append(f"// crossing\ndensity*=smoothstep(0,1,g.box(p,float2({f(x)},{f(y)}),float2({f(deck['span_m']/2)},{f(deck['width_m']/2)})));")
    for clearing in layout["clearings"]:
        x, y = clearing["center_m"]
        radius = clearing["radius_m"]
        lines.append(f"density*=lerp(.6,1,smoothstep({f(radius*.65)},{f(radius)},length(p-float2({f(x)},{f(y)}))));")
    lines.append("return saturate(density*(.8+.2*sin(p.x*.025)*sin(p.y*.033)));")
    return "\n".join(lines)
