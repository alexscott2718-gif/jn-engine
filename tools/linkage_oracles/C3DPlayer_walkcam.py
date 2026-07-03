#!/usr/bin/env python3
"""Linkage oracle: C3DPlayer / walking-camera-record.

Certifies the native port of the UpdateWalkingCameraB (00439900) record
write, normal path (motion_submode != 2), against the recovered math (L1 in
docs/decomp/evidence/walking_camera_record_write.md):

  snap = rec.pos
  eye  = ProjectNoisyCameraTarget(0, 200, -350)   # 0043a5d0: pitch index 0,
                                                  # yaw + turn lead, negative
                                                  # yaw folded 360-|int(deg)|
  look = transform_local(0, 80, 0)                # 00472980 via slot +0x384
  k    = ProbePlayerRayBlend(look, &eye)          # 0043b820: 1.0 free / 1.5
                                                  # hit, eye pulled 75% to hit
  rec.pos += (eye - snap) * (1.2, 2.0, 1.2) * k * dt
  rec.angle_x/y snap to OMedia3DVector::angles(look - snap)

The angle extraction is the OMT2.dll import OMedia3DVector::angles; the
reference below transcribes the LGPL Open Media Toolkit source
(OMedia3DVector.cpp / OMediaTrigo.h, (c) Yves Schmid / GarageCube) including
the short-truncated acos table and its float32 index arithmetic.

Native side: the REAL, unmodified src/game/camera_record.c compiled into
tools/linkage_oracles/c3dplayer_walkcam_dump.c and driven over the case grid;
the ray-probe hook is driven synthetically to certify the hit path.
Reference side: this file's implementation with float32 emulation at every
recovered float step, float64 only inside sin/cos/acos.

Case grid: like the retarget row there is NO authored data table feeding this
function — record state, player pose, turn lead, and dt are pure runtime
state — so the grid is synthetic by construction: yaw sweeps exercising both
quantization paths (plain ftol+mask vs the noisy negative/>=360 folds),
nonzero roll (the recovered Rz half), turn leads, both dt magnitudes, the
14-bit record-angle octants (incl. wrap-negative shorts), position sweeps at
shipped-level magnitudes, the probe/blend path, and degenerate zero-direction
cases. Angle comparisons allow +/-2 table units (sinf vs double-rounded sin
at the acos index boundary); positions use the module-standard 0.10 EPS.

Out of scope (documented deviations, see the certificate note): the A-variant
(00438bc0) pan offsets and 0x854..0x85c scales, B's submode-2 scales and
height gates, the 0x851 modes, and the original world-ray internals
(FUN_0047c210) — the demo leaves the probe hook NULL; the follow wrapper's
observed-yaw turn lead stands in for the 0x6d4 input ramp.
"""
import math
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent

EPS = 0.10          # float32 native vs emulated-float32 reference, positions
ANG_TOL = 2         # 14-bit table units (0.044 deg)

STEP32 = struct.unpack("<f", struct.pack("<f", 2.0 * math.pi / 16384.0))[0]
DEG2IDX = struct.unpack("<f", struct.pack("<f", 45.511112))[0]


def f32(x: float) -> float:
    return struct.unpack("<f", struct.pack("<f", x))[0]


def bits_f32(bits: int) -> float:
    return struct.unpack("<f", struct.pack("<I", bits))[0]


def i16(v: int) -> int:
    v &= 0xFFFF
    return v - 0x10000 if v >= 0x8000 else v


# OMediaCosSinTable::init_table acos table: short(acos(l/0x2000 - 1)*8192/pi / 2)
_ACOS = [int((math.acos(l / 8192.0 - 1.0) * 8192.0 / math.pi) / 2.0)
         for l in range(0x4000)] + [0]


def omedia_atan2(opp: float, adj: float) -> int:
    if opp == 0.0 and adj == 0.0:
        return 0
    adj2 = f32(adj * adj)
    hyp = f32(adj2 + f32(opp * opp))
    t = int(f32(f32(adj2 * 16384.0) / hyp))
    t = min(max(t, 0), 0x4000)
    angle = _ACOS[t]
    if adj < 0.0:
        return 8192 + angle if opp < 0.0 else 8192 - angle
    return 16384 - angle if opp < 0.0 else angle


def trig14(idx: int) -> tuple[float, float]:
    th = f32(f32(idx & 0x3FFF) * STEP32)
    return f32(math.sin(th)), f32(math.cos(th))


def omedia_angles(v) -> tuple[int, int]:
    ay = (-omedia_atan2(v[0], v[2])) & 0x3FFF
    s, c = trig14(-ay)
    rz = f32(f32(v[0] * s) + f32(v[2] * c))
    ax = ((-omedia_atan2(rz, v[1])) + 4096) & 0x3FFF
    if ax > 8192:
        ax = -(16384 - ax)
    return ax, ay


def project(pos, yaw_deg: float, roll_deg: float, lead: float,
            noisy: bool, local) -> tuple[float, float, float]:
    yaw = f32(yaw_deg + (lead if noisy else 0.0))
    if noisy:
        if yaw < 0.0:
            yaw = f32(360 - abs(int(yaw)))       # integer-degree fold quirk
        elif yaw >= 360.0:
            yaw = f32(yaw - 360.0)
    yidx = int(f32(yaw * DEG2IDX)) & 0x3FFF
    ridx = int(f32(roll_deg * DEG2IDX)) & 0x3FFF
    sy, cy = trig14(yidx)
    sr, cr = trig14(ridx)
    t1 = f32(f32(cr * local[1]) - f32(sr * local[0]))
    t2 = f32(f32(cr * local[0]) + f32(sr * local[1]))
    u = f32(local[2])
    return (f32(pos[0] + f32(f32(t2 * cy) - f32(u * sy))),
            f32(pos[1] + t1),
            f32(pos[2] - f32(f32(u * cy) + f32(t2 * sy))))


EYE_LOCAL = (0.0, 200.0, -350.0)
LOOK_LOCAL = (0.0, 80.0, 0.0)


def reference_walkcam(rec_pos, rec_ang, ppos, yaw, roll, lead, dt,
                      probe, hit):
    """The recovered B-variant record write, native output space."""
    snap = rec_pos
    eye = project(ppos, yaw, roll, lead, True, EYE_LOCAL)
    look = project(ppos, yaw, roll, 0.0, False, LOOK_LOCAL)
    k = 1.0
    if probe:
        eye = tuple(f32(look[i] - f32(f32(look[i] - hit[i]) * 0.75))
                    for i in range(3))
        k = 1.5
    dyy = f32(f32(f32(eye[1] - snap[1]) * k) * dt)
    npos = (f32(snap[0] + f32(f32(f32(f32(eye[0] - snap[0]) * 1.2) * k) * dt)),
            f32(snap[1] + f32(dyy + dyy)),
            f32(snap[2] + f32(f32(f32(f32(eye[2] - snap[2]) * 1.2) * k) * dt)))
    d = (f32(look[0] - snap[0]), f32(look[1] - snap[1]),
         f32(-(f32(look[2] - snap[2]))))
    ax, ay = omedia_angles(d)
    a0 = i16(rec_ang[0] + i16(ax - rec_ang[0]))
    dy = (ay - rec_ang[1]) & 0x3FFF
    if dy > 0x2000:
        dy -= 0x4000
    a1 = i16(rec_ang[1] + dy)
    return npos, (a0, a1, rec_ang[2])


def build_cases():
    cases = []
    rec_pos = (9600.0, 320.0, -880.0)
    rec_ang = (150, 5400, 0)
    ppos = (9870.0, 63.0, -1034.0)      # level1 spawn-area magnitudes
    yaws = [0.0, 45.0, 90.5, 179.0, 180.0, 271.25, 359.0, 360.0, 450.0,
            -10.7, -180.0, -359.9, 720.25]
    for yaw in yaws:                     # fold-path coverage x lead x roll x dt
        for lead in (0.0, 12.5, -30.0):
            for roll in (0.0, 15.0, -30.0):
                for dt in (0.0166667, 0.05):
                    cases.append((*rec_ang, *rec_pos, *ppos,
                                  yaw, roll, lead, dt, 0, 0.0, 0.0, 0.0))
    octants = [0, 1024, 4096, 6000, 8192, 12288, 16000, -2048]
    for a in octants:                    # record-angle wrap coverage
        for yaw in (0.0, 135.0, -90.0, 300.0):
            cases.append((a, a, a, *rec_pos, *ppos,
                          yaw, 0.0, 0.0, 0.0166667, 0, 0.0, 0.0, 0.0))
    positions = [(0.0, 0.0, 0.0), (9870.0, 63.0, -1034.0),
                 (-4500.0, 250.0, 7300.0), (120.5, -80.25, 999.75)]
    for rp in positions:                 # position sweeps
        for pp in positions:
            cases.append((150, 5400, 0, *rp, *pp,
                          77.0, 0.0, 0.0, 0.0166667, 0, 0.0, 0.0, 0.0))
    hits = [(9800.0, 100.0, -900.0), (9900.0, 150.0, -1100.0),
            (0.0, 50.0, 0.0), (-4400.0, 260.0, 7200.0)]
    for i, hit in enumerate(hits):       # probe/blend path (k = 1.5)
        for yaw in (30.0, -120.0):
            cases.append((150, 5400, 0, *rec_pos, *ppos,
                          yaw, 0.0, 10.0 * i, 0.0166667, 1, *hit))
    # degenerates: player at the record position, straight-down look,
    # exact 360, tiny negative yaw
    cases.append((150, 5400, 0, *ppos, *ppos,
                  0.0, 0.0, 0.0, 0.0166667, 0, 0.0, 0.0, 0.0))
    cases.append((0, 0, 0, 0.0, 10000.0, 0.0, 0.0, 0.0, 0.0,
                  0.0, 0.0, 0.0, 0.0166667, 0, 0.0, 0.0, 0.0))
    cases.append((150, 5400, 0, *rec_pos, *ppos,
                  360.0, 0.0, 0.0, 0.05, 0, 0.0, 0.0, 0.0))
    cases.append((150, 5400, 0, *rec_pos, *ppos,
                  -0.5, 0.0, 0.0, 0.05, 0, 0.0, 0.0, 0.0))
    return cases


def build_dumper(tmp: Path) -> Path:
    binp = tmp / "c3dplayer_walkcam_dump"
    cmd = [
        "cc", "-O0",
        str(HERE / "c3dplayer_walkcam_dump.c"),
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
        r = subprocess.run([str(binp)], input=feed, capture_output=True,
                           text=True)
        if r.returncode != 0:
            print("MISMATCH: dumper run failed\n" + r.stderr)
            return 1

    got_lines = [ln for ln in r.stdout.splitlines() if ln.startswith("P ")]
    if len(got_lines) != len(cases):
        print(f"MISMATCH: expected {len(cases)} result lines, "
              f"got {len(got_lines)}")
        return 1

    checked = 0
    max_ang = 0
    for case, line in zip(cases, got_lines):
        ax, ay, az = case[0:3]
        rp = case[3:6]
        pp = case[6:9]
        yaw, roll, lead, dt = case[9:13]
        probe = case[13]
        hit = case[14:17]
        exp_pos, exp_ang = reference_walkcam(rp, (ax, ay, az), pp,
                                             yaw, roll, lead, dt, probe, hit)
        parts = line.split()
        got_pos = tuple(bits_f32(int(x, 16)) for x in parts[1:4])
        got_ang = tuple(int(x) for x in parts[5:8])

        pos_diffs = [abs(a - b) for a, b in zip(got_pos, exp_pos)]
        ang_diffs = [abs(i16(a - b)) for a, b in zip(got_ang, exp_ang)]
        max_ang = max(max_ang, *ang_diffs)
        if any(d > EPS for d in pos_diffs) or any(d > ANG_TOL
                                                  for d in ang_diffs):
            print(f"MISMATCH in walkcam case rec=({ax},{ay},{az})@{rp} "
                  f"player={pp} yaw={yaw} roll={roll} lead={lead} dt={dt} "
                  f"probe={probe}:")
            print(f"  native   pos={tuple(f'{x:.6f}' for x in got_pos)} "
                  f"ang={got_ang}")
            print(f"  reference pos={tuple(f'{x:.6f}' for x in exp_pos)} "
                  f"ang={exp_ang}")
            return 1
        checked += 1

    print(f"PASS C3DPlayer/walking-camera-record: camera_record_walkcam_write "
          f"matches the recovered UpdateWalkingCameraB record write (00439900) "
          f"on {checked} synthetic cases covering both yaw-quantization paths "
          f"(plain 00472980 ftol+mask vs the 0043a5d0 negative/360 folds), "
          f"nonzero roll, turn leads, 14-bit record-angle octants (incl. "
          f"wrap-negative shorts), shipped-level position magnitudes, the "
          f"ProbePlayerRayBlend 75%-blend/1.5x path, and zero-direction "
          f"degenerates; axis scales (1.2, 2.0, 1.2), the OMedia angles() "
          f"snap-from-snapshot write, and the native Z mirror verified "
          f"(max angle deviation {max_ang} table units); no authored data "
          f"table feeds this function (runtime-state-only inputs)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
