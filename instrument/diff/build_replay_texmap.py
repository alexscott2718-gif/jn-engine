#!/usr/bin/env python3
r"""
build_replay_texmap.py -- pair captured TEXTURE_DEFs to local asset PNGs.

The proxy currently captures texture SHA-1 + dimensions but not pixel bytes
(M5 design — see docs/replay_v0_findings.md follow-up). For the .omtc replay
to render with real textures rather than white silhouettes, we need a
captured-tex-id -> PNG path mapping. SHA-1 matching almost never works
(decoded-PNG vs locked-surface bytes differ); we fall back to dimension
matching, drawing from the asset PNG pools in a priority order that
loosely reflects where each kind of texture comes from in JNBG:

  1. assets/parsed/level1/level1_images    (per-level textures, our scene)
  2. assets/png                            (named textures: characters, props)
  3. assets/parsed/sprites/sprites_images  (HUD/effects sprites)
  4. assets/parsed/objects/objects_images  (objects atlas)

Within a (w,h) bucket, PNGs are dealt in alphabetical order to captured tex
IDs in first-seen order. Output: JSON mapping {hex tex_id -> PNG path}, plus
unresolved-count for the run log. Imperfect by construction; the right fix
is to extend the proxy to include pixel payloads (see findings doc).

Usage:
  build_replay_texmap.py <capture.omtc> [--out build/replay_texmap.json]
"""
import os
import sys
import json
import struct
import hashlib
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "receiver"))
from diff import iter_records  # noqa: E402
from protocol import *  # noqa: E402,F403

ASSET_ROOT = os.path.join(HERE, "..", "..", "assets")
PRIORITY_DIRS = [
    os.path.join(ASSET_ROOT, "parsed", "level1", "level1_images"),
    os.path.join(ASSET_ROOT, "png"),
    os.path.join(ASSET_ROOT, "parsed", "sprites", "sprites_images"),
    os.path.join(ASSET_ROOT, "parsed", "objects", "objects_images"),
]


def png_dimensions(path):
    """Return (w, h) from a PNG without a full decode (IHDR is at +16). Avoids
    importing PIL for every PNG just to read dims."""
    try:
        with open(path, "rb") as f:
            hdr = f.read(24)
        if hdr[:8] != b"\x89PNG\r\n\x1a\n":
            return None
        w, h = struct.unpack(">II", hdr[16:24])
        return w, h
    except (OSError, struct.error):
        return None


def build_png_pool():
    """Group local PNGs by (w,h); within a dim, sort by path (priority dir
    first via order in PRIORITY_DIRS, then alpha)."""
    pool = defaultdict(list)
    for prio_idx, d in enumerate(PRIORITY_DIRS):
        if not os.path.isdir(d):
            continue
        for fn in sorted(os.listdir(d)):
            if not fn.lower().endswith(".png"):
                continue
            path = os.path.join(d, fn)
            dim = png_dimensions(path)
            if dim is None:
                continue
            pool[dim].append((prio_idx, path))
    # Sort each bucket: priority dir first, alpha within.
    for dim in pool:
        pool[dim].sort(key=lambda t: (t[0], t[1]))
    return pool


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__); sys.exit(1)
    src = args[0]
    out = os.path.join(HERE, "..", "..", "build", "replay_texmap.json")
    i = 1
    while i < len(args):
        if args[i] == "--out":
            out = args[i+1]; i += 2
        else:
            print(f"unknown arg {args[i]}"); sys.exit(1)

    # Collect captured TEXTURE_DEFs in first-seen order.
    captured = []
    seen = set()
    for item in iter_records(src):
        if item[0] != "rec":
            continue
        rt, payload = item[1], item[2]
        if rt == RECORD_TEXTURE_DEF:
            td = TextureDef.unpack(payload)
            if td.tex_id in seen:
                continue
            seen.add(td.tex_id)
            captured.append((td.tex_id, td.width, td.height, td.sha1.hex()))
    print(f"[texmap] {len(captured)} captured texture defs", file=sys.stderr)

    # Build local PNG pool grouped by (w,h).
    pool = build_png_pool()
    # Deal pool entries to captured by dim, in first-seen order. Cursor per dim.
    cursor = defaultdict(int)
    mapping = {}
    unresolved = []
    for tid, w, h, sha in captured:
        bucket = pool.get((w, h), [])
        idx = cursor[(w, h)]
        if idx < len(bucket):
            _, png = bucket[idx]
            cursor[(w, h)] += 1
            mapping[f"{tid:08x}"] = png
        else:
            unresolved.append((tid, w, h))
    print(f"[texmap] mapped {len(mapping)}/{len(captured)} "
          f"({len(unresolved)} unresolved -- no same-dim PNG left in pool)",
          file=sys.stderr)
    # Quick summary by dim
    used_by_dim = defaultdict(int)
    for tid_hex, png in mapping.items():
        # re-derive dim from captured list (cheap)
        pass
    # actually just count exhaustion
    for dim, count in sorted(cursor.items(), key=lambda x: -x[1]):
        avail = len(pool.get(dim, []))
        print(f"  dim {dim[0]}x{dim[1]}: used {count}/{avail}",
              file=sys.stderr)

    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    with open(out, "w") as f:
        json.dump(mapping, f, indent=2, sort_keys=True)
    # Also write a parallel tab-separated .txt for easy C-side consumption.
    txt_out = os.path.splitext(out)[0] + ".txt"
    with open(txt_out, "w") as f:
        for tid_hex in sorted(mapping):
            f.write(f"{tid_hex}\t{mapping[tid_hex]}\n")
    print(f"[texmap] wrote {out} + {txt_out}", file=sys.stderr)


if __name__ == "__main__":
    main()
