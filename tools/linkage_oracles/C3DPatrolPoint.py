#!/usr/bin/env python3
"""Linkage oracle: C3DPatrolPoint `NextPatrolPoint` graph-edge resolution.

Proves (L3) that the native `behavior_ai_find_patrol_point`
(src/game/behaviors/behavior_ai.c), as used by `behavior_ai_update_patrol`'s
arrival routing, reproduces the "next-select" half of the decompiled
`C3DPatrolPoint::OnArrive` (Neutron.exe @ `00434ea0`,
docs/decomp/C3DPatrolPoint.md: "`other.next_target = lookup(NextPatrolPoint)`")
exactly, over **every real shipped `3PAT` waypoint** (742 instances across the
35 levels, `assets/gam/*.gam`, tracked in git):

  - `NextPatrolPoint`'s case-insensitive same-level tag resolution: does the
    authored edge resolve to the correct placed `3PAT` entity (or to nothing,
    if the tag is absent/dangling)?
  - `WaitTime` read-through (`gam_prop_f`), byte-exact (IEEE-754 bit pattern).

Compiles and runs the real, unmodified `gam_load()` +
`behavior_ai_find_patrol_point()`/`gam_prop_f()` over each real `.gam` file
(`c3dpatrolpoint_dump.c`) and diffs the result against an independently-built
Python graph from `tools/gam_parser.py`.

Deliberately OUT OF SCOPE (see "Not covered" in
docs/decomp/C3DPatrolPoint.md's Native Linkage section): the arrival
*trigger* mechanism itself (native polls toward the waypoint each frame and
declares arrival by distance; the decompiled body is a collision-volume
"on-enter" callback -- a different architecture, not provably equivalent
without a captured trace), the arrival radius/speed constants (native
defaults, not decompiled from Neutron.exe), the facing/heading formula during
seek (not decompiled either), and `SoundDatabase`/`SoundIndex`/`CallObjectTag`/
`ActivateAnim`/`WaitAnim` dispatch (not ported at all -- see
`behavior_walker.c`'s own comment).

See docs/linked_parity_plan.md for the Linkage Certificate. Exit 0 + PASS on
faithful reproduction; non-zero + a diff on any mismatch.
"""
from __future__ import annotations

import struct
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(REPO / "tools"))
import gam_parser  # noqa: E402  (reference parser, reused for real 3PAT data)
from asset_paths import gam_root  # noqa: E402

GAM_DIR = gam_root()


def f32_bits(x: float) -> int:
    return struct.unpack("<I", struct.pack("<f", float(x)))[0]


def build_dumper(tmp: Path) -> Path:
    binp = tmp / "c3dpat_dump"
    cmd = [
        "cc", "-O0",
        str(HERE / "c3dpatrolpoint_dump.c"),
        str(REPO / "src" / "game" / "behaviors" / "behavior_ai.c"),
        str(REPO / "src" / "engine" / "assets" / "gam_loader.c"),
        str(REPO / "src" / "engine" / "player_physics.c"),
        "-lm", "-o", str(binp),
    ]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print("MISMATCH: dumper failed to compile\n" + r.stderr)
        sys.exit(1)
    return binp


def run_dumper(dumper: Path, gam_path: Path) -> dict[str, tuple]:
    r = subprocess.run([str(dumper), str(gam_path)], capture_output=True, text=True)
    if r.returncode != 0:
        print(f"MISMATCH: dumper failed on {gam_path}\n{r.stderr}")
        sys.exit(1)
    out = {}
    for line in r.stdout.splitlines():
        if not line.startswith("P|"):
            continue
        _, idx, tag, next_tag, resolved, wait_bits = line.split("|")
        out[(idx, tag)] = (next_tag, resolved, int(wait_bits, 16))
    return out


def reference_rows(gam_path: Path) -> dict[str, tuple]:
    d = gam_parser.parse_gam(str(gam_path))
    # Case-insensitive tag -> object, restricted to 3PAT rows in this file
    # (the "same-level" scope behavior_ai_find_patrol_point actually walks).
    by_tag_ci = {}
    for obj in d["objects"]:
        if obj["type"] != "3PAT":
            continue
        tag = str(obj["properties"].get("ObjectTag", ""))
        by_tag_ci[tag.casefold()] = tag

    out = {}
    for idx, obj in enumerate(d["objects"]):
        if obj["type"] != "3PAT":
            continue
        p = obj["properties"]
        tag = str(p.get("ObjectTag", ""))
        next_tag = str(p.get("NextPatrolPoint", ""))
        if next_tag.strip().lower() in ("", "none"):
            resolved = ""
        else:
            resolved = by_tag_ci.get(next_tag.casefold(), "")
        wait = float(p.get("WaitTime", 0.0))
        out[(str(idx), tag)] = (next_tag if next_tag.strip().lower() != "none" else "",
                                 resolved, f32_bits(wait))
    return out


def main() -> int:
    gam_files = sorted(GAM_DIR.glob("*.gam"))
    if not gam_files:
        print("MISMATCH: no .gam files found under assets/gam/")
        return 1

    with tempfile.TemporaryDirectory() as d:
        dumper = build_dumper(Path(d))

        total_waypoints = 0
        total_resolved_edges = 0
        for gam_path in gam_files:
            native = run_dumper(dumper, gam_path)
            reference = reference_rows(gam_path)

            if native != reference:
                print(f"MISMATCH in {gam_path.name}: 3PAT resolution diverged")
                for key in sorted(set(native) | set(reference)):
                    if native.get(key) != reference.get(key):
                        print(f"  {key}: native={native.get(key)} reference={reference.get(key)}")
                return 1

            total_waypoints += len(reference)
            total_resolved_edges += sum(1 for v in reference.values() if v[1])

        if total_waypoints != 742:
            print(f"MISMATCH: expected 742 real 3PAT waypoints, found {total_waypoints} "
                  f"-- corpus changed, re-verify this oracle's coverage claim")
            return 1

    print(
        f"PASS C3DPatrolPoint/on-arrive: behavior_ai_find_patrol_point + gam_prop_f "
        f"reproduce NextPatrolPoint resolution and WaitTime byte-exact across all "
        f"{total_waypoints} shipped 3PAT waypoints in {len(gam_files)} levels "
        f"({total_resolved_edges} edges resolve to a real neighbor)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
