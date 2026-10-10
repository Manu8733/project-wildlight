# Windows laptop handoff

The user already has Unreal installed and wants as much work as possible completed in cloud. This handoff covers checks the cloud cannot execute. It does not imply the laptop has been inspected or a Windows build has passed.

## Prepare

1. Install/select Unreal **5.8.1**, matching `engine-lock.json`. Confirm its `Engine/Build/Build.version`. If the installed newest engine differs, keep this project pinned until a tested migration is recorded.
2. Install Visual Studio 2022 with Game development with C++, supported MSVC and Windows SDK, plus Git and Git LFS. Verify the current Epic requirements for 5.8.1.
3. Extract the provided source archive to a short path, for example `C:\Projects\Wildlight`, or clone [the verified private repository](https://github.com/Manu8733/project-wildlight) after the first push is confirmed in `PROGRESS.md`. Avoid synchronized desktop folders for the working build tree.
4. In the clone, run `git lfs install` and `git lfs pull`. Confirm FBX/Blender files are actual binary assets rather than LFS pointer text. Keep credentials in Git's credential manager.
5. Connect power, use the GPU's appropriate performance profile, close expensive applications and leave enough disk space for engine shaders and build outputs. First shader compilation is not a steady-state FPS test.

## Build, import and start

Run the three Verify/Editor/Import commands in `BUILD_AND_VERIFY.md`. If Verify reports an engine mismatch, do not bypass it casually. An explicit mismatch flag exists for a deliberate migration test, not normal first startup.

Open `Wildlight.uproject`, open `/Game/Wildlight/Maps/Valley`, and press Play. The native game mode bootstraps the simulation and presentation. Record any compiler/API errors before attempting graphical polish. The importer may need adjustment for the actual engine Python API; cloud syntax checks cannot validate that API.

Use the default Low graphics configuration. Begin with one small match, a handful of units and no optional advanced rendering experiments. Confirm gameplay and save/load first; then capture the performance matrix.

## Controls to validate

The initial native controller is intended to expose the following bindings. Verify them in the actual build before treating them as player documentation; remapping and edge cases remain separate acceptance checks.

| Input | Action |
| --- | --- |
| WASD / edge scrolling | Pan camera |
| Q / E | Rotate camera |
| Mouse wheel | Zoom |
| Left click / drag | Select one / box select |
| Shift + selection | Add to selection |
| Ctrl + 1–9 | Store selection group |
| 1–9; double press | Recall group; focus group |
| Right click | Context move, attack or gather |
| B | Begin building preview (default house) |
| R during placement | Rotate preview |
| Escape | Cancel placement / pause |
| F5 / F9 | Save / load |
| HUD catalog | Choose buildings, training, spells and other orders |

English HUD labels explain costs and failures. The control list is not proof that remapping, collision avoidance or UI scaling is finished.

## First acceptance session

Complete a start-to-victory match; gather and deliver wood; place a building and inspect cost; train a unit; connect and cut a beacon chain; cast bridge and roots; save, reload and continue. Then try pause, restart, surrender and a second map. Capture the packaged build separately after Shipping.

Send or record only the first blocking error and its log context; do not paste tokens or private keys. Update requirement evidence with actual machine details, command and result. User acceptance remains pending until the user explicitly accepts the tested outcome.
