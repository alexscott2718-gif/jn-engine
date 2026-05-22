#!/usr/bin/env python3
r"""
match_textures.py -- recover mesh textures from the original capture by geometry.

The original capture's textures are SHA-1'd from raw locked surfaces, which don't
hash to the decoded asset PNGs (M5 caveat) -- so we can't name them directly. But
the DEMO already textures ~96 meshes with KNOWN level1 PNGs. By matching demo
draws to the original's draws in the shared (camera-matched) view space, we learn
a dictionary  original-texture-SHA -> asset PNG  from the agreed cases, then apply
it to the unresolved meshes.

Phase VALIDATE (this mode): build and score the SHA->PNG dictionary from the
demo's textured draws. If each PNG maps to one dominant SHA (high agreement), the
approach is sound and we proceed to apply it.

Usage:
  match_textures.py --orig-cache build/orig_frame.json --demo build/m7c/demo.omtc
"""
import os
import sys
import json
import math
from collections import defaultdict, Counter

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "receiver"))
from diff import extract_frame, load_demo_tex_sidecar, tex_label  # noqa: E402


def nearest(orig_draws, c, kept_dims=None):
    """Nearest original draw to view-space center c. If kept_dims is given,
    restrict to original draws whose texture dims are in that set (tightens the
    match using the demo PNG's known size)."""
    best = None
    bd = 1e30
    for o in orig_draws:
        if kept_dims is not None and (o["dim"] is None
                                      or tuple(o["dim"]) not in kept_dims):
            continue
        oc = o["center"]
        d = (oc[0]-c[0])**2 + (oc[1]-c[1])**2 + (oc[2]-c[2])**2
        if d < bd:
            bd = d
            best = o
    return best, math.sqrt(bd) if best else (None, 1e30)


def main():
    args = sys.argv[1:]
    orig_cache = os.path.join(HERE, "..", "..", "build", "orig_frame.json")
    demo = os.path.join(HERE, "..", "..", "build", "m7c", "demo.omtc")
    i = 0
    while i < len(args):
        if args[i] == "--orig-cache":
            orig_cache = args[i+1]; i += 2
        elif args[i] == "--demo":
            demo = args[i+1]; i += 2
        else:
            print(f"unknown arg {args[i]}"); sys.exit(1)

    with open(orig_cache) as f:
        oc = json.load(f)
    orig_draws = [o for o in oc["draws"] if o["sha1"]]
    print(f"[match] original frame {oc['frame']}: {len(orig_draws)} textured draws")

    fd = extract_frame(demo, 0, "gl")
    names = load_demo_tex_sidecar(demo)
    demo_draws = [d for d in fd.draws if d.persp and d.tex_id]
    print(f"[match] demo frame 0: {len(demo_draws)} textured draws")

    # Vote (PNG -> Counter of original SHA) using nearest original draw.
    png_to_sha = defaultdict(Counter)
    dists = []
    for d in demo_draws:
        png = os.path.basename(names.get(d.tex_id, "") or
                               tex_label(d.tex_id, fd, names) or "")
        if not png:
            continue
        o, dist = nearest(orig_draws, d.center)
        if o is None:
            continue
        dists.append(dist)
        png_to_sha[png][(o["sha1"], tuple(o["dim"]) if o["dim"] else None)] += 1

    dists.sort()
    if dists:
        print(f"[match] match distances: min {dists[0]:.0f} "
              f"median {dists[len(dists)//2]:.0f} max {dists[-1]:.0f}")

    print("\nPNG -> dominant original SHA (votes, agreement):")
    coherent = 0
    total = 0
    for png in sorted(png_to_sha):
        c = png_to_sha[png]
        total += 1
        top, n = c.most_common(1)[0]
        allv = sum(c.values())
        agree = n / allv
        if agree >= 0.6:
            coherent += 1
        sha, dim = top
        print(f"  {png:42s} -> {sha[:12]} {str(dim):12s} "
              f"{n}/{allv} ({agree*100:.0f}%)")
    print(f"\n[match] {coherent}/{total} PNGs map to a dominant SHA (>=60% agree)")


if __name__ == "__main__":
    main()
