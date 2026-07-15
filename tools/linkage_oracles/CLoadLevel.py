#!/usr/bin/env python3
"""Linkage oracle: CLoadLevel `.gam` deserialization (general record parser).

Proves (L3) that the native C loader (src/engine/assets/gam_loader.c::gam_load,
via its gam_str/gam_prop_f/gam_prop_i accessors) decodes the shipped `.gam`
binary format identically to the reference parser tools/gam_parser.py::parse_gam,
using **every real shipped level** (assets/gam/*.gam, 35 files, ~3299 object
instances, tracked in git -- no proprietary binary needed).

Two levels of evidence:
  1. Object framing (E rows): (type, ObjectTag) for *every* object in *every*
     file, in on-disk order. A single record-length or checksum-skip bug
     anywhere upstream would desync this and start mismatching immediately --
     this is what actually stresses the generic pstring/type-1/3/6-value/
     checksum record walk, not just CLoadLevel's own fields.
  2. CLoadLevel's registered property set (L rows): for every real `LOAD`
     object (97 across the corpus), the 9 properties in the field map of
     docs/decomp/CLoadLevel.md -- LevelName, StartPoint, RequiredTask,
     RequiredLevel, ExactLevel, Radius, SoundIndex, FadeType, FadeTime --
     compared byte-exact (strings, ints) or bit-exact (floats, via the raw
     IEEE-754 pattern each side decoded from the same 4 source bytes).

See docs/linked_parity_plan.md for the Linkage Certificate. Exit 0 + PASS on
faithful reproduction; non-zero + a diff on any mismatch.
"""
from __future__ import annotations

import glob
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(REPO / "tools"))
import gam_parser  # noqa: E402  (reference parser)
from asset_paths import gam_root  # noqa: E402

GAM_DIR = gam_root()

# Comparison-harness defaults for properties a given LOAD row didn't author
# (2/97 rows omit ExactLevel, 4/97 omit FadeType+FadeTime). Applied identically
# on both sides below -- the specific constant doesn't matter for the diff,
# only that both sides pick the same one.
DEF_INT = -1
DEF_FLOAT = 0.0
DEF_STR = "none"


def f32_bits_from_value(x: float) -> int:
    return struct.unpack("<I", struct.pack("<f", float(x)))[0]


def build_dumper(tmp: Path) -> Path:
    binp = tmp / "gamload_dump"
    assets = REPO / "src" / "engine" / "assets"
    cmd = [
        "cc", "-O0",
        str(HERE / "gamload_dump.c"),
        str(assets / "gam_loader.c"),
        str(REPO / "src" / "engine" / "player_physics.c"),
        "-o", str(binp),
    ]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print("MISMATCH: dumper failed to compile\n" + r.stderr)
        sys.exit(1)
    return binp


def run_dumper(dumper: Path, gam_path: Path) -> tuple[list[tuple], dict[int, tuple]]:
    r = subprocess.run([str(dumper), str(gam_path)], capture_output=True, text=True)
    if r.returncode != 0:
        print(f"MISMATCH: dumper failed on {gam_path}\n{r.stderr}")
        sys.exit(1)
    e_rows: list[tuple] = []
    l_rows: dict[int, tuple] = {}
    for line in r.stdout.splitlines():
        parts = line.split("|")
        if parts[0] == "E":
            idx, typ, tag = int(parts[1]), parts[2], parts[3]
            e_rows.append((idx, typ, tag))
        elif parts[0] == "L":
            idx = int(parts[1])
            l_rows[idx] = (
                parts[2],              # LevelName
                parts[3],              # StartPoint
                parts[4],              # RequiredTask
                int(parts[5]),         # RequiredLevel
                int(parts[6]),         # ExactLevel
                int(parts[7], 16),     # Radius bits
                int(parts[8]),         # SoundIndex
                int(parts[9]),         # FadeType
                int(parts[10], 16),    # FadeTime bits
            )
    return e_rows, l_rows


def reference_rows(gam_path: Path) -> tuple[list[tuple], dict[int, tuple]]:
    d = gam_parser.parse_gam(str(gam_path))
    e_rows = []
    l_rows = {}
    for idx, obj in enumerate(d["objects"]):
        p = obj["properties"]
        tag = str(p.get("ObjectTag", ""))
        e_rows.append((idx, obj["type"], tag))
        if obj["type"] == "LOAD":
            l_rows[idx] = (
                str(p.get("LevelName", "")),
                str(p.get("StartPoint", "")),
                str(p.get("RequiredTask", DEF_STR)),
                int(p.get("RequiredLevel", DEF_INT)),
                int(p.get("ExactLevel", DEF_INT)),
                f32_bits_from_value(p.get("Radius", DEF_FLOAT)),
                int(p.get("SoundIndex", DEF_INT)),
                int(p.get("FadeType", DEF_INT)),
                f32_bits_from_value(p.get("FadeTime", DEF_FLOAT)),
            )
    return e_rows, l_rows


def main() -> int:
    gam_files = sorted(GAM_DIR.glob("*.gam"))
    if not gam_files:
        print("MISMATCH: no .gam files found under assets/gam/")
        return 1

    with tempfile.TemporaryDirectory() as d:
        dumper = build_dumper(Path(d))

        total_objects = 0
        total_load = 0
        for gam_path in gam_files:
            c_e, c_l = run_dumper(dumper, gam_path)
            py_e, py_l = reference_rows(gam_path)

            if c_e != py_e:
                print(f"MISMATCH in {gam_path.name}: object framing (type/tag) diverged")
                for a, b in zip(c_e, py_e):
                    if a != b:
                        print(f"  native={a} reference={b}")
                        break
                return 1

            if c_l != py_l:
                print(f"MISMATCH in {gam_path.name}: LOAD property set diverged")
                for idx in sorted(set(c_l) | set(py_l)):
                    if c_l.get(idx) != py_l.get(idx):
                        print(f"  idx={idx} native={c_l.get(idx)} reference={py_l.get(idx)}")
                return 1

            total_objects += len(py_e)
            total_load += len(py_l)

    print(
        f"PASS CLoadLevel: gam_load == gam_parser.parse_gam byte-exact on "
        f"{len(gam_files)} shipped .gam files ({total_objects} objects, "
        f"{total_load} LOAD rows -- all 9 docs/decomp/CLoadLevel.md properties)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
