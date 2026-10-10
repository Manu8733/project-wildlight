# Backup and recovery

Git history and Git LFS version source/assets. A separate archive is a second recovery route. A cloud-local archive is a verified recovery artifact but is not an off-site backup until copied to independent storage controlled by the owner.

## Create and inspect a source backup

```bash
python3 Tools/backup.py /workspace/wildlight-backups/Wildlight-source.zip
python3 Tools/backup.py /workspace/wildlight-backups/Wildlight-source.zip --verify
```

The backup tool excludes Git internals, build/caches, common secret names, selected key files and symlinks; it records SHA-256 for each included file. Exclusions are safeguards, not proof that arbitrary secrets cannot be present in source. Review new files before backup and commit.

The archive contains a `Wildlight/` folder and `BACKUP_MANIFEST.json`. Archive CRC and file hashes are verified. Do not store backups under tracked art/source directories or commit the archive back into the project.

## Prove restoration

Restore to a new directory with the verified restore mode:

```bash
python3 Tools/backup.py /workspace/wildlight-backups/Wildlight-source.zip --restore /workspace/wildlight-restored
```

The restore mode verifies archive entries and checksums before extracting through staging, refuses an existing destination and validates safe paths. Run `python3 Tools/verify.py` from the restored project; it performs a clean portable-core compilation and tests. A successful listing alone does not demonstrate a working restore. On Windows, repeat editor build/import from the restored project after removing caches. Record backup timestamp, source commit, archive hash, restore directory and verification result.

## Asset versioning and external copy

Keep source Blender scripts, `.blend`, FBX, textures and imported engine assets versioned according to `.gitattributes`. After every push, confirm `git lfs fsck` and no pending LFS objects. Record exports against their editable sources; never keep only an opaque engine asset as the sole master.

Copy the verified source archive and release evidence to storage independent of this cloud and the GitHub repository. Keep one known-good prior archive when rotating. Owner credentials and recovery keys stay outside the archive. Off-site sync and owner recovery access cannot be confirmed from this environment and remain outstanding.
