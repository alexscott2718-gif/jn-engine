#!/usr/bin/env python3
"""Cache one frame of an OMTC capture to JSON for fast repeated matching.

Streams the (huge) original capture once and writes, per perspective draw:
view-space AABB center + extent, bound-texture SHA-1 (hex) and WxH. The matcher
(match_textures.py) then reads this JSON instead of re-streaming 3.6 GB.

Usage: cache_orig_frame.py <orig.omtc> --frame F [--out build/orig_frame.json]
"""
import os
import sys
import json

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from diff import extract_frame  # noqa: E402


def main():
    args = sys.argv[1:]
    path = args[0]
    frame = None
    out = os.path.join(HERE, "..", "..", "build", "orig_frame.json")
    i = 1
    while i < len(args):
        if args[i] == "--frame":
            frame = int(args[i + 1]); i += 2
        elif args[i] == "--out":
            out = args[i + 1]; i += 2
        else:
            print(f"unknown arg {args[i]}"); sys.exit(1)
    if frame is None:
        print("--frame F required"); sys.exit(1)

    print(f"[cache] streaming {path} frame {frame} (d3d)...", file=sys.stderr)
    fd = extract_frame(path, frame, "d3d")
    draws = []
    for d in fd.draws:
        if not d.persp:
            continue
        td = fd.textures.get(d.tex_id)
        draws.append({
            "center": [round(c, 2) for c in d.center],
            "footprint": [round(d.footprint[0], 2), round(d.footprint[1], 2)],
            "y_extent": round(d.y_extent, 2),
            "tex_id": d.tex_id,
            "sha1": td.sha1.hex() if td else None,
            "dim": [td.width, td.height] if td else None,
        })
    meta = {"frame": frame, "source": os.path.basename(path),
            "n_persp_draws": len(draws), "draws": draws}
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    with open(out, "w") as f:
        json.dump(meta, f)
    print(f"[cache] wrote {out}: {len(draws)} perspective draws", file=sys.stderr)


if __name__ == "__main__":
    main()
