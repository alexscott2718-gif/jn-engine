#!/usr/bin/env python3
"""Validate the multi-frame world capture-backed renderer.

What we test:
1. Spawn screenshot in multi-frame mode is close to the accepted reference
   frame (anchor keyframe 8881 covers the spawn region).
2. Far-movement screenshot DIFFERS materially from the spawn screenshot --
   i.e. the multi-frame world fixture is actually contributing new geometry
   as the camera tracks Jimmy. Single-frame world-pan can't do this.
3. Multi-frame run loaded `scene_world.bin` and reported multi-frame world
   reproject enabled.

A successful run prints MAE numbers and PASS. Failures raise.
"""
from __future__ import annotations

import argparse
import math
import os
import shutil
import subprocess
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError as exc:  # pragma: no cover - environment guard
    raise SystemExit("Pillow is required for screenshot comparison") from exc


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def run_capture(
    root: Path,
    screenshot: Path,
    test_delta: str | None,
    verbose: bool,
) -> str:
    screenshot.parent.mkdir(parents=True, exist_ok=True)
    if screenshot.exists():
        screenshot.unlink()

    env = os.environ.copy()
    env["JN_CAPTURE_BACKED_LEVEL1"] = "1"
    env["JN_CAPTURE_BACKED_LIVE_JIMMY"] = "1"
    env["JN_CAPTURE_BACKED_LIVE_JIMMY_BOUNDS"] = "1"
    env["JN_CAPTURE_BACKED_MULTIFRAME"] = "1"
    env.pop("JN_CAPTURE_BACKED_WORLD_PAN", None)
    env["JN_SCREENSHOT"] = "1"
    env["JN_SCREENSHOT_PATH"] = str(screenshot)
    if test_delta:
        env["JN_CAPTURE_BACKED_TEST_JIMMY_DELTA"] = test_delta
    else:
        env.pop("JN_CAPTURE_BACKED_TEST_JIMMY_DELTA", None)

    lib_dir = root.parent / "sdl2" / "lib"
    if lib_dir.exists():
        env["LD_LIBRARY_PATH"] = str(lib_dir)

    cmd = [str(root / "jnengine")]
    if not os.environ.get("DISPLAY") and shutil.which("xvfb-run"):
        cmd = ["xvfb-run", "-a", "-s", "-screen 0 1280x720x24"] + cmd

    proc = subprocess.run(
        cmd,
        cwd=root,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    saved = screenshot.exists() and screenshot.stat().st_size > 0
    if verbose or (proc.returncode != 0 and not saved):
        sys.stdout.write(proc.stdout)
        sys.stderr.write(proc.stderr)
    if proc.returncode != 0 and not saved:
        raise RuntimeError(f"capture command exited {proc.returncode}")
    if not saved:
        raise RuntimeError(f"screenshot missing: {screenshot.relative_to(root)}")
    return proc.stdout + proc.stderr


def validate_startup(log: str, label: str) -> None:
    if "scene_world.bin" not in log:
        raise RuntimeError(f"{label}: scene_world.bin was not loaded")
    if "multi-frame world reproject enabled" not in log:
        raise RuntimeError(f"{label}: multi-frame world reproject was not enabled")
    if "[capture_level1] skipping old visual-only OMT placements" not in log:
        raise RuntimeError(f"{label}: capture-backed startup did not skip old OMT placements")


def compare(reference: Path, actual: Path) -> tuple[float, float]:
    ref = Image.open(reference).convert("RGB")
    got = Image.open(actual).convert("RGB")
    if ref.size != got.size:
        raise RuntimeError(f"image size mismatch: {reference} {ref.size}, {actual} {got.size}")
    total_abs = 0
    total_sq = 0
    channels = ref.size[0] * ref.size[1] * 3
    for rp, gp in zip(ref.getdata(), got.getdata()):
        for rv, gv in zip(rp, gp):
            diff = int(rv) - int(gv)
            total_abs += abs(diff)
            total_sq += diff * diff
    return total_abs / channels, math.sqrt(total_sq / channels)


def main() -> int:
    root = repo_root()
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--reference",
        default=root / "build/frame_v4_hudfix.png",
        type=Path,
    )
    parser.add_argument(
        "--spawn-screenshot",
        default=root / "build/capture_backed_multiframe_spawn.png",
        type=Path,
    )
    parser.add_argument(
        "--far-screenshot",
        default=root / "build/capture_backed_multiframe_far_move.png",
        type=Path,
    )
    parser.add_argument(
        "--far-delta",
        default="900,0,500",
        help="visual-only x,y,z delta used to drive the multi-frame camera follow",
    )
    parser.add_argument("--max-spawn-mae", default=50.0, type=float,
                        help="multi-frame spawn differs from the anchor because"
                             " the current fixture unions eye-space draws from"
                             " several keyframes; this threshold is intentionally"
                             " loose until per-frame VIEW recovery lands")
    parser.add_argument("--min-far-vs-spawn-mae", default=4.0, type=float,
                        help="far must materially differ from spawn (proof that"
                             " multi-frame geometry contributes new pixels)")
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()

    reference = args.reference if args.reference.is_absolute() else root / args.reference
    spawn = args.spawn_screenshot if args.spawn_screenshot.is_absolute() else root / args.spawn_screenshot
    far = args.far_screenshot if args.far_screenshot.is_absolute() else root / args.far_screenshot

    if not reference.exists():
        raise FileNotFoundError(f"reference screenshot missing: {reference.relative_to(root)}")

    world_fixture = root / "assets/capture/level1_hudfix/scene_world.bin"
    if not world_fixture.exists():
        raise SystemExit(
            f"missing {world_fixture.relative_to(root)} -- run `make capture-world-fixture` first"
        )

    spawn_log = run_capture(root, spawn, None, args.verbose)
    far_log = run_capture(root, far, args.far_delta, args.verbose)
    validate_startup(spawn_log, "spawn")
    validate_startup(far_log, "far")

    spawn_mae, spawn_rms = compare(reference, spawn)
    far_mae, far_rms = compare(reference, far)
    diff_mae, diff_rms = compare(spawn, far)

    print(f"reference: {reference.relative_to(root)}")
    print(f"spawn screenshot: {spawn.relative_to(root)}  MAE {spawn_mae:.3f}  RMS {spawn_rms:.3f}")
    print(f"far screenshot:   {far.relative_to(root)}  MAE {far_mae:.3f}  RMS {far_rms:.3f}")
    print(f"far-vs-spawn diff:  MAE {diff_mae:.3f}  RMS {diff_rms:.3f}")

    failures = []
    if spawn_mae > args.max_spawn_mae:
        failures.append(f"spawn MAE {spawn_mae:.3f} > {args.max_spawn_mae:.3f}")
    if diff_mae < args.min_far_vs_spawn_mae:
        failures.append(
            f"far-vs-spawn MAE {diff_mae:.3f} < {args.min_far_vs_spawn_mae:.3f}"
            " (multi-frame world reproject does not appear to be revealing new geometry)"
        )
    if failures:
        raise RuntimeError("; ".join(failures))
    print("capture-backed multi-frame PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
