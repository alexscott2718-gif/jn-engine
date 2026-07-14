#!/usr/bin/env python3
"""Generate or byte-compare deterministic software-rendered PNG frames."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ARTIFACTS = ROOT / "build" / "check-artifacts"


def run(level: str, frames: int, output: Path) -> None:
    command = [
        str(ROOT / "jnengine"), "--level", level, "--headless",
        "--frames", str(frames), "--seed", "1", "--dump-png", str(output),
    ]
    subprocess.run(command, cwd=ROOT, check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)


def copy_tree(source: Path, destination: Path) -> None:
    if destination.exists():
        shutil.rmtree(destination)
    shutil.copytree(source, destination)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--level", required=True)
    parser.add_argument("--frames", required=True, type=int)
    parser.add_argument("--goldens", required=True, type=Path)
    parser.add_argument("--update", action="store_true")
    args = parser.parse_args()
    goldens = args.goldens if args.goldens.is_absolute() else ROOT / args.goldens

    with tempfile.TemporaryDirectory(prefix=f"jn-golden-{args.level}-") as tmp:
        actual = Path(tmp) / "actual"
        run(args.level, args.frames, actual)
        generated = sorted(actual.glob("frame_*.png"))
        if len(generated) != args.frames:
            raise SystemExit(
                f"golden FAIL: generated {len(generated)} frames, expected {args.frames}"
            )
        if args.update:
            goldens.parent.mkdir(parents=True, exist_ok=True)
            copy_tree(actual, goldens)
            print(f"updated {len(generated)} {args.level} goldens in {goldens.relative_to(ROOT)}")
            return 0

        expected = sorted(goldens.glob("frame_*.png"))
        mismatches = []
        if len(expected) != args.frames:
            mismatches.append(
                f"expected directory has {len(expected)} frames, wanted {args.frames}"
            )
        for path in generated:
            reference = goldens / path.name
            if not reference.exists() or path.read_bytes() != reference.read_bytes():
                mismatches.append(path.name)
        if mismatches:
            artifact = ARTIFACTS / f"golden-{args.level}"
            copy_tree(actual, artifact / "actual")
            if goldens.exists():
                copy_tree(goldens, artifact / "expected")
            raise SystemExit(
                "golden FAIL: " + ", ".join(mismatches) +
                f"; inspect {artifact.relative_to(ROOT)}"
            )

    print(f"golden PASS: {args.level}, {args.frames} byte-identical PNGs")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
