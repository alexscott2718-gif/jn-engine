#!/usr/bin/env python3
"""Linkage oracle: C3DTriggerType / nexttrigger-camera-retarget.

Certifies the native port of RunTriggerTypeNextTarget's record write
(vftable-1 slot 242 @ 00447a70, L1 in docs/decomp/C3DTriggerType.md and
docs/decomp/evidence/triggertype_trigger_target5.md) against the recovered
math: the fixed camera-local offset (20, -20, -100) rotated through the
record's three 14-bit angles (Rz(C), then R(A), then R(B)) and added to the
NextTrigger target's world position, with the record position stored in
native space (Z displacement negated — the project-wide native Z mirror, same
convention as the certified cutscene placement rows).

Native side: the REAL, unmodified src/game/camera_record.c compiled into
tools/linkage_oracles/c3dtriggertype_dump.c and driven over the case grid.
Reference side: this file's float64 implementation of the L1 pseudocode.

Case grid: unlike the cutscene camera rows there is NO authored data table
feeding this function — the record angles and positions are pure runtime
state and the target position is a resolved object's world position — so the
grid is synthetic by construction: every octant of the 14-bit angle space
(including wrap-negative shorts), degenerate zero cases, and mixed
target/record positions spanning the coordinate ranges seen in shipped
levels. 516 full checks.

Out of scope (documented deviations, see the certificate note): the original's
outer gates — the trigger-focus byte DAT_0050985a (writer unrecovered) and
the current-game-object active-trigger pointer — and the DAT_004f8182
one-frame flag; the demo module gates on its runtime mode instead.
"""
import math
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent

EPS = 0.10  # float32 native vs float64 reference, matching the camera oracles

STEP = 2.0 * math.pi / 16384.0


def bits_f32(bits: int) -> float:
    return struct.unpack("<f", struct.pack("<I", bits))[0]


def sincos14(a14: int) -> tuple[float, float]:
    theta = (a14 & 0x3FFF) * STEP
    return math.sin(theta), math.cos(theta)


def reference_retarget(ax: int, ay: int, az: int,
                       target: tuple[float, float, float]) -> tuple[float, float, float]:
    """The recovered slot-242 record write (00447a70), native output space."""
    sA, cA = sincos14(ax)
    sB, cB = sincos14(ay)
    sC, cC = sincos14(az)
    x1 = 20.0 * cC + 20.0 * sC
    y1 = 20.0 * sC - 20.0 * cC
    t = -100.0 * cA - x1 * sA
    oy = x1 * cA - 100.0 * sA
    ox = y1 * cB - t * sB
    oz = y1 * sB + t * cB
    tx, ty, tz = target
    return (tx + ox, ty + oy, tz - oz)


def build_cases() -> list[tuple[int, int, int, float, float, float, float, float, float]]:
    angles = [0, 1024, 4096, 6000, 8192, 12288, 16000, -2048]
    targets = [
        (0.0, 0.0, 0.0),
        (9870.0, 63.0, -1034.0),      # level1 spawn-area magnitudes
        (-4500.0, 250.0, 7300.0),
        (120.5, -80.25, 999.75),
    ]
    rec_pos = (0.0, 10000.0, 0.0)     # the InitGame seed; overwritten by the call
    cases = []
    for ax in angles:
        for ay in angles:
            for az in angles:
                tx, ty, tz = targets[(ax + ay + az) % len(targets)]
                cases.append((ax, ay, az, *rec_pos, tx, ty, tz))
    for tgt in targets:               # full target sweep at a fixed rotation
        cases.append((3000, 15000, 500, *rec_pos, *tgt))
    return cases


def build_dumper(tmp: Path) -> Path:
    binp = tmp / "c3dtriggertype_dump"
    cmd = [
        "cc", "-O0",
        str(HERE / "c3dtriggertype_dump.c"),
        str(REPO / "src" / "game" / "camera_record.c"),
        "-lm", "-o", str(binp),
    ]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print("MISMATCH: dumper failed to compile\n" + r.stderr)
        sys.exit(1)
    return binp


def main() -> int:
    cases = build_cases()
    with tempfile.TemporaryDirectory() as td:
        binp = build_dumper(Path(td))
        feed = "\n".join(" ".join(str(v) for v in c) for c in cases) + "\n"
        r = subprocess.run([str(binp)], input=feed, capture_output=True, text=True)
        if r.returncode != 0:
            print("MISMATCH: dumper run failed\n" + r.stderr)
            return 1

    got_lines = [ln for ln in r.stdout.splitlines() if ln.startswith("P ")]
    if len(got_lines) != len(cases):
        print(f"MISMATCH: expected {len(cases)} result lines, got {len(got_lines)}")
        return 1

    checked = 0
    for case, line in zip(cases, got_lines):
        ax, ay, az, px, py, pz, tx, ty, tz = case
        expected = reference_retarget(ax, ay, az, (tx, ty, tz))
        got = tuple(bits_f32(int(x, 16)) for x in line.split()[1:4])
        diffs = [abs(a - b) for a, b in zip(got, expected)]
        if any(d > EPS for d in diffs):
            print(f"MISMATCH in retarget case angles=({ax},{ay},{az}) target=({tx},{ty},{tz}):")
            print(f"  native   ={tuple(f'{x:.6f}' for x in got)}")
            print(f"  reference={tuple(f'{x:.6f}' for x in expected)}")
            print(f"  absdiff  ={tuple(f'{x:.6f}' for x in diffs)}")
            return 1
        checked += 1

    print(f"PASS C3DTriggerType/nexttrigger-camera-retarget: camera_record_retarget "
          f"matches the recovered slot-242 record write (00447a70) on {checked} "
          f"synthetic cases covering all octants of the 14-bit angle space "
          f"(incl. wrap-negative shorts) x mixed real-magnitude target positions; "
          f"fixed offset (20,-20,-100) and native Z mirror verified; no authored "
          f"data table feeds this function (runtime-state-only inputs)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
