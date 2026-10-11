"""Original organic ground shading for the approved valley; no asset inputs."""

import math

import author_valley_terrain as terrain


def _layout(layout):
    terrain._validate_layout(layout)
    if layout["name"] != "WoodedValley1600x1200":
        raise ValueError("Surface shading supports only the approved rectangle")


def _smooth(low, high, value):
    return terrain._smooth((value - low) / (high - low))


def _hash(x, y):
    value = math.sin(x * 127.1 + y * 311.7) * 43758.5453
    return value - math.floor(value)


def _noise(x, y):
    x, y = x * .819152 + y * .573576, y * .819152 - x * .573576
    ix, iy = math.floor(x), math.floor(y)
    fx, fy = terrain._smooth(x - ix), terrain._smooth(y - iy)
    a = _hash(ix, iy) * (1 - fx) + _hash(ix + 1, iy) * fx
    b = _hash(ix, iy + 1) * (1 - fx) + _hash(ix + 1, iy + 1) * fx
    return a * (1 - fy) + b * fy


def _mix(a, b, weight):
    return tuple(x * (1 - weight) + y * weight for x, y in zip(a, b))


def shore_weight(height_m, water_m):
    height = terrain._number(height_m, "surface height", -10000, 10000)
    water = terrain._number(water_m, "water height", -10000, 10000)
    return 1 - _smooth(water + .25, water + .75, height)


def sample_surface(layout, x_m, y_m, *, normal_z):
    _layout(layout)
    n = terrain._number(normal_z, "normal Z", 0, 1)
    h = terrain.height_metres(layout, x_m, y_m)
    x, y = x_m, y_m
    water = layout["terrain"]["river"]["water_surface_m"]
    macro = .55 * _noise(x / 60, y / 60) + .30 * _noise(x / 17 + 13, y / 17 + 13) + .15 * _noise(x / 3 + 27, y / 3 + 27)
    fine = _noise(x / .65, y / .65)
    moss = _smooth(.37, .78, _noise(x / 9 + 7, y / 9 + 7)) * .65
    patch = _smooth(.57, .80, _noise(x / 4 + 43, y / 4 + 43)) * _smooth(.2, .65, _noise(x / 19 + 11, y / 19 + 11)) * .25
    distance = min(terrain._route_distance(route, x, y) for route in layout["routes"])
    path = (1 - _smooth(3, 5.5, distance)) * .8
    path *= _smooth(water + .2, water + 1, h)
    rock = min(1, _smooth(.12, .36, 1 - n) + .3 * _smooth(50, 75, h))
    grass = _mix((.017, .045, .022), (.066, .12, .035), macro)
    grass = tuple(c * (.93 + .10 * fine) for c in grass)
    grass = _mix(grass, (.040, .068, .023), moss * .3)
    soil = _mix((.058, .042, .024), (.105, .083, .044), macro)
    wet = shore_weight(h, water)
    color = _mix(grass, soil, min(1, patch + path))
    color = _mix(color, (.022, .036, .028), wet)
    color = _mix(color, _mix((.13, .154, .147), (.25, .276, .239), macro), rock)
    dx = _noise((x + .12) / .7, y / .7) - _noise((x - .12) / .7, y / .7)
    dy = _noise(x / .7, (y + .12) / .7) - _noise(x / .7, (y - .12) / .7)
    normal = (-dx * .10, -dy * .10, 1)
    length = math.sqrt(sum(c*c for c in normal))
    return {"base_color": color, "normal": tuple(c / length for c in normal),
            "roughness": .92 - .22 * wet, "path_weight": path, "rock_weight": rock}


_COHERENT = """struct SurfaceDetail {
 float hash(float2 q) {return frac(sin(dot(q,float2(127.1,311.7)))*43758.5453);}
 float noise(float2 q) {q=float2(q.x*.819152+q.y*.573576,q.y*.819152-q.x*.573576);float2 i=floor(q),f=frac(q);f=f*f*(3-2*f);return lerp(lerp(hash(i),hash(i+float2(1,0)),f.x),lerp(hash(i+float2(0,1)),hash(i+1),f.x),f.y);}
 float segment(float2 p,float2 a,float2 b) {float2 v=b-a;return length(p-a-v*saturate(dot(p-a,v)/dot(v,v)));}
}; SurfaceDetail c;
float2 p=P.xy*.01;
"""


def build_surface_shaders(layout):
    _layout(layout)
    f = lambda value: format(value, ".10g")
    water = layout["terrain"]["river"]["water_surface_m"]
    distances = [f"c.segment(p,float2({f(a[0])},{f(a[1])}),float2({f(b[0])},{f(b[1])}))"
                 for route in layout["routes"] for a, b in zip(route["polyline_m"], route["polyline_m"][1:])]
    path = distances[0]
    for distance in distances[1:]:
        path = f"min({path},{distance})"
    color = _COHERENT + """float h=P.z*.01;
float macro=.55*c.noise(p/60)+.30*c.noise(p/17+13)+.15*c.noise(p/3+27);
float fine=c.noise(p/.65);
float moss=smoothstep(.37,.78,c.noise(p/9+7))*.65;
float patch=smoothstep(.57,.80,c.noise(p/4+43))*smoothstep(.2,.65,c.noise(p/19+11))*.25;
float3 grass=lerp(float3(.017,.045,.022),float3(.066,.12,.035),macro)*(.93+.10*fine);
grass=lerp(grass,float3(.040,.068,.023),moss*.3);
float3 soil=lerp(float3(.058,.042,.024),float3(.105,.083,.044),macro);
float rock=saturate(smoothstep(.12,.36,1-N.z)+.3*smoothstep(50,75,h));
""" + f"float path=(1-smoothstep(3,5.5,{path}))*.8*smoothstep({f(water+.2)},{f(water+1)},h);\n" + f"float wet=1-smoothstep({f(water+.25)},{f(water+.75)},h);\n" + """float3 color=lerp(grass,soil,saturate(patch+path));
color=lerp(color,float3(.022,.036,.028),wet);
return lerp(color,lerp(float3(.13,.154,.147),float3(.25,.276,.239),macro),rock);"""
    normal = _COHERENT + """float dx=c.noise((p+float2(.12,0))/.7)-c.noise((p-float2(.12,0))/.7);
float dy=c.noise((p+float2(0,.12))/.7)-c.noise((p-float2(0,.12))/.7);
return normalize(float3(-dx*.10,-dy*.10,1));"""
    roughness = f"return .92-.22*(1-smoothstep({f(water+.25)},{f(water+.75)},P.z*.01));"
    return {"color": color, "normal": normal, "roughness": roughness}
