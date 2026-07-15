#!/usr/bin/env python3
"""Run the fixed-step engine twice and require byte-identical state dumps."""

from __future__ import annotations

import argparse
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def run(level: str, frames: int, seed: int, output: Path) -> None:
    command = [
        str(ROOT / "jnengine"), "--level", level, "--headless",
        "--frames", str(frames), "--seed", str(seed),
        "--dump-state", str(output),
    ]
    subprocess.run(command, cwd=ROOT, check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--level", default="fixture0")
    parser.add_argument("--frames", type=int, default=300)
    parser.add_argument("--seed", type=int, default=1)
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix="jn-determinism-") as tmp:
        first = Path(tmp) / "first.txt"
        second = Path(tmp) / "second.txt"
        run(args.level, args.frames, args.seed, first)
        run(args.level, args.frames, args.seed, second)
        a = first.read_bytes()
        b = second.read_bytes()
        if a != b:
            for offset, (left, right) in enumerate(zip(a, b)):
                if left != right:
                    raise SystemExit(
                        f"determinism FAIL: first byte mismatch at offset {offset}"
                    )
            raise SystemExit(
                f"determinism FAIL: lengths differ ({len(a)} vs {len(b)})"
            )
    print(f"determinism PASS: {args.level}, {args.frames} fixed steps, seed {args.seed}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
