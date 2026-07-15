#!/usr/bin/env python3
"""Linkage oracle: C3DStartPoint `spawn` (PlacePlayer STRT resolution).

Proves (L3) that the native `place_player` (src/game/spawn.c -- extracted
verbatim from main.c on 2026-07-02 precisely to unblock this row; see
docs/decomp/C3DStartPoint.md "Native Linkage") reproduces the deterministic,
headless observables of the decompiled `C3DStartPoint::PlacePlayer`
(Neutron.exe @ 00442740):

  1. Start-point resolution: the requested tag (or, for an empty request, the
     player's own authored `StartPoint`) matched **case-insensitively**
     against the placed `STRT` entities.
  2. "place the player at this transform": the player's position AND rotation
     become the matched STRT's stored world transform, byte-exact.
  3. `MusicDatabase`/`MusicIndex` selection from the matched start point
     (`gam_prop_i(e, "MusicIndex", -1)`).
  4. A miss (unknown tag) must NOT teleport and must NOT select music.

Coverage: **every real shipped `STRT` row** (100 across the 35
`assets/gam/*.gam` levels, tracked in git), each requested with its
exact-case tag and a case-varied tag, plus per level one `@default` request
(the player's own authored `StartPoint` -- all 35 `3JIM` rows author one) and
one guaranteed-miss tag. Expectations are computed independently from
`tools/gam_parser.py`; the loader's import convention (`z = -PositionZ`,
degrees->radians rotations) is recomputed from the raw parsed floats and
cross-checked on the dumped STRT table, so a convention drift also goes red.
`JN_DEMO_SPAWN_XYZ` is stripped from the dumper's environment (its
preserve-position branch is a native-only demo convenience, out of certified
scope, same spirit as `task_load`'s baked-default fallback exclusion).

Also asserted, as a native regression guard (NOT a certified decomp
equivalence -- the decompiled body only shows `C3DSprite::Reset()` there):
the velocity-zero + airborne reset on return.

Deliberately OUT OF SCOPE (see "Not covered" in docs/decomp/C3DStartPoint.md's
Native Linkage section): the `ViewportP*`/`ViewportR*` initial camera pose and
`StartTrigger` one-shot -- both unported gaps, unchanged from the
linked-blocked investigation. Also out of scope: the `gam_prop_i`
MusicIndex fallback default (`-1`) -- no shipped STRT omits MusicIndex and no
decompiled default is recovered, so it is unreachable by real data and
uncertified.

See docs/linked_parity_plan.md for the Linkage Certificate. Exit 0 + PASS on
faithful reproduction; non-zero + a diff on any mismatch.
"""
from __future__ import annotations

import os
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(REPO / "tools"))
import gam_parser  # noqa: E402  (reference parser, reused for real STRT data)
from asset_paths import gam_root  # noqa: E402

# gam_parser rounds floats to 6 decimals at parse time (round(..., 6) -- fine
# for its human-facing catalogs, but this oracle diffs IEEE-754 bit patterns).
# `round` is looked up in the module's globals before builtins, so shadowing it
# on the module restores the exact unpacked f32 without touching the parser.
gam_parser.round = lambda v, ndigits=None: v

GAM_DIR = gam_root()
MISS_TAG = "zz_no_such_start_zz"


def f32(x: float) -> float:
    return struct.unpack("<f", struct.pack("<f", float(x)))[0]


def bits(x: float) -> int:
    return struct.unpack("<I", struct.pack("<f", float(x)))[0]


DEG2RAD = f32(0.017453293)  # gam_loader.c's exact float constant


def pose_bits(p: dict) -> tuple[int, ...]:
    """Entity pose bit patterns exactly as gam_loader.c stores them
    (z-flip, degrees->radians); an unauthored field stays calloc-zero."""
    def g(name: str, flip: bool = False, rad: bool = False) -> int:
        if name not in p:
            return 0
        v = f32(p[name])
        if flip:
            v = -v
        if rad:
            v = v * DEG2RAD  # f64 product of two f32s is exact; bits() rounds once
        return bits(v)
    return (
        g("PositionX"), g("PositionY"), g("PositionZ", flip=True),
        g("RotationX", rad=True), g("RotationY", rad=True),
        g("RotationZ", flip=True, rad=True),
    )


def build_dumper(tmp: Path) -> Path:
    binp = tmp / "c3dstrt_dump"
    cmd = [
        "cc", "-O0",
        str(HERE / "c3dstartpoint_dump.c"),
        str(REPO / "src" / "game" / "spawn.c"),
        str(REPO / "src" / "engine" / "assets" / "gam_loader.c"),
        str(REPO / "src" / "engine" / "player_physics.c"),
        "-lm", "-o", str(binp),
    ]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print("MISMATCH: dumper failed to compile\n" + r.stderr)
        sys.exit(1)
    return binp


def reference_file(gam_path: Path):
    d = gam_parser.parse_gam(str(gam_path))
    objs = d["objects"]
    strt_file_order = []
    for o in objs:
        if o["type"] != "STRT":
            continue
        p = o["properties"]
        strt_file_order.append((
            str(p.get("ObjectTag", "")),
            pose_bits(p),
            str(p.get("MusicDatabase", "")),
            int(p.get("MusicIndex", -1)),
        ))
    jim = None
    for o in reversed(objs):  # world list is push-front: last-loaded seen first
        if o["type"] == "3JIM":
            jim = o
            break
    return strt_file_order, jim


def main() -> int:
    gam_files = sorted(GAM_DIR.glob("*.gam"))
    if not gam_files:
        print("MISMATCH: no .gam files found under assets/gam/")
        return 1

    env = {k: v for k, v in os.environ.items() if k != "JN_DEMO_SPAWN_XYZ"}

    with tempfile.TemporaryDirectory() as tdir:
        dumper = build_dumper(Path(tdir))

        total_strt = 0
        total_requests = 0
        for gam_path in gam_files:
            strt_rows, jim = reference_file(gam_path)
            if jim is None:
                print(f"MISMATCH: {gam_path.name} has no 3JIM -- corpus changed, "
                      f"re-verify this oracle's coverage claim")
                return 1
            jp = jim["properties"]
            jim_sp = str(jp.get("StartPoint", ""))
            jim_pose = pose_bits(jp)

            # Requests: default, guaranteed miss, every real tag, case-varied tag.
            requests = ["@default", MISS_TAG]
            for tag, _, _, _ in strt_rows:
                requests.append(tag)
                if tag.swapcase() != tag:
                    requests.append(tag.swapcase())

            r = subprocess.run([str(dumper), str(gam_path), *requests],
                               capture_output=True, text=True, env=env)
            if r.returncode != 0:
                print(f"MISMATCH: dumper failed on {gam_path.name}\n{r.stderr}")
                return 1

            got_t, got_r = [], {}
            got_j = None
            for line in r.stdout.splitlines():
                parts = line.split("|")
                if parts[0] == "T":
                    got_t.append((parts[1], tuple(int(x, 16) for x in parts[2:8]),
                                  parts[8], int(parts[9])))
                elif parts[0] == "J":
                    got_j = parts[1:]
                elif parts[0] == "R":
                    got_r[int(parts[1])] = parts[2:]

            # 1. STRT table exactly as the real loader stored it (import
            #    convention cross-check: z-flip, deg->rad, MusicIndex read).
            if got_t != strt_rows:
                print(f"MISMATCH in {gam_path.name}: loader STRT table diverged")
                for a, b in zip(got_t, strt_rows):
                    if a != b:
                        print(f"  native={a}\n  reference={b}")
                        break
                return 1

            # 2. Player as loaded (StartPoint request + pose baseline).
            if got_j is None or got_j[0] == "none":
                print(f"MISMATCH in {gam_path.name}: dumper found no 3JIM")
                return 1
            if got_j[1] != jim_sp or tuple(int(x, 16) for x in got_j[2:8]) != jim_pose:
                print(f"MISMATCH in {gam_path.name}: 3JIM baseline diverged")
                print(f"  native   sp={got_j[1]!r} pose={got_j[2:8]}")
                print(f"  reference sp={jim_sp!r} pose={tuple(f'{b:08x}' for b in jim_pose)}")
                return 1

            # 3. Every place_player request.
            rev = list(reversed(strt_rows))  # native world-list scan order

            def resolve(want: str):
                if not want:
                    return None
                for row in rev:
                    if row[0].casefold() == want.casefold():
                        return row
                return None

            for idx, req in enumerate(requests):
                want = jim_sp if req == "@default" else req
                sel = resolve(want)
                exp_pose = sel[1] if sel else jim_pose
                exp_music = (1, sel[2], sel[3]) if sel and sel[2] else (0, "", -999)
                expected = (
                    "1",                                   # returned the player
                    tuple(f"{b:08x}" for b in exp_pose),   # pose
                    ("00000000",) * 3,                     # velocity reset
                    "0",                                   # airborne
                    str(exp_music[0]), exp_music[1], str(exp_music[2]),
                )
                g = got_r.get(idx)
                if g is None:
                    print(f"MISMATCH in {gam_path.name}: no result for request {req!r}")
                    return 1
                got = (g[1], tuple(g[2:8]), tuple(g[8:11]), g[11], g[12], g[13], g[14])
                if got != expected:
                    print(f"MISMATCH in {gam_path.name} request {req!r} "
                          f"(want={want!r}, resolved={'yes' if sel else 'no'}):")
                    print(f"  native   ={got}")
                    print(f"  reference={expected}")
                    return 1
                total_requests += 1

            total_strt += len(strt_rows)

        if total_strt != 100:
            print(f"MISMATCH: expected 100 real STRT rows, found {total_strt} "
                  f"-- corpus changed, re-verify this oracle's coverage claim")
            return 1

    print(
        f"PASS C3DStartPoint/spawn: place_player reproduces PlacePlayer (00442740) "
        f"STRT resolution (case-insensitive), teleport-to-transform, and "
        f"MusicDatabase/MusicIndex selection byte-exact across all {total_strt} "
        f"shipped STRT rows in {len(gam_files)} levels ({total_requests} requests "
        f"incl. case-varied, @default and miss cases)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
