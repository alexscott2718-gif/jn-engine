#!/usr/bin/env python3
"""Cross-check the accepted capture fixture against native Level 1 state.

This gate validates the derived replay artifacts against their manifest, then
compares their captured camera contract with a short native run and confirms
that the native gameplay state contains the expected player entity.
"""

from __future__ import annotations

import json
import re
import subprocess
import tempfile
from pathlib import Path

from asset_paths import asset_root

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = asset_root() / "capture" / "level1_hudfix"
REQUIRED = ["draws.json", "draw_summary.json", "frame_meta.json", "scene_world.bin"]


def run_native(state: Path) -> str:
    command = [
        str(ROOT / "jnengine"), "--level", "level1", "--headless",
        "--frames", "2", "--seed", "7", "--dump-state", str(state),
    ]
    proc = subprocess.run(command, cwd=ROOT, check=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          text=True)
    return proc.stdout


def main() -> int:
    missing = [name for name in REQUIRED if not (FIXTURE / name).exists()]
    if missing:
        print("oracle diff SKIP: capture assets absent (" + ", ".join(missing) + ")")
        return 0

    draws = json.loads((FIXTURE / "draws.json").read_text())
    summary = json.loads((FIXTURE / "draw_summary.json").read_text())
    meta = json.loads((FIXTURE / "frame_meta.json").read_text())
    count = summary.get("draw_count")
    if len(draws) != count or meta["expected_replay"]["gl_draws"] != count:
        raise SystemExit("oracle diff FAIL: capture draw counts disagree")
    if (FIXTURE / "scene_world.bin").read_bytes()[:4] != b"JNW1":
        raise SystemExit("oracle diff FAIL: scene_world.bin has invalid magic")

    with tempfile.TemporaryDirectory(prefix="jn-oracle-diff-") as tmp:
        native = Path(tmp) / "native.txt"
        log = run_native(native)
        if b"type=3JIM" not in native.read_bytes():
            raise SystemExit("oracle diff FAIL: native state has no player")

    camera = re.search(
        r"viewport=(\d+)x(\d+) fov_y=([0-9.]+) near=([0-9.]+) far=([0-9.]+)",
        log,
    )
    if not camera:
        raise SystemExit("oracle diff FAIL: native camera contract was not logged")
    projection = meta["projection"]
    actual = tuple(float(camera.group(i)) for i in range(3, 6))
    expected = (
        float(projection["world_fov_y_degrees"]),
        float(projection["world_near_z"]),
        float(projection["world_far_z"]),
    )
    if any(abs(got - want) > 0.01 for got, want in zip(actual, expected)):
        raise SystemExit(
            f"oracle diff FAIL: replay/native camera mismatch {actual} vs {expected}"
        )

    print(f"oracle diff PASS: {count} replay draws; native camera/state contract matches")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
