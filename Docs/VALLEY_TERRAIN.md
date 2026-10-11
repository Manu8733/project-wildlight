# Wooded Valley Terrain Input

The current authoring target is user-approved **Unreal Engine 5.8.3**. This milestone provides original editable layout source and offline Landscape input. It does not establish native Landscape creation/import, complete environment art, wind/weather integration, navigation, working six-participant gameplay or user acceptance.

## Current Rectangular Map

On 11 October 2026 the user approved one **1600 by 1200 metre** wooded valley with six irregular starts. `Art/Maps/WoodedValley1600x1200.layout.json` is the current fixed authored source. It has no seed or random map generator. The historical 768 metre source below remains available with its original topology, filenames, transforms and byte-identical exports under the recorded runtime.

The new clearing centres are (250,250), (590,190), (330,930), (1200,260), (1070,750) and (1370,970) m. All six X and Y coordinates are distinct. Their flat cores have a 110 m radius at Z=8 m with 22 m smooth margins; cores and margins are disjoint and clear of the river banks. The original Wood/Stone/Gold template, quantities, offsets and Castle/Farm physical footprints are retained. A larger terrain does not scale buildings, units or resources.

Two bent bank routes link the three western and three eastern starts. Three paired transverse routes approach the bridge axes at Y=300, 650 and 950 m. All roads are 24 m wide with 10 m smooth margins at Z=8 m. Bridge records are permanent separate geometry proposals, each centred at X=800 m, 160 m long and 24 m wide. No terrain fill is generated for them. The source river retains a continuous floor beneath every crossing.

The fixed river centreline ranges from X=760 to 840 m over Y=0..1200 m. Its 32 m bed transitions to 70 m total bank width; the floor is `-4 + 0.001 * Y` m beneath the proposed Z=0 water surface. Edge ridges have a nominal 130 m width and 120 m peak parameter, with the same original irregular analytic profiles.

Rectangular route validation allows bends only on a bank where the full road width and blend margin stay outside the global channel envelope. Any segment entering that protected corridor must align horizontally with its own deck's Y. Every portion passing through the channel envelope must fit inside the reserved deck span, match deck width/elevation and move in one direction without a second crossing. Each start requires both bank and transverse exits, and the whole graph must be connected. The historical profile retains its stricter entirely straight transverse-route rule.

| Property | Rectangular native input |
| --- | --- |
| Height samples | 1009 x 1009; 1018081 samples |
| Quads per section | 63 |
| Sections per component | 2 per axis, meaning 2 x 2 subsections |
| Components | 8 x 8; 64 total |
| Quads across each axis | 1008 |
| X scale | 160000/1008 = 158.73015873015873 cm |
| Y scale | 120000/1008 = 119.04761904761905 cm |
| Measured data extent | 160000 x 120000 cm |
| ZScale / actor Z | 100 / 0 m |
| Future logical cells | 800 x 600 at 200 cm; legacy Core remains unchanged |

`xy_scale_cm` is a two-element `[X,Y]` pair only for the rectangle. The historical profile keeps its scalar. Column 0 is X=0 and row 0 is Y=0; columns increase +X, rows increase +Y without transpose or flip. Vertex `(column,row)` maps to `(column*xy_scale_cm[0],row*xy_scale_cm[1])` cm. The last vertices lie at 160000 and 120000 cm. Future logical lookup is `floor(world_x_cm/200),floor(world_y_cm/200)` on the half-open 160000 by 120000 cm domain. This source metadata does not change current simulation/save dimensions.

Export with the same pinned runtime documented below:

```powershell
& "$terrainVenv/Scripts/python.exe" Tools/author_valley_terrain.py --layout Art/Maps/WoodedValley1600x1200.layout.json --output <new-private-directory>
```

The fixed validated profile selects `WoodedValley1600x1200.png`, `WoodedValley1600x1200.r16` (2036162 bytes) and `WoodedValley1600x1200.manifest.json`. Only these two named layouts and their approved extents/topologies are supported. Names never become arbitrary output paths. The exporter preserves exclusive creation, full-buffer preparation, little-endian UInt16 and 16-bit grayscale encoding. All native, navigation, bridge/water, balance, weather/wind, gameplay, performance and user-acceptance flags remain false: source export is not native verification.

Rectangle regressions cover unequal XY spacing and bounds, valid native topology, irregular starts, equal physical resource templates, route connectivity, safe bends, unsafe off-deck/double crossings, malformed profiles and full PNG/R16 equality and orientation. The historical suite includes the pre-change R16 hash and, on Python 3.12.14/Pillow 12.3.0, exact PNG and manifest hashes. Run the combined full suite using the command below; neither suite verifies Unreal navigation or gameplay.

## Historical 768 Metre Proposal

`Art/Maps/WoodedValley768.layout.json` defines one explicit analytic terrain field, with no seed, random map generation, downloaded assets or licensed binary content. Six proposed clearing centres use X=144/624 m and Y=144/384/624 m. Each flat clearing core has a 60 m radius at Z=8 m, followed by a 16 m smooth margin. The cores and margins are disjoint.

Two bank routes connect the three starts on each side. Three transverse routes connect the pairs at Y=144, 384 and 624 m. Every start has a bank exit and a transverse exit, including the four endpoint starts. Proposed road and crossing width is 20 m, with 8 m smooth road margins. These dimensions are editable blockout proposals, not approved balance or certified formation clearance.

This initial schema requires each transverse polyline to remain straight along X at its reserved deck's Y, without doubling back; extra collinear points are supported. Curved or additional river crossings are rejected and require a later explicit route/deck design. Validation checks the river control points inside the full deck width as well as its edges, so a hidden interior river bend cannot escape the reserved deck span.

The baseline valley has small, fixed sinusoidal undulations and a gentle cross-valley rise. Four irregular edge ridges vary their width and elevation along the boundary. Smooth tapering and a root-sum-square combination avoid hard rectangular mountain walls. Clearings and road corridors flatten the field with cubic smoothstep transitions. River carving occurs last, so no road can fill or dam the river beneath a crossing.

The river follows a fixed nine-point centerline near X=384 m, spanning the whole Y=0..768 m extent. Each segment interpolates X as a cubic smoothstep function of Y; values and first derivatives meet continuously. Cross-section widths are measured along X at fixed Y, not asserted to be exact perpendicular widths. The bed core is 24 m wide, transitioning through banks to 52 m total width. The floor is `-4 + 0.002 * Y` metres and remains below the proposed Z=0 water surface. Increasing Y rises slightly, so downhill is toward Y=0. Water is a proposal for subsequent native integration, not a generated water asset.

Three permanent bridge deck records are centred at (384,144), (384,384) and (384,624) m, each 72 m long along X, 20 m wide along Y and at Z=8 m. The records do not modify terrain height. Actual deck geometry, supports, collision, approach grading and water integration still need native authoring and inspection.

Every clearing references the same editable initial resource template: one Wood, Stone and Gold anchor, each with an initial quantity of 1800. The quantity is the current portable scenario's starting-node value; the local offsets are new blockout proposals. All anchors are within their flat clearing and away from its route axes. This establishes equal source templates only, not equivalent extraction time, depot access, final placement or balance.

Current `Core/src/Simulation.cpp` building definitions have Castle=4x4 and Farm=3x3 logical cells. At 2 m per cell these are **8x8 m** and **6x6 m** footprints, recorded in the layout. Physical unit radii have not been established. No inferred collision radius or formation clearance is supplied, and the current Core two-faction scenarios remain unchanged.

## Dimensions And Transform

| Property | Native input |
| --- | --- |
| Height samples | 505 x 505; 255025 samples |
| Quads per section | 63 |
| Sections per component | 1 |
| Components | 8 x 8 |
| Quads across each axis | 504 |
| XY scale | 76800/504 = 152.38095238095238 cm |
| Measured data extent | 76800 x 76800 cm |
| ZScale / actor Z | 100 / 0 m |
| Sample at world Z=0 | 32768 |
| Logical cells | 384 x 384 at 200 cm; separate from vertices |

Data origin is world X=0,Y=0. Column 0 maps to X=0, row 0 maps to Y=0. Columns increase toward +X and rows toward +Y. Vertex `(column,row)` maps to world centimetres `(column*xy_scale_cm,row*xy_scale_cm)`, with both last vertices at 76800 cm. No transpose or vertical flip is performed.

For logical cell lookup, `cell_x=floor(world_x_cm/200)` and `cell_y=floor(world_y_cm/200)`. The logical domain is half-open, `0 <= X,Y < 76800` cm; the upper terrain edge is a boundary, not cell 384. Native vertex coordinates are `column=world_x_cm/xy_scale_cm`, `row=world_y_cm/xy_scale_cm`. Engine actor location/orientation and the measured native import must be verified against this explicit transform.

Height encoding is `round(32768 + (height_m-actor_z_m)*100*128/z_scale)`. At the default transform this is exactly `round(32768 + height_m*100*128/100)`. Python's `round` uses ties to even. Decode is `actor_z_m + (sample-32768)*z_scale/(100*128)`. The default representable range is -256..255.9921875 m, with 0.0078125 m spacing and at most 0.00390625 m rounding error. The encoder rejects any unrounded value outside UInt16 range, including nonfinite data, rather than clipping or wrapping.

## Offline Export

Use Python 3.12 and install the tested export dependency from `Tools/requirements-terrain.txt` in an isolated environment **outside the public checkout**. From the checkout root, choose a private work directory and run:

```powershell
$terrainVenv = '<private-work-directory>/.venv-terrain'
python -m venv $terrainVenv
& "$terrainVenv/Scripts/python.exe" -m pip install -r Tools/requirements-terrain.txt
```

Pure layout loading and sampling use the Python standard library. Export and the complete test suite require the pinned `Pillow==12.3.0`; Pillow is imported only by `export_terrain`. No engine, render or C++ build is required. CI selects Python 3.12 with `actions/setup-python` and installs this requirements file before running all tests. On Linux/macOS, the isolated interpreter is `$terrainVenv/bin/python` instead of `Scripts/python.exe`.

Use an output directory **outside the public checkout**, for example the chat's sibling `outputs/` directory:

```powershell
& "$terrainVenv/Scripts/python.exe" Tools/author_valley_terrain.py --layout Art/Maps/WoodedValley768.layout.json --output <new-private-directory>
```

The documented interfaces are `load_layout(path: Path) -> dict`, `height_metres(layout: dict, x_m: float, y_m: float) -> float`, `encode_height(height_m: float, z_scale: float, actor_z_m: float) -> int`, `decode_height(sample: int, z_scale: float, actor_z_m: float) -> float`, and `export_terrain(layout: dict, output_dir: Path) -> dict`. Pass a validated `load_layout` result to the height sampler; export validates its input again.

Outputs are `WoodedValley768.png` (primary single-channel 16-bit grayscale PNG), `WoodedValley768.r16` (row-major explicitly little-endian UInt16, exactly 510050 bytes), and `WoodedValley768.manifest.json`. The manifest records file hashes/sizes, sample extrema, metre extrema, round-trip error, the canonical layout hash, exporter versions, scale/transform and all pending engine/visual gates. Its layout hash uses UTF-8 JSON with sorted keys, compact separators and finite numbers. The manifest cannot include its own hash; an external evidence ledger can hash it separately.

Layout validation rejects malformed/duplicate JSON keys, files larger than 256 KiB, nonfinite data, noninteger Landscape topology counts (including integral floats and booleans), invalid dimensions/scale, overlapping clearings, disconnected or misplaced routes, missing exits, unequal initial templates, discontinuous river control points and unsafe deck records. Complete encoded buffers are prepared before any output path is created. Existing target files cause a refusal; exclusive file creation also prevents races from overwriting another writer's output. A write failure removes only files created by that attempt.

The source is deterministic with its recorded Python/Pillow export runtime. Export compression hashes are runtime-specific; compare the decoded samples as well as file hashes if the exporter runtime changes. Generated data is terrain input, **not an Unreal screenshot**. Do not commit it or third-party assets to the public source repository.

## Verification And Remaining Gates

Focused tests cover height literals, overflow and invalid input; native dimensions; flat/disjoint cores; smooth margins; deterministic samples; irregular raised edges; route connectivity and endpoint exits; equal initial resource anchors; separate permanent decks; continuous river floor; no overwrite/partial invalid export; exact R16 length/order/orientation; PNG IHDR bit depth/dimensions; equality of all PNG and R16 samples; sampled metre round trips and manifest hashes. Run:

```powershell
& "$terrainVenv/Scripts/python.exe" -m unittest discover -s Tests -p test_valley_terrain.py -v
& "$terrainVenv/Scripts/python.exe" -m unittest discover -s Tests -p 'test_*.py' -v
```

Unreal 5.8.3 must still verify actual Landscape creation/import API or editor workflow, component/extent placement, vertical calibration, orientation and native R16 byte order. Preparation references were the [Landscape technical guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/landscape-technical-guide-in-unreal-engine?application_version=5.6) and [heightmap format documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/importing-and-exporting-landscape-heightmaps-in-unreal-engine?application_version=5.6); their 5.6 reference version is not proof of 5.8.3 compatibility.

After native import: validate the whole map's collision/navigation, supported units/formations, building/depot access, bridge geometry/approaches/water flow, natural deposits, terrain materials and playable boundaries. Then integrate licensed private foliage with preserved wind, approved clear/cloudy/light-rain weather and simulation-clock lighting. Capture actual native views and motion at day/dusk/night/rain and measure Low/High CPU/GPU/RAM/VRAM cost. Working six-participant gameplay and user acceptance remain separate milestones. Every unverified gate is explicitly false in the export manifest.
