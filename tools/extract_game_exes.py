#!/usr/bin/env python3
"""Populate assets/exe/ from a contributor's own copy of the game.

The repository ships the *data* it is allowed to ship (``assets/gam``,
``assets/omt`` and the derived catalogs). It deliberately does not ship the
original executables — ``Neutron.exe``, ``NeutronSW.exe`` and ``OMT2.dll`` are
THQ / Nickelodeon property and are gitignored under ``assets/exe/``.

Some of the audit and reverse-engineering tooling needs those files. This script
copies them out of a disc you own and checks them against ``docs/binaries.sha256``
so you know you have the same build everyone else is reasoning about.

Three source modes, in order of how well they work:

1. ``--source /path/to/installed/game`` — an installed copy. Zero dependencies,
   always works. This is the recommended route.
2. ``--source /media/cdrom`` (or ``D:\\``) — a mounted disc. The retail discs are
   InstallShield, so the payload sits in ``data1.cab``; ``unshield`` is required.
3. ``--source game.iso`` — an ISO image. This script walks ISO 9660 itself (no
   dependencies) to pull ``data1.cab`` out, then hands it to ``unshield``.

Nothing is downloaded and nothing is redistributed.
"""
from __future__ import annotations

import argparse
import hashlib
import os
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent

TITLES = {
    "jnbg": {
        "label": "Jimmy Neutron: Boy Genius",
        "volume": "J_NEUTRONPC",
        "files": ("Neutron.exe", "NeutronSW.exe", "OMT2.dll"),
    },
    "jnvsjn": {
        "label": "Jimmy Neutron vs. Jimmy Negatron",
        "volume": None,
        "files": ("Neutron2.exe", "Neutron2SW.exe", "OMT2.dll", "granny.dll"),
    },
    "mechanix": {
        "label": "Hot Wheels: Mechanix",
        "volume": None,
        "files": ("MECHANIX.exe", "Mechanixsw.exe", "OMT2.dll"),
    },
}


# --------------------------------------------------------------------------
# checksum manifest
# --------------------------------------------------------------------------
def load_manifest(path: Path) -> dict[str, dict[str, tuple[str, int]]]:
    """docs/binaries.sha256 -> {title: {filename: (sha256, size)}}"""
    out: dict[str, dict[str, tuple[str, int]]] = {t: {} for t in TITLES}
    title = None
    if not path.is_file():
        return out
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line:
            continue
        if line.startswith("#"):
            key = line.lstrip("# ").split()[0].lower() if len(line) > 1 else ""
            if key in TITLES:
                title = key
            continue
        parts = line.split()
        if title and len(parts) >= 3:
            digest, size, name = parts[0], parts[1], " ".join(parts[2:])
            out[title][name.lower()] = (digest, int(size))
    return out


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


# --------------------------------------------------------------------------
# ISO 9660 — enough to find and extract one file from the root directory
# --------------------------------------------------------------------------
SECTOR = 2048


def iso_root_entries(iso: Path):
    """Yield (name, extent_lba, length) for every entry in the ISO root directory."""
    with iso.open("rb") as f:
        for sector in range(16, 32):
            f.seek(sector * SECTOR)
            vd = f.read(SECTOR)
            if len(vd) < SECTOR or vd[1:6] != b"CD001":
                continue
            if vd[0] == 1:  # primary volume descriptor
                root = vd[156:190]
                lba = struct.unpack("<I", root[2:6])[0]
                length = struct.unpack("<I", root[10:14])[0]
                break
            if vd[0] == 255:
                return
        else:
            return

        data = b""
        f.seek(lba * SECTOR)
        data = f.read(length)

    off = 0
    while off < len(data):
        rec_len = data[off]
        if rec_len == 0:
            off = (off // SECTOR + 1) * SECTOR
            if off >= len(data):
                break
            continue
        rec = data[off:off + rec_len]
        if len(rec) >= 33:
            ext_lba = struct.unpack("<I", rec[2:6])[0]
            ext_len = struct.unpack("<I", rec[10:14])[0]
            name_len = rec[32]
            name = rec[33:33 + name_len].decode("latin-1")
            name = name.split(";")[0]
            if name not in ("\x00", "\x01"):
                yield name, ext_lba, ext_len
        off += rec_len


def iso_extract(iso: Path, wanted: str, dest: Path) -> Path | None:
    for name, lba, length in iso_root_entries(iso):
        if name.lower() == wanted.lower():
            dest.parent.mkdir(parents=True, exist_ok=True)
            with iso.open("rb") as f, dest.open("wb") as o:
                f.seek(lba * SECTOR)
                remaining = length
                while remaining > 0:
                    chunk = f.read(min(1 << 20, remaining))
                    if not chunk:
                        break
                    o.write(chunk)
                    remaining -= len(chunk)
            return dest
    return None


# --------------------------------------------------------------------------
# source resolution
# --------------------------------------------------------------------------
def find_loose(root: Path, names: tuple[str, ...]) -> dict[str, Path]:
    """Case-insensitive search for the wanted filenames anywhere under root."""
    want = {n.lower(): n for n in names}
    found: dict[str, Path] = {}
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if not d.startswith(".")]
        for fn in filenames:
            key = fn.lower()
            if key in want and want[key] not in found:
                found[want[key]] = Path(dirpath) / fn
        if len(found) == len(want):
            break
    return found


def run_unshield(cab: Path, workdir: Path) -> Path | None:
    exe = shutil.which("unshield")
    if not exe:
        return None
    out = workdir / "unshielded"
    out.mkdir(parents=True, exist_ok=True)
    r = subprocess.run([exe, "-d", str(out), "x", str(cab)],
                       capture_output=True, text=True)
    if r.returncode != 0:
        print("  unshield failed: %s" % (r.stderr.strip().splitlines()[-1:] or [""])[0],
              file=sys.stderr)
        return None
    return out


def resolve_sources(source: Path, names: tuple[str, ...], workdir: Path) -> dict[str, Path]:
    # 1. an ISO image
    if source.is_file() and source.suffix.lower() in (".iso", ".img", ".bin"):
        print("source looks like a disc image: %s" % source.name)
        cab = iso_extract(source, "data1.cab", workdir / "data1.cab")
        hdr = iso_extract(source, "data1.hdr", workdir / "data1.hdr")
        if cab is None:
            loose = {}
            for name, lba, length in iso_root_entries(source):
                if name.lower() in {n.lower() for n in names}:
                    tgt = workdir / name
                    iso_extract(source, name, tgt)
                    loose[name] = tgt
            if loose:
                print("  found the executables directly in the image root")
                return loose
            raise SystemExit(
                "  this image has neither data1.cab nor loose executables in its root.\n"
                "  Install the game from it and re-run with --source <install dir>.")
        print("  extracted data1.cab (%s bytes)%s"
              % (f"{cab.stat().st_size:,}", " + data1.hdr" if hdr else ""))
        out = run_unshield(cab, workdir)
        if out is None:
            raise SystemExit(
                "\n  data1.cab is an InstallShield archive and 'unshield' is not installed.\n"
                "  Either:\n"
                "    - install unshield  (apt install unshield / brew install unshield), or\n"
                "    - install the game from the disc and re-run with:\n"
                "        --source \"C:\\Program Files\\...\\Program Executable Files\"")
        return find_loose(out, names)

    # 2. a directory: installed game, mounted disc, or an extracted ISO tree
    if source.is_dir():
        found = find_loose(source, names)
        if found:
            return found
        cabs = list(source.rglob("data1.cab"))
        if cabs:
            print("source contains an InstallShield archive: %s" % cabs[0])
            out = run_unshield(cabs[0], workdir)
            if out is None:
                raise SystemExit(
                    "\n  'unshield' is not installed, so the archive cannot be opened.\n"
                    "  Install the game from this disc and point --source at the install directory.")
            return find_loose(out, names)
        return {}

    raise SystemExit("source not found: %s" % source)


# --------------------------------------------------------------------------
def main() -> int:
    ap = argparse.ArgumentParser(
        description="Copy the original game executables out of your own disc into assets/exe/.")
    ap.add_argument("--source", required=False, type=Path,
                    help="installed game directory, mounted disc, or .iso image")
    ap.add_argument("--title", default="jnbg", choices=sorted(TITLES),
                    help="which game (default: jnbg)")
    ap.add_argument("--dest", type=Path, default=None,
                    help="destination (default: assets/exe/<title>)")
    ap.add_argument("--manifest", type=Path, default=REPO / "docs" / "binaries.sha256")
    ap.add_argument("--verify-only", action="store_true",
                    help="check what is already in assets/exe/ and exit")
    args = ap.parse_args()

    spec = TITLES[args.title]
    dest = args.dest or (REPO / "assets" / "exe" / args.title)
    manifest = load_manifest(args.manifest).get(args.title, {})

    print("%s  (%s)" % (spec["label"], args.title))
    print("destination: %s" % dest)
    if not manifest:
        print("note: no checksums for this title in %s — copies will not be verified"
              % args.manifest.name)
    print()

    if args.verify_only:
        return report(dest, spec["files"], manifest)

    if args.source is None:
        ap.error("--source is required unless --verify-only is given")

    with tempfile.TemporaryDirectory(prefix="jn-extract-") as tmp:
        workdir = Path(tmp)
        found = resolve_sources(args.source, spec["files"], workdir)

        if not found:
            print("Could not find any of %s under %s" % (", ".join(spec["files"]), args.source),
                  file=sys.stderr)
            print("\nIf you have the disc but not an install, install the game first and point\n"
                  "--source at its 'Program Executable Files' directory.", file=sys.stderr)
            return 1

        dest.mkdir(parents=True, exist_ok=True)
        for name in spec["files"]:
            src = found.get(name)
            if src is None:
                print("  %-16s NOT FOUND in the source" % name)
                continue
            shutil.copy2(src, dest / name)
            print("  %-16s copied (%s bytes)" % (name, f"{(dest / name).stat().st_size:,}"))

    print()
    return report(dest, spec["files"], manifest)


def report(dest: Path, names: tuple[str, ...], manifest: dict) -> int:
    print("verification against docs/binaries.sha256")
    ok = missing = mismatch = 0
    for name in names:
        p = dest / name
        if not p.is_file():
            print("  %-16s MISSING" % name)
            missing += 1
            continue
        expected = manifest.get(name.lower())
        if not expected:
            print("  %-16s present (no checksum on file)" % name)
            ok += 1
            continue
        digest = sha256(p)
        if digest == expected[0]:
            print("  %-16s OK" % name)
            ok += 1
        else:
            print("  %-16s CHECKSUM MISMATCH" % name)
            print("      expected %s" % expected[0])
            print("      got      %s" % digest)
            mismatch += 1
    print()
    if mismatch:
        print("%d file(s) did not match. That usually means a different regional release or\n"
              "a patched copy. The tooling may still work, but any offset in the docs is\n"
              "relative to the build in the manifest." % mismatch)
        return 2
    if missing:
        print("%d file(s) missing — see docs/GAME_FILES.md" % missing)
        return 1
    print("All present and verified. Binary-backed tooling is ready:")
    print("  python3 tools/audit/spec_check.py --binary assets/exe/jnbg/Neutron.exe")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
