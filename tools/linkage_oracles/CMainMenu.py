#!/usr/bin/env python3
"""Linkage oracle: CMainMenu level-routing table.

Proves (L3) that the native front-end menu (src/game/menu.c) routes exactly
the executable's decoded level-routing string table (Neutron.exe
`.rdata:004ec71c`, docs/decomp/CMainMenu.md "Level Routing Table"):

    index 0        NewGame.tsk   -> the NewGame task route (level1b via
                                    CTaskList -- the NewGame->level1b.gam
                                    binding is itself certified by the
                                    CTaskList/tsk-deserialization oracle)
    indices 1..8   the 8 VR challenge levels in the executable's menu order:
                       VR01, VR03, VR02, VR08, VR06, VR07, VR05, VR04

The oracle compiles the real, unmodified menu.c and walks the table through
the real public API (menu_open / menu_input Down presses / menu_take_confirm)
via cmainmenu_dump.c, then diffs each routed (level, is_newgame) pair against
the table above, independently transcribed here from the decomp doc. Probing
past the table end must wrap back to index 0 (the dumper probes 16 slots),
which pins the item count at 10 without reaching into menu.c's statics.

Scope notes (see docs/decomp/CMainMenu.md's Native Linkage section):
  - Certified: the ROUTING TABLE (selection index -> level target ->
    NewGame-task flag) and its order.
  - The trailing "Quit" row is asserted only as the no-route terminator; a
    dedicated Quit menu item is a native UI convention (the original's quit
    control lives in the undecompiled menu-manager screen graph).
  - NOT certified: everything else about the menu -- the CMenuElement screen
    graph, rollover/activation logic, drawing, and input mapping
    (LoadMyMenu/displayMenu are not decompiled; the native keyboard list UI
    is a deliberate stand-in).
  - The `VRxx.gam` -> "vrxx" normalization (lowercase, drop extension) is the
    native loader's level-name convention, applied identically here.

See docs/linked_parity_plan.md for the Linkage Certificate. Exit 0 + PASS on
faithful reproduction; non-zero + a diff on any mismatch.
"""
from __future__ import annotations

import subprocess
import shlex
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]

# The executable's routing table (.rdata:004ec71c), transcribed from
# docs/decomp/CMainMenu.md: NewGame.tsk, then VR01, VR03, VR02, VR08, VR06,
# VR07, VR05, VR04 (menu order), then the native no-route terminator.
DOC_TABLE_RDATA = ["NewGame.tsk", "VR01.gam", "VR03.gam", "VR02.gam",
                   "VR08.gam", "VR06.gam", "VR07.gam", "VR05.gam", "VR04.gam"]


def expected_route(entry: str) -> tuple[str, int]:
    if entry == "NewGame.tsk":
        # NewGame task -> level1b.gam (CTaskList golden table, SCENE=30 row).
        return ("level1b", 1)
    assert entry.upper().endswith(".GAM")
    return (entry[:-4].lower(), 0)


EXPECTED = [expected_route(e) for e in DOC_TABLE_RDATA] + [("(quit)", 0)]
COUNT = len(EXPECTED)  # 10


def build_dumper(tmp: Path) -> Path:
    binp = tmp / "cmainmenu_dump"
    home = Path.home()
    pkg = subprocess.run(
        ["pkg-config", "--cflags", "sdl2"], capture_output=True, text=True
    )
    sdl_cflags = shlex.split(pkg.stdout) if pkg.returncode == 0 else [
        "-I", str(home / "sdl2" / "include"),
        "-I", str(home / "sdl2" / "include" / "SDL2"),
    ]
    cmd = [
        "cc", "-O0",
        "-I", str(REPO / "src" / "engine"),
        *sdl_cflags,
        str(HERE / "cmainmenu_dump.c"),
        str(REPO / "src" / "game" / "menu.c"),
        "-o", str(binp),
    ]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print("MISMATCH: dumper failed to compile\n" + r.stderr)
        sys.exit(1)
    return binp


def main() -> int:
    with tempfile.TemporaryDirectory() as d:
        dumper = build_dumper(Path(d))
        r = subprocess.run([str(dumper)], capture_output=True, text=True)
        if r.returncode != 0:
            print(f"MISMATCH: dumper exited {r.returncode}\n{r.stderr}")
            return 1

        got: dict[int, tuple[str, int]] = {}
        for line in r.stdout.splitlines():
            parts = line.split("|")
            if parts[0] != "M":
                continue
            got[int(parts[1])] = (parts[2], int(parts[3]))

        # 1. The routed table matches the decoded .rdata order exactly.
        for i, exp in enumerate(EXPECTED):
            if got.get(i) != exp:
                print(f"MISMATCH at menu index {i}: native={got.get(i)} "
                      f"reference={exp} (doc order: NewGame, VR01, VR03, VR02, "
                      f"VR08, VR06, VR07, VR05, VR04, quit)")
                return 1

        # 2. Wrap: indices COUNT.. repeat the head -> item count is exactly 10.
        for i in range(COUNT, 16):
            if got.get(i) != EXPECTED[i % COUNT]:
                print(f"MISMATCH: index {i} = {got.get(i)}, expected wrap to "
                      f"{EXPECTED[i % COUNT]} -- item count is not {COUNT}")
                return 1

    print(
        f"PASS CMainMenu/level-routing-table: menu.c routes the executable's "
        f".rdata:004ec71c table exactly (NewGame->level1b task route + 8 VR "
        f"levels in menu order VR01,VR03,VR02,VR08,VR06,VR07,VR05,VR04), "
        f"item count pinned at {COUNT} via selection wrap, driven through the "
        f"real menu_open/menu_input/menu_take_confirm path"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
