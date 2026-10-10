"""Initial weighted Worker/Wizard rigs and measured animation studies.

Keeps the static library unchanged. These are prototype animation studies,
not production-approved rigs or a claim of Unreal skeletal integration.
Run with Blender 4.3.2: blender -b --python Art/Source/build_unit_rigs.py
"""
from pathlib import Path
import bpy
import math
import json
import struct

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'Exports'/'Rigged';OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'Source'/'Wildlight_Valley.blend'))

copies={}
for name in ('Worker','Wizard'):
    src=bpy.data.objects['SM_'+name]
    obj=bpy.data.objects.new('SK_'+name,src.data.copy())
    bpy.context.scene.collection.objects.link(obj)
    obj.hide_render=False;obj.hide_viewport=False
    copies[name]=obj
for obj in list(bpy.data.objects):
    if obj not in copies.values():bpy.data.objects.remove(obj,do_unlink=True)

BONES=[
    ('Root',(0,0,0),(0,0,.15),None),
    ('Pelvis',(0,0,.68),(0,0,.81),'Root'),
    ('Spine',(0,0,.81),(0,0,1.20),'Pelvis'),
    ('Neck',(0,0,1.20),(0,0,1.31),'Spine'),
    ('Head',(0,0,1.31),(0,0,1.55),'Neck'),
    ('UpperArm.L',(-.27,0,1.20),(-.32,-.012,1.05),'Spine'),
    ('Forearm.L',(-.32,-.012,1.05),(-.43,-.044,.87),'UpperArm.L'),
    ('Hand.L',(-.43,-.044,.87),(-.46,-.05,.83),'Forearm.L'),
    ('UpperArm.R',(.27,0,1.20),(.32,-.012,1.05),'Spine'),
    ('Forearm.R',(.32,-.012,1.05),(.43,-.044,.94),'UpperArm.R'),
    ('Hand.R',(.43,-.044,.94),(.46,-.05,.89),'Forearm.R'),
    ('Thigh.L',(-.12,0,.68),(-.12,.016,.43),'Pelvis'),
    ('Shin.L',(-.12,.016,.43),(-.12,0,.14),'Thigh.L'),
    ('Foot.L',(-.12,0,.14),(-.12,-.15,.075),'Shin.L'),
    ('Thigh.R',(.12,0,.68),(.12,.016,.43),'Pelvis'),
    ('Shin.R',(.12,.016,.43),(.12,0,.14),'Thigh.R'),
    ('Foot.R',(.12,0,.14),(.12,-.15,.075),'Shin.R')
]


def assign_vertex(name,v,material_names):
    x,y,z=v.co
    # Explicit accessory regions keep tools and the whole crystal staff rigidly
    # attached to the hand rather than accidentally weighting their tops to Head.
    if x>.374 and (z<.76 or z>1.12):return [('Hand.R',1)]
    if name=='Wizard' and x>.19 and z>1.88:return [('Hand.R',1)]
    if name=='Wizard' and x<-.29 and 'WL_RedCloth' in material_names:return [('Hand.L',1)]
    if abs(x)>.295 and .72<z<1.30:
        side='R' if x>0 else 'L'
        if z>1.12:return [('UpperArm.'+side,1)]
        if z<.96:return [('Hand.'+side,1)]
        t=min(1,max(0,(z-.96)/.16))
        return [('Forearm.'+side,1-t),('UpperArm.'+side,t)]
    if z>1.30:return [('Head',1)]
    if z>1.22:return [('Neck',.35),('Spine',.65)]
    if z>.79:return [('Spine',1)]
    if name=='Wizard':return [('Pelvis',1)]
    if z>.64:return [('Pelvis',1)]
    side='R' if x>=0 else 'L'
    if z<.17:return [('Foot.'+side,1)]
    if z>.49:return [('Thigh.'+side,1)]
    if z<.37:return [('Shin.'+side,1)]
    t=(z-.37)/.12
    return [('Shin.'+side,1-t),('Thigh.'+side,t)]


def make_rig(name,obj):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True);bpy.context.view_layer.objects.active=obj
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.subdivide(number_cuts=1,smoothness=0)
    bpy.ops.object.mode_set(mode='OBJECT')
    data=bpy.data.armatures.new(name+'_Skeleton')
    rig=bpy.data.objects.new(name+'_Rig',data)
    bpy.context.scene.collection.objects.link(rig)
    obj.select_set(False);rig.select_set(True);bpy.context.view_layer.objects.active=rig
    bpy.ops.object.mode_set(mode='EDIT')
    for bone_name,head,tail,parent in BONES:
        bone=data.edit_bones.new(bone_name);bone.head=head;bone.tail=tail
        if parent:bone.parent=data.edit_bones[parent]
    bpy.ops.object.mode_set(mode='OBJECT')
    rig.show_in_front=True;data.display_type='OCTAHEDRAL'
    obj.parent=rig
    mod=obj.modifiers.new('Original Wildlight prototype skeleton','ARMATURE');mod.object=rig
    groups={bone_name:obj.vertex_groups.new(name=bone_name) for bone_name,*_ in BONES}
    vertex_mats={v.index:set() for v in obj.data.vertices}
    for p in obj.data.polygons:
        mat=obj.data.materials[p.material_index].name
        for vi in p.vertices:vertex_mats[vi].add(mat)
    for v in obj.data.vertices:
        for bone,weight in assign_vertex(name,v,vertex_mats[v.index]):
            if weight>0:groups[bone].add([v.index],weight,'REPLACE')
    for pb in rig.pose.bones:pb.rotation_mode='XYZ'
    return rig


def pose(rig,frame,rotations=None,root_z=0):
    bpy.context.scene.frame_set(frame)
    rotations=rotations or {}
    for pb in rig.pose.bones:
        pb.rotation_euler=rotations.get(pb.name,(0,0,0))
        pb.location=(0,0,root_z if pb.name=='Root' else 0)
        pb.keyframe_insert(data_path='rotation_euler',frame=frame)
        pb.keyframe_insert(data_path='location',frame=frame)


def action(rig,name,frames,pose_builder,loop=False):
    act=bpy.data.actions.new(name);act.use_fake_user=True
    rig.animation_data_create();rig.animation_data.action=act
    for frame in frames:
        rotations,root_z=pose_builder(frame)
        pose(rig,frame,rotations,root_z)
    for fc in act.fcurves:
        for key in fc.keyframe_points:key.interpolation='BEZIER'
        if loop:fc.modifiers.new('CYCLES')
    rig.animation_data.action=None
    track=rig.animation_data.nla_tracks.new();track.name=name
    strip=track.strips.new(name,1,act);strip.extrapolation='NOTHING'
    return act


def walk(frame):
    phase=(frame-1)*math.tau/24;s=math.sin(phase)
    return {'Thigh.L':(.28*s,0,0),'Thigh.R':(-.28*s,0,0),
        'Shin.L':(.35*max(0,-s),0,0),'Shin.R':(.35*max(0,s),0,0),
        'UpperArm.L':(-.17*s,0,0),'UpperArm.R':(.17*s,0,0),
        'Spine':(0,0,.025*s),'Head':(0,0,-.02*s)},.018*abs(math.sin(phase))


def work(frame):
    strength={1:0,10:.55,16:1,22:-.35,29:.1,37:0}[frame]
    return {'UpperArm.R':(-.95*strength,0,-.1*strength),
        'Forearm.R':(.28*strength,0,0),'Spine':(.07*strength,0,-.06*strength),
        'Head':(.05*strength,0,0)},0


def idle(frame):
    t=(frame-1)*math.tau/48
    return {'Spine':(.012*math.sin(t),0,0),'Head':(0,0,.025*math.sin(t)),
        'UpperArm.R':(.013*math.sin(t),0,0)},.004*math.sin(t)


def cast(frame):
    power={1:0,10:.3,19:1,27:1,36:.4,49:0}[frame]
    return {'UpperArm.R':(-.42*power,-.13*power,.08*power),
        'Forearm.R':(.20*power,0,-.1*power),'UpperArm.L':(-.57*power,0,-.17*power),
        'Forearm.L':(.35*power,0,0),'Spine':(-.025*power,0,.07*power),
        'Head':(.02*power,0,-.07*power)},0


def glb_json(path):
    b=path.read_bytes();size,kind=struct.unpack('<I4s',b[12:20]);return json.loads(b[20:20+size])


records=[];rigs={}
for name,obj in copies.items():
    rig=make_rig(name,obj);rigs[name]=rig
    if name=='Worker':
        actions=[action(rig,'Worker_Walk',range(1,26,3),walk,True),
                 action(rig,'Worker_Work',(1,10,16,22,29,37),work)]
    else:
        actions=[action(rig,'Wizard_Idle',range(1,50,6),idle,True),
                 action(rig,'Wizard_Cast',(1,10,19,27,36,49),cast)]
    # Include terminal loop frames explicitly, guaranteeing the first/last pose.
    first=actions[0];rig.animation_data.action=first
    if name=='Worker':pose(rig,25,*walk(25))
    else:pose(rig,49,*idle(49))
    rig.animation_data.action=None
    for tr in rig.animation_data.nla_tracks:tr.mute=False
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True);rig.select_set(True);bpy.context.view_layer.objects.active=rig
    bpy.context.scene.render.fps=24;bpy.context.scene.frame_start=1;bpy.context.scene.frame_end=49
    glb=OUT/('SK_'+name+'.glb');fbx=OUT/('SK_'+name+'.fbx')
    bpy.ops.export_scene.gltf(filepath=str(glb),export_format='GLB',use_selection=True,
        export_animations=True,export_animation_mode='NLA_TRACKS',export_skins=True,
        export_apply=False,export_force_sampling=True)
    bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={'MESH','ARMATURE'},
        axis_forward='-Y',axis_up='Z',apply_unit_scale=True,apply_scale_options='FBX_SCALE_UNITS',
        add_leaf_bones=False,bake_anim=True,bake_anim_use_all_actions=False,
        bake_anim_use_nla_strips=True,bake_anim_simplify_factor=0)
    data=glb_json(glb)
    normalized=all(abs(sum(g.weight for g in v.groups)-1)<.0001 for v in obj.data.vertices)
    # Test the evaluated mesh in a chosen clip, isolating NLA tracks.
    for tr in rig.animation_data.nla_tracks:tr.mute=True
    rig.animation_data.action=actions[1]
    def coords(frame):
        bpy.context.scene.frame_set(frame)
        graph=bpy.context.evaluated_depsgraph_get();ev=obj.evaluated_get(graph)
        evaluated=ev.to_mesh();out=[v.co.copy() for v in evaluated.vertices];ev.to_mesh_clear();return out
    a=coords(1);b=coords(16 if name=='Worker' else 19)
    maximum_movement=max((x-y).length for x,y in zip(a,b))
    record={'id':name,'mesh_name':'SK_'+name,'rig_bones':len(rig.data.bones),
        'weighted_vertices':len(obj.data.vertices),'weights_normalized':normalized,
        'glb_skins':len(data.get('skins',[])),
        'glb_animations':[a.get('name') for a in data.get('animations',[])],
        'fbx':str(fbx.relative_to(ROOT)),'glb':str(glb.relative_to(ROOT)),
        'animation_frames':{a.name:list(a.frame_range) for a in actions},'fps':24,
        'maximum_measured_work_cast_displacement_m':maximum_movement,
        'status':'initial weighted prototype; deformation/foot sliding and Unreal import need visual review'}
    record['validation_pass']=normalized and record['glb_skins']==1 and len(record['glb_animations'])==2 and maximum_movement>.03
    records.append(record)
    rig.animation_data.action=actions[0]
    rig.location.x=-1.2 if name=='Worker' else 1.2

bpy.context.scene.frame_set(1);bpy.context.scene.frame_end=49
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'Source'/'Wildlight_Unit_Rigs.blend'))
manifest={'format_version':1,'units':'meters','blender_version':bpy.app.version_string,
    'authoring':'Original prototype armatures, explicit normalized bone weights and keyframes',
    'source':'Source/Wildlight_Unit_Rigs.blend','static_library_unchanged':True,
    'unreal_skeletal_integration_tested':False,'production_animation_accepted':False,
    'unrigged_unit_ids':['Militia','Spearman','Swordsman','Archer','Cavalry','Engineer','ArcaneGuard','ForestAlly','Ram','Catapult'],
    'assets':records,'passed':all(r['validation_pass'] for r in records)}
(ROOT/'animation_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('WILDLIGHT_PROTOTYPE_RIGS',manifest['passed'],json.dumps(records))
if not manifest['passed']:raise RuntimeError('Prototype rig validation failed')
