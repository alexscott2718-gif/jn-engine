#!/usr/bin/env python3
"""Linkage oracle: CTaskList `set-task-state` (FUN_0045f990's table-write half).

Proves (L3) that the native `task_set_entity_state` (src/game/task_loader.c)
reproduces the **table-write loop** of the decompiled `FUN_0045f990`
(Neutron.exe @ 0045f990; recovered 2026-07-01 via
`tools/ghidra/DumpFunctions.java` against the committed Ghidra project) exactly:

    piVar3 = *DAT_004fc5fc;
    if (piVar3 != DAT_004fc5fc) {
        do {
            _Str1 = (char *)piVar3[2];
            if (__strcmpi(_Str1, param_1) == 0)
                *(int *)(_Str1 + 100) = param_2;      // write EXISTING entry only
            piVar3 = (int *)*piVar3;
        } while (piVar3 != DAT_004fc5fc);
    }

i.e. a **case-insensitive** (`__strcmpi`) linear scan of the task-state list that
writes the first matching entry and does **nothing** (no append, no error) when
no entry matches. `PY_REFERENCE_set_state` below is an independent transcription
of that loop (not a copy of task_loader.c) run against the documented NewGame
table (docs/decomp/CTaskList.md) -- the same golden table `CTaskList.py`
(the tsk-deserialization oracle) already certifies as the parse ground truth.

Out of scope for this aspect (see "Deliberate deviations" in
docs/decomp/CTaskList.md's Native Linkage section): the SCENE-specific
objective-ordinal flag (`FUN_00406f50`) and the live-object `TaskName`-match
broadcast (vtable `+0x424`) that the same native function also performs --
both are HUD/menu-subsystem side effects with no store-visible effect on the
task-state table itself, and are not implemented natively (deferred).

See docs/linked_parity_plan.md for the Linkage Certificate. Exit 0 + PASS on
faithful reproduction; non-zero + a diff on any mismatch.
"""
from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]

# ---- documented NewGame ground truth (docs/decomp/CTaskList.md) -------------
# Independently transcribed from the doc (same golden table CTaskList.py uses),
# not imported from the native port -- so a bug shared between the doc
# transcription and the C port would still surface as a diff against this.
NEWGAME_ENTITIES: list[tuple[str, int]] = [
    ("MOM", 0), ("LIBBY", 0), ("BENNY", 0), ("SCENE", 30),
    ("DINO", 0), ("CLONE", 0), ("KITTY1", 0), ("KITTY2", 0),
    ("KITTY3", 0), ("HYDRANT1", 0), ("HYDRANT2", 0), ("REACTOR", 0),
]


def py_set_state(entities: list[list], tag: str, value: int) -> int:
    """Transcription of FUN_0045f990's table-write loop. Mutates `entities`
    in place; returns 1 if an existing entry was written, 0 otherwise (no
    append -- matches the C port's task_set_entity_state return convention)."""
    if not tag:
        return 0
    for e in entities:
        if e[0].casefold() == tag.casefold():
            e[1] = value
            return 1
    return 0


def build_dumper(tmp: Path) -> Path:
    binp = tmp / "tasklist_setstate_dump"
    src_game = REPO / "src" / "game"
    src_assets = REPO / "src" / "engine" / "assets"
    cmd = [
        "cc", "-O0", "-I", str(src_game),
        str(HERE / "tasklist_setstate_dump.c"), str(src_game / "task_loader.c"),
        str(src_assets / "asset_paths.c"),
        "-o", str(binp),
    ]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print("MISMATCH: dumper failed to compile\n" + r.stderr)
        sys.exit(1)
    return binp


def run_case(dumper: Path, writes: list[tuple[str, int]]) -> tuple[list[tuple[str, int, int]], list[tuple[str, int]]]:
    args = [f"{tag}={value}" for tag, value in writes]
    r = subprocess.run([str(dumper), *args], capture_output=True, text=True)
    if r.returncode != 0:
        print(f"MISMATCH: dumper failed on {args}\n{r.stderr}")
        sys.exit(1)
    c_writes: list[tuple[str, int, int]] = []
    c_table: list[tuple[str, int]] = []
    for line in r.stdout.splitlines():
        parts = line.split("|")
        if parts[0] == "W":
            c_writes.append((parts[1], int(parts[2]), int(parts[3])))
        elif parts[0] == "E":
            c_table.append((parts[1], int(parts[2])))
    return c_writes, c_table


def reference_case(writes: list[tuple[str, int]]) -> tuple[list[tuple[str, int, int]], list[tuple[str, int]]]:
    table = [[tag, state] for tag, state in NEWGAME_ENTITIES]
    py_writes = []
    for tag, value in writes:
        ok = py_set_state(table, tag, value)
        py_writes.append((tag, value, ok))
    return py_writes, [(t, s) for t, s in table]


def main() -> int:
    cases: dict[str, list[tuple[str, int]]] = {
        # Every documented tag, case-varied, each to a distinct new value --
        # proves the case-insensitive match and per-entry write.
        "all_tags_case_varied": [
            ("mom", 111), ("Libby", 222), ("BENNY", 333), ("scene", 444),
            ("Dino", 555), ("CLONE", 666), ("kitty1", 777), ("Kitty2", 888),
            ("KITTY3", 999), ("hydrant1", 1010), ("Hydrant2", 1111), ("REACTOR", 1212),
        ],
        # Unknown tags: must no-op (return 0), never append.
        "unknown_tags_noop": [
            ("NOPE", 5), ("reactor2", 9), ("", 1), ("scenex", 2),
        ],
        # Same tag written twice -- last write wins (matches a plain linear
        # first-match-and-stop scan being called twice, not a "latch").
        "overwrite_last_wins": [
            ("SCENE", 30), ("SCENE", 35), ("scene", 40),
        ],
        # Mixed known/unknown interleaved, plus the empty-tag edge case.
        "mixed": [
            ("MOM", 1), ("GHOST", 2), ("", 3), ("KITTY1", 4), ("Unknown", 5),
        ],
    }

    with tempfile.TemporaryDirectory() as d:
        dumper = build_dumper(Path(d))
        checked = 0
        for name, writes in cases.items():
            c_writes, c_table = run_case(dumper, writes)
            py_writes, py_table = reference_case(writes)

            if c_writes != py_writes:
                print(f"MISMATCH in case {name!r}: write return codes diverged")
                print("--- native (task_set_entity_state) ---\n" + repr(c_writes))
                print("--- reference (FUN_0045f990 transcription) ---\n" + repr(py_writes))
                return 1
            if c_table != py_table:
                print(f"MISMATCH in case {name!r}: final table diverged")
                print("--- native ---\n" + repr(c_table))
                print("--- reference ---\n" + repr(py_table))
                return 1
            checked += 1

    print(
        f"PASS CTaskList/set-task-state: task_set_entity_state == FUN_0045f990's "
        f"table-write loop (case-insensitive match, write-existing-only, never "
        f"append) across {checked} write sequences over the documented NewGame table"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
