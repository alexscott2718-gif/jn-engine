#!/usr/bin/env python3
"""Audit Level5a QA audio handles and Yokian class texture prerequisites."""

from __future__ import annotations

import csv
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gam_parser import parse_gam  # noqa: E402

REPO = Path(__file__).resolve().parents[1]


def load_map(stem: str) -> dict[int, tuple[str, str]]:
    path = REPO / "assets" / "parsed" / stem / f"{stem}_audio_handles.tsv"
    rows: dict[int, tuple[str, str]] = {}
    with path.open() as f:
        for row in csv.reader((line for line in f if not line.startswith("#")), delimiter="\t"):
            if len(row) < 3:
                continue
            rows[int(row[0])] = (row[1], row[2])
    return rows


def assert_map(stem: str, handle: int, suffix: str) -> None:
    rows = load_map(stem)
    if handle not in rows:
        raise AssertionError(f"{stem}[{handle}] missing from handle map")
    wav, name = rows[handle]
    if not wav.endswith(suffix):
        raise AssertionError(f"{stem}[{handle}] -> {wav}, expected *{suffix} ({name})")


def main() -> int:
    expected = [
        ("soundeffects", 99, "0279_holon.wav"),
        ("soundeffects", 107, "0059_jimmyitem9.wav"),
        ("soundeffects", 111, "0063_jimmyitem5.wav"),
        ("soundeffects", 112, "0079_jimmyitem4.wav"),
        ("soundeffects", 114, "0076_jimmyitem2.wav"),
        ("soundeffects", 115, "0101_jimmyitem10.wav"),
        ("soundeffects", 119, "0066_goddarddogbark6.wav"),
        ("soundeffects", 247, "0288_jimmy_yahoo.wav"),
        ("soundeffects", 248, "0302_jimmy_dizzy.wav"),
        ("musicship", 1, "0001_Snip1_22k.wav"),
        ("musicship", 2, "0002_Spooky_22k.wav"),
        ("musicarea51", 0, "0000_Hunting3_11k.wav"),
        ("musicrocketmaze", 0, "0000_Surf_Music_Demo_End.wav"),
    ]
    for stem, handle, suffix in expected:
        assert_map(stem, handle, suffix)

    objs = parse_gam(REPO / "assets" / "gam" / "level5a.gam")["objects"]
    rows = {(i, o["type"], o["properties"].get("ObjectTag", "")): o["properties"]
            for i, o in enumerate(objs)}
    checks = [
        ((112, "3PIC", "C3DPICKUPITEM"), "SoundIndex", 99),
        ((78, "3PIC", "barrelroll"), "SoundIndex", 247),
        ((150, "3PIC", "barrelroll2"), "SoundIndex", 248),
        ((161, "3MUS", "music"), "MusicIndex2", 1),
        ((162, "3MUS", "C3DMUSICTRIGGER"), "MusicIndex2", 2),
        ((2, "STRT", "cave"), "MusicDatabase", "musicrocketmaze.omt"),
    ]
    for key, prop, expected_value in checks:
        if key not in rows:
            raise AssertionError(f"level5a GAM row {key} missing")
        got = rows[key].get(prop)
        if got != expected_value:
            raise AssertionError(f"level5a row {key} {prop}={got!r}, expected {expected_value!r}")

    for ase in ["guardwalk.ASE", "soldwalk.ASE"]:
        text = (REPO / "assets" / "ase" / ase).read_text(errors="replace")
        if "*MESH_TVERT" in text or "*MESH_TFACE" in text:
            raise AssertionError(f"{ase} unexpectedly has authored UVs; revisit fallback rule")
    for png in ["yokguard.png", "yoksold.png"]:
        if not (REPO / "assets" / "png" / png).exists():
            raise AssertionError(f"missing class-attached Yokian texture {png}")

    print("Level5a QA audio/Yokian audit passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
