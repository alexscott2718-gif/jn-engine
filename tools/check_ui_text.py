#!/usr/bin/env python3
"""Headless unit + mutation check for the shared menu/HUD atlas renderer."""

from __future__ import annotations

import argparse
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/game/ui_text.c"
DUMPER = ROOT / "tools/ui_text_dump.c"


def build_and_run(source: Path, tmp: Path) -> str:
    binary = tmp / "ui_text_dump"
    command = [
        "cc", "-std=c99", "-Wall", "-Werror", "-O0",
        "-I", str(ROOT / "src/engine"),
        "-I", str(ROOT / "src/game"),
        str(DUMPER), str(source), "-o", str(binary),
    ]
    subprocess.run(command, cwd=ROOT, check=True, capture_output=True, text=True)
    return subprocess.run(
        [str(binary)], cwd=ROOT, check=True, capture_output=True, text=True
    ).stdout


def validate(output: str) -> list[str]:
    errors: list[str] = []
    lines = output.splitlines()
    glyphs = {}
    draws = []
    textures = []
    measure = None
    for line in lines:
        parts = line.split("|")
        if parts[0] == "G":
            glyphs[int(parts[1])] = int(parts[2])
        elif parts[0] == "T":
            textures.append(parts[1])
        elif parts[0] == "M":
            measure = (float(parts[1]), float(parts[2]))
        elif parts[0] == "D":
            draws.append(parts)

    # Cells 62..90 are the atlas's punctuation run; 91..108 are accented
    # Latin-1 forms this port does not map. ' ' has no glyph by design (it is
    # an advance, not a mark), and '~'/'^' have no cell at all -- they are the
    # sentinels proving the map still says no to something.
    expected_glyphs = {
        ord("A"): 0, ord("Z"): 25, ord("a"): 26, ord("z"): 51,
        ord("0"): 52, ord("9"): 61, ord(" "): -1,
        ord("("): 62, ord("-"): 73, ord("."): 64, ord(":"): 69,
        ord("@"): 78, ord("%"): 88, ord("~"): -1, ord("^"): -1,
    }
    if glyphs != expected_glyphs:
        errors.append(f"glyph map {glyphs} != {expected_glyphs}")
    if textures != ["assets/png/fontsmall.png"]:
        errors.append(f"texture paths {textures} != ['assets/png/fontsmall.png']")
    if measure != (166.0, 20.0):
        errors.append(f"measure {measure} != (166.0, 20.0)")
    if len(draws) != 7:
        errors.append(f"draw count {len(draws)} != 7")
        return errors

    expected_x = [10.0, 32.0, 54.0, 88.0, 110.0, 132.0, 154.0]
    expected_indices = [13, 30, 48, 6, 26, 38, 30]  # N e w G a m e
    u0, u1 = 3.5 / 22.0, 15.5 / 22.0
    for n, (parts, x, glyph) in enumerate(zip(draws, expected_x, expected_indices)):
        got = {
            "seq": int(parts[1]), "tex": int(parts[2]),
            "viewport": (int(parts[3]), int(parts[4])),
            "rect": tuple(float(v) for v in parts[5:9]),
            "uv": tuple(float(v) for v in parts[9:13]),
            "color": tuple(float(v) for v in parts[13:17]),
        }
        v_top = 1.0 - (glyph * 10.0 + 0.5) / 1109.0
        v_bottom = 1.0 - (glyph * 10.0 + 9.5) / 1109.0
        if got["seq"] != n or got["tex"] != 77 or got["viewport"] != (640, 480):
            errors.append(f"draw {n} identity {got}")
        if got["rect"] != (x, 20.0, 26.0, 20.0):
            errors.append(f"draw {n} rect {got['rect']}")
        expected_uv = (u0, v_top, u1, v_bottom)
        if any(abs(a - b) > 5e-8 for a, b in zip(got["uv"], expected_uv)):
            errors.append(f"draw {n} uv {got['uv']} != {expected_uv}")
        if got["color"] != (0.25, 0.50, 0.75, 1.0):
            errors.append(f"draw {n} color {got['color']}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix="jn-ui-text-") as raw:
        tmp = Path(raw)
        errors = validate(build_and_run(SOURCE, tmp))
        if errors:
            raise SystemExit("ui_text FAIL: " + "; ".join(errors))

        if args.selftest:
            mutant = tmp / "ui_text_mutant.c"
            text = SOURCE.read_text()
            needle = "return c - 'A';"
            if text.count(needle) != 1:
                raise SystemExit("ui_text selftest FAIL: mutation site missing")
            mutant.write_text(text.replace(needle, "return c - 'A' + 1;"))
            if not validate(build_and_run(mutant, tmp)):
                raise SystemExit("ui_text selftest FAIL: glyph-index mutant survived")

    suffix = "; glyph-index mutant rejected" if args.selftest else ""
    print("ui_text PASS: A-Z/a-z/0-9 + punctuation mapping, measurement, "
          "atlas UVs, and draw layout" + suffix)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
