"""Run inside Unreal Editor Python, never ordinary CPython.

Imports authored static meshes and builds original palette materials. This script
is prepared in cloud; successful Unreal execution must be recorded separately.
"""
from __future__ import annotations
import json
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / "Art"
DATA = json.loads((ART / "manifest.json").read_text(encoding="utf-8"))
TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
LIB = unreal.EditorAssetLibrary
EDIT = unreal.MaterialEditingLibrary
MESH_FOLDER = "/Game/Wildlight/Meshes"
MAT_FOLDER = "/Game/Wildlight/Materials"
MASTER_PATH = MAT_FOLDER + "/M_Wildlight"


def master_material():
    material = LIB.load_asset(MASTER_PATH)
    if material:
        return material
    LIB.make_directory(MAT_FOLDER)
    material = TOOLS.create_asset("M_Wildlight", MAT_FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    tint = EDIT.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -500, -100)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(0.48, 0.55, 0.40, 1.0))
    roughness = EDIT.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -500, 100)
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", 0.8)
    emission = EDIT.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -500, 260)
    emission.set_editor_property("parameter_name", "Emission")
    emission.set_editor_property("default_value", 0.0)
    multiply = EDIT.create_material_expression(material, unreal.MaterialExpressionMultiply, -240, 260)
    EDIT.connect_material_expressions(tint, "", multiply, "A")
    EDIT.connect_material_expressions(emission, "", multiply, "B")
    EDIT.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
    EDIT.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    EDIT.connect_material_property(multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    EDIT.recompile_material(material)
    LIB.save_loaded_asset(material)
    return material


def palette_materials(parent):
    result = {}
    for name, rgba in DATA.get("palette", {}).items():
        asset_name = "MI_" + "".join(c for c in name if c.isalnum() or c == "_")
        path = MAT_FOLDER + "/" + asset_name
        instance = LIB.load_asset(path)
        if not instance:
            instance = TOOLS.create_asset(asset_name, MAT_FOLDER, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        EDIT.set_material_instance_parent(instance, parent)
        EDIT.set_material_instance_vector_parameter_value(instance, "Tint", unreal.LinearColor(*rgba[:3], rgba[3] if len(rgba) > 3 else 1.0))
        glow = 1.8 if any(word in name.lower() for word in ("glow", "light", "crystal", "emissive")) else 0.0
        EDIT.set_material_instance_scalar_parameter_value(instance, "Emission", glow)
        EDIT.set_material_instance_scalar_parameter_value(instance, "Roughness", 0.8)
        LIB.save_loaded_asset(instance)
        result[name] = instance
    return result


def import_mesh(entry, palette):
    filename = (ART / entry["fbx"]).resolve()
    if ART.resolve() not in filename.parents or not filename.is_file():
        raise RuntimeError("Missing or unsafe mesh path: " + str(filename))
    asset_name = entry.get("mesh_name", "SM_" + entry["id"])
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(filename))
    task.set_editor_property("destination_path", MESH_FOLDER)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    mesh_options = options.get_editor_property("static_mesh_import_data")
    mesh_options.set_editor_property("combine_meshes", True)
    mesh_options.set_editor_property("auto_generate_collision", True)
    mesh_options.set_editor_property("convert_scene", True)
    mesh_options.set_editor_property("convert_scene_unit", True)
    task.set_editor_property("options", options)
    TOOLS.import_asset_tasks([task])
    imported = list(task.get_editor_property("imported_object_paths"))
    mesh = LIB.load_asset(MESH_FOLDER + "/" + asset_name)
    if not imported or not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError("FBX import did not produce the expected StaticMesh: " + asset_name)
    for index, slot in enumerate(mesh.get_editor_property("static_materials")):
        name = str(slot.get_editor_property("material_slot_name"))
        if name in palette:
            mesh.set_material(index, palette[name])
    LIB.save_loaded_asset(mesh)
    unreal.log("WILDLIGHT imported " + asset_name)


def make_level():
    map_path = "/Game/Wildlight/Maps/Valley"
    subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not LIB.does_asset_exist(map_path):
        LIB.make_directory("/Game/Wildlight/Maps")
        if not subsystem.new_level(map_path):
            raise RuntimeError("Could not create Valley level")
    elif not subsystem.load_level(map_path):
        raise RuntimeError("Could not open Valley level")
    world = unreal.EditorLevelLibrary.get_editor_world()
    mode = unreal.load_class(None, "/Script/Wildlight.WildlightGameMode")
    if not world or not mode:
        raise RuntimeError("World or native game mode missing. Build WildlightEditor first.")
    world.get_world_settings().set_editor_property("default_game_mode", mode)
    if not subsystem.save_current_level():
        raise RuntimeError("Could not save Valley level")


def import_audio():
    manifest = ART / "Audio" / "manifest.json"
    if not manifest.is_file():
        unreal.log_warning("WILDLIGHT: no audio study manifest; continuing mesh import")
        return
    folder = "/Game/Wildlight/Audio"
    LIB.make_directory(folder)
    for entry in json.loads(manifest.read_text(encoding="utf-8"))["assets"]:
        filename = (ART / entry["path"]).resolve()
        if ART.resolve() not in filename.parents or not filename.is_file():
            raise RuntimeError("Missing or unsafe audio path: " + str(filename))
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(filename))
        task.set_editor_property("destination_path", folder)
        task.set_editor_property("destination_name", entry["id"])
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        TOOLS.import_asset_tasks([task])
        sound = LIB.load_asset(folder + "/" + entry["id"])
        if not isinstance(sound, unreal.SoundWave):
            raise RuntimeError("Audio import did not produce SoundWave: " + entry["id"])
        sound.set_editor_property("looping", bool(entry.get("loop", False)))
        LIB.save_loaded_asset(sound)


LIB.make_directory(MESH_FOLDER)
palette = palette_materials(master_material())
for asset in DATA["assets"]:
    import_mesh(asset, palette)
import_audio()
make_level()
unreal.log("WILDLIGHT: import and map creation finished. Next: Play-in-Editor and packaged-build checks.")
