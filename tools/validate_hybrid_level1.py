#!/usr/bin/env python3
"""Smoke-test the hybrid Level 1 runtime path.

Hybrid mode is the pivot away from treating capture reprojection as the world
backbone. It should run the normal GAM/entity simulation, load OMT-derived
native placements, use the native follow camera, and avoid loading
capture_scene fixtures.
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError as exc:  # pragma: no cover - environment guard
    raise SystemExit("Pillow is required for screenshot validation") from exc


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def run_hybrid(root: Path, screenshot: Path, include_capture_flag: bool = False) -> str:
    screenshot.parent.mkdir(parents=True, exist_ok=True)
    if screenshot.exists():
        screenshot.unlink()

    env = os.environ.copy()
    env["JN_HYBRID_LEVEL1"] = "1"
    env["JN_SCREENSHOT"] = "1"
    env["JN_SCREENSHOT_PATH"] = str(screenshot)
    if include_capture_flag:
        env["JN_CAPTURE_BACKED_LEVEL1"] = "1"
    else:
        env.pop("JN_CAPTURE_BACKED_LEVEL1", None)
    env.pop("JN_CAPTURE_BACKED_MULTIFRAME", None)
    env.pop("JN_CAPTURE_BACKED_WORLD_PAN", None)

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
    if proc.returncode != 0 and not saved:
        sys.stdout.write(proc.stdout)
        sys.stderr.write(proc.stderr)
        raise RuntimeError(f"hybrid command exited {proc.returncode}")
    if not saved:
        raise RuntimeError(f"screenshot missing: {screenshot.relative_to(root)}")
    return proc.stdout + proc.stderr


def validate_log(log: str, expect_supersede: bool = False) -> None:
    required = [
        "[hybrid_level1] native runtime enabled",
        "gam_load: assets/gam/Level1.gam",
        "placements_load: assets/ase/omt/level1_placements.txt",
        "placements_loaded=",
    ]
    for marker in required:
        if marker not in log:
            raise RuntimeError(f"hybrid startup missing marker: {marker}")
    if expect_supersede and "native hybrid mode supersedes JN_CAPTURE_BACKED_LEVEL1" not in log:
        raise RuntimeError("hybrid startup did not report capture-backed supersede")

    forbidden = [
        "[capture_scene] loaded",
        "scene_world.bin",
        "scene_reproject.bin",
        "assets/capture/level1_hudfix/scene.bin",
    ]
    for marker in forbidden:
        if marker in log:
            raise RuntimeError(f"hybrid mode unexpectedly used capture fixture: {marker}")


def validate_nonblank(path: Path) -> None:
    img = Image.open(path).convert("RGB")
    if img.size != (1280, 720):
        raise RuntimeError(f"unexpected screenshot size: {img.size}")

    pixels = list(img.getdata())
    channels = img.size[0] * img.size[1] * 3
    mean = sum(sum(px) for px in pixels) / channels
    unique_sample = len(set(pixels[:: max(1, len(pixels) // 5000)]))
    if mean < 2.0 or unique_sample < 32:
        raise RuntimeError(
            f"hybrid screenshot looks blank: mean={mean:.2f}, sampled_unique={unique_sample}"
        )


def main() -> int:
    root = repo_root()
    screenshot = root / "build/hybrid_level1_validation.png"
    log = run_hybrid(root, screenshot)
    validate_log(log)
    validate_nonblank(screenshot)
    conflict_screenshot = root / "build/hybrid_level1_capture_supersede.png"
    conflict_log = run_hybrid(root, conflict_screenshot, include_capture_flag=True)
    validate_log(conflict_log, expect_supersede=True)
    validate_nonblank(conflict_screenshot)
    print(f"hybrid screenshot: {screenshot.relative_to(root)}")
    print(f"hybrid supersede screenshot: {conflict_screenshot.relative_to(root)}")
    print("hybrid Level 1 smoke PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
