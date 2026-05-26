#!/usr/bin/env python3
"""Build a side-by-side native-vs-capture review image.

This is a review artifact generator only. It does not run the engine, alter
assets, or modify capture data. Defaults target the keyframe-8881 Phase 0 loop.
"""

from __future__ import annotations

import argparse
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError as exc:  # pragma: no cover
    raise SystemExit("Pillow is required for side-by-side review images") from exc


ROOT = Path(__file__).resolve().parents[1]


def resolve(path: str) -> Path:
    p = Path(path)
    return p if p.is_absolute() else ROOT / p


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--capture",
        default="build/frame_v4_hudfix_candidate_8881.png",
        help="Capture/reference PNG",
    )
    ap.add_argument(
        "--native",
        default="build/native_level1_keyframe_8881.png",
        help="Native renderer PNG",
    )
    ap.add_argument(
        "--out",
        default="build/native_vs_capture_8881_side_by_side.png",
        help="Output side-by-side PNG",
    )
    ap.add_argument("--capture-label", default="capture frame 8881")
    ap.add_argument(
        "--native-label",
        default="native JN_NATIVE_LEVEL1 keyframe 8881",
    )
    args = ap.parse_args()

    capture_path = resolve(args.capture)
    native_path = resolve(args.native)
    out_path = resolve(args.out)
    if not capture_path.is_file():
        raise FileNotFoundError(capture_path)
    if not native_path.is_file():
        raise FileNotFoundError(native_path)

    capture = Image.open(capture_path).convert("RGB")
    native = Image.open(native_path).convert("RGB")
    w = max(capture.width, native.width)
    h = max(capture.height, native.height)
    label_h = 32

    canvas = Image.new("RGB", (w * 2, h + label_h), (32, 32, 32))
    canvas.paste(capture, (0, label_h))
    canvas.paste(native, (w, label_h))

    draw = ImageDraw.Draw(canvas)
    draw.text((12, 9), args.capture_label, fill=(255, 255, 255))
    draw.text((w + 12, 9), args.native_label, fill=(255, 255, 255))

    out_path.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(out_path)
    print(out_path.relative_to(ROOT) if out_path.is_relative_to(ROOT) else out_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
