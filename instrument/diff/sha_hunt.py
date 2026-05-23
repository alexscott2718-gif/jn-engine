#!/usr/bin/env python3
r"""
sha_hunt.py -- attempt EXACT SHA-1 matches between captured TEXTURE_DEFs and
local PNGs by trying multiple D3D-surface byte orderings.

The proxy locks DDraw surfaces and hashes row-by-row (no pitch padding) of
32-bit pixel data. The actual byte order on the surface depends on the
D3DFORMAT the original chose -- commonly A8R8G8B8 (= little-endian byte order
BGRA), X8R8G8B8 (BGRA with alpha forced to 0xFF), R8G8B8 (24-bit BGR), or
R5G6B5 (16-bit). PNG decode through stbi gives RGB(A) by default; we try
several conversions per PNG to maximize hits.

Updates an existing texmap (build/replay_texmap.json + .txt) by replacing
heuristic-paired entries with confirmed SHA matches where they exist.
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


def png_byteorderings(path):
    """Decode the PNG via PIL and return a dict of variant-name -> raw bytes
    (for each plausible D3D surface byte layout). Also returns (w,h)."""
    from PIL import Image
    try:
        img = Image.open(path)
    except Exception:
        return None, None
    w, h = img.size
    out = {}
    try:
        rgba = img.convert("RGBA").tobytes()           # R,G,B,A
        rgb  = img.convert("RGB").tobytes()             # R,G,B
        out["RGBA"] = rgba
        out["RGB"]  = rgb
        # BGRA (A8R8G8B8 little-endian)
        ba = bytearray(rgba)
        for i in range(0, len(ba), 4):
            ba[i], ba[i+2] = ba[i+2], ba[i]
        out["BGRA"] = bytes(ba)
        # BGRA with alpha forced 0xFF (X8R8G8B8)
        bx = bytearray(ba)
        for i in range(3, len(bx), 4):
            bx[i] = 0xFF
        out["BGRX"] = bytes(bx)
        # BGR (24-bit)
        br = bytearray(rgb)
        for i in range(0, len(br), 3):
            br[i], br[i+2] = br[i+2], br[i]
        out["BGR"] = bytes(br)
        # Vertical flip (some D3D surfaces are top-down vs PNG bottom-up)
        # For each variant, also a row-flipped version (rare but cheap).
        for nm in ("RGBA","BGRA","BGRX","RGB","BGR"):
            stride = len(out[nm]) // h
            rows = [out[nm][y*stride:(y+1)*stride] for y in range(h)]
            out[nm + "_flip"] = b"".join(reversed(rows))
    except Exception:
        return None, None
    return (w, h), out


def gather_png_pool():
    pool = {}  # path -> ((w,h), {variant: bytes})
    for d in PRIORITY_DIRS:
        if not os.path.isdir(d):
            continue
        for fn in sorted(os.listdir(d)):
            if not fn.lower().endswith(".png"):
                continue
            path = os.path.join(d, fn)
            dim, variants = png_byteorderings(path)
            if dim is not None:
                pool[path] = (dim, variants)
    return pool


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__); sys.exit(1)
    src = args[0]
    texmap_path = os.path.join(HERE, "..", "..", "build", "replay_texmap.json")

    # Collect captured TEXTURE_DEFs (tex_id, w, h, sha).
    captured = {}
    for item in iter_records(src):
        if item[0] != "rec":
            continue
        if item[1] == RECORD_TEXTURE_DEF:
            td = TextureDef.unpack(item[2])
            captured[td.tex_id] = (td.width, td.height, td.sha1.hex())
    # Index captured by SHA (might be duplicates).
    sha_to_tex = defaultdict(list)
    for tid, (w, h, sha) in captured.items():
        sha_to_tex[sha].append((tid, w, h))
    print(f"[sha_hunt] {len(captured)} captured textures, "
          f"{len(sha_to_tex)} unique SHAs", file=sys.stderr)

    print("[sha_hunt] indexing local PNGs (this can take a few seconds)...",
          file=sys.stderr)
    pool = gather_png_pool()
    print(f"[sha_hunt] {len(pool)} local PNGs decoded", file=sys.stderr)

    # For every PNG variant, compute SHA-1 and check membership.
    hits = {}             # tex_id -> (png_path, variant)
    variant_stats = defaultdict(int)
    for path, (dim, variants) in pool.items():
        w, h = dim
        for vname, raw in variants.items():
            sha = hashlib.sha1(raw).hexdigest()
            if sha in sha_to_tex:
                for tid, tw, th in sha_to_tex[sha]:
                    if (tw, th) != (w, h):
                        continue   # paranoia: dims must agree too
                    if tid in hits:
                        continue   # first hit wins
                    hits[tid] = (path, vname)
                    variant_stats[vname] += 1
    print(f"[sha_hunt] CONFIRMED SHA matches: {len(hits)}/{len(captured)}",
          file=sys.stderr)
    print(f"  by variant: {dict(variant_stats)}", file=sys.stderr)

    # Merge into existing texmap (sha hits override heuristic pairings).
    if os.path.exists(texmap_path):
        with open(texmap_path) as f:
            existing = json.load(f)
    else:
        existing = {}
    replaced = 0
    added = 0
    for tid, (path, vname) in hits.items():
        key = f"{tid:08x}"
        if key in existing and existing[key] != path:
            replaced += 1
        elif key not in existing:
            added += 1
        existing[key] = path
    with open(texmap_path, "w") as f:
        json.dump(existing, f, indent=2, sort_keys=True)
    txt_path = os.path.splitext(texmap_path)[0] + ".txt"
    with open(txt_path, "w") as f:
        for k in sorted(existing):
            f.write(f"{k}\t{existing[k]}\n")
    print(f"[sha_hunt] merged into {texmap_path}: "
          f"replaced {replaced}, added {added}; total entries {len(existing)}",
          file=sys.stderr)


if __name__ == "__main__":
    main()
