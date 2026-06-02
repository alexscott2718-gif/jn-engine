#!/usr/bin/env python3
"""Backward-compatible wrapper for the native Level 1 map build."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    args = sys.argv[1:] or ["--level", "level1"]
    return subprocess.call([
        sys.executable,
        str(ROOT / "tools" / "build_native_map.py"),
        *args,
    ])


if __name__ == "__main__":
    raise SystemExit(main())
