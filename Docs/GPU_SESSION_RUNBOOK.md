# Windows GPU session runbook

Current state: **not executed**. This Linux cloud has no usable Unreal installation, Windows packaging toolchain or accessible GPU workstation. The user's only confirmed machine is the RTX 3050 laptop. No rental budget or paid session has been authorized. This runbook prepares List 2 so an actual session can be efficient later; it does not satisfy its hardware execution requirements.

## Before any future session

1. Select a Windows machine against `PERFORMANCE_BUDGETS.md`; record provider, exact GPU/VRAM, CPU, RAM, free disk, hourly price, minimum billing and shutdown rule. Do not rent until tasks and budget are explicitly established.
2. Use a hardened remote connection supported by the host, MFA and an encrypted channel. Do not expose RDP directly to public internet. Use a temporary scoped Git credential from a secret store; avoid tokens in command history and repository files.
3. Install pinned Unreal 5.8.1, supported Visual Studio C++/Windows SDK, Blender, Git LFS and current stable driver. Check the current Epic toolchain requirements rather than assuming an SDK number from this document.
4. Clone the private repository, run `git lfs pull`, confirm the exact commit, and verify asset checksums. Capture an independent pre-session backup and prove that it can be listed and restored.
5. Queue one session objective: import/readability, skeleton/animation, lighting/VFX, performance or Shipping QA. Estimate duration and checkpoint time. Keep a log of actual cost and completed requirement IDs.

## Import and visual session

Run the project's importer in Unreal's Python environment. Review centimeters, pivot/ground contact, material names, collision, UV/lightmap channels and normal orientation. Open the generated map and confirm every referenced mesh loads. Import success alone does not validate skeletal rigs or final animation.

Capture day/night and Low/High comparisons with the same camera. Review walkable ground, gate clearance, bridge anchors, projectile contact, selection rings, ownership and network status. Integrate effects against actual gameplay events. Use `Art/ASSET_REVIEW.md` and the project's asset registry for the review matrix. Retain original sources and import settings with any changed asset.

## Verification session

Run editor startup, a full skirmish and save/reload before profiling. Then execute `QA_PLAN.md` at 100, 200 and 500 units only where the current pathing and spawn tools support it. Capture Unreal Insights, GPU time, VRAM and long-session growth. A crash or unfinishable scenario stops performance interpretation until fixed.

Build Windows Shipping with the documented wrapper, cook/package, and install the package in a fresh user environment without developer tools. Confirm no missing-content dialogs, writable user saves, stable startup and valid shutdown. The exact packaged binary must be tested; Development Editor success is insufficient.

## Closeout

Push code and LFS asset changes; confirm all large-object uploads completed. Export QA reports, raw trace indexes and checksums. Create independent post-session backup and restore-check it. Verify the laptop still runs the Low preset after changes. Sign out, revoke temporary credentials, explicitly stop/delete billed compute as appropriate, and record end time and total cost. Rental shutdown is an explicit provider action, not assumed from disconnecting the remote window.
