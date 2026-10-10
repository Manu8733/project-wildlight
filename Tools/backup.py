#!/usr/bin/env python3
"""Create and verify a portable source/assets backup with per-file SHA256."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import shutil
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
EXCLUDED = {".git", ".artifacts", "Builds", "Binaries", "Intermediate", "Saved", "DerivedDataCache", ".vs", "__pycache__"}
BLOCKED_SUFFIXES = {".key", ".pem", ".pyc", ".tmp", ".log", ".blend1", ".blend2"}


def files():
    for path in sorted(ROOT.rglob("*")):
        relative = path.relative_to(ROOT)
        if not path.is_file() or EXCLUDED.intersection(relative.parts):
            continue
        if path.is_symlink() or path.suffix.lower() in BLOCKED_SUFFIXES:
            continue
        if path.name.startswith((".env", "credentials", "secrets")):
            continue
        yield path, relative.as_posix()


def create(output):
    output = output.resolve()
    manifest = {"format_version": 1, "project": "Wildlight", "files": {}}
    for path, name in files():
        if path.resolve() == output:
            continue
        manifest["files"][name] = hashlib.sha256(path.read_bytes()).hexdigest()
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix(output.suffix + ".tmp")
    with zipfile.ZipFile(temporary, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for name in manifest["files"]:
            archive.write(ROOT / name, "Wildlight/" + name)
        archive.writestr("Wildlight/BACKUP_MANIFEST.json", json.dumps(manifest, indent=2) + "\n")
    temporary.replace(output)
    verify(output)
    print(f"Backup: {output} ({output.stat().st_size:,} bytes)")


def verify(source):
    with zipfile.ZipFile(source) as archive:
        if archive.testzip():
            raise SystemExit("Archive CRC verification failed")
        data = json.loads(archive.read("Wildlight/BACKUP_MANIFEST.json"))
        if data.get("format_version") != 1 or data.get("project") != "Wildlight" or not isinstance(data.get("files"), dict):
            raise SystemExit("Unsupported backup manifest")
        for name, digest in data["files"].items():
            relative = PurePosixPath(name)
            if relative.is_absolute() or ".." in relative.parts or "\\" in name or ":" in name or relative.as_posix() != name:
                raise SystemExit("Unsafe backup path: " + name)
            if not isinstance(digest, str) or len(digest) != 64:
                raise SystemExit("Invalid backup digest: " + name)
        expected = {"Wildlight/" + name for name in data["files"]}
        expected.add("Wildlight/BACKUP_MANIFEST.json")
        if set(archive.namelist()) != expected or len(archive.namelist()) != len(expected):
            raise SystemExit("Unexpected or missing archive entries")
        for name, digest in data["files"].items():
            if hashlib.sha256(archive.read("Wildlight/" + name)).hexdigest() != digest:
                raise SystemExit("SHA256 mismatch: " + name)
    print(f"Verified all {len(data['files'])} backup entries")
    return data


def restore(source, destination):
    """Verify first, extract to staging, then install into a new directory only."""
    data = verify(source)
    destination = destination.resolve()
    if destination.exists():
        raise SystemExit("Restore destination must not exist; existing work is never overwritten")
    destination.parent.mkdir(parents=True, exist_ok=True)
    staging = Path(tempfile.mkdtemp(prefix=".wildlight-restore-", dir=destination.parent))
    try:
        with zipfile.ZipFile(source) as archive:
            for name in data["files"]:
                path = staging / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(archive.read("Wildlight/" + name))
                if hashlib.sha256(path.read_bytes()).hexdigest() != data["files"][name]:
                    raise SystemExit("Restored file mismatch: " + name)
        (staging / "BACKUP_MANIFEST.json").write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
        staging.rename(destination)
    finally:
        if staging.exists():
            shutil.rmtree(staging)
    print(f"Restored and hash-verified {len(data['files'])} files to {destination}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--verify", action="store_true")
    mode.add_argument("--restore", type=Path, metavar="NEW_DIRECTORY")
    args = parser.parse_args()
    if args.restore:
        restore(args.output, args.restore)
    elif args.verify:
        verify(args.output)
    else:
        create(args.output)
