# Generated Unreal content

Run `Tools/Build-Windows.ps1 -EngineRoot <UE_5.8 folder> -Stage Import` after building the Editor target. It imports the original FBX models from `Art/Exports`, creates the palette materials, and saves `/Game/Wildlight/Maps/Valley`.

The cloud environment cannot generate or validate Unreal `.uasset`/`.umap` files because Unreal is not installed. The default game mode bootstraps the scene from the authoritative simulation even when opening `/Engine/Maps/Entry`. The authored meshes become available after import.

Generated assets are tracked with Git LFS. Do not commit engine caches, local credentials, or saved games.
