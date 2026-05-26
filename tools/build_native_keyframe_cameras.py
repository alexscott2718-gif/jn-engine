#!/usr/bin/env python3
"""Emit native renderer camera descriptors from solved capture keyframes.

Generates one descriptor per keyframe under assets/native/keyframe_cameras/.
Each descriptor carries view + proj matrices in GL column-major form, used by
src/game/main.c when JN_NATIVE_LEVEL1_KEYFRAME=<frame> is set.

Basis transform (capture -> native): positions and the forward vector are
flipped on Y and Z. The native camera up is then recomputed via gl_lookat
with world-up = (0, 1, 0). This produces a right-side-up image for level
cameras (verified at keyframe 8881) but discards any roll/pitch in the
original capture. Tilted keyframes (7633, 7636) currently render with a
"camera looking straight ahead" approximation rather than the steep downward
tilt of the original capture; alignment validation surfaces the gap.

Resolving full basis transfer for tilted cameras requires nailing down the
capture R convention exactly (row- vs column-vector, Y-up vs Y-down, hand);
prior attempts to lift R rows directly produced inverted images. Until that
is resolved, lookat(up=Y) is the safer default.
"""

from __future__ import annotations

import json
import math
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "assets/capture/level1_hudfix/keyframe_views.json"
OUT_DIR = ROOT / "assets/native/keyframe_cameras"


def dot(a, b) -> float:
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def norm(v):
    n = math.sqrt(dot(v, v))
    if n < 1e-9:
        return (0.0, 0.0, -1.0)
    return (v[0] / n, v[1] / n, v[2] / n)


def gl_lookat(eye, fwd):
    f = norm(fwd)
    r = norm(cross(f, (0.0, 1.0, 0.0)))
    if dot(r, r) < 1e-9:
        r = (1.0, 0.0, 0.0)
    u = cross(r, f)
    return [
        r[0], u[0], -f[0], 0.0,
        r[1], u[1], -f[1], 0.0,
        r[2], u[2], -f[2], 0.0,
        -dot(eye, r), -dot(eye, u), dot(eye, f), 1.0,
    ]


def gl_perspective(fov_deg: float, aspect: float, near: float, far: float):
    f = 1.0 / math.tan(math.radians(fov_deg) * 0.5)
    return [
        f / aspect, 0.0, 0.0, 0.0,
        0.0, f, 0.0, 0.0,
        0.0, 0.0, (far + near) / (near - far), -1.0,
        0.0, 0.0, (2.0 * far * near) / (near - far), 0.0,
    ]


def fmt(vals) -> str:
    return " ".join(f"{v:.9g}" for v in vals)


# Heuristic: a capture camera is "tilted" (and therefore lookat(up=Y) will
# discard significant pitch/roll) when the Y component of the capture forward
# axis exceeds this threshold in magnitude.
TILTED_FWD_Y_THRESHOLD = 0.30


def main() -> int:
    data = json.loads(SRC.read_text())
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    written = 0
    tilted = []
    for frame, sol in sorted(data["frames"].items(), key=lambda kv: int(kv[0])):
        R = sol["R"]
        capture_eye = sol["eye"]
        capture_fwd = (R[2], R[5], R[8])
        eye = [capture_eye[0], -capture_eye[1], -capture_eye[2]]
        fwd = (capture_fwd[0], -capture_fwd[1], -capture_fwd[2])
        fov = float(sol.get("fov_deg", data.get("anchor_fov_deg", 59.985353)))
        near = float(sol.get("near", data.get("anchor_near", 20.0)))
        far = 28000.0
        view = gl_lookat(eye, fwd)
        proj = gl_perspective(fov, 1280.0 / 720.0, near, far)
        is_tilted = abs(capture_fwd[1]) > TILTED_FWD_Y_THRESHOLD
        if is_tilted:
            tilted.append(frame)
        out = OUT_DIR / f"{frame}.txt"
        out.write_text(
            "\n".join(
                [
                    "# native Level 1 keyframe camera descriptor",
                    "# GL right-handed, column-major; generated from keyframe_views.json",
                    "# view = gl_lookat(eye_native, fwd_native, up=(0,1,0))",
                    f"# tilted = {str(is_tilted).lower()}  "
                    f"# |capture_fwd.y|={abs(capture_fwd[1]):.3f}, "
                    f"threshold={TILTED_FWD_Y_THRESHOLD}",
                    f"frame {frame}",
                    f"inliers {int(sol.get('inliers', 0))}",
                    f"residual {float(sol.get('residual', 0.0)):.6f}",
                    f"yaw_deg {float(sol.get('yaw_deg', 0.0)):.6f}",
                    f"tilted {1 if is_tilted else 0}",
                    "screen 1280 720",
                    f"fovy {fov:.6f}",
                    f"near {near:.6f}",
                    f"far {far:.6f}",
                    f"eye {eye[0]:.6f} {eye[1]:.6f} {eye[2]:.6f}",
                    f"capture_eye {capture_eye[0]:.6f} "
                    f"{capture_eye[1]:.6f} {capture_eye[2]:.6f}",
                    f"capture_fwd {capture_fwd[0]:.9g} "
                    f"{capture_fwd[1]:.9g} {capture_fwd[2]:.9g}",
                    f"view {fmt(view)}",
                    f"proj {fmt(proj)}",
                    "",
                ]
            )
        )
        written += 1
    print(f"wrote {written} camera descriptors to {OUT_DIR.relative_to(ROOT)}")
    print(f"anchor={data.get('anchor')}")
    if tilted:
        print(
            f"tilted keyframes (lookat(up=Y) discards roll/pitch): "
            f"{', '.join(tilted)}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
