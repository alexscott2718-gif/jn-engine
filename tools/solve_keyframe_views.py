#!/usr/bin/env python3
"""Solve per-keyframe VIEW matrices for the multi-frame world fixture.

JNBG bakes the camera into every WORLD matrix: WORLD_baked = MODEL_true . VIEW.
The single-frame `scene_reproject.bin` works because all baked draws share one
VIEW; the multi-frame fixture currently does not because it unions baked draws
from 10 different VIEWs into one file. This tool runs the per-frame camera
solve from `instrument/diff/extract_camera.py` on every keyframe and writes a
`to_anchor` 4x4 row-major matrix per keyframe so the fixture builder can
multiply each baked vertex by `VIEW_kf^-1 . VIEW_anchor`, landing every draw
in the anchor frame's eye-space (which is what `CAPTURE_LEVEL1_PROJ_GL`
already expects, so the runtime needs no changes).

Phase 12 confirmed mirroring = IDENTITY for this level, so we lock
force_flip=False rather than letting registration pick per-frame and risk
inconsistent flips across keyframes.

Output: assets/capture/level1_hudfix/keyframe_views.json
"""
from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "instrument/diff"))

from extract_camera import (  # noqa: E402
    DEFAULT_PLACEMENTS,
    extract_frames_batch,
    load_placements,
    mat4_mul_row,
    solve_view,
)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--source",
                    default=str(ROOT / "build/level1_v4_hudfix.omtc"))
    ap.add_argument("--keyframes",
                    default=str(ROOT / "assets/capture/level1_hudfix/keyframes.json"))
    ap.add_argument("--placements", default=DEFAULT_PLACEMENTS)
    ap.add_argument("--out",
                    default=str(ROOT / "assets/capture/level1_hudfix/keyframe_views.json"))
    ap.add_argument("--eps", type=float, default=150.0,
                    help="placement-vs-capture registration epsilon (world units)")
    ap.add_argument("--steps", type=int, default=360,
                    help="coarse yaw sweep resolution")
    ap.add_argument("--allow-flip", action="store_true",
                    help="let registration pick flip per frame (default: locked False)")
    args = ap.parse_args()

    src = Path(args.source)
    if not src.exists():
        raise SystemExit(f"source capture not found: {src}")

    keyframes_meta = json.loads(Path(args.keyframes).read_text())
    picked = sorted({int(kf["frame"]) for kf in keyframes_meta["keyframes"]})
    anchor = int(keyframes_meta["anchor"])
    if anchor not in picked:
        raise SystemExit(f"anchor {anchor} not in keyframes list")
    if not picked:
        raise SystemExit("no keyframes")
    print(f"[solve] keyframes: {picked}  anchor={anchor}", file=sys.stderr)

    placements = load_placements(args.placements)
    print(f"[solve] {len(placements)} level1 placements", file=sys.stderr)

    print(f"[solve] batch-extracting frames from {src} ...", file=sys.stderr)
    t0 = time.time()
    frames = extract_frames_batch(str(src), picked)
    print(f"[solve] extract done in {time.time()-t0:.1f}s", file=sys.stderr)

    force_flip = None if args.allow_flip else False
    per_frame = {}
    for idx in picked:
        proj, worlds = frames.get(idx, (None, []))
        if proj is None or not worlds:
            print(f"[solve] frame {idx}: NO perspective draws (skipping)",
                  file=sys.stderr)
            per_frame[idx] = None
            continue
        t1 = time.time()
        sol = solve_view(proj, worlds, placements,
                         eps=args.eps, steps=args.steps,
                         force_flip=force_flip)
        dt = time.time() - t1
        if sol is None:
            print(f"[solve] frame {idx}: SOLVE FAILED in {dt:.1f}s",
                  file=sys.stderr)
            per_frame[idx] = None
            continue
        print(f"[solve] frame {idx}: inliers={sol['inliers']:3d}/{len(placements)}"
              f"  residual={sol['residual']:7.2f}"
              f"  m1_support={sol['m1_support']*100:5.1f}%"
              f"  fov={sol['fov']:.2f}  eye=({sol['eye'][0]:.1f},{sol['eye'][1]:.1f},{sol['eye'][2]:.1f})"
              f"  flip={sol['flip']}  ({dt:.1f}s)",
              file=sys.stderr)
        per_frame[idx] = sol

    if per_frame[anchor] is None:
        raise SystemExit(f"anchor frame {anchor} did not solve -- cannot proceed")

    anchor_view16 = per_frame[anchor]["view16"]

    # Compose: v_anchor_eye = v_baked_kf . VIEW_kf^-1 . VIEW_anchor
    # In row-vector form the combined matrix is mat4_mul_row(VIEW_kf^-1, VIEW_anchor).
    to_anchor = {}
    for idx in picked:
        sol = per_frame[idx]
        if sol is None:
            to_anchor[idx] = None
            continue
        M = mat4_mul_row(sol["view_inv16"], anchor_view16)
        to_anchor[idx] = M

    # Sanity: anchor's to_anchor should be ~identity.
    M_anc = to_anchor[anchor]
    diag_err = sum(abs(M_anc[i] - (1.0 if i in (0, 5, 10, 15) else 0.0))
                   for i in range(16))
    print(f"[solve] anchor to_anchor identity error: {diag_err:.3e}",
          file=sys.stderr)

    out_obj = {
        "source": str(src),
        "anchor": anchor,
        "anchor_fov_deg": per_frame[anchor]["fov"],
        "anchor_near": per_frame[anchor]["near"],
        "anchor_far": per_frame[anchor]["far"],
        "eps": args.eps,
        "steps": args.steps,
        "force_flip": force_flip,
        "frames": {},
    }
    for idx in picked:
        sol = per_frame[idx]
        if sol is None:
            out_obj["frames"][str(idx)] = None
            continue
        out_obj["frames"][str(idx)] = {
            "inliers": sol["inliers"],
            "residual": sol["residual"],
            "yaw_deg": sol["yaw"] * 180.0 / 3.141592653589793,
            "flip": sol["flip"],
            "m1_support": sol["m1_support"],
            "fov_deg": sol["fov"],
            "near": sol["near"],
            "far": sol["far"],
            "eye": list(sol["eye"]),
            "R": list(sol["R"]),
            "tv": list(sol["tv"]),
            "view16": list(sol["view16"]),
            "view_inv16": list(sol["view_inv16"]),
            "to_anchor16": list(to_anchor[idx]),
        }

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(out_obj, indent=2) + "\n")
    print(f"wrote {out_path}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
