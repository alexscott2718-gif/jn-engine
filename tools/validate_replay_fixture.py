#!/usr/bin/env python3
"""Validate a replay fixture against its source-controlled manifest."""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path


REPLAY_RE = re.compile(
    r"\[replay\] frame: issued (?P<draws>\d+) GL draws, "
    r"registered (?P<textures>\d+) textures, "
    r"skipped (?P<skipped>\d+) missing-texture draws"
)


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def require_path(root: Path, rel: str, label: str) -> Path:
    path = root / rel
    if not path.exists():
        raise FileNotFoundError(f"{label} missing: {rel}")
    return path


def run_replay(root: Path, manifest: dict, screenshot: Path) -> dict[str, int]:
    source = manifest["source"]
    expected = manifest["expected_replay"]
    omtc = require_path(root, source["frame_capture"], "frame capture")
    require_path(root, source["reference_screenshot"], "reference screenshot")
    require_path(root, source["inspect_dir"], "inspect directory")

    env = os.environ.copy()
    env["JN_REPLAY"] = str(omtc)
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
    sys.stdout.write(proc.stdout)
    sys.stderr.write(proc.stderr)
    match = REPLAY_RE.search(proc.stderr)
    if not match:
        if proc.returncode != 0:
            raise RuntimeError(f"replay command exited {proc.returncode}")
        raise RuntimeError("could not find replay counter line in stderr")

    actual = {
        "gl_draws": int(match.group("draws")),
        "registered_textures": int(match.group("textures")),
        "skipped_missing_texture_draws": int(match.group("skipped")),
    }
    for key, want in expected.items():
        got = actual.get(key)
        if got != want:
            raise RuntimeError(f"{key}: got {got}, expected {want}")
    if proc.returncode != 0:
        print(
            f"warning: replay wrapper exited {proc.returncode} after "
            "emitting matching counters",
            file=sys.stderr,
        )
    return actual


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "manifest",
        nargs="?",
        default="assets/capture/level1_hudfix/frame_meta.json",
        type=Path,
    )
    parser.add_argument(
        "--screenshot",
        default=Path("build/replay_hudfix_validation.png"),
        type=Path,
    )
    args = parser.parse_args()

    root = repo_root()
    manifest_path = args.manifest if args.manifest.is_absolute() else root / args.manifest
    manifest = json.loads(manifest_path.read_text())
    screenshot = args.screenshot if args.screenshot.is_absolute() else root / args.screenshot
    screenshot.parent.mkdir(parents=True, exist_ok=True)

    actual = run_replay(root, manifest, screenshot)
    expected_records = manifest.get("expected_records", {})
    inspect_dir = root / manifest["source"]["inspect_dir"]
    textures_json = inspect_dir / "textures.json"
    if textures_json.exists() and expected_records.get("texture_pixels") is not None:
        textures = json.loads(textures_json.read_text())
        got = len(textures)
        want = expected_records["texture_pixels"]
        if got != want:
            raise RuntimeError(f"textures.json entries: got {got}, expected {want}")

    print(
        "replay fixture PASS: "
        f"{actual['gl_draws']} draws, "
        f"{actual['registered_textures']} textures, "
        f"{actual['skipped_missing_texture_draws']} skipped missing-texture draws"
    )
    print(f"validation screenshot: {screenshot.relative_to(root)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
