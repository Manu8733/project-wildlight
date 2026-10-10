"""Blender independent FBX round-trip checks against the exported manifest."""
from pathlib import Path
import bpy
import json
import math
import struct

ROOT=Path(__file__).resolve().parents[1]
manifest=json.loads((ROOT/'manifest.json').read_text())
report={'format_version':1,'blender_version':bpy.app.version_string,
        'test':'Independent FBX import and GLB container validation',
        'gameplay_or_gpu_test':False,'assets':[],'failures':[]}

for entry in manifest['assets']:
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.fbx(filepath=str(ROOT/entry['fbx']))
    meshes=[o for o in bpy.context.scene.objects if o.type=='MESH']
    issues=[]
    if len(meshes)!=1:
        issues.append('Expected one joined static mesh, found '+str(len(meshes)))
    imported_triangles=0
    imported_min=[];imported_max=[]
    if meshes:
        o=meshes[0]
        vertices=[o.matrix_world@v.co for v in o.data.vertices]
        imported_min=[min(v[i] for v in vertices) for i in range(3)]
        imported_max=[max(v[i] for v in vertices) for i in range(3)]
        for actual,expected in zip(imported_min+imported_max,entry['bounds_min']+entry['bounds_max']):
            if not math.isfinite(actual) or abs(actual-expected)>.002:
                issues.append('Round-trip bounds / scale mismatch')
                break
        if abs(imported_min[2])>.001:
            issues.append('Asset does not sit on ground zero')
        if o.location.length>.001:
            issues.append('Object ground pivot moved during FBX round trip')
        if any(not math.isfinite(v) for p in vertices for v in p):
            issues.append('Nonfinite vertices')
        imported_triangles=sum(len(p.vertices)-2 for p in o.data.polygons)
        if imported_triangles!=entry['triangles']:
            issues.append('Triangle count changed')
        material_names=sorted(m.name for m in o.data.materials)
        # Blender appends .001 suffix on retained datablocks from prior imports.
        if len(material_names)!=len(entry['material_names']):
            issues.append('Material slot count changed')
    blob=(ROOT/entry['glb']).read_bytes()
    magic,version,length=struct.unpack('<4sII',blob[:12])
    if magic!=b'glTF' or version!=2 or length!=len(blob):
        issues.append('Invalid GLB header')
    size,kind=struct.unpack('<I4s',blob[12:20])
    gltf=json.loads(blob[20:20+size])
    if len(gltf.get('meshes',[]))!=1:
        issues.append('GLB expected one mesh')
    if any('uri' in b for b in gltf.get('buffers',[])) or gltf.get('images'):
        issues.append('GLB unexpected external texture/buffer dependency')
    result={'id':entry['id'],'pass':not issues,'issues':issues,
        'fbx_mesh_count':len(meshes),'triangles':imported_triangles,
        'bounds_min':imported_min,'bounds_max':imported_max,
        'glb_bytes':len(blob)}
    report['assets'].append(result)
    report['failures'].extend(entry['id']+': '+i for i in issues)

report['passed']=not report['failures']
report['count']=len(report['assets'])
(ROOT/'validation_report.json').write_text(json.dumps(report,indent=2)+'\n')
print('WILDLIGHT_ART_VALIDATION', 'PASS' if report['passed'] else 'FAIL', report['count'], report['failures'])
if not report['passed']:
    raise RuntimeError('Wildlight art round-trip validation failed')
