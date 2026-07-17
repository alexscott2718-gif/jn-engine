#!/usr/bin/env python3
"""Linkage oracle: CGameType / InitGame camera-record seed.

L1 is the recovered CGameType::InitGame body at 00474a10 in
docs/decomp/CGameType.md.  This certificate intentionally covers only the
three DAT_00509a50 position writes:

    rec.pos = (0.0f, 10000.0f, 0.0f)

The real native path is camera_record_init_game() in
src/game/camera_record.c.  The recovered body does not write the neighboring
angle or mode fields, so sentinels prove they survive both calls.  No shipped
asset table feeds this lifecycle seed; L3 therefore uses decomp-derived state
vectors, as permitted by docs/linked_parity_plan.md.
"""
from __future__ import annotations

import struct
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
REAL_SOURCE = REPO / "src" / "game" / "camera_record.c"


def bits(value: float) -> int:
    return struct.unpack("<I", struct.pack("<f", value))[0]


def build(tmp: Path, source: Path) -> Path:
    binary = tmp / "cgametype_dump"
    cmd = [
        "cc", "-O0", "-I", str(REPO / "src" / "engine"),
        str(HERE / "cgametype_dump.c"), str(source), "-lm", "-o", str(binary),
    ]
    run = subprocess.run(cmd, capture_output=True, text=True)
    if run.returncode:
        raise RuntimeError("dumper failed to compile:\n" + run.stderr)
    return binary


def run_check(source: Path) -> tuple[bool, str]:
    with tempfile.TemporaryDirectory() as directory:
        tmp = Path(directory)
        try:
            binary = build(tmp, source)
        except RuntimeError as exc:
            return False, str(exc)
        run = subprocess.run([str(binary)], capture_output=True, text=True)
        if run.returncode:
            return False, f"dumper exited {run.returncode}: {run.stderr.strip()}"

    rows: dict[str, tuple[int, ...]] = {}
    for line in run.stdout.splitlines():
        fields = line.split("|")
        if len(fields) != 10 or fields[0] != "STATE":
            continue
        rows[fields[1]] = (
            int(fields[2], 16), int(fields[3], 16), int(fields[4], 16),
            *(int(value) for value in fields[5:]),
        )

    pre = rows.get("PRE")
    post1 = rows.get("POST1")
    post2 = rows.get("POST2")
    if pre is None or post1 is None or post2 is None:
        return False, f"missing state rows: {sorted(rows)}"

    preserved = pre[3:]
    expected = (bits(0.0), bits(10000.0), bits(0.0), *preserved)
    if post1 != expected:
        return False, f"first seed {post1} != recovered state {expected}"
    if post2 != expected:
        return False, f"repeat seed {post2} != idempotent recovered state {expected}"
    return True, "two exact seeds; adjacent angles/mode preserved"


def selftest() -> int:
    with tempfile.TemporaryDirectory() as directory:
        tmp = Path(directory)
        mutant = tmp / "camera_record.c"
        text = REAL_SOURCE.read_text()
        needle = "g_rec.pos[1] = 10000.0f;"
        if needle not in text:
            print("selftest FAIL: mutation anchor missing")
            return 1
        mutant.write_text(text.replace(needle, "g_rec.pos[1] = 9999.0f;", 1))
        ok, _ = run_check(mutant)
        if ok:
            print("selftest FAIL: wrong-seed mutant escaped")
            return 1

        overreset = tmp / "camera_record_overreset.c"
        text = REAL_SOURCE.read_text()
        anchor = "g_rec.pos[2] = 0.0f;"
        overreset.write_text(text.replace(
            anchor, anchor + "\n    g_rec.angle[0] = 0;", 1
        ))
        ok, _ = run_check(overreset)
        if ok:
            print("selftest FAIL: adjacent-field over-reset mutant escaped")
            return 1

    print("selftest PASS: rejects wrong seed and adjacent-field over-reset mutants")
    return 0


def main() -> int:
    if "--selftest" in sys.argv[1:]:
        return selftest()
    ok, detail = run_check(REAL_SOURCE)
    if not ok:
        print("MISMATCH CGameType/initgame-camera-record-seed: " + detail)
        return 1
    print(
        "PASS CGameType/initgame-camera-record-seed: camera_record_init_game "
        "matches CGameType::InitGame 00474a10 position writes exactly " + detail
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
