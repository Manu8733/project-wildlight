# Wildlight art previews

These images are **Blender asset showcases — not Unreal gameplay**. They use the
original mesh library exported by `Art/Source/build_wildlight_art.py`, arranged in
an authored valley with a river, village, bridge and light network. No image
generation service, purchased mesh or stock texture was used.

- `Wildlight_Asset_Valley_Day.png`: warm cream plaster, terracotta roofs, curved
  stone turrets and an olive meadow in daylight.
- `Wildlight_Asset_Valley_Night.png`: the same geometry under moonlight with amber
  windows, restrained mint crystals and small firefly meshes.
- `Wildlight_Asset_Catalog.png`: the exported building, unit and environment kit.

Open `Art/Source/Wildlight_Valley.blend` in Blender 4.3.2 for the editable scene.
Original joined meshes are in the **Authored Asset Library** collection; unhide
the object being inspected. The valley uses linked instances of those meshes.
The Python source preserves the architectural construction and proportions.

Characters are static mesh studies. The previews do not establish production
animation, collision, Unreal rendering, final art acceptance or GPU performance.
See `Art/ASSET_REVIEW.md` and the numeric export/round-trip validation reports.
