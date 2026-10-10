"""Wildlight original asset kit and presentation scene, Blender 4.3.2.

All geometry is authored here. No downloaded meshes, image textures or image
generation are used. Run: blender -b --python Art/Source/build_wildlight_art.py
Use -- --skip-render to build/export only, -- --render-only to render saved scene.
"""
from pathlib import Path
import bpy
import math
import json
import sys
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
FBX = ROOT / 'Exports' / 'FBX'
GLB = ROOT / 'Exports' / 'GLB'
RENDERS = ROOT / 'Renders'
for d in (FBX, GLB, RENDERS):
    d.mkdir(parents=True, exist_ok=True)
PI = math.pi
MATS = {}
PARTS = []
ASSETS = {}


def linear(v):
    return v / 12.92 if v < .04045 else ((v + .055) / 1.055) ** 2.4


def material(name, color, rough=.78, metallic=0, emission=0):
    m = bpy.data.materials.new('WL_' + name)
    m.use_fake_user=True
    m.use_nodes = True
    rgb = tuple(linear(int(color[i:i+2], 16) / 255) for i in (0, 2, 4))
    m.diffuse_color = (*rgb, 1)
    p = m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value = (*rgb, 1)
    p.inputs['Roughness'].default_value = rough
    p.inputs['Metallic'].default_value = metallic
    p.inputs['Emission Color'].default_value = (*rgb, 1)
    p.inputs['Emission Strength'].default_value = emission
    MATS[name] = m
    return m


PALETTE = {
    'Plaster': 'F2E5CA', 'PlasterShade': 'D7C8AA',
    'Darkwood': '493C32', 'Wood': '866542', 'CutWood': 'B09460',
    'Stone': 'A3AD98', 'StoneLight': 'C1CAB4', 'StoneDark': '73836F',
    'Roof': 'B96750', 'RoofLight': 'D58B6C', 'RoofShadow': '8B5145',
    'Copper': 'C4824E', 'Gold': 'DAB967', 'Metal': '57717D',
    'Leaf': '77945B', 'LeafLight': 'A3B576', 'LeafDark': '536F55',
    'Grass': '9CAC77', 'GrassLight': 'B5BC83', 'Earth': '8A7156',
    'Path': 'D7BD92', 'Crystal': '8CCFBC', 'Rune': 'C5EBD4',
    'Window': 'FFD99A', 'Cloth': '74958A', 'Banner': 'E2BC76',
    'Skin': 'CAA881', 'Hair': '5D4938', 'RedCloth': 'A45043',
    'Water': '3F979B', 'Foam': 'BCE4CF', 'Mushroom': 'C77656',
    'Flower': 'E8D29C', 'Violet': '9276A6', 'Black': '273D38'
}


def init():
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    for m in list(bpy.data.materials):
        bpy.data.materials.remove(m)
    for key, value in PALETTE.items():
        material(key, value, .3 if key in ('Crystal', 'Water') else .78,
                 .35 if key in ('Copper', 'Gold', 'Metal') else 0,
                 1.5 if key == 'Window' else .65 if key == 'Crystal' else 1.2 if key == 'Rune' else 0)
    bpy.context.scene.unit_settings.system = 'METRIC'
    bpy.context.scene.unit_settings.scale_length = 1


def register(obj, mat, name='detail', bevel=0):
    obj.name = name
    obj.data.materials.clear()
    obj.data.materials.append(MATS[mat])
    if bevel:
        mod = obj.modifiers.new('Crafted edge', 'BEVEL')
        mod.width = bevel
        mod.segments = 2 if mat in ('Stone','StoneLight','StoneDark','Plaster') else 1
        mod.affect = 'EDGES'
    PARTS.append(obj)
    return obj


def box(p, size, mat, name='beam', bevel=.03, rot=None):
    bpy.ops.mesh.primitive_cube_add(size=1, location=p)
    o = bpy.context.object
    o.dimensions = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if rot:
        o.rotation_euler = rot
    return register(o, mat, name, bevel)


def mesh(verts, faces, mat, name='crafted mesh', bevel=0):
    data = bpy.data.meshes.new(name)
    data.from_pydata(verts, [], faces)
    data.update()
    o = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(o)
    return register(o, mat, name, bevel)


def cyl(p, radius, depth, mat, vertices=10, radius2=None, name='post'):
    bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=radius,
        radius2=radius if radius2 is None else radius2, depth=depth, location=p)
    return register(bpy.context.object, mat, name, .018 if radius > .12 else 0)


def ball(p, scale, mat, subdivisions=1, name='faceted shape'):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=subdivisions, radius=1, location=p)
    o = bpy.context.object
    o.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return register(o, mat, name)


def beam(a, b, width, mat, name='timber'):
    a, b = Vector(a), Vector(b)
    o = box((a+b)/2, (width, width, (b-a).length), mat, name, .012)
    o.rotation_euler = (b-a).to_track_quat('Z', 'Y').to_euler()
    return o


def ring(p, major, minor, mat, rot=None, name='ring'):
    bpy.ops.mesh.primitive_torus_add(major_radius=major, minor_radius=minor,
        major_segments=16, minor_segments=4, location=p)
    o = bpy.context.object
    if rot:
        o.rotation_euler = rot
    return register(o, mat, name)


def crystal(p, r=.45, h=1.7, mat='Crystal', tilt=(0,0,0)):
    x,y,z=p
    verts=[(x + r*math.cos(a*PI/3), y + r*math.sin(a*PI/3), z + h*.18) for a in range(6)]
    verts += [(x+r*.78*math.cos(a*PI/3), y+r*.78*math.sin(a*PI/3), z+h*.68) for a in range(6)]
    verts += [(x,y,z+h), (x,y,z)]
    faces=[(a,(a+1)%6,(a+1)%6+6,a+6) for a in range(6)]
    faces += [(a+6,(a+1)%6+6,12) for a in range(6)]
    faces += [(a,(a+1)%6,13) for a in range(6)]
    o=mesh(verts,faces,mat,'six sided light crystal')
    if tilt != (0,0,0):
        # Rotate about the crystal's foot instead of the world origin.
        bpy.context.scene.cursor.location=p
        bpy.context.view_layer.objects.active=o
        o.select_set(True)
        bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
        o.rotation_euler=tilt
        o.select_set(False)
    return o


def begin(name):
    global PARTS
    PARTS = []


def finish(name, kind, description, collision='box'):
    bpy.ops.object.select_all(action='DESELECT')
    for o in PARTS:
        o.select_set(True)
        bpy.context.view_layer.objects.active=o
        for mod in list(o.modifiers):
            bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.context.view_layer.objects.active=PARTS[0]
    bpy.ops.object.join()
    o=bpy.context.object
    o.name='SM_'+name
    bpy.context.scene.cursor.location=(0,0,0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    # Anchor every asset exactly at ground zero, including beveled root details.
    minimum_z=min(v.co.z for v in o.data.vertices)
    body_scale=.86 if name=='Castle' else .8 if kind=='unit' and name not in ('Cavalry','ForestAlly','Ram','Catapult') else 1
    for v in o.data.vertices:
        v.co.x*=body_scale
        v.co.y*=body_scale
        v.co.z=(v.co.z-minimum_z)*body_scale
    triang=o.modifiers.new('Export triangulation','TRIANGULATE')
    bpy.ops.object.modifier_apply(modifier=triang.name)
    for p in o.data.polygons:
        p.use_smooth=(name=='TreeOak' and o.data.materials[p.material_index].name.startswith('WL_Leaf'))
    # Coherent material names are preserved through FBX and GLB export.
    bpy.ops.export_scene.fbx(filepath=str(FBX/(o.name+'.fbx')),
        use_selection=True, object_types={'MESH'}, global_scale=1,
        apply_unit_scale=True, apply_scale_options='FBX_SCALE_UNITS',
        axis_forward='-Y', axis_up='Z', mesh_smooth_type='FACE',
        use_mesh_modifiers=True, add_leaf_bones=False, bake_anim=False)
    bpy.ops.export_scene.gltf(filepath=str(GLB/(o.name+'.glb')),
        export_format='GLB', use_selection=True, export_apply=True)
    coords=[tuple(v.co) for v in o.data.vertices]
    lo=[round(min(p[i] for p in coords),4) for i in range(3)]
    hi=[round(max(p[i] for p in coords),4) for i in range(3)]
    ASSETS[name]={'object':o, 'id':name, 'mesh_name':o.name, 'identifier':name, 'kind':kind,
        'description':description, 'vertices':len(o.data.vertices),
        'triangles':len(o.data.polygons), 'bounds_m':{'min':lo,'max':hi},
        'bounds_min':lo,'bounds_max':hi,
        'pivot_m':[0,0,0], 'unreal_path':'/Game/Wildlight/Meshes/'+o.name,
        'fbx':'Exports/FBX/'+o.name+'.fbx', 'glb':'Exports/GLB/'+o.name+'.glb',
        'materials':[m.name for m in o.data.materials],
        'material_names':[m.name for m in o.data.materials],
        'material_colors':{m.name:list(m.diffuse_color) for m in o.data.materials},
        'collision':{'strategy':collision,'bounds_m':{'min':lo,'max':hi}},
        'animation':'static mesh; no production rig' if kind=='unit' else 'not applicable'}
    o.hide_render=True
    o.hide_viewport=True
    return o


def foundation(w,d,h=.55):
    box((0,0,h/2),(w+.4,d+.4,h),'StoneDark','foundation',.08)
    box((0,0,h-.08),(w+.55,d+.55,.16),'StoneLight','stone sill',.05)
    # Readable masonry bands, rather than texture-only decoration.
    n=max(2,int(w/.8))
    for i in range(n):
        x=-w/2 + (i+.5)*w/n
        box((x,-d/2-.21,h*.46),(w/n-.045,.1,h*.58),'Stone','front stone blocks',.03)
    for i in range(max(2,int(d/.8))):
        y=-d/2+(i+.5)*d/max(2,int(d/.8))
        box((w/2+.2,y,h*.46),(.11,.72,h*.6),'Stone','side stone blocks',.025)


def window(x,y,z,width=.52,height=.7,side=False,lit=True):
    pane='Window' if lit else 'Black'
    if side:
        box((x,y,z),(.08,width+.18,height+.16),'Darkwood','window frame')
        box((x+.055,y,z),(.055,width,height),pane,'window panes',.01)
        box((x+.09,y,z),(.045,.05,height+.1),'Wood','window mullion',.01)
        box((x+.095,y,z),(.05,width,.06),'Wood','window crossbar',.01)
        box((x+.11,y,z-height/2-.08),(.25,width+.3,.1),'CutWood','window sill')
    else:
        box((x,y,z),(width+.18,.08,height+.16),'Darkwood','window frame')
        box((x,y-.055,z),(width,.055,height),pane,'window panes',.01)
        box((x,y-.09,z),(.05,.045,height+.1),'Wood','window mullion',.01)
        box((x,y-.095,z),(width,.05,.06),'Wood','window crossbar',.01)
        box((x,y-.11,z-height/2-.08),(width+.3,.25,.1),'CutWood','window sill')


def door(x,y,z,width=.85,height=1.65):
    radius=width/2
    shoulder=z+height-radius
    # True rounded doorway: board tops follow the arch, with a ring of
    # individually fitted voussoirs rather than a rectangular primitive frame.
    for xx in (x-radius-.095,x+radius+.095):
        box((xx,y, z+(height-radius)/2),(.19,.24,height-radius),'StoneLight' if width>1.2 else 'Darkwood','arched doorway jamb',.04)
    for i in range(10):
        a0=i*PI/10;a1=(i+1)*PI/10
        ri=radius+.018;ro=radius+.21
        v=[]
        for yy in (y-.15,y+.09):
            for rr,aa in ((ri,a0),(ro,a0),(ro,a1),(ri,a1)):
                v.append((x+rr*math.cos(aa),yy,shoulder+rr*math.sin(aa)))
        mesh(v,[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)],
            'StoneLight' if width>1.2 else 'Darkwood','fitted arch voussoir',.018)
    for i in range(6):
        left=-radius+i*width/6+.009;right=-radius+(i+1)*width/6-.009
        top_l=shoulder+math.sqrt(max(0,radius*radius-left*left))
        top_r=shoulder+math.sqrt(max(0,radius*radius-right*right))
        v=[(x+left,y-.13,z),(x+right,y-.13,z),(x+right,y-.13,top_r),(x+left,y-.13,top_l),
           (x+left,y-.05,z),(x+right,y-.05,z),(x+right,y-.05,top_r),(x+left,y-.05,top_l)]
        mesh(v,[(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)],'Wood','arched door boards',.008)
    for zz in (z+.36,z+height-.35):
        box((x,y-.18,zz),(width,.045,.07),'Metal','door iron strap',.008)
    ball((x+width*.27,y-.21,z+height*.48),(.055,.03,.055),'Gold')


def steps(x,y,count=3,width=1.5,rise=.18,depth=.34):
    for i in range(count):
        box((x,y+i*depth, (i+1)*rise/2), (width,depth+.02,(i+1)*rise), 'StoneLight','cut stone step',.04)


def roof(cx,cy,z,w,d,rise=1.8,mat='Roof',tiles=True):
    # Gabled roof with real overhang and separated weathered shingle rows.
    ww=w/2+.3
    dd=d/2+.33
    verts=[(cx-ww,cy-dd,z),(cx-ww,cy+dd,z),(cx,cy-dd,z+rise),(cx,cy+dd,z+rise),
           (cx+ww,cy-dd,z),(cx+ww,cy+dd,z)]
    mesh(verts,[(0,1,3,2),(2,3,5,4)],mat,'gabled roof',.035)
    mesh([(cx-w/2,cy-d/2,z-.03),(cx+w/2,cy-d/2,z-.03),(cx,cy-d/2,z+rise-.08)],
        [(0,1,2)],'Plaster','front gable')
    for yy in (cy-dd,cy+dd):
        beam((cx-ww,yy,z),(cx,yy,z+rise),.15,'Darkwood','carved bargeboard')
        beam((cx,yy,z+rise),(cx+ww,yy,z),.15,'Darkwood','carved bargeboard')
        beam((cx-ww,yy,z),(cx+ww,yy,z),.15,'Darkwood','eave brace')
    beam((cx,cy-dd-.1,z+rise+.05),(cx,cy+dd+.1,z+rise+.05),.18,'Copper','ridge cap')
    if tiles:
        for side in (-1,1):
            for row in range(5):
                t0=row/5
                t1=(row+.95)/5
                for col in range(max(4,int(d/.72))):
                    n=max(4,int(d/.72))
                    a=cy-dd+col*2*dd/n+.025
                    b=cy-dd+(col+1)*2*dd/n-.025
                    xa=cx+side*ww*t0
                    xb=cx+side*ww*t1
                    za=z+rise*(1-t0)+.035
                    zb=z+rise*(1-t1)+.035
                    shade='RoofLight' if (row*3+col)%7 in (1,4) else mat
                    mesh([(xa,a,za),(xa,b,za),(xb,b,zb),(xb,a,zb)],[(0,1,2,3)],shade,'individual roof shingle')


def cottage(w=3.8,d=3.2,h=3.2,porch=True):
    foundation(w,d)
    box((0,0,.55+h/2),(w,d,h),'Plaster','limewash body',.05)
    for x in (-w/2,w/2):
        for y in (-d/2,d/2):
            box((x,y,.55+h/2),(.19,.19,h+.08),'Darkwood','corner timber')
    for z in (.72,2.25,.55+h-.1):
        box((0,-d/2-.04,z),(w,.17,.16),'Darkwood','front horizontal timber')
        box((w/2+.04,0,z),(.17,d,.16),'Darkwood','side horizontal timber')
    for x in (-w*.25,w*.25):
        beam((x-.3,-d/2-.1,2.32),(x+.3,-d/2-.1,.88),.12,'Wood','diagonal half timber')
    door(0,-d/2-.07,.55)
    window(-w*.3,-d/2-.08,2.85,.65,.68)
    window(w*.3,-d/2-.08,2.85,.65,.68)
    window(w/2+.06,.3,2.85,.65,.7,True)
    roof(0,0,.55+h,w,d,1.7)
    steps(0,-d/2-1.05,3)
    if porch:
        box((0,-d/2-.6,2.57),(1.65,1.22,.16),'Roof','porch canopy',.06,(-.12,0,0))
        for x in (-.65,.65):
            beam((x,-d/2-1.05,.55),(x,-d/2-1.05,2.52),.12,'Wood','porch posts')
    box((-.9,.35,.55+h+1),( .68,.68,2.15),'Stone','chimney',.08)
    box((-.9,.35,.55+h+2.1),(.86,.86,.17),'StoneLight','chimney crown')
    # Two climbing ivy tendrils follow a real timber corner with shaped leaves.
    for side in (-1,1):
        x=side*(w/2-.1)
        for j in range(6):
            zz=.8+j*.36
            xx=x+.12*math.sin(j*1.1)
            beam((xx,-d/2-.13,zz),(xx+.08,-d/2-.13,zz+.4),.027,'LeafDark','climbing ivy stem')
            ball((xx+side*.12,-d/2-.16,zz+.18),(.15,.04,.12),'Leaf',1,'heart shaped ivy leaf')


def battlement(cx,cy,z,w,d):
    box((cx,cy,z),(w,d,.26),'StoneLight','tower cornice',.05)
    for x in (-w/2+.22,w/2-.22):
        for j in range(max(2,int(d/.75))):
            y=-d/2+.23 + j*(d-.46)/(max(2,int(d/.75))-1)
            box((cx+x,cy+y,z+.4),(.46,.48,.65),'Stone','crenellation',.035)
    for i in range(1,max(2,int(w/.75))-1):
        x=-w/2+.23+i*(w-.46)/(max(2,int(w/.75))-1)
        for y in (-d/2+.22,d/2-.22):
            box((cx+x,cy+y,z+.4),(.46,.46,.65),'Stone','crenellation',.035)


def tower(cx,cy,h=6.5,w=2.3,roofed=False):
    r=w*.58
    cyl((cx,cy,.3),r+.18,.6,'StoneDark',16,name='rounded tower foot')
    cyl((cx,cy,h/2+.3),r,h,'Stone',16,name='round tower masonry')
    for z in (1.2,3.3,h-.6):
        cyl((cx,cy,z),r+.06,.18,'StoneLight',16,name='rounded masonry belt')
    window(cx,cy-r-.04,h-1.6,.34,.95)
    window(cx+r+.04,cy,h-1.6,.34,.95,True)
    if roofed:
        cyl((cx,cy,h+1.25),r+.36,2.3,'Roof',16,0,'conical turret roof')
        crystal((cx,cy,h+2.4),.16,.58,'Gold')
    else:
        cyl((cx,cy,h+.4),r+.15,.26,'StoneLight',16,name='circular tower cornice')
        ring((cx,cy,h+.53),r+.1,.1,'StoneLight')
        for i in range(10):
            a=i*2*PI/10
            box((cx+math.cos(a)*(r-.09),cy+math.sin(a)*(r-.09),h+.84),(.48,.35,.64),
                'Stone','rounded turret crenellation',.075,(0,0,a+PI/2))


def barrel(x,y,z=0,r=.35):
    cyl((x,y,z+.42),r,.8,'Wood',10,r*.94,'stave barrel')
    for zz in (z+.16,z+.67):
        ring((x,y,zz),r+.008,.025,'Metal')


def crate(x,y,z=0,s=.65):
    box((x,y,z+s/2),(s,s,s),'CutWood','crate',.025)
    for zz in (z+.08,z+s-.08):
        box((x,y-s/2-.01,zz),(s,.04,.08),'Darkwood','crate rail',.01)
    beam((x-s*.38,y-s/2-.03,z+.1),(x+s*.38,y-s/2-.03,z+s-.1),.075,'Wood','crate diagonal')


def flag(x,y,z,color='Banner',height=2):
    beam((x,y,z),(x,y,z+height),.055,'Darkwood','flag pole')
    mesh([(x,y,z+height-.08),(x+.75,y+.03,z+height-.17),(x+.7,y+.02,z+height-.78),
          (x+.33,y-.06,z+height-.68),(x,y,z+height-.78)],[(0,1,2,3,4)],color,'kingdom swallowtail banner')


def build_buildings():
    begin('Castle')
    foundation(7.6,6.4,.65)
    box((0,0,2.8),(6.8,5.5,4.4),'Stone','central keep',.08)
    box((0,0,5.05),(6.98,5.68,.23),'StoneLight','keep cornice')
    roof(0,0,5.2,6.8,5.5,2.7)
    for x in (-3.5,3.5):
        for y in (-2.8,2.8):
            tower(x,y,6.7,2,roofed=y>0)
    door(0,-2.8,.65,1.65,2.4)
    for x in (-1.7,1.7):
        window(x,-2.82,4,.72,1.2)
    window(3.45,0,4,.8,1.1,True)
    steps(0,-4.05,4,2.7,.17,.34)
    for x in (-1.35,1.35):
        box((x,-2.99,2.8),(.42,.045,1.5),'Banner','hanging keep banner')
    flag(0,0,7.95,height=2)
    finish('Castle','building','Four corner towers, gabled royal keep, banners, broad entrance staircase')

    begin('House'); cottage();
    box((1.18,-1.94,.78),(.85,.42,.22),'Wood','window planter')
    for x in (.86,1.2,1.5): ball((x,-1.97,1),(.24,.18,.22),'LeafLight')
    finish('House','building','Half timber cottage with shingled eaves, porch, chimney and planted window box')

    begin('Farm'); cottage(3.7,3,2.55,False)
    for row in range(4):
        x=2.7+row*.63
        box((x,0,.09),(.42,4.7,.18),'Earth','tilled field row',.04)
        for j in range(9):
            y=-2+j*.48
            beam((x,y,.18),(x,y,.86),.036,'GrassLight','grain stalk')
            ball((x,y,.77),(.1,.1,.22),'Banner',1,'wheat head')
    for y in (-2.5,2.5):
        for x in (2.35,3.6,5.1): beam((x,y,0),(x,y,1.05),.09,'Wood','field fence post')
        for z in (.45,.83): beam((2.3,y,z),(5.2,y,z),.07,'CutWood','field fence rail')
    finish('Farm','building','Working cottage and fenced golden wheat rows')

    begin('Mill'); foundation(3.1,3.1)
    cyl((0,0,2.7),1.45,4.4,'Plaster',10,1.05,'tapered windmill body')
    for z in (.65,2.3,4.35): ring((0,0,z),1.45-(z-.5)*.1,.09,'Darkwood')
    cyl((0,0,5.75),1.55,1.9,'Roof',10,0,'windmill cap')
    door(0,-1.45,.55)
    window(.92,-1.1,3.4,.44,.58)
    hub=(0,-1.43,4.3)
    ball(hub,(.24,.25,.24),'Gold')
    for a in (PI/4,3*PI/4,5*PI/4,7*PI/4):
        dx,dz=math.sin(a),math.cos(a)
        beam(hub,(dx*2.7,-1.6,4.3+dz*2.7),.11,'Wood','windmill sail spar')
        # Each sail has a real timber ladder supporting cream fabric.
        for t in (1.25,1.7,2.15,2.6):
            x,z=dx*t,4.3+dz*t
            beam((x,-1.6,z),(x+dz*.55,-1.6,z-dx*.55),.055,'Darkwood','sail ladder rung')
        verts=[(dx*1.1,-1.59,4.3+dz*1.1),(dx*2.65,-1.59,4.3+dz*2.65),
            (dx*2.65+dz*.55,-1.59,4.3+dz*2.65-dx*.55),(dx*1.1+dz*.55,-1.59,4.3+dz*1.1-dx*.55)]
        mesh(verts,[(0,1,2,3)],'Plaster','canvas wind sail')
    barrel(1.7,-.7);finish('Mill','building','Tapered windmill with four wooden ladder sails')

    begin('Depot');cottage(4.5,3.8,2.7,False)
    for x,y in ((-1.8,-2.9),(-1,-2.9),(2.7,.3),(2.7,1.1)):crate(x,y,s=.8)
    barrel(1.5,-2.9);barrel(2.3,-2.8)
    box((0,-2.15,2.95),(1.45,.13,.48),'Wood','depot hanging sign')
    box((0,-2.24,2.95),(.7,.02,.13),'Banner','depot sign emblem')
    finish('Depot','building','Broad storehouse with visible shipping crates, barrels and wooden trade sign')

    begin('LumberCamp');foundation(3.4,3,.3)
    for x in (-1.4,1.4):
        for y in (-1.2,1.2):beam((x,y,.3),(x,y,2.7),.22,'Darkwood','open shed post')
    roof(0,0,2.7,3.4,3,1.2)
    box((0,-.2,.98),(2.6,.8,.16),'Wood','sawbench')
    for x in (-1,1):beam((x,-.2,.3),(x,-.2,.96),.13,'Darkwood')
    for z,y in ((.34,2),(.34,2.5),(.78,2.25)):
        o=cyl((0,y,z),.3,3.2,'Wood',10,name='stacked cut logs');o.rotation_euler[1]=PI/2
        for x in (-1.62,1.62):
            o=cyl((x,y,z),.275,.035,'CutWood',10,name='log growth rings');o.rotation_euler[1]=PI/2
    box((.15,-.1,1.22),(1.3,.045,.28),'Metal','saw blade')
    finish('LumberCamp','building','Open saw shed and stacked timber with exposed pale cut ends')

    begin('Quarry');foundation(4.7,3.8,.35)
    for i,(x,y,z,s) in enumerate(((-1.3,0,.8,1.3),(.3,.5,.7,1.15),(1.5,.8,.7,.9))):
        ball((x,y,z),(s,s*.8,s),'StoneLight',2,'cut rock outcrop')
    beam((-1.8,-1.3,.3),(-1.8,-1.3,3.6),.22,'Darkwood','quarry crane')
    beam((-1.8,-1.3,3.6),(1.1,-1.3,3.6),.2,'Wood','quarry crane jib')
    beam((-1.8,-1.3,2.4),(-.2,-1.3,3.6),.14,'Darkwood','crane brace')
    beam((.95,-1.3,3.5),(.95,-1.3,1.3),.036,'Darkwood','lifting rope')
    box((.95,-1.3,.95),(.8,.65,.65),'Stone','suspended quarry block')
    for x in (-.8,0,.8):box((x,-2.3,.3),(.6,.6,.6),'StoneLight','dressed stone stack')
    finish('Quarry','building','Faceted stone cut and timber jib crane carrying a dressed block')

    begin('GoldMine');foundation(4.8,3.8,.4)
    ball((0,.5,1.65),(2.7,2.3,1.8),'StoneDark',2,'mine hill')
    box((0,-1.65,1.2),(1.75,.2,1.95),'Black','dark mine entrance')
    for x in (-.94,.94):beam((x,-1.83,.4),(x,-1.83,2.4),.3,'Wood','mine pit prop')
    beam((-1.1,-1.83,2.38),(1.1,-1.83,2.38),.32,'Darkwood','mine entrance lintel')
    for x in (-.45,.45):beam((x,-1.9,.45),(x,-3.5,.1),.055,'Metal','minecart rail')
    box((0,-2.65,.75),(1.1,.85,.75),'Wood','ore cart')
    for x in (-.59,.59):
        for y in (-2.94,-2.36):
            o=cyl((x,y,.35),.22,.1,'Metal',10,name='ore cart wheel');o.rotation_euler[1]=PI/2
    for x,y in ((-.2,-2.8),(.2,-2.5)):
        ball((x,y,1.2),(.25,.25,.26),'Gold')
    for p in ((-1.5,.2,2.25),(1.7,-.2,1.8)):
        crystal(p,.17,.55,'Gold',(.2,.1,0))
    finish('GoldMine','building','Rocky pit entrance with timber props, rails, loaded ore wagon and gold veins')

    begin('Barracks');cottage(5.8,4,3.4,False)
    for x in (-2.7,2.7):flag(x,-2.75,0,height=3.5)
    for x in (-1.8,-1.4,-1):beam((x,-2.65,.5),(x,-2.65,2.1),.04,'Wood','weapon rack spear')
    beam((-2,-2.6,.75),(-.8,-2.6,.75),.12,'Darkwood')
    ball((2.2,-2.4,1.1),(.5,.12,.6),'Metal',1,'practice shield')
    finish('Barracks','building','Garrison longhouse with training weapons and proud royal banners')

    begin('ArcherRange');cottage(4.3,3,2.8,False)
    for x in (-1.8,0,1.8):
        beam((x,-3.6,0),(x,-3.6,1.5),.12,'Wood','target stand')
        for r,mat,y in ((.55,'Plaster',-3.64),(.36,'RedCloth',-3.72),(.17,'Banner',-3.79)):
            o=cyl((x,y,1.5),r,.07,mat,16,name='archery target ring');o.rotation_euler[0]=PI/2
    finish('ArcherRange','building','Training lodge with three large concentric archery targets')

    begin('Forge');cottage(4.5,3.5,2.8,False)
    box((2.75,.4,1.5),(1.55,1.7,2.6),'StoneDark','forge furnace',.1)
    box((2.75,-.47,1.1),(.88,.045,1.1),'Window','furnace mouth')
    box((2.75,.4,3.6),(.85,.85,1.8),'Stone','furnace chimney',.06)
    box((2.75,.4,4.55),(1,1,.16),'StoneLight','chimney cap')
    box((.55,-2.8,.42),(1.1,.8,.8),'Wood','anvil stump')
    mesh([(-.2,-3.2,.82),(1.3,-3.2,.82),(.95,-2.4,.82),(-.2,-2.4,.82),
          (.05,-3.1,1.15),(1.5,-3.1,1.15),(1.15,-2.5,1.15),(.05,-2.5,1.15)],
        [(0,1,2,3),(4,7,6,5),(0,4,5,1),(1,5,6,2),(2,6,7,3),(3,7,4,0)],'Metal','horned anvil',.03)
    finish('Forge','building','Smith workshop with separate glowing stone furnace and horned anvil')

    begin('Market');foundation(5.4,4.4,.3)
    for x in (-2.2,2.2):
        for y in (-1.7,1.7):beam((x,y,.3),(x,y,2.9),.15,'Darkwood','market canopy pole')
    for i in range(8):
        x=-2.6+(i+.5)*.65
        color='Plaster' if i%2 else 'Cloth'
        mesh([(x-.325,-2.2,2.75),(x+.325,-2.2,2.75),(x+.325,0,3.6),(x-.325,0,3.6)],
            [(0,1,2,3)],color,'striped front awning')
        mesh([(x-.325,0,3.6),(x+.325,0,3.6),(x+.325,2.2,2.75),(x-.325,2.2,2.75)],
            [(0,1,2,3)],color,'striped back awning')
        box((x,-2.22,2.6),(.64,.07,.3),color,'scalloped market valance')
    box((0,-1.6,.95),(4.6,.9,.8),'Wood','market counter')
    for x in (-1.6,0,1.6):
        box((x,-1.6,1.42),(1.15,.72,.22),'Darkwood','produce tray')
        for j in range(4):ball((x-.35+j*.22,-1.6,1.58),(.16,.17,.15),'Banner' if x<0 else 'Mushroom' if x==0 else 'LeafLight')
    barrel(-2.65,1);crate(2.7,1,s=.7)
    finish('Market','building','Striped linen canopy, scalloped valance and three visible produce displays')

    begin('Wall');foundation(5,1.3,.4)
    box((0,0,1.7),(5,1.1,2.7),'Stone','fortress wall',.06)
    for z in (1.1,2.1):box((0,-.57,z),(5,.09,.11),'StoneDark','masonry course')
    battlement(0,0,3.18,5.25,1.5)
    finish('Wall','building','Stone wall segment with machicolation-like sill and crenellated walk')

    begin('Gate')
    for x in (-2.3,2.3):tower(x,0,4.9,2.15,False)
    box((0,0,3.6),(2.9,1.3,1),'Stone','gate overhead lintel',.06)
    battlement(0,0,4.2,3.2,1.6)
    for x in (-1.15,-.76,-.38,0,.38,.76,1.15):beam((x,-.2,.2),(x,-.2,3.05),.09,'Darkwood','portcullis bar')
    for z in (.5,1.3,2.1,2.8):beam((-1.3,-.22,z),(1.3,-.22,z),.085,'Metal','portcullis crossbar')
    flag(-2.3,0,5.65,height=1.8)
    finish('Gate','building','Twin crenellated towers and visible timber and iron portcullis')

    begin('Watchtower');tower(0,0,6.1,2.5,False)
    for x in (-.8,.8):
        box((x,-1.3,2.4),(.12,.09,.9),'Black','arrow slit')
    door(0,-1.32,.3,.72,1.4)
    flag(0,0,6.7,height=2.1)
    finish('Watchtower','building','Tall stone lookout with arrow slits and signal banner')

    begin('Beacon')
    cyl((0,0,.2),1.55,.4,'StoneDark',12,name='beacon foundation')
    cyl((0,0,.49),1.35,.22,'StoneLight',12,name='beacon stepped plinth')
    cyl((0,0,1.37),.78,1.6,'Stone',8,.6,'octagonal beacon pedestal')
    ring((0,0,2.13),.91,.11,'Gold')
    crystal((0,0,2.2),.64,2.2)
    for a in (0,PI/2,PI,3*PI/2):
        x,y=math.cos(a),math.sin(a)
        beam((x*.9,y*.9,.58),(x*.84,y*.84,2.6),.13,'Darkwood','beacon claw')
        beam((x*.84,y*.84,2.6),(x*.4,y*.4,3.3),.1,'Gold','beacon curved cage')
    ring((0,0,3.05),.91,.048,'Copper')
    for a in range(8):
        x,y=math.cos(a*PI/4),math.sin(a*PI/4)
        box((x*.68,y*.68,1.45),(.09,.06,.33),'Rune','beacon runic inlay',.01,(0,0,a*PI/4))
    finish('Beacon','building','Large floating teal crystal in octagonal stone pedestal and gold-tipped cage')

    begin('Sanctuary')
    cyl((0,0,.23),3.1,.46,'StoneDark',12,name='sanctuary outer plinth')
    cyl((0,0,.54),2.8,.18,'StoneLight',12,name='sanctuary engraved platform')
    for a in range(8):
        theta=a*PI/4
        x,y=math.cos(theta)*2.25,math.sin(theta)*2.25
        if a==6:continue
        cyl((x,y,1.7),.28,2.15,'Stone',8,.22,'sanctuary column')
        cyl((x,y,2.8),.43,.22,'StoneLight',8,name='sanctuary capital')
    ring((0,0,2.94),2.28,.22,'StoneLight')
    ring((0,0,.65),1.9,.06,'Gold')
    cyl((0,0,1.07),.7,.82,'Stone',8,name='sanctuary altar')
    crystal((0,0,1.5),.56,2.3)
    for a in range(5):
        angle=a*2*PI/5
        crystal((math.cos(angle)*1.25,math.sin(angle)*1.25,.67),.2,.8)
    steps(0,-3.4,3,2.1,.17,.35)
    finish('Sanctuary','building','Open circular ritual sanctuary with column ring and five supporting crystals')

    begin('MageTower');foundation(3.5,3.5,.55)
    cyl((0,0,3.3),1.5,5.5,'Stone',10,1.2,'mage tower shaft')
    for z,r in ((.75,1.52),(3.2,1.4),(5.6,1.26)):
        ring((0,0,z),r,.12,'StoneLight')
    cyl((0,0,6.18),1.8,1.4,'Plaster',10,1.7,'overhanging wizard chamber')
    for a in range(10):
        theta=2*PI*a/10
        x,y=math.cos(theta)*1.75,math.sin(theta)*1.75
        beam((x*.72,y*.72,4.75),(x,y,5.65),.16,'Darkwood','chamber corbel')
    window(0,-1.77,6.2,.75,.82)
    window(1.77,0,6.2,.75,.82,True)
    cyl((0,0,8.3),2,2.9,'Roof',10,.13,'mage tower steep cap')
    ring((0,0,7.28),1.57,.07,'Gold')
    crystal((0,0,9.78),.25,.8)
    door(0,-1.55,.55,.8,1.6)
    steps(0,-2.5,3,1.45)
    finish('MageTower','building','Tapering arcane tower, corbelled observatory and crystal-tipped conical roof')

    begin('SiegeWorkshop');cottage(6.4,4.2,3.25,False)
    # Open machinery yard with a recognizable unfinished throwing arm.
    box((3.8,-1.3,.42),(2.2,3,.45),'Darkwood','machine platform')
    for x in (3,4.6):
        beam((x,-1.3,.65),(x,-1.3,2.75),.21,'Wood','siege arm trestle')
        beam((x,-2.6,.65),(x,-1.3,2.3),.16,'Darkwood','trestle brace')
    beam((3.8,-2.9,1.1),(3.8,.2,3.4),.23,'Wood','unfinished throwing arm')
    ring((3.8,-1.3,2.4),.52,.11,'Metal',(0,PI/2,0))
    for x in (-2,-1,0):crate(x,-3,s=.65)
    finish('SiegeWorkshop','building','Long engineering hall with unfinished catapult mechanism in the workshop yard')


def shield(p, color='Metal'):
    x,y,z=p
    verts=[(x-.25,y-.05,z+.32),(x+.25,y-.05,z+.32),(x+.27,y-.05,z-.02),
        (x,y-.1,z-.37),(x-.27,y-.05,z-.02),
        (x-.22,y+.055,z+.29),(x+.22,y+.055,z+.29),(x+.24,y+.055,z-.02),
        (x,y+.055,z-.33),(x-.24,y+.055,z-.02)]
    mesh(verts,[(0,1,2,3,4),(5,9,8,7,6),(0,5,6,1),(1,6,7,2),
        (2,7,8,3),(3,8,9,4),(4,9,5,0)],color,'pointed kite shield',.015)
    beam((x,y-.08,z+.2),(x,y-.12,z-.23),.055,'Gold','shield insignia')


def humanoid(role, armor=False, robe=False, offset=(0,0,0)):
    # Faceted hand-authored body proportions; face and equipment remain readable
    # under an RTS camera. This is a deliberately static mesh, not a fake rig.
    ox,oy,oz=offset
    def p(x,y,z):return(x+ox,y+oy,z+oz)
    color='Metal' if armor else 'Cloth'
    if robe:
        cyl(p(0,0,.67),.38,1.25,'Roof',8,.25,'wizard robe')
        ring(p(0,0,.18),.34,.035,'Gold')
    else:
        for x in (-.15,.15):
            beam(p(x,0,.19),p(x,.02,.9),.18,'Darkwood','fitted trousers')
            box(p(x,-.08,.09),(.23,.38,.18),'Darkwood','leather boots',.025)
    # Eight vertex tapered chest avoids the stock sphere-body silhouette.
    verts=[p(-.23,-.13,.89),p(.23,-.13,.89),p(.23,.13,.89),p(-.23,.13,.89),
        p(-.34,-.17,1.52),p(.34,-.17,1.52),p(.34,.17,1.52),p(-.34,.17,1.52)]
    mesh(verts,[(0,1,2,3),(4,7,6,5),(0,4,5,1),(1,5,6,2),(2,6,7,3),(3,7,4,0)],color,'tailored torso',.025)
    box(p(0,0,1),(.5,.32,.09),'Wood','waist belt',.018)
    box(p(0,-.18,1),(.09,.05,.1),'Gold','belt buckle',.01)
    cyl(p(0,0,1.59),.085,.18,'Skin',8,name='neck')
    ball(p(0,-.005,1.81),(.22,.2,.26),'Skin',2,'sculpted head')
    ball(p(0,-.204,1.81),(.055,.047,.055),'Skin',1,'nose')
    for x in (-.074,.074):
        box(p(x,-.191,1.86),(.025,.017,.034),'Black','eyes',0)
    if armor:
        ball(p(0,.02,1.98),(.255,.235,.19),'Metal',2,'helmet crown')
        box(p(0,0,1.98),(.04,.36,.26),'Gold','helmet central crest',.015)
        for x in (-.36,.36):ball(p(x,0,1.46),(.2,.22,.18),'Metal',1,'shoulder plate')
    else:
        ball(p(0,.035,1.99),(.23,.19,.13),'Hair',2,'hair cap')
    for x in (-.4,.4):
        handz=1.08 if x<0 else 1.17
        beam(p(x,0,1.42),p(x*1.3,-.04,handz),.14,color,'upper sleeve')
        ball(p(x*1.3,-.055,handz),(.105,.105,.11),'Skin',1,'hands')


def sword(x,y,z):
    beam((x,y,z-.24),(x,y,z+.04),.055,'Wood','sword hilt')
    beam((x-.16,y,z),(x+.16,y,z),.055,'Gold','sword crossguard')
    mesh([(x-.055,y-.022,z+.04),(x+.055,y-.022,z+.04),(x+.038,y-.022,z+.72),
          (x,y-.022,z+.86),(x-.038,y-.022,z+.72)],[(0,1,2,3,4)],'Metal','steel blade')


def wagonbase(length=3,width=1.65):
    box((0,0,.68),(width,length,.24),'Darkwood','siege carriage')
    for x in (-width*.55,width*.55):
        for y in (-length*.33,length*.33):
            ring((x,y,.47),.42,.095,'Wood',(0,PI/2,0),'wooden wheel rim')
            for a in range(4):
                dy,dz=math.sin(a*PI/2)*.35,math.cos(a*PI/2)*.35
                beam((x,y-dy,.47-dz),(x,y+dy,.47+dz),.07,'CutWood','wheel spokes')
            o=cyl((x,y,.47),.12,.22,'Metal',8,name='wheel axle hub');o.rotation_euler[1]=PI/2
    for y in (-length*.33,length*.33):beam((-width*.6,y,.47),(width*.6,y,.47),.09,'Metal','wheel axle')


def build_units():
    begin('Worker');humanoid('Worker')
    box((0,-.01,2.01),(.55,.44,.055),'CutWood','broad work hat')
    cyl((0,.015,2.1),.21,.16,'CutWood',8,.18,'work hat crown')
    beam((.51,-.08,.56),(.51,-.08,1.9),.047,'Wood','woodcutting axe handle')
    box((.64,-.08,1.75),(.35,.085,.26),'Metal','axe blade',.03)
    box((0,.2,1.19),(.46,.17,.42),'Wood','worker pack')
    finish('Worker','unit','Broad work hat, pack and hand axe')
    begin('Militia');humanoid('Militia');shield((-.57,-.12,1.13),'Wood');sword(.52,-.08,1.3)
    finish('Militia','unit','Cloth levy with wooden kite shield and short steel blade')
    begin('Spearman');humanoid('Spearman',True);shield((-.57,-.12,1.13))
    beam((.52,-.08,.1),(.52,-.08,2.8),.046,'Wood','long spear shaft')
    crystal((.52,-.08,2.8),.11,.38,'Metal')
    finish('Spearman','unit','Helmed infantry with long spear and metal kite shield')
    begin('Swordsman');humanoid('Swordsman',True);shield((-.57,-.12,1.13));sword(.52,-.08,1.32)
    box((0,.12,1.23),(.53,.12,.68),'Banner','short captain cape',.01)
    finish('Swordsman','unit','Armored sword captain with gold-tabarded shoulders and kite shield')
    begin('Archer');humanoid('Archer')
    ball((0,.07,1.98),(.25,.24,.17),'LeafDark',2,'archer hood')
    # Curved segmented wooden bow, distinct from straight spear profile.
    bow=[]
    for i in range(9):
        t=-PI*.47+i*PI*.94/8
        bow.append((-.53-.28*math.cos(t),-.13,1.32+.64*math.sin(t)))
    for a,b in zip(bow,bow[1:]):beam(a,b,.045,'Wood','bow stave')
    beam(bow[0],bow[-1],.011,'Plaster','bow string')
    cyl((.17,.24,1.5),.12,.65,'Wood',8,name='back quiver')
    for x in (.1,.18,.24):beam((x,.26,1.75),(x,.26,2.18),.018,'CutWood','quiver arrow')
    finish('Archer','unit','Hooded archer with bent segmented bow, string and visible back quiver')

    begin('Cavalry')
    # Angular horse silhouette: long neck, barrel, hocks, muzzle, saddle.
    ball((0,0,1.24),(.48,.95,.52),'Wood',2,'horse barrel')
    for x in (-.3,.3):
        for y in (-.55,.58):
            beam((x,y,1.25),(x,y+.1,.55),.18,'Wood','horse upper leg')
            beam((x,y+.1,.55),(x,y,.12),.13,'Darkwood','horse cannon bone')
            box((x,y-.025,.08),(.19,.25,.16),'Black','horse hoof')
    beam((0,-.63,1.3),(0,-.97,2),.43,'Wood','horse neck')
    ball((0,-1.16,2.07),(.27,.5,.29),'Wood',2,'horse head')
    ball((0,-1.48,1.98),(.25,.22,.18),'Darkwood',1,'horse muzzle')
    for x in (-.17,.17):cyl((x,-1.02,2.4),.085,.31,'Wood',5,0,'horse ear')
    beam((0,.85,1.4),(0,1.2,.8),.11,'Darkwood','horse tail')
    box((0,0,1.65),(.9,.83,.16),'RedCloth','saddle blanket')
    humanoid('Cavalry',True,offset=(0,.13,1.05))
    beam((.52,-.09,1.5),(.52,-.09,3.6),.04,'Wood','cavalry lance')
    crystal((.52,-.09,3.6),.09,.32,'Metal')
    flag(.52,-.09,2.65,height=.75)
    finish('Cavalry','unit','Armored mounted rider with long lance and sculpted horse')

    begin('Engineer');humanoid('Engineer')
    box((0,-.18,1.25),(.4,.06,.48),'CutWood','engineer leather apron')
    for x in (-.09,.09):
        ring((x,-.198,1.91),.063,.016,'Copper',(PI/2,0,0),'engineer goggles')
    beam((.52,-.07,.95),(.52,-.07,1.7),.06,'Wood','engineer hammer')
    box((.52,-.07,1.7),(.4,.18,.17),'Metal','hammer head')
    box((-.51,-.1,1.04),(.48,.32,.35),'Wood','toolbox')
    finish('Engineer','unit','Goggled engineer with apron, hammer and carried toolbox')

    begin('ArcaneGuard');humanoid('ArcaneGuard',True)
    shield((-.57,-.12,1.13),'Roof');crystal((-.57,-.27,1.03),.08,.27)
    beam((.52,-.07,.2),(.52,-.07,2.5),.055,'Darkwood','arcane glaive shaft')
    crystal((.52,-.07,2.5),.14,.48)
    for x in (-.34,.34):crystal((x,.02,1.6),.075,.3)
    finish('ArcaneGuard','unit','Teal glaive and runic shield guard with crystal shoulder crests')

    begin('ForestAlly')
    cyl((0,0,1.5),.4,1.45,'Darkwood',8,.32,'living timber torso')
    ball((0,0,2.4),(.36,.27,.44),'Wood',1,'ancient tree face')
    for x in (-.16,.16):ball((x,-.255,2.49),(.058,.023,.049),'Rune')
    for x in (-.27,.27):
        beam((x,0,1),(x*1.65,-.06,.2),.25,'Darkwood','root leg')
        for y in (-.27,.1):beam((x*1.65,0,.2),(x*1.85,y,.055),.13,'Wood','root toe')
    for side in (-1,1):
        beam((side*.31,0,1.97),(side*.9,-.03,1.5),.21,'Wood','branch arm')
        beam((side*.9,-.03,1.5),(side*1.09,-.1,1.72),.13,'Darkwood','branch claw')
        beam((side*.2,0,2.6),(side*.5,.03,3.15),.13,'Wood','antler branch')
        beam((side*.5,.03,3.15),(side*.86,.03,3.33),.08,'Wood','antler twig')
        ball((side*.5,.08,3.03),(.44,.3,.23),'LeafLight')
    ball((0,.21,2),(.65,.32,.38),'Leaf',1,'moss mantle')
    finish('ForestAlly','unit','Living grove guardian with branch antlers, root feet and light eyes')

    begin('Ram');wagonbase(3.4,1.8)
    for x in (-.75,.75):
        for y in (-1.1,1.1):beam((x,y,.77),(x,y,2.05),.16,'Darkwood','ram frame')
    roof(0,0,2.02,1.9,3.6,.7,tiles=False)
    o=cyl((0,-.15,1.22),.24,3.6,'Wood',10,name='suspended ram beam');o.rotation_euler[0]=PI/2
    o=cyl((0,-2.01,1.22),.28,.3,'Metal',10,name='iron ram head');o.rotation_euler[0]=PI/2
    for y in (-.8,.8):beam((0,y,1.48),(0,y,2.21),.034,'Metal','ram chain')
    finish('Ram','unit','Four wheeled covered battering ram with suspended timber and iron head')

    begin('Catapult');wagonbase(3.3,1.9)
    for x in (-.77,.77):
        beam((x,0,.79),(x,0,2.4),.19,'Wood','catapult upright')
        for y in (-1.3,1.3):beam((x,y,.79),(x,0,2.05),.15,'Darkwood','catapult brace')
    beam((-.9,0,2.21),(.9,0,2.21),.2,'Metal','catapult pivot axle')
    beam((0,-1.2,1.1),(0,1.33,3.11),.21,'Wood','throwing arm')
    ball((0,1.39,3.11),(.5,.47,.18),'Darkwood',1,'catapult throwing cup')
    ball((0,1.39,3.3),(.24,.24,.24),'Stone',1,'catapult rock')
    ring((0,0,1.35),.32,.08,'CutWood',(0,PI/2,0),'torsion skein')
    finish('Catapult','unit','Four wheeled torsion catapult, trestle, pivot and loaded throwing cup')

    begin('Wizard');humanoid('Wizard',robe=True)
    cyl((0,0,2.19),.3,.45,'Roof',8,0,'pointed wizard cap')
    cyl((0,0,2.01),.38,.045,'Roof',10,name='wizard hat brim')
    ring((0,0,2.03),.275,.021,'Gold')
    ball((0,-.185,1.65),(.15,.08,.22),'Plaster',1,'wizard beard')
    beam((.52,-.07,.12),(.52,-.07,2.54),.065,'Darkwood','wizard staff')
    ring((.52,-.07,2.57),.2,.038,'Gold',(PI/2,0,0),'staff cage')
    crystal((.52,-.07,2.41),.1,.37)
    box((-.52,-.12,1.2),(.31,.11,.4),'RedCloth','spellbook')
    box((-.52,-.181,1.2),(.19,.02,.24),'Banner','spellbook rune')
    finish('Wizard','unit','Bearded robed caster with crystal staff and red spellbook')


def tree_geometry(p=(0,0,0),scale=1,pine=False):
    x,y,z=p
    s=scale
    if pine:
        cyl((x,y,z+1.75*s),.24*s,3.5*s,'Darkwood',7,.16*s,'pine trunk')
        for h,r in ((1.35,1.45),(2.25,1.2),(3.1,.95),(3.8,.66)):
            cyl((x,y,z+h*s),r*s,1.8*s,'LeafDark' if h==1.35 else 'Leaf',8,0,'layered pine branches')
    else:
        cyl((x,y,z+1.65*s),.34*s,3.3*s,'Darkwood',7,.22*s,'oak trunk')
        for a,b in (((0,0,1.8),(-.85,.15,3)),((0,0,2),(1,.2,3.2)),((0,0,2.5),(.1,-.8,3.6))):
            beam((x+a[0]*s,y+a[1]*s,z+a[2]*s),(x+b[0]*s,y+b[1]*s,z+b[2]*s),.22*s,'Wood','oak branch')
        for i,(xx,yy,zz,sz) in enumerate(((-.8,.1,3.2,1.3),(.75,.12,3.6,1.35),(.1,-.6,3.8,1.3),(.0,.4,4.35,1.35))):
            ball((x+xx*s,y+yy*s,z+zz*s),(sz*s,sz*.9*s,sz*.8*s),'LeafLight' if i==3 else 'Leaf' if i!=0 else 'LeafDark',2,'oak canopy cluster')
        for a in range(5):
            t=a*2*PI/5
            beam((x,y,z+.5*s),(x+math.cos(t)*.7*s,y+math.sin(t)*.7*s,z+.07*s),.16*s,'Darkwood','buttress root')


def build_props():
    begin('TreeOak');tree_geometry();finish('TreeOak','environment','Broad clustered oak canopy with buttress roots and branching trunk','convex trunk / foliage no collision')
    begin('TreePine');tree_geometry(pine=True);finish('TreePine','environment','Tiered eight sided pine silhouette','convex trunk / foliage no collision')
    begin('Rock')
    ball((0,0,.7),(1.3,.9,.86),'Stone',2,'weathered boulder')
    ball((.6,-.33,.28),(.6,.5,.36),'StoneDark',1,'rock companion')
    ball((-.4,.12,1.32),(.62,.45,.13),'Grass',1,'rock moss cap')
    finish('Rock','resource','Moss capped faceted harvestable stone','convex hull')
    begin('Crystal')
    cyl((0,0,.2),1.1,.4,'StoneDark',8,name='crystal bed')
    crystal((0,0,.15),.36,1.9)
    crystal((.5,.2,.15),.24,1.1,tilt=(.25,.2,0))
    crystal((-.45,-.2,.15),.24,.96,tilt=(-.18,-.26,0))
    finish('Crystal','resource','Three differently angled six sided light crystals','convex hull')
    begin('Mushroom')
    for x,y,z,r in ((0,0,.45,.45),(.53,.22,.3,.3),(-.32,.4,.23,.24)):
        cyl((x,y,z/2),r*.18,z,'Plaster',8,r*.13,'mushroom stem')
        ball((x,y,z),(r,r,r*.33),'Mushroom',2,'umbrella mushroom cap')
        for a in range(3):
            t=a*2*PI/3
            ball((x+math.cos(t)*r*.43,y+math.sin(t)*r*.43,z+r*.23),(.06,.06,.035),'Plaster')
    finish('Mushroom','environment','Terracotta woodland mushroom cluster with cream flecks','none')
    begin('Ruin')
    foundation(3.7,2.7,.3)
    for x,h in ((-1.4,2.5),(1.4,1.7)):
        box((x,0,h/2+.3),(.7,.75,h),'Stone','ancient broken pillar',.065)
        box((x,0,h+.4),(.95,.92,.18),'StoneLight','broken capital')
    beam((-1.4,0,2.8),(.6,0,3),.38,'StoneLight','broken lintel')
    for x,y in ((-.1,-.8),(.7,-.5),(1.5,.7)):
        box((x,y,.38),(.75,.52,.3),'Stone','fallen masonry',.06,(.07,.02,x*.3))
    crystal((-.3,.5,.3),.23,1)
    finish('Ruin','environment','Broken old gateway, fallen cut stones and rediscovered light crystal')
    begin('GrassTuft')
    for a in range(5):
        t=a*2*PI/5
        h=.3+(a%3)*.12
        mesh([(0,0,0),(.13*math.cos(t),.13*math.sin(t),0),(.23*math.cos(t),.23*math.sin(t),h)],[(0,1,2)],'GrassLight','grass blade')
    finish('GrassTuft','environment','Five angular leaf blades','none')
    begin('Flowers')
    for i,(x,y,h) in enumerate(((0,0,.42),(.22,.13,.32),(-.22,.1,.27),(.1,-.24,.35))):
        beam((x,y,0),(x,y,h),.025,'Leaf','meadow flower stalk')
        for a in range(5):
            t=a*2*PI/5
            ball((x+.09*math.cos(t),y+.09*math.sin(t),h),(.075,.075,.027),
                'Flower' if i%2==0 else 'Violet',1,'five meadow flower petals')
        ball((x,y,h+.016),(.035,.035,.025),'Banner')
    finish('Flowers','environment','Buttercream and lavender five petal meadow cluster','none')
    begin('Lantern')
    beam((0,0,0),(0,0,2.15),.1,'Darkwood','lantern post')
    beam((0,0,2.15),(.48,0,2.15),.08,'Wood','lantern arm')
    box((.4,0,1.85),(.25,.25,.42),'Window','lantern light',.01)
    for x in (.255,.545):
        for y in (-.145,.145):beam((x,y,1.6),(x,y,2.1),.025,'Darkwood','lantern cage')
    cyl((.4,0,2.12),.25,.2,'Roof',4,0,'lantern cap')
    finish('Lantern','environment','Hanging amber light in timber roadside lantern','none')
    begin('Bridge')
    for j in range(18):
        y=-3.25+(j+.5)*6.5/18
        z=.52+.45*(1-(y/3.25)**2)
        box((0,y,z),(3.1,.34,.17),'CutWood','bridge deck plank',.025)
    for x in (-1.38,1.38):
        for j in range(7):
            y=-3.2+j*6.4/6
            z=.6+.45*(1-(y/3.25)**2)
            beam((x,y,z),(x,y,z+1),.11,'Darkwood','bridge railing post')
        for j in range(6):
            a=-3.2+j*6.4/6;b=-3.2+(j+1)*6.4/6
            beam((x,a,1.6+.45*(1-(a/3.25)**2)),(x,b,1.6+.45*(1-(b/3.25)**2)),.1,'Wood','bridge curved handrail')
    finish('Bridge','environment','Arched plank footbridge with fitted posts and curved handrail','multiple boxes / nav-link ends')


def place(name,p,scale=1,angle=0):
    source=ASSETS[name]['object']
    o=source.copy();o.data=source.data
    bpy.context.collection.objects.link(o)
    o.name=name+'_placed'
    o.hide_render=False;o.hide_viewport=False
    o.location=p;o.scale=(scale,scale,scale);o.rotation_euler[2]=angle
    return o


def pointlight(p,power=120,color=(1,.59,.24),radius=1.8):
    data=bpy.data.lights.new('Warm village lamp','POINT')
    data.energy=power;data.color=color;data.shadow_soft_size=radius
    o=bpy.data.objects.new('Warm village lamp',data);bpy.context.collection.objects.link(o);o.location=p
    return o


def area(name,p,power,size,color):
    data=bpy.data.lights.new(name,'AREA');data.energy=power;data.shape='DISK';data.size=size;data.color=color
    o=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(o);o.location=p
    o.rotation_euler=(Vector((0,1,0))-o.location).to_track_quat('-Z','Y').to_euler()
    return o


def terrain_z(x,y):
    # Two banks form the authored river valley. River channel itself remains flat.
    bank=min(1,max(0,(abs(x-2-math.sin(y*.14)*1.2)-2)/2))
    return .1+bank*(.28+.55*math.exp(-((x+12)**2+(y-8)**2)/95))


def make_scene():
    global PARTS
    PARTS=[]
    scene=bpy.context.scene
    coll=bpy.data.collections.new('Wildlight | Presentation Valley')
    scene.collection.children.link(coll)
    layer=bpy.context.view_layer.layer_collection.children.get(coll.name)
    bpy.context.view_layer.active_layer_collection=layer
    # Sculpted oval terrain base, with visible stone strata around its edge.
    cyl((0,1,-1.05),1,1.5,'StoneDark',64,name='diorama lower stratum').scale=(23.7,18.5,1)
    cyl((0,1,-.4),1,.45,'Earth',64,name='diorama earth stratum').scale=(23.5,18.25,1)
    segments=96;rings=24
    verts=[(0,1,terrain_z(0,1))]
    for r in range(1,rings+1):
        fraction=r/rings
        for i in range(segments):
            a=i*2*PI/segments
            x=math.cos(a)*23.5*fraction;y=1+math.sin(a)*18.25*fraction
            verts.append((x,y,terrain_z(x,y)))
    faces=[(0,1+i,1+(i+1)%segments) for i in range(segments)]
    for r in range(rings-1):
        for i in range(segments):
            a=1+r*segments+i;b=1+r*segments+(i+1)%segments
            c=a+segments;d=b+segments
            faces.extend(((a,c,d),(a,d,b)))
    o=mesh(verts,faces,'Grass','sculpted forest valley')
    o.data.materials.append(MATS['GrassLight']);o.data.materials.append(MATS['Leaf'])
    for p in o.data.polygons:
        c=p.center
        # Index pattern is deliberate low-contrast grass variation.
        p.material_index=0
        p.use_smooth=True
    # Winding water strip and polygonal reflective highlights.
    riververts=[]
    for j in range(38):
        y=-17+j*35.5/37
        x=2+math.sin(y*.14)*1.2
        w=2.1+.18*math.cos(y*.36)
        riververts.extend(((x-w,y,.135),(x+w,y,.135)))
    mesh(riververts,[(i*2,i*2+1,i*2+3,i*2+2) for i in range(37)],'Water','luminous winding river')
    for j in range(24):
        y=-15+j*1.3
        x=2+math.sin(y*.14)*1.2
        for side in (-1,1):
            xx=x+side*(2.12+.12*math.cos(y*.36))
            ball((xx,y,.3),(.36,.58,.25),'Stone' if j%3 else 'StoneLight',1,'river bank stone')
        if j%3==0:
            beam((x-.7,y,.144),(x+.2,y+.2,.144),.038,'Foam','stylized river glint')
    # Paths to the castle, market, mill, farm and bridge.
    def path(points,width=1.2):
        for a,b in zip(points,points[1:]):
            v=Vector(b)-Vector(a);v.normalize();side=Vector((-v.y,v.x,0))*width/2
            va=Vector((a[0],a[1],terrain_z(a[0],a[1])+.025));vb=Vector((b[0],b[1],terrain_z(b[0],b[1])+.025))
            mesh([va+side,va-side,vb-side,vb+side],[(0,1,2,3)],'Path','village footpath')
    path([(-10,6),(-9,1),(-7,-4),(-4,-5),(0,-5),(4,-5),(8,-3),(12,0)],1.6)
    path([(-8,-3),(-13,-5),(-15,-8)],1.15)
    path([(-7,-4),(-4,-9)],1.1)
    path([(-9,1),(-14,0),(-16,3)],1.1)
    placements=[('Castle',(-10,7,.58),.95,0),('House',(-12.8,-3,.51),.76,-.15),
        ('House',(-6.8,-7.6,.44),.74,.12),('Farm',(-15.5,-8.3,.4),.68,.15),
        ('Mill',(-16.5,1.6,.61),.78,-.1),('Market',(-6.5,-2.2,.45),.75,.2),
        ('Depot',(-16,6,.75),.65,.12),('Forge',(-5,4,.5),.62,0),
        ('MageTower',(11.2,8,.45),.86,-.15),('Sanctuary',(11.8,-.6,.4),1,0),
        ('Beacon',(7,-6,.42),1.1,0),('Beacon',(-3.7,.7,.36),.9,0),
        ('Ruin',(16.5,-8,.4),.85,.3),('Watchtower',(-1.5,7.8,.45),.62,0)]
    for name,p,s,r in placements:place(name,p,s,r)
    # Boundary wall and gate flank the castle without hiding the village.
    for x in (-15,-10,-5):place('Wall',(x,2.65,.51),.64,0)
    place('Gate',(-8.2,2.65,.5),.6,0)
    bridge=place('Bridge',(1.17,-5.15,.04),1,PI/2)
    # Living settlement figures are actual exported meshes.
    for name,p,s,r in [('Worker',(-10,-3,.5),1,0),('Worker',(-13,-6,.45),1,.7),
            ('Archer',(-3,5,.4),1.1,-.5),('Spearman',(-8,1,.5),1,.2),
            ('Wizard',(9,-3,.45),1.1,-.2),('ForestAlly',(13,-4,.44),1,0),
            ('Cavalry',(-6,-4.5,.44),.85,1),('Catapult',(-2,6,.42),.7,.15)]:
        place(name,p,s,r)
    # Place designed forest clusters around negative space and settlement edges.
    tree_points=[(-19,3,1.05),(-19,7,.95),(-17.5,11,1.1),(-14,13,1.1),
        (-10,14,.95),(-6,14,1.08),(-2,12,1),(-1,15,.98),(3,14,.9),
        (6,14,1.1),(15,12,1.1),(18,8,1),(20,4,.98),(17,2,.8),
        (18,-2,1.07),(20,-5,.92),(15,-11,1.1),(11,-12,.9),(7,-13,1.02),
        (-18,-7,.95),(-18,-3,.9),(-15,-12,.96),(-10,-12,.78),(-4,-13,.85)]
    for i,(x,y,s) in enumerate(tree_points):
        place('TreePine' if i%3==0 else 'TreeOak',(x,y,terrain_z(x,y)),s,(i%5)*.37)
        if i%2==0:place('Mushroom',(x+.7,y-.9,terrain_z(x,y)),.8,i*.2)
    for i,(x,y,s) in enumerate([(-19,0,1),(-2,11,.8),(6,11,1),(18,-7,1),(9,-10,.7),(-10,-10,.75),(15,4,.8)]):
        place('Rock',(x,y,terrain_z(x,y)),s,i*.6)
    for x,y in ((9,3),(15.5,7),(-2,10)):
        place('Crystal',(x,y,terrain_z(x,y)),.7,.3)
    for i in range(480):
        # A deterministic low-discrepancy distribution, not random asset scatter.
        x=-21+(i*7.313)%42;y=-14+(i*3.917)%29
        if (x/22)**2+((y-1)/17)**2>.91:continue
        if abs(x-2-math.sin(y*.14)*1.2)<2.6:continue
        if -18<x<-3 and -10<y<12:continue
        place('GrassTuft',(x,y,terrain_z(x,y)),.7+(i%4)*.13,i*.7)
        if i%3==0:place('Flowers',(x+.3,y+.1,terrain_z(x,y)),1,i*.4)
    for x,y in ((-9,-2),(-4,-5),(5,-5),(9,-2),(-15,-5)):
        place('Lantern',(x,y,terrain_z(x,y)),1)
        pointlight((x+.4,y,terrain_z(x,y)+1.85),55)
    for p in ((7,-6,3.5),(11.8,-.6,3),(-3.7,.7,2.8),(11.2,8,8.4)):
        pointlight(p,110,(.21,.91,.81),2.5)
    for p in ((-10,4,3),(-12.8,-4,2),(-6.8,-9,2),(-5,3,2),(-16.5,1,3)):
        pointlight(p,105,(1,.5,.15),2.7)
    # Discrete emissive firefly meshes. Visibility is controlled per render mode.
    fireflies=[]
    for i in range(80):
        x=-18+(i*5.213)%37;y=-12+(i*4.773)%27;z=1.3+(i%7)*.35
        if abs(x-2-math.sin(y*.14)*1.2)>3 and i%3:continue
        fireflies.append(ball((x,y,z),(.028,.028,.028),'Window',1,'Firefly'))
    # Visible light network links encode a clear intentional scenic identity.
    for a,b in [((-3.7,.7,2.8),(7,-6,3)),((7,-6,3),(11.8,-.6,2.9)),((11.8,-.6,2.9),(11.2,8,8.8))]:
        steps_n=18
        prev=None
        for i in range(steps_n+1):
            t=i/steps_n
            p=Vector(a).lerp(Vector(b),t)
            p.z+=.65*math.sin(PI*t)
            if prev is not None:beam(prev,p,.035,'Crystal','light network thread')
            prev=p
    # Matte studio floor gives the valley a tangible object edge.
    box((0,0,-2.1),(200,200,.2),'Black','presentation floor',0)
    area('Key warm sky',(-16,-10,28),2600,18,(1,.86,.65))
    area('Soft sky fill',(18,10,20),1800,20,(.58,.78,1))
    data=bpy.data.lights.new('Sun','SUN');data.energy=2;data.angle=.17
    sun=bpy.data.objects.new('Sun',data);bpy.context.collection.objects.link(sun)
    sun.rotation_euler=(.45,-.45,-.6)
    world=bpy.data.worlds.new('Wildlight atmospheric world')
    world.use_nodes=True;scene.world=world
    world.node_tree.nodes['Background'].inputs['Color'].default_value=(.21,.3,.3,1)
    world.node_tree.nodes['Background'].inputs['Strength'].default_value=.6
    data=bpy.data.cameras.new('Valley showcase camera')
    cam=bpy.data.objects.new('Valley showcase camera',data);bpy.context.collection.objects.link(cam)
    cam.location=(35,-45,39)
    cam.rotation_euler=(Vector((0,1,1.6))-cam.location).to_track_quat('-Z','Y').to_euler()
    data.type='ORTHO';data.ortho_scale=58
    scene.camera=cam
    scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=64
    # Debian Blender build has no OpenImageDenoise support. Pure CPU samples
    # retain an inspectable render without depending on unavailable GPU kernels.
    scene.cycles.use_denoising=False
    scene.render.resolution_x=1280;scene.render.resolution_y=720;scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG';scene.render.film_transparent=False
    scene.view_settings.view_transform='AgX';scene.view_settings.look='AgX - Medium High Contrast'
    scene.view_settings.exposure=.7
    scene.use_nodes=True
    nodes=scene.node_tree.nodes;nodes.clear()
    rl=nodes.new('CompositorNodeRLayers')
    glare=nodes.new('CompositorNodeGlare');glare.glare_type='FOG_GLOW';glare.quality='MEDIUM';glare.threshold=1.7
    comp=nodes.new('CompositorNodeComposite')
    scene.node_tree.links.new(rl.outputs['Image'],glare.inputs['Image'])
    scene.node_tree.links.new(glare.outputs['Image'],comp.inputs['Image'])
    return scene


def scene_lighting(mode):
    scene=bpy.context.scene
    night=mode=='night'
    bpy.data.lights['Sun'].energy=.19 if night else 2
    bpy.data.lights['Sun'].color=(.42,.64,1) if night else (1,.88,.66)
    bpy.data.lights['Key warm sky'].energy=180 if night else 2600
    bpy.data.lights['Soft sky fill'].energy=1450 if night else 1800
    scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.14 if night else .6
    scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.026,.064,.12,1) if night else (.21,.3,.3,1)
    scene.view_settings.exposure=1.55 if night else .7
    for o in bpy.data.objects:
        if o.name.startswith('Firefly'):o.hide_render=not night


def render(mode):
    scene_lighting(mode)
    bpy.context.scene.cycles.use_denoising=False
    bpy.context.scene.cycles.samples=192 if mode=='night' else 96
    bpy.context.scene.render.filepath=str(RENDERS/('Wildlight_Asset_Valley_'+mode.title()+'.png'))
    bpy.ops.render.render(write_still=True)


def catalog():
    scene=bpy.context.scene
    valley=bpy.data.collections['Wildlight | Presentation Valley']
    valley.hide_render=True
    cat=bpy.data.collections.new('Wildlight | Asset Catalog');scene.collection.children.link(cat)
    bpy.context.view_layer.active_layer_collection=bpy.context.view_layer.layer_collection.children[cat.name]
    names=list(ASSETS)
    for i,name in enumerate(names):
        row=i//8;col=i%8
        p=(-30+col*8.5,20-row*10.4,0)
        bounds=ASSETS[name]['bounds_m'];width=max(bounds['max'][0]-bounds['min'][0],bounds['max'][1]-bounds['min'][1])
        scale=min(1,6.8/width)
        place(name,p,scale,0)
        bpy.ops.object.text_add(location=(p[0],p[1]-4.15,.02))
        text=bpy.context.object;text.name='Label_'+name;text.data.body=name
        text.data.align_x='CENTER';text.data.size=.5;text.data.extrude=.005;text.data.materials.append(MATS['Plaster'])
    box((1,-5,-.16),(80,76,.2),'Black','catalog floor',0)
    area('Catalog key',(-10,-5,35),4200,30,(1,.9,.75))
    area('Catalog fill',(28,22,30),3600,35,(.65,.8,1))
    data=bpy.data.cameras.new('Catalog camera');cam=bpy.data.objects.new('Catalog camera',data)
    cat.objects.link(cam);cam.location=(35,-51,67)
    cam.rotation_euler=(Vector((0,-4,0))-cam.location).to_track_quat('-Z','Y').to_euler()
    data.type='ORTHO';data.ortho_scale=93
    scene.camera=cam
    scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.5
    scene.view_settings.exposure=1
    scene.render.resolution_x=1600;scene.render.resolution_y=1250
    scene.render.filepath=str(RENDERS/'Wildlight_Asset_Catalog.png')
    bpy.ops.render.render(write_still=True)
    cat.hide_render=True;valley.hide_render=False
    scene.camera=bpy.data.objects['Valley showcase camera']
    scene.render.resolution_x=1280;scene.render.resolution_y=720


def main():
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    if '--render-only' in args:
        bpy.ops.wm.open_mainfile(filepath=str(ROOT/'Source'/'Wildlight_Valley.blend'))
        for name in PALETTE:
            existing=bpy.data.materials.get('WL_'+name)
            if existing:MATS[name]=existing
            else:material(name,PALETTE[name])
        if '--night-only' in args:render('night')
        elif '--catalog-only' in args:
            manifest=json.loads((ROOT/'manifest.json').read_text())
            for entry in manifest['assets']:
                ASSETS[entry['id']]={**entry,'object':bpy.data.objects[entry['mesh_name']]}
            bpy.context.scene.cycles.use_denoising=False
            bpy.context.scene.cycles.samples=64
            catalog()
        else:render('day');render('night')
        return
    init()
    # Source originals live in an explicit library, independent from diorama copies.
    source=bpy.data.collections.new('Wildlight | Authored Asset Library')
    bpy.context.scene.collection.children.link(source)
    bpy.context.view_layer.active_layer_collection=bpy.context.view_layer.layer_collection.children[source.name]
    build_buildings();build_units();build_props()
    manifest={'schema_version':1,'project':'Wildlight','authoring':'Original geometry authored with Blender Python',
        'blender_version':bpy.app.version_string,'unit':'metres','unreal_unit_conversion':'FBX metres to Unreal centimetres, import uniform scale 1',
        'material_palette_srgb':{'WL_'+k:'#'+v for k,v in PALETTE.items()},
        'format_version':1,'units':'meters',
        'palette':{'WL_'+k:[linear(int(v[i:i+2],16)/255) for i in (0,2,4)]+[1] for k,v in PALETTE.items()},
        'texture_dependency':'none','source':'Source/Wildlight_Valley.blend',
        'review_status':'Source/export geometry validated; pending Unreal integration and GPU visual acceptance',
        'assets':[{k:v for k,v in asset.items() if k!='object'} for asset in ASSETS.values()]}
    (ROOT/'asset_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    (ROOT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    scene=make_scene()
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Source'/'Wildlight_Valley.blend'))
    print('WILDLIGHT_ASSETS_EXPORTED',len(ASSETS),sum(a['triangles'] for a in ASSETS.values()))
    if '--skip-render' not in args:
        render('day');render('night');catalog()
        scene_lighting('day')
        bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Source'/'Wildlight_Valley.blend'))


if __name__=='__main__':
    main()
