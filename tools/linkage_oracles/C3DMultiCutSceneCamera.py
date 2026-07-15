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

The oracle also proves the full world-space camera placement for in-range
CameraTypeN entries after the recovered `transform_local_00472980` port: the
local offset table is transformed through the resolved target object's three
authored rotations, then the native look point is checked as
`target.y + LookatVOffsetN - 60`. Synthetic non-zero rotation cases keep the
three-axis transform non-vacuous.

See docs/linked_parity_plan.md for the Linkage Certificate. Exit 0 + PASS on
faithful reproduction; non-zero + a diff on any mismatch.
"""
from __future__ import annotations

import math
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
from asset_paths import gam_root  # noqa: E402

GAM_DIR = gam_root()
T_SAMPLES = (0.0, 1.0, 5.0, 10.0, 20.0, 60.0)  # sweeps at/above the 100-unit floor
WORLD_T_SAMPLES = (0.0, 5.0, 20.0)
FULL_EPS = 0.10


def f32_bits(x: float) -> int:
    return struct.unpack("<I", struct.pack("<f", float(x)))[0]


def bits_f32(bits: int) -> float:
    return struct.unpack("<f", struct.pack("<I", bits))[0]


def native_pose(props: dict) -> tuple[float, float, float, float, float, float]:
    return (
        float(props.get("PositionX", 0.0)),
        float(props.get("PositionY", 0.0)),
        -float(props.get("PositionZ", 0.0)),
        math.radians(float(props.get("RotationX", 0.0))),
        math.radians(float(props.get("RotationY", 0.0))),
        -math.radians(float(props.get("RotationZ", 0.0))),
    )


def trig14(original_rad: float) -> tuple[float, float]:
    idx = int(original_rad * (8192.0 / math.pi)) & 0x3fff
    theta = idx * (2.0 * math.pi / 16384.0)
    return math.sin(theta), math.cos(theta)


def transform_local_native(pose: tuple[float, float, float, float, float, float],
                           local: tuple[float, float, float]) -> tuple[float, float, float]:
    x, y, z, rx, ry, rz = pose
    sx, cx = trig14(rx)
    sy, cy = trig14(ry)
    sz, cz = trig14(-rz)
    # Native world mirrors the original Z axis, so mirror the authored local
    # offset too before applying native rotations. This keeps cutscene +Z
    # camera offsets on Jimmy's front side under the native facing convention.
    lx, ly, lz = local[0], local[1], -local[2]

    xz = cz * ly - sz * lx
    yz = cz * lx + sz * ly
    t = cx * lz - xz * sx
    dx = yz * cy - t * sy
    dy = xz * cx + sx * lz
    dz = t * cy + yz * sy
    return (x + dx, y + dy, z - dz)


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


def load_camera_types() -> list[dict]:
    """Every real (file, tag, CameraTypeN) entry across all 3MCA rows/steps."""
    entries = []
    for f in sorted(GAM_DIR.glob("*.gam")):
        d = gam_parser.parse_gam(str(f))
        targets = {}
        for obj in d["objects"]:
            tag = obj["properties"].get("ObjectTag", "")
            if tag:
                targets[tag.lower()] = native_pose(obj["properties"])
        for obj in d["objects"]:
            if obj["type"] != "3MCA":
                continue
            p = obj["properties"]
            tag = p.get("ObjectTag", "")
            for i in range(8):
                key = f"CameraType{i}"
                if key in p:
                    target = p.get(f"CameraTarget{i}", "")
                    entries.append({
                        "file": f.name,
                        "tag": f"{tag}#{i}",
                        "camera_type": int(p[key]),
                        "target": target,
                        "target_pose": targets.get(target.lower()),
                        "look_voffset": float(p.get(f"LookatVOffset{i}", 100.0)),
                    })
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

    dist = Counter(e["camera_type"] for e in entries)
    in_range = [e for e in entries if 0 <= e["camera_type"] <= 4]
    out_of_range = [e for e in entries if not (0 <= e["camera_type"] <= 4)]
    if len(out_of_range) != 6:
        print(f"MISMATCH: expected 6 out-of-table CameraTypeN entries, found {len(out_of_range)} "
              f"({dict(dist)}) -- update the doc's scope note before trusting this oracle")
        return 1

    with tempfile.TemporaryDirectory() as d:
        dumper = build_dumper(Path(d))

        lines = []
        expected = {}
        idx = 0
        for e in in_range:
            ct = e["camera_type"]
            for t in T_SAMPLES:
                key = f"{e['file']}:{e['tag']}:CameraType={ct}:t={t}"
                lines.append(f"M|{idx}|{ct}|{f32_bits(t):08x}")
                expected[idx] = (key, tuple(f32_bits(v) for v in py_mca_offset(ct, t)))
                idx += 1

        world_entries = [e for e in in_range if e["target_pose"] is not None]
        if len(world_entries) == 0:
            print("MISMATCH: no in-range 3MCA CameraTargetN entries resolved for full placement")
            return 1

        world_lines = []
        world_expected = {}
        world_idx = 0
        for e in world_entries:
            ct = e["camera_type"]
            pose = e["target_pose"]
            for t in WORLD_T_SAMPLES:
                local = py_mca_offset(ct, t)
                world = transform_local_native(pose, local)
                look = (pose[0], pose[1] + e["look_voffset"] - 60.0, pose[2])
                world_lines.append(
                    f"W|{world_idx}|{ct}|{f32_bits(t):08x}|"
                    f"{f32_bits(pose[0]):08x}|{f32_bits(pose[1]):08x}|{f32_bits(pose[2]):08x}|"
                    f"{f32_bits(pose[3]):08x}|{f32_bits(pose[4]):08x}|{f32_bits(pose[5]):08x}|"
                    f"{f32_bits(e['look_voffset']):08x}"
                )
                world_expected[world_idx] = (
                    f"{e['file']}:{e['tag']}->{e['target']}:CameraType={ct}:t={t}",
                    (*world, *look),
                )
                world_idx += 1

        synthetic_pose = (100.0, 25.0, -650.0, math.radians(17.0), math.radians(43.0), -math.radians(29.0))
        for ct in range(5):
            for t in WORLD_T_SAMPLES:
                local = py_mca_offset(ct, t)
                world = transform_local_native(synthetic_pose, local)
                look = (synthetic_pose[0], synthetic_pose[1] + 125.0 - 60.0, synthetic_pose[2])
                world_lines.append(
                    f"W|{world_idx}|{ct}|{f32_bits(t):08x}|"
                    f"{f32_bits(synthetic_pose[0]):08x}|{f32_bits(synthetic_pose[1]):08x}|{f32_bits(synthetic_pose[2]):08x}|"
                    f"{f32_bits(synthetic_pose[3]):08x}|{f32_bits(synthetic_pose[4]):08x}|{f32_bits(synthetic_pose[5]):08x}|"
                    f"{f32_bits(125.0):08x}"
                )
                world_expected[world_idx] = (
                    f"@synthetic:CameraType={ct}:t={t}",
                    (*world, *look),
                )
                world_idx += 1

        proc = subprocess.run([str(dumper)], input="\n".join(lines + world_lines) + "\n",
                               capture_output=True, text=True)
        if proc.returncode != 0:
            print(f"MISMATCH: dumper exited {proc.returncode}\n{proc.stderr}")
            return 1

        checked = world_checked = 0
        for line in proc.stdout.splitlines():
            parts = line.split("|")
            if parts[0] == "M":
                i = int(parts[1])
                key, expected_bits = expected[i]
                got_bits = tuple(int(x, 16) for x in parts[2:5])
                if got_bits != expected_bits:
                    print(f"MISMATCH in case {key!r}:")
                    print(f"  native   ={tuple(f'{x:08x}' for x in got_bits)}")
                    print(f"  reference={tuple(f'{x:08x}' for x in expected_bits)}")
                    return 1
                checked += 1
            elif parts[0] == "W":
                i = int(parts[1])
                key, expected_vals = world_expected[i]
                got_vals = tuple(bits_f32(int(x, 16)) for x in parts[2:8])
                diffs = [abs(a - b) for a, b in zip(got_vals, expected_vals)]
                if any(d > FULL_EPS for d in diffs):
                    print(f"MISMATCH in world-placement case {key!r}:")
                    print(f"  native   ={tuple(f'{x:.6f}' for x in got_vals)}")
                    print(f"  reference={tuple(f'{x:.6f}' for x in expected_vals)}")
                    print(f"  absdiff  ={tuple(f'{x:.6f}' for x in diffs)}")
                    return 1
                world_checked += 1

        if checked != len(lines) or world_checked != len(world_lines):
            print(f"MISMATCH: expected {len(lines)} local + {len(world_lines)} world "
                  f"results, got {checked} + {world_checked}")
            return 1

    print(
        f"PASS C3DMultiCutSceneCamera/3mca-offset-table: cutscene_mca_local_offset "
        f"byte-exact on {checked} (entry x t) samples across {len(in_range)} real "
        f"CameraTypeN entries in the documented 0..4 range (all 114 shipped 3MCA "
        f"rows x up to 8 steps); full world placement and look point match "
        f"recovered transform_local on {world_checked} real/synthetic cases; "
        f"{len(out_of_range)} out-of-table entries excluded per the doc's scope note"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
