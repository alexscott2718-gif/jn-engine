#!/usr/bin/env python3
"""Validate bounded live-Jimmy capture-backed screenshots."""

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
    world_pan: bool,
    verbose: bool,
) -> str:
    screenshot.parent.mkdir(parents=True, exist_ok=True)
    if screenshot.exists():
        screenshot.unlink()

    env = os.environ.copy()
    env["JN_CAPTURE_BACKED_LEVEL1"] = "1"
    env["JN_CAPTURE_BACKED_LIVE_JIMMY"] = "1"
    env["JN_CAPTURE_BACKED_LIVE_JIMMY_BOUNDS"] = "1"
    if world_pan:
        env["JN_CAPTURE_BACKED_WORLD_PAN"] = "1"
    else:
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


def validate_capture_startup(log: str, label: str, expect_reproject: bool) -> None:
    if "[capture_level1] skipping old visual-only OMT placements" not in log:
        raise RuntimeError(f"{label}: capture-backed startup did not skip old OMT placements")
    if "placements_load:" in log:
        raise RuntimeError(f"{label}: old OMT placement loader ran in capture-backed mode")
    if "placements_loaded=" in log:
        raise RuntimeError(f"{label}: old OMT placement loader ran in capture-backed mode")
    if expect_reproject and "scene_reproject.bin" not in log:
        raise RuntimeError(f"{label}: reproject scene was not loaded")


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
        default=root / "build/capture_backed_live_jimmy_bounded_spawn.png",
        type=Path,
    )
    parser.add_argument(
        "--far-screenshot",
        default=root / "build/capture_backed_live_jimmy_bounded_far_move.png",
        type=Path,
    )
    parser.add_argument(
        "--pan-screenshot",
        default=root / "build/capture_backed_live_jimmy_pan_far_move.png",
        type=Path,
    )
    parser.add_argument(
        "--reproject-spawn-screenshot",
        default=root / "build/capture_backed_live_jimmy_reproject_spawn.png",
        type=Path,
    )
    parser.add_argument(
        "--far-delta",
        default="900,0,500",
        help="visual-only x,y,z delta used to test bounded movement",
    )
    parser.add_argument("--max-spawn-mae", default=3.0, type=float)
    parser.add_argument("--max-far-mae", default=4.0, type=float)
    parser.add_argument("--max-reproject-spawn-mae", default=3.0, type=float)
    parser.add_argument("--max-pan-mae", default=80.0, type=float)
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()

    reference = args.reference if args.reference.is_absolute() else root / args.reference
    spawn = args.spawn_screenshot if args.spawn_screenshot.is_absolute() else root / args.spawn_screenshot
    far = args.far_screenshot if args.far_screenshot.is_absolute() else root / args.far_screenshot
    pan = args.pan_screenshot if args.pan_screenshot.is_absolute() else root / args.pan_screenshot
    reproject_spawn = (
        args.reproject_spawn_screenshot
        if args.reproject_spawn_screenshot.is_absolute()
        else root / args.reproject_spawn_screenshot
    )
    if not reference.exists():
        raise FileNotFoundError(f"reference screenshot missing: {reference.relative_to(root)}")

    spawn_log = run_capture(root, spawn, None, False, args.verbose)
    far_log = run_capture(root, far, args.far_delta, False, args.verbose)
    reproject_spawn_log = run_capture(root, reproject_spawn, None, True, args.verbose)
    pan_log = run_capture(root, pan, args.far_delta, True, args.verbose)
    validate_capture_startup(spawn_log, "spawn", False)
    validate_capture_startup(far_log, "far", False)
    validate_capture_startup(reproject_spawn_log, "reproject spawn", True)
    validate_capture_startup(pan_log, "pan", True)

    spawn_mae, spawn_rms = compare(reference, spawn)
    far_mae, far_rms = compare(reference, far)
    reproject_spawn_mae, reproject_spawn_rms = compare(reference, reproject_spawn)
    pan_mae, pan_rms = compare(reference, pan)
    failures = []
    if spawn_mae > args.max_spawn_mae:
        failures.append(f"spawn MAE {spawn_mae:.3f} > {args.max_spawn_mae:.3f}")
    if far_mae > args.max_far_mae:
        failures.append(f"far MAE {far_mae:.3f} > {args.max_far_mae:.3f}")
    if reproject_spawn_mae > args.max_reproject_spawn_mae:
        failures.append(
            f"reproject spawn MAE {reproject_spawn_mae:.3f} > "
            f"{args.max_reproject_spawn_mae:.3f}"
        )
    if pan_mae > args.max_pan_mae:
        failures.append(f"pan MAE {pan_mae:.3f} > {args.max_pan_mae:.3f}")

    print(f"reference: {reference.relative_to(root)}")
    print(f"spawn screenshot: {spawn.relative_to(root)}  MAE {spawn_mae:.3f}  RMS {spawn_rms:.3f}")
    print(f"far screenshot:   {far.relative_to(root)}  MAE {far_mae:.3f}  RMS {far_rms:.3f}")
    print(
        f"reproject spawn:  {reproject_spawn.relative_to(root)}  "
        f"MAE {reproject_spawn_mae:.3f}  RMS {reproject_spawn_rms:.3f}"
    )
    print(f"pan screenshot:   {pan.relative_to(root)}  MAE {pan_mae:.3f}  RMS {pan_rms:.3f}")
    if failures:
        raise RuntimeError("; ".join(failures))
    print("capture-backed live Jimmy bounded PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
