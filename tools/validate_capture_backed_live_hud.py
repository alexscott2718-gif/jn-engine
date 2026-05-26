#!/usr/bin/env python3
"""Validate the opt-in live HUD overlay for capture-backed Level 1."""

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


def run_capture(root: Path, screenshot: Path, verbose: bool) -> str:
    screenshot.parent.mkdir(parents=True, exist_ok=True)
    if screenshot.exists():
        screenshot.unlink()

    env = os.environ.copy()
    env["JN_CAPTURE_BACKED_LEVEL1"] = "1"
    env["JN_CAPTURE_BACKED_LIVE_HUD"] = "1"
    env.pop("JN_CAPTURE_BACKED_LIVE_JIMMY", None)
    env.pop("JN_CAPTURE_BACKED_WORLD_PAN", None)
    env["JN_SCREENSHOT"] = "1"
    env["JN_SCREENSHOT_PATH"] = str(screenshot)

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


def compare(reference: Path, actual: Path, box: tuple[int, int, int, int] | None = None) -> tuple[float, float]:
    ref = Image.open(reference).convert("RGB")
    got = Image.open(actual).convert("RGB")
    if ref.size != got.size:
        raise RuntimeError(f"image size mismatch: {reference} {ref.size}, {actual} {got.size}")
    if box:
        ref = ref.crop(box)
        got = got.crop(box)

    total_abs = 0
    total_sq = 0
    channels = ref.size[0] * ref.size[1] * 3
    for rp, gp in zip(ref.getdata(), got.getdata()):
        for rv, gv in zip(rp, gp):
            diff = int(rv) - int(gv)
            total_abs += abs(diff)
            total_sq += diff * diff
    return total_abs / channels, math.sqrt(total_sq / channels)


def validate_startup(log: str) -> None:
    required = [
        "[capture_level1] live HUD overlay enabled; captured HUD group hidden",
        "[capture_level1] skipping native Jimmy visual assets",
        "[capture_level1] skipping old visual-only OMT placements",
    ]
    for marker in required:
        if marker not in log:
            raise RuntimeError(f"live HUD startup missing marker: {marker}")
    forbidden = [
        "placements_load:",
        "placements_loaded=",
        "tex_load: assets/png/jimycarl.png",
        "ase_load: assets/ase/jim",
    ]
    for marker in forbidden:
        if marker in log:
            raise RuntimeError(f"live HUD startup loaded visual-only asset: {marker}")


def main() -> int:
    root = repo_root()
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--reference",
        default=root / "build/frame_v4_hudfix.png",
        type=Path,
    )
    parser.add_argument(
        "--screenshot",
        default=root / "build/capture_backed_live_hud_validation.png",
        type=Path,
    )
    parser.add_argument("--max-full-mae", default=4.0, type=float)
    parser.add_argument("--min-hud-mae", default=10.0, type=float)
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()

    reference = args.reference if args.reference.is_absolute() else root / args.reference
    screenshot = args.screenshot if args.screenshot.is_absolute() else root / args.screenshot
    if not reference.exists():
        raise FileNotFoundError(f"reference screenshot missing: {reference.relative_to(root)}")

    log = run_capture(root, screenshot, args.verbose)
    validate_startup(log)
    full_mae, full_rms = compare(reference, screenshot)
    hud_mae, hud_rms = compare(reference, screenshot, (0, 0, 260, 90))
    print(f"reference: {reference.relative_to(root)}")
    print(f"live HUD screenshot: {screenshot.relative_to(root)}  MAE {full_mae:.3f}  RMS {full_rms:.3f}")
    print(f"live HUD region:     MAE {hud_mae:.3f}  RMS {hud_rms:.3f}")
    if full_mae > args.max_full_mae:
        raise RuntimeError(f"live HUD full MAE {full_mae:.3f} > {args.max_full_mae:.3f}")
    if hud_mae < args.min_hud_mae:
        raise RuntimeError(f"live HUD region MAE {hud_mae:.3f} < {args.min_hud_mae:.3f}")
    print("capture-backed live HUD PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
