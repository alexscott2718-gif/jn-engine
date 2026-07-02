#!/usr/bin/env python3
"""Linkage oracle: C3DCutSceneCamera (3CAM) distance formula + mode precedence.

Proves (L3) that the native `cutscene_3cam_dist` / `cutscene_3cam_place`
(src/game/behaviors/behavior_cutscene.c) reproduce two fully-decompiled pieces
of C3DCutSceneCamera's per-frame update (Neutron.exe @ 00415f90, recovered
2026-06-25 -- see "Per-frame camera update" in docs/decomp/C3DCutSceneCamera.md)
exactly, over **every real shipped `3CAM` row** (assets/gam/*.gam, 136 rows,
tracked in git):

  1. The distance/zoom formula:
         dist = InitialDist - ZoomSpeed * t
         dist = max(dist, MinDist)      # floor first
         dist = min(dist, MaxDist)      # ceiling second
     Real data exercises the floor-then-ceiling clamp *order* directly: two
     shipped rows (Level7.gam "podcam", level4c.gam "rescue") author
     MinDist > MaxDist, where floor-then-ceiling and ceiling-then-floor give
     different answers -- the oracle doesn't need a synthesized case for this.

  2. `CameraType==2` (static) precedence over `ViewFromCamera`: the decompiled
     body tests `CameraType==2` *before* `ViewFromCamera`, so a CT==2 shot
     must produce the static formula (cam := the 3CAM's own placement,
     look := the target's raw position) regardless of its authored VFC. Real
     data makes this non-vacuous: of the 16 real CT==2 rows, 14 author
     VFC==1 (the "not 0" -> dolly value) and 2 author VFC==0 (orbit) -- if
     the native code checked VFC first, those 16 rows would take the wrong
     branch and the byte-exact assertion below would fail.

Deliberately OUT OF SCOPE (see "Not covered" in docs/decomp/C3DCutSceneCamera.md's
Native Linkage section): the exact 3D camera position for the **orbit/dolly**
branches. Both depend on `entity_local_to_world`, a native-only **yaw-only**
approximation of the target's still-undecompiled `transform_local` (vtable
`+0x384` on the target object -- attempted to recover it 2026-07-02 via
`tools/ghidra/DumpClass.java` (C3DGoddard, as a representative CameraTarget
class) + `tools/ghidra/DumpFunctions.java` on the resolved vtable slot address
(`00472980`); Ghidra has no Function defined there in the committed project
("function not found"), so a byte-exact orbit/dolly position oracle is not
buildable this pass without a fresh disassembly/function-creation effort. That
work is out of scope here; certifying only what IS fully decompiled (the two
pieces above) keeps this row honest per L1/L4 rather than baking the
unverified approximation into the oracle as if it were ground truth.

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
import gam_parser  # noqa: E402  (reference parser, reused for real 3CAM data)

GAM_DIR = REPO / "assets" / "gam"

# Doc-cited distributions (docs/decomp/C3DCutSceneCamera.md "Per-frame camera
# update"), reproduced here as an independent regression check against the
# live corpus.
EXPECT_VFC = {1: 121, 0: 9, 3: 6}
EXPECT_CT = {0: 95, 2: 16, 3: 15, 1: 10}

T_SAMPLES = (0.0, 1.0, 5.0, 20.0)  # sweeps below-floor / mid / above-ceiling


def f32_bits(x: float) -> int:
    return struct.unpack("<I", struct.pack("<f", float(x)))[0]


def py_dist(initial_dist: float, zoom_speed: float, min_dist: float, max_dist: float, t: float) -> float:
    d = initial_dist - zoom_speed * t
    d = max(d, min_dist)   # floor first
    d = min(d, max_dist)   # ceiling second (matches the doc's stated order)
    return d


def load_3cam_rows() -> list[dict]:
    rows = []
    for f in sorted(GAM_DIR.glob("*.gam")):
        d = gam_parser.parse_gam(str(f))
        for obj in d["objects"]:
            if obj["type"] != "3CAM":
                continue
            p = obj["properties"]
            rows.append({
                "file": f.name,
                "tag": p.get("ObjectTag", ""),
                "initial_dist": float(p.get("InitialDist", 0.0)),
                "zoom_speed": float(p.get("ZoomSpeed", 0.0)),
                "min_dist": float(p.get("MinDist", 0.0)),
                "max_dist": float(p.get("MaxDist", 0.0)),
                "camera_type": int(p.get("CameraType", -999)),
                "view_from_camera": int(p.get("ViewFromCamera", -999)),
                "pos": (float(p.get("PositionX", 0.0)), float(p.get("PositionY", 0.0)), float(p.get("PositionZ", 0.0))),
            })
    return rows


def build_dumper(tmp: Path) -> Path:
    binp = tmp / "c3dcam_dump"
    cmd = [
        "cc", "-O0",
        str(HERE / "c3dcutscenecamera_dump.c"),
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
    rows = load_3cam_rows()
    if len(rows) != 136:
        print(f"MISMATCH: expected 136 real 3CAM rows, found {len(rows)} "
              f"-- corpus changed, re-derive EXPECT_VFC/EXPECT_CT before trusting this oracle")
        return 1

    vfc_counts = Counter(r["view_from_camera"] for r in rows)
    ct_counts = Counter(r["camera_type"] for r in rows)
    if dict(vfc_counts) != EXPECT_VFC:
        print(f"MISMATCH: ViewFromCamera distribution diverged from the doc: {dict(vfc_counts)} != {EXPECT_VFC}")
        return 1
    if dict(ct_counts) != EXPECT_CT:
        print(f"MISMATCH: CameraType distribution diverged from the doc: {dict(ct_counts)} != {EXPECT_CT}")
        return 1

    ct2_rows = [r for r in rows if r["camera_type"] == 2]
    ct2_vfc = Counter(r["view_from_camera"] for r in ct2_rows)
    if ct2_vfc.get(0, 0) == 0 or (ct2_vfc.get(1, 0) + ct2_vfc.get(3, 0)) == 0:
        print("MISMATCH: real CT==2 rows no longer span multiple VFC values -- "
              "the precedence test would be vacuous")
        return 1

    with tempfile.TemporaryDirectory() as d:
        dumper = build_dumper(Path(d))

        # ---- 1. distance formula over all 136 rows x 4 t samples ----------
        dist_lines = []
        dist_expected = {}
        idx = 0
        for r in rows:
            for t in T_SAMPLES:
                key = f"{r['file']}:{r['tag']}:t={t}"
                dist_lines.append(
                    f"D|{idx}|{f32_bits(r['initial_dist']):08x}|{f32_bits(r['zoom_speed']):08x}|"
                    f"{f32_bits(r['min_dist']):08x}|{f32_bits(r['max_dist']):08x}|{f32_bits(t):08x}"
                )
                dist_expected[idx] = (key, f32_bits(py_dist(r["initial_dist"], r["zoom_speed"],
                                                             r["min_dist"], r["max_dist"], t)))
                idx += 1

        # ---- 2. CameraType==2 static-branch precedence over all 16 rows ---
        static_lines = []
        static_expected = {}
        for i, r in enumerate(ct2_rows):
            # Synthetic but deterministic target pose -- the static branch never
            # touches transform_local/rotation, so any target values equally
            # prove the point; varying them per-row still catches a formula bug.
            tgt = (10.0 * i + 1.0, -5.0 * i + 2.0, 3.0 * i - 4.0, 0.1 * i)
            static_lines.append(
                f"S|{i}|{r['camera_type']}|{r['view_from_camera']}|"
                f"{f32_bits(r['pos'][0]):08x}|{f32_bits(r['pos'][1]):08x}|{f32_bits(r['pos'][2]):08x}|"
                f"{f32_bits(0.0):08x}|{f32_bits(0.0):08x}|{f32_bits(0.0):08x}|{f32_bits(0.0):08x}|"
                f"{f32_bits(tgt[0]):08x}|{f32_bits(tgt[1]):08x}|{f32_bits(tgt[2]):08x}|{f32_bits(tgt[3]):08x}|"
                f"{f32_bits(1.0):08x}"
            )
            static_expected[i] = (
                f"{r['file']}:{r['tag']}(vfc={r['view_from_camera']})",
                (f32_bits(r["pos"][0]), f32_bits(r["pos"][1]), f32_bits(r["pos"][2]),
                 f32_bits(tgt[0]), f32_bits(tgt[1]), f32_bits(tgt[2])),
            )

        proc = subprocess.run([str(dumper)], input="\n".join(dist_lines + static_lines) + "\n",
                               capture_output=True, text=True)
        if proc.returncode != 0:
            print(f"MISMATCH: dumper exited {proc.returncode}\n{proc.stderr}")
            return 1

        dist_checked = static_checked = 0
        for line in proc.stdout.splitlines():
            parts = line.split("|")
            if parts[0] == "D":
                i = int(parts[1])
                key, expected_bits = dist_expected[i]
                got_bits = int(parts[2], 16)
                if got_bits != expected_bits:
                    print(f"MISMATCH in dist case {key!r}: native={parts[2]} reference={expected_bits:08x}")
                    return 1
                dist_checked += 1
            elif parts[0] == "S":
                i = int(parts[1])
                key, expected = static_expected[i]
                got = tuple(int(x, 16) for x in parts[2:8])
                if got != expected:
                    print(f"MISMATCH in static-precedence case {key!r}:")
                    print(f"  native   ={tuple(f'{x:08x}' for x in got)}")
                    print(f"  reference={tuple(f'{x:08x}' for x in expected)}")
                    return 1
                static_checked += 1

        if dist_checked != len(dist_lines) or static_checked != len(static_lines):
            print(f"MISMATCH: expected {len(dist_lines)} dist + {len(static_lines)} static "
                  f"results, got {dist_checked} + {static_checked}")
            return 1

    print(
        f"PASS C3DCutSceneCamera/3cam-camera-math: cutscene_3cam_dist byte-exact "
        f"on {dist_checked} (row x t) samples across all 136 shipped 3CAM rows; "
        f"cutscene_3cam_place's CameraType==2 static-branch precedence over "
        f"ViewFromCamera byte-exact on all {static_checked} real CT==2 rows; "
        f"ViewFromCamera/CameraType distributions match docs/decomp/C3DCutSceneCamera.md"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
