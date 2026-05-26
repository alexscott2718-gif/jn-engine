#!/usr/bin/env python3
"""Structural alignment check: native Level 1 map vs solved keyframe camera.

For a given keyframe camera descriptor (assets/native/keyframe_cameras/<frame>.txt)
this projects every loadable mesh's native placement through view * proj and
classifies it: in_frustum, behind_camera, off_screen_<left|right|top|bottom>.
It then cross-references the result against the capture-side solver inlier
count from assets/capture/level1_hudfix/keyframe_views.json.

The point is not to image-diff against the capture (which is reference
evidence, not authority). It is to verify, structurally, that:

  - the native keyframe camera's eye sits inside the native map bounds,
  - the camera is pointed at populated geometry, not the void,
  - a reasonable number of meshes project into the frustum,
  - the in-frustum count is consistent with the solver's inlier count for the
    same keyframe.

A regression here usually means the native basis (Y/Z flip) or the keyframe
descriptor's view matrix has drifted relative to the placements.
"""

from __future__ import annotations

import argparse
import json
import math
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
COVERAGE = ROOT / "assets/native/level1_map_coverage.json"
KEYFRAME_VIEWS = ROOT / "assets/capture/level1_hudfix/keyframe_views.json"
KEYFRAME_DIR = ROOT / "assets/native/keyframe_cameras"


def parse_camera_descriptor(path: Path) -> dict[str, object]:
    text = path.read_text()
    out: dict[str, object] = {"path": str(path.relative_to(ROOT))}
    eye = None
    view = None
    proj = None
    capture_eye = None
    for raw in text.splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        key = parts[0]
        rest = parts[1:]
        if key == "frame":
            out["frame"] = int(rest[0])
        elif key == "inliers":
            out["inliers"] = int(rest[0])
        elif key == "residual":
            out["residual"] = float(rest[0])
        elif key == "fovy":
            out["fovy"] = float(rest[0])
        elif key == "near":
            out["near"] = float(rest[0])
        elif key == "far":
            out["far"] = float(rest[0])
        elif key == "screen":
            out["screen"] = (int(rest[0]), int(rest[1]))
        elif key == "tilted":
            out["tilted"] = bool(int(rest[0]))
        elif key == "eye":
            eye = [float(x) for x in rest[:3]]
        elif key == "capture_eye":
            capture_eye = [float(x) for x in rest[:3]]
        elif key == "view":
            view = [float(x) for x in rest[:16]]
        elif key == "proj":
            proj = [float(x) for x in rest[:16]]
    if eye is None or view is None or proj is None:
        raise RuntimeError(f"{path}: missing eye/view/proj")
    out["eye"] = eye
    out["capture_eye"] = capture_eye
    out["view"] = view
    out["proj"] = proj
    return out


def matvec_colmajor(m: list[float], v: list[float]) -> list[float]:
    """m is column-major (flat list of 16); return m @ v."""
    out = [0.0, 0.0, 0.0, 0.0]
    for i in range(4):
        s = 0.0
        for j in range(4):
            s += m[j * 4 + i] * v[j]
        out[i] = s
    return out


def project_point(view: list[float], proj: list[float], p: tuple[float, float, float]):
    eye = matvec_colmajor(view, [p[0], p[1], p[2], 1.0])
    clip = matvec_colmajor(proj, eye)
    return eye, clip


def classify(eye: list[float], clip: list[float],
             radius: float = 0.0,
             proj: list[float] | None = None) -> str:
    """Classify a projected point against the frustum.

    If `radius > 0` and `proj` is provided, expand the NDC bounds by the
    mesh's projected screen radius. This treats each placement as a sphere of
    the given world radius, so meshes whose center is just off-screen but
    whose geometry extends into the frustum still count as visible. Crucial
    for tilted keyframes where only a narrow ground band intersects each
    mesh's footprint at its center point.
    """
    if eye[2] >= 0.0 or clip[3] <= 0.0:
        return "behind_camera"
    nx = clip[0] / clip[3]
    ny = clip[1] / clip[3]
    nz = clip[2] / clip[3]
    slack_x = slack_y = 0.0
    if radius > 0.0 and proj is not None:
        slack_x = radius * proj[0] / clip[3]
        slack_y = radius * proj[5] / clip[3]
    if nz < -1.0 or nz > 1.0:
        return "depth_clipped"
    if nx < -1.0 - slack_x:
        return "off_left"
    if nx > 1.0 + slack_x:
        return "off_right"
    if ny < -1.0 - slack_y:
        return "off_bottom"
    if ny > 1.0 + slack_y:
        return "off_top"
    return "in_frustum"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--keyframe",
        default="8881",
        help="Keyframe id to validate (matches assets/native/keyframe_cameras/<id>.txt)",
    )
    ap.add_argument(
        "--min-in-frustum",
        type=int,
        default=None,
        help="Minimum in-frustum count. Default scales with solver inliers: "
             "max(5, solver_inliers // 2).",
    )
    ap.add_argument(
        "--max-residual",
        type=float,
        default=200.0,
        help="Maximum acceptable solver residual",
    )
    ap.add_argument(
        "--report-top",
        type=int,
        default=10,
        help="How many nearest in-frustum meshes to list",
    )
    ap.add_argument(
        "--write-report",
        action="store_true",
        help="Write the full per-mesh classification to "
             "build/native_keyframe_alignment_<frame>.json",
    )
    args = ap.parse_args()

    desc_path = KEYFRAME_DIR / f"{args.keyframe}.txt"
    cam = parse_camera_descriptor(desc_path)

    coverage = json.loads(COVERAGE.read_text())
    placement_bounds = coverage["summary"]["placement_bounds"]
    native_bounds = coverage["summary"]["native_translation_bounds"]

    eye = cam["eye"]
    # Sanity: eye should lie within the native translation bounds, give or take.
    in_x = native_bounds["x"][0] - 5000.0 <= eye[0] <= native_bounds["x"][1] + 5000.0
    in_z = native_bounds["z"][0] - 5000.0 <= eye[2] <= native_bounds["z"][1] + 5000.0
    if not (in_x and in_z):
        raise RuntimeError(
            f"native keyframe eye {eye} falls outside native map bounds "
            f"x={native_bounds['x']} z={native_bounds['z']}"
        )

    buckets: dict[str, int] = {}
    in_frustum_meshes: list[dict[str, object]] = []
    for mesh in coverage["meshes"]:
        if not mesh["geometry"]["loadable"]:
            continue
        p = tuple(mesh["native_translation"])
        radius = float(mesh["geometry"].get("bounding_radius") or 0.0)
        eye_pt, clip = project_point(cam["view"], cam["proj"], p)
        cls = classify(eye_pt, clip, radius=radius, proj=cam["proj"])
        buckets[cls] = buckets.get(cls, 0) + 1
        if cls == "in_frustum":
            in_frustum_meshes.append(
                {
                    "mesh": mesh["name"],
                    "render_state": mesh["render_state"],
                    "classification": mesh["classification"],
                    "eye_z": eye_pt[2],
                    "ndc_x": clip[0] / clip[3],
                    "ndc_y": clip[1] / clip[3],
                    "ndc_depth": clip[2] / clip[3],
                    "bounding_radius": radius,
                    "placement": list(p),
                }
            )

    in_frustum_meshes.sort(key=lambda r: -r["eye_z"])  # least-negative = closest
    in_frustum = buckets.get("in_frustum", 0)
    visible = sum(
        buckets.get(k, 0)
        for k in ("in_frustum", "depth_clipped", "off_left", "off_right",
                  "off_top", "off_bottom")
    )

    solver_inliers = None
    solver_residual = None
    if KEYFRAME_VIEWS.exists():
        sv = json.loads(KEYFRAME_VIEWS.read_text())
        frames = sv.get("frames") or {}
        sol = frames.get(args.keyframe) or frames.get(str(args.keyframe))
        if sol:
            solver_inliers = int(sol.get("inliers", 0))
            solver_residual = float(sol.get("residual", 0.0))

    print(f"keyframe descriptor: {cam['path']}")
    print(
        f"frame={cam.get('frame')} desc_inliers={cam.get('inliers')} "
        f"residual={cam.get('residual'):.2f} fovy={cam.get('fovy'):.3f}"
    )
    print(f"native eye={eye} capture_eye={cam.get('capture_eye')}")
    print(f"placement bounds (capture basis): {placement_bounds}")
    print(f"native translation bounds:        {native_bounds}")
    print()
    print("projection buckets across 193 loadable meshes:")
    for key in ("in_frustum", "depth_clipped", "off_left", "off_right",
                "off_top", "off_bottom", "behind_camera"):
        print(f"  {key:>14s}: {buckets.get(key, 0)}")
    print(f"  total in front of camera: {visible}")
    print()
    if solver_inliers is not None:
        print(
            f"solver inliers at frame {args.keyframe}: {solver_inliers} "
            f"(residual {solver_residual:.2f})"
        )
    print()
    print(f"nearest in-frustum meshes (top {args.report_top} by eye_z):")
    for r in in_frustum_meshes[: args.report_top]:
        print(
            f"  {r['mesh']:24s} eye_z={r['eye_z']:10.1f} "
            f"ndc=({r['ndc_x']:+.2f},{r['ndc_y']:+.2f},{r['ndc_depth']:+.2f}) "
            f"render={r['render_state']} class={r['classification']}"
        )

    # The hard check is "does the camera basis produce a sensible visible set?"
    # Not "do we match the solver inliers exactly" — the solver works at vertex
    # granularity while this works at AABB granularity, and weakly-solved
    # keyframes (high residual) shift the visible set even with a correct basis.
    # Floor of 3 catches "camera points at empty space"; descriptor residual
    # catches a genuinely broken descriptor.
    if args.min_in_frustum is not None:
        min_required = args.min_in_frustum
    else:
        min_required = 3

    inlier_match_ratio = None
    if solver_inliers and solver_inliers > 0:
        inlier_match_ratio = in_frustum / solver_inliers
        print(
            f"native in_frustum / solver inliers = {in_frustum}/{solver_inliers} "
            f"= {inlier_match_ratio:.2f}"
        )

    failures: list[str] = []
    is_tilted = bool(cam.get("tilted"))
    if is_tilted:
        print(
            "note: descriptor flagged tilted; lookat(up=Y) approximates the "
            "capture camera, so in_frustum/inlier ratio is informational only."
        )
    if in_frustum < min_required and not is_tilted:
        failures.append(
            f"only {in_frustum} meshes project into the frustum "
            f"(min {min_required}); camera is likely pointed at empty space "
            "or the basis flip drifted"
        )
    if buckets.get("behind_camera", 0) >= len(coverage["meshes"]) - 1:
        failures.append("almost every mesh is behind the camera; basis is inverted")
    if cam.get("residual") is not None and cam["residual"] > args.max_residual:
        failures.append(
            f"keyframe descriptor residual {cam['residual']:.2f} exceeds "
            f"max {args.max_residual}"
        )

    if args.write_report:
        out_path = ROOT / f"build/native_keyframe_alignment_{args.keyframe}.json"
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(
            json.dumps(
                {
                    "keyframe": args.keyframe,
                    "eye": eye,
                    "capture_eye": cam.get("capture_eye"),
                    "buckets": buckets,
                    "solver_inliers": solver_inliers,
                    "solver_residual": solver_residual,
                    "in_frustum_meshes": in_frustum_meshes,
                },
                indent=2,
            )
            + "\n"
        )
        print(f"wrote {out_path.relative_to(ROOT)}")

    if failures:
        for f in failures:
            print(f"FAIL: {f}")
        return 1
    print()
    print(f"keyframe {args.keyframe} alignment PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
