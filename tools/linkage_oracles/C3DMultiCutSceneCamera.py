#!/usr/bin/env python3
"""Linkage oracle: C3DMultiCutSceneCamera (3MCA) CameraTypeN local-offset table.

Proves (L3) that the native `cutscene_mca_local_offset`
(src/game/behaviors/behavior_cutscene.c) reproduces the recovered `CameraTypeN`
jump table (Neutron.exe @ 004311d0, see "Per-frame camera update deepening" in
docs/decomp/C3DMultiCutSceneCamera.md) byte-exact, over **every real shipped
`CameraTypeN` value** across all 114 `3MCA` rows x 8 steps (assets/gam/*.gam,
tracked in git):

    CameraType  local offset (x, y, z)
    0 / default (0, 40, 200)
    1           (0, 140, max(100, 300 - 15*t))
    2           (0, 240, max(100, 500 - 35*t))
    3           (200, 240, max(100, 700 - 55*t))
    4           (-200, 190, max(100, 700 - 55*t))

`t` is the step timer in seconds.

Scope note: 906 of the 912 real (row, step) `CameraTypeN` entries fall inside
the documented 0..4 range; 6 author out-of-table values (5x `-1`, 1x `5`) --
docs/decomp/C3DMultiCutSceneCamera.md's recovered jump table only covers 0..4,
so this oracle only asserts byte-exactness for the documented range and
excludes the 6 out-of-range entries (native falls to the `default:` case for
them, which is a plausible but *undecompiled* jump-table-bounds guess -- not
certified here; see "Not covered" in the doc's Native Linkage section).

Also deliberately out of scope, same as `C3DCutSceneCamera`/`3cam-camera-math`:
the world-space camera position (this local offset transformed through
`entity_local_to_world`, the native-only yaw-only approximation of the
still-undecompiled `transform_local`) and the look-point formula
(`target.y + LookatVOffsetN - 60`, a single inline arithmetic line in
`cutscene_update`, not its own testable function -- verified by direct
inspection, not by this oracle).

See docs/linked_parity_plan.md for the Linkage Certificate. Exit 0 + PASS on
faithful reproduction; non-zero + a diff on any mismatch.
"""
from __future__ import annotations

import glob
import struct
import subprocess
import sys
import tempfile
from collections import Counter
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(REPO / "tools"))
import gam_parser  # noqa: E402  (reference parser, reused for real 3MCA data)

GAM_DIR = REPO / "assets" / "gam"
T_SAMPLES = (0.0, 1.0, 5.0, 10.0, 20.0, 60.0)  # sweeps at/above the 100-unit floor


def f32_bits(x: float) -> int:
    return struct.unpack("<I", struct.pack("<f", float(x)))[0]


def py_mca_offset(camera_type: int, t: float) -> tuple[float, float, float]:
    if camera_type == 1:
        z = max(100.0, 300.0 - 15.0 * t)
        return (0.0, 140.0, z)
    if camera_type == 2:
        z = max(100.0, 500.0 - 35.0 * t)
        return (0.0, 240.0, z)
    if camera_type == 3:
        z = max(100.0, 700.0 - 55.0 * t)
        return (200.0, 240.0, z)
    if camera_type == 4:
        z = max(100.0, 700.0 - 55.0 * t)
        return (-200.0, 190.0, z)
    return (0.0, 40.0, 200.0)  # 0 / default


def load_camera_types() -> list[tuple[str, str, int]]:
    """Every real (file, tag, CameraTypeN) entry across all 3MCA rows/steps."""
    entries = []
    for f in sorted(GAM_DIR.glob("*.gam")):
        d = gam_parser.parse_gam(str(f))
        for obj in d["objects"]:
            if obj["type"] != "3MCA":
                continue
            p = obj["properties"]
            tag = p.get("ObjectTag", "")
            for i in range(8):
                key = f"CameraType{i}"
                if key in p:
                    entries.append((f.name, f"{tag}#{i}", int(p[key])))
    return entries


def build_dumper(tmp: Path) -> Path:
    binp = tmp / "c3dmca_dump"
    cmd = [
        "cc", "-O0",
        str(HERE / "c3dmulticutscenecamera_dump.c"),
        str(REPO / "src" / "engine" / "assets" / "gam_loader.c"),
        str(REPO / "src" / "engine" / "player_physics.c"),
        "-lm", "-o", str(binp),
    ]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print("MISMATCH: dumper failed to compile\n" + r.stderr)
        sys.exit(1)
    return binp


def main() -> int:
    entries = load_camera_types()
    if len(entries) != 912:
        print(f"MISMATCH: expected 912 real CameraTypeN entries, found {len(entries)} "
              f"-- corpus changed, re-derive the in/out-of-range split before trusting this oracle")
        return 1

    dist = Counter(ct for _, _, ct in entries)
    in_range = [(f, tag, ct) for f, tag, ct in entries if 0 <= ct <= 4]
    out_of_range = [(f, tag, ct) for f, tag, ct in entries if not (0 <= ct <= 4)]
    if len(out_of_range) != 6:
        print(f"MISMATCH: expected 6 out-of-table CameraTypeN entries, found {len(out_of_range)} "
              f"({dict(dist)}) -- update the doc's scope note before trusting this oracle")
        return 1

    with tempfile.TemporaryDirectory() as d:
        dumper = build_dumper(Path(d))

        lines = []
        expected = {}
        idx = 0
        for f, tag, ct in in_range:
            for t in T_SAMPLES:
                key = f"{f}:{tag}:CameraType={ct}:t={t}"
                lines.append(f"M|{idx}|{ct}|{f32_bits(t):08x}")
                expected[idx] = (key, tuple(f32_bits(v) for v in py_mca_offset(ct, t)))
                idx += 1

        proc = subprocess.run([str(dumper)], input="\n".join(lines) + "\n",
                               capture_output=True, text=True)
        if proc.returncode != 0:
            print(f"MISMATCH: dumper exited {proc.returncode}\n{proc.stderr}")
            return 1

        checked = 0
        for line in proc.stdout.splitlines():
            parts = line.split("|")
            if parts[0] != "M":
                continue
            i = int(parts[1])
            key, expected_bits = expected[i]
            got_bits = tuple(int(x, 16) for x in parts[2:5])
            if got_bits != expected_bits:
                print(f"MISMATCH in case {key!r}:")
                print(f"  native   ={tuple(f'{x:08x}' for x in got_bits)}")
                print(f"  reference={tuple(f'{x:08x}' for x in expected_bits)}")
                return 1
            checked += 1

        if checked != len(lines):
            print(f"MISMATCH: expected {len(lines)} results, got {checked}")
            return 1

    print(
        f"PASS C3DMultiCutSceneCamera/3mca-offset-table: cutscene_mca_local_offset "
        f"byte-exact on {checked} (entry x t) samples across {len(in_range)} real "
        f"CameraTypeN entries in the documented 0..4 range (all 114 shipped 3MCA "
        f"rows x up to 8 steps); {len(out_of_range)} out-of-table entries excluded "
        f"per the doc's scope note"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
