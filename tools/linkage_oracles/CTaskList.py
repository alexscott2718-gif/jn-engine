#!/usr/bin/env python3
"""Linkage oracle: CTaskList `.tsk` deserialization.

Proves (L3) that the native C port `src/game/task_loader.c::task_parse_file`
decodes `.tsk` streams **byte-identically** to the reference parser
`tools/tsk_parser.py`, across the documented NewGame table and the format's edge
cases (stray non-zero byte in the pre-table zero gap, short zero tail, min count,
long tag). It also checks the NewGame case against the golden table transcribed
from `docs/decomp/CTaskList.md` (the L1/L4 anchor -- values trace to the doc, not
to tuning).

No shipped `.tsk` binary is needed (they are proprietary and uncommitted): the
oracle synthesizes streams from the measured LV1B format, so it is fully static
and headless. See `docs/linked_parity_plan.md` for the Linkage Certificate.

Exit 0 + a `PASS` line on faithful reproduction; non-zero + a diff on any mismatch.
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
import tsk_parser  # noqa: E402  (reference parser)

# ---- documented NewGame ground truth (docs/decomp/CTaskList.md) -------------
NEWGAME_GAM = "level1b.gam"
NEWGAME_SPAWN = (470.5965, 609.2417, -87.7631)
NEWGAME_ENTITIES = [
    ("MOM", 0), ("LIBBY", 0), ("BENNY", 0), ("SCENE", 30),
    ("DINO", 0), ("CLONE", 0), ("KITTY1", 0), ("KITTY2", 0),
    ("KITTY3", 0), ("HYDRANT1", 0), ("HYDRANT2", 0), ("REACTOR", 0),
]


# ---- synthetic LV1B `.tsk` builder ------------------------------------------
def pstr(s: str) -> bytes:
    b = s.encode("ascii")
    assert len(b) < 256
    return bytes([len(b)]) + b


def build_tsk(gam: str, spawn, entities, pre_zero: int = 0x20,
              gap: bytes = b"\x00" * 8, tail: bytes = b"") -> bytes:
    out = b"LV1B" + bytes([0x41, 0xA0])
    out += b"\x00" * pre_zero
    out += pstr("STARTEXP") + pstr("none") + pstr(gam) + pstr("none")
    out += struct.pack(">3f", *spawn)
    out += gap                       # variable-length (mostly zero) gap
    out += bytes([len(entities)])    # u8 count
    for tag, state in entities:
        out += pstr(tag) + struct.pack(">I", state)
    out += tail                      # tolerated short zero tail
    return out


# ---- canonical dumps (must match ctasklist_dump.c line for line) ------------
def f32_bits(x: float) -> str:
    # native-endian (LE) bit pattern of the float, matching C `memcpy` + %08x
    return f"{struct.unpack('<I', struct.pack('<f', x))[0]:08x}"


def canonical_from_py(path: str) -> str:
    tsk = tsk_parser.parse_tsk(path)
    si = tsk.start_info
    lines = [
        f"name|{si.task_name if si else ''}",
        f"gam|{si.gam_file if si else ''}",
        f"start|{1 if si else 0}",
        f"sx|{f32_bits(si.start_x if si else 0.0)}",
        f"sy|{f32_bits(si.start_y if si else 0.0)}",
        f"sz|{f32_bits(si.start_z if si else 0.0)}",
        f"n|{len(tsk.entities)}",
    ]
    for e in tsk.entities:
        lines.append(f"e|{e.name}|{e.state}")
    lines.append("valid|1")
    return "\n".join(lines) + "\n"


def golden_newgame() -> str:
    lines = [
        "name|STARTEXP",
        f"gam|{NEWGAME_GAM}",
        "start|1",
        f"sx|{f32_bits(NEWGAME_SPAWN[0])}",
        f"sy|{f32_bits(NEWGAME_SPAWN[1])}",
        f"sz|{f32_bits(NEWGAME_SPAWN[2])}",
        f"n|{len(NEWGAME_ENTITIES)}",
    ]
    for tag, state in NEWGAME_ENTITIES:
        lines.append(f"e|{tag}|{state}")
    lines.append("valid|1")
    return "\n".join(lines) + "\n"


def build_dumper(tmp: Path) -> Path:
    binp = tmp / "ctasklist_dump"
    src_game = REPO / "src" / "game"
    cmd = [
        "cc", "-O0", "-I", str(src_game),
        str(HERE / "ctasklist_dump.c"), str(src_game / "task_loader.c"),
        "-o", str(binp),
    ]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print("MISMATCH: dumper failed to compile\n" + r.stderr)
        sys.exit(1)
    return binp


def main() -> int:
    cases = {
        "newgame": build_tsk(
            NEWGAME_GAM, NEWGAME_SPAWN, NEWGAME_ENTITIES,
            gap=b"\x00\x00\x00\x00\x01\x00\x00\x00",   # documented lone 0x01 in the gap
        ),
        "stray_gap": build_tsk(
            "level2.gam", (1.0, 2.0, 3.0), [("REACTOR", 7)],
            gap=b"\x00\x05\x00\x00",                   # stray non-zero must be skipped (EOF anchor)
        ),
        "min": build_tsk("l.gam", (0.0, 0.0, 0.0), [("X", 1)], gap=b""),
        "zero_tail": build_tsk(
            "level3.gam", (-5.5, 10.25, 0.0), [("MOM", 0), ("SCENE", 42)],
            tail=b"\x00\x00\x00",                      # tolerated short zero tail
        ),
        "long_tag": build_tsk(
            "level4.gam", (9.0, 9.0, 9.0), [("A" * 31, 123456)],
        ),
    }

    with tempfile.TemporaryDirectory() as d:
        tmp = Path(d)
        dumper = build_dumper(tmp)
        checked = 0
        for name, blob in cases.items():
            f = tmp / f"{name}.tsk"
            f.write_bytes(blob)
            r = subprocess.run([str(dumper), str(f)], capture_output=True, text=True)
            c_out = r.stdout
            py_out = canonical_from_py(str(f))
            if c_out != py_out:
                print(f"MISMATCH in case {name!r}: C port != tsk_parser.py")
                print("--- C (task_parse_file) ---\n" + c_out)
                print("--- Python (parse_tsk) ---\n" + py_out)
                return 1
            if name == "newgame" and py_out != golden_newgame():
                print("MISMATCH: newgame parse != docs/decomp/CTaskList.md golden")
                print("--- parsed ---\n" + py_out)
                print("--- golden ---\n" + golden_newgame())
                return 1
            checked += 1

    print(
        f"PASS CTaskList: task_parse_file == tsk_parser.py byte-exact on {checked} "
        f"synthesized .tsk streams; NewGame table matches CTaskList.md (SCENE=30)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
