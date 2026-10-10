# Original Wildlight mesh authoring

Use Blender **4.3.2**. All dimensions are metres, origins are at ground zero,
architectural fronts face -Y, and every FBX contains one joined static mesh.
Named palette materials are kept through export. The companion GLB files embed
their mesh data and have no external image or buffer dependency.

From the repository root:

```bash
blender -b --python Art/Source/build_wildlight_art.py -- --skip-render
blender -b --python Art/Source/validate_wildlight_art.py
blender -b --python Art/Source/build_wildlight_art.py -- --render-only
blender -b --python Art/Source/build_wildlight_art.py -- --render-only --catalog-only
```

The first command deliberately rebuilds the Blender source and overwrites the
exported FBX/GLB kit. Edit the explicit architectural functions and palette in
the source script, then export again. There are 19 buildings, 12 unit studies and
10 environment/prop studies. Character meshes remain static and are labelled
accordingly in the manifest.

Validation independently imports every FBX into a clean Blender scene, checks
ground origins, finite vertex positions, metre bounds, triangle/material counts,
then reads every GLB container to check mesh count and embedded dependencies.
It writes `Art/validation_report.json`; this verifies interchange geometry and
does not claim Unreal integration or GPU performance.

CPU Cycles is used for presentation renders because this cloud session has no
GPU. Denoising is disabled because this Blender build lacks OpenImageDenoise.
The warm day scene uses 96 samples and the moonlit scene uses 192 samples.
The images are composed asset previews and do not represent gameplay.
