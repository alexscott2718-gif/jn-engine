#!/usr/bin/env python3
"""Unit + mutation check for the picture-flag economy (src/game/gamestate.c).

Asset-free by default so it can run in `make check`:
  * compiles the real gamestate.c against tools/pictures_dump.c and validates
    the picture counts, the (level, PickupIndex) collected-state table, the
    persistence rules, and the generated cold-entry pre-grant table;
  * --selftest additionally proves the check catches three mutants that the
    plan calls out as silently wrong.

  * --corpus cross-checks the generated pre-grant header against the .gam
    corpus and re-derives the gating graph's closure. Skips when no corpus is
    present, so it is safe in an asset-free checkout.

Usage:
  python3 tools/check_pictures.py --selftest
  python3 tools/check_pictures.py --corpus
"""

from __future__ import annotations

import argparse
import collections
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/game/gamestate.c"
DUMPER = ROOT / "tools/pictures_dump.c"
PREGRANTS = ROOT / "src/game/picture_pregrants_generated.h"

# Every value the dumper emits, and why it has to be that value.
EXPECTED = {
    # picture counts
    "award1": 1,
    "award2": 3,
    "consume_ok": 1,
    "after_consume": 1,
    "consume_short": 0,          # short -> refuse
    "after_short": 1,            # ...and do not partially drain
    "consume_exact": 1,
    "after_exact": 0,
    "consume_empty": 0,
    "consume_zero_amount": 1,    # ReqPicNumAmount < 1 means 1
    "after_zero_amount": 0,
    "neg_count": 0,
    "over_count": 0,
    "neg_consume": 0,
    "over_consume": 0,
    "id_zero": 1,
    "id_top": 1,
    # collected-state table, keyed on (level, PickupIndex)
    "level_echo": 1,
    "l3_taken": 1,
    "l5a_not_taken": 0,          # 1901 exists in Level3 AND level5a
    "l5a_taken": 1,
    "l3_still_taken": 1,
    "other_level_clear": 0,
    "case_folded": 1,
    "neighbour_clear": 0,
    "zero_index": 0,             # PickupIndex <= 0 is the non-table pickup
    "neg_index": 0,
    "empty_level": 0,
    # persistence
    "swap_keeps_count": 4,
    "swap_keeps_taken": 1,
    "newgame_clears_count": 0,
    "newgame_clears_taken": 0,
    # generated pre-grant table (4 levels, 5 ids -- see the header)
    "pregrant_level1c": 18,
    "pregrant_level1c_id23": 18,
    "pregrant_level1b": 5,
    "pregrant_level1b_id12": 4,
    "pregrant_level1b_id14": 1,
    "pregrant_level1a": 4,
    "pregrant_level4a": 12,
    "pregrant_level1": 0,        # self-supplying levels keep their gate
    "pregrant_level2": 0,
    "pregrant_unknown": 0,
    "pregrant_empty": 0,
    # clear (SetPickupItemState state 1: the vending-machine re-arm)
    "clear_before": 1,
    "clear_after": 0,
    "clear_leaves_neighbour": 1,   # clearing must not evict the probe chain
    "remark_after_clear": 1,
    "clear_unmarked": 0,
    "clear_is_level_scoped": 1,
    "clear_index_zero": 0,
    # table capacity
    "bulk_first": 1,
    "bulk_last": 1,
    "bulk_miss": 0,
    "bulk_other_level": 0,
}

# (label, needle, replacement). Each is a defect the plan names as silently
# wrong, so the check has to be able to see it.
MUTANTS = [
    (
        "flat index-keyed collected table",
        "    pickup_level_key(level, key, sizeof key);\n    if (!key[0]) return NULL;",
        '    (void)level;\n    snprintf(key, sizeof key, "%s", "L");',
    ),
    (
        "level swap clears the picture store",
        "void gamestate_reset_for_new_level(void) {",
        "void gamestate_reset_for_new_level(void) {\n"
        "    memset(g_state.pic_count, 0, sizeof g_state.pic_count);",
    ),
    (
        "off-by-one in the required-amount test",
        "    if (g_state.pic_count[id] < n) return 0;",
        "    if (g_state.pic_count[id] < n - 1) return 0;",
    ),
    (
        "vending-machine re-arm does not actually clear",
        "    if (s) s->taken = 0;",
        "    if (s) s->taken = 1;",
    ),
]


def build_and_run(source: Path, tmp: Path, tag: str) -> str:
    binary = tmp / ("pictures_dump_" + tag)
    command = [
        "cc", "-std=c99", "-Wall", "-Werror", "-O0",
        "-I", str(ROOT / "src/engine"),
        "-I", str(ROOT / "src/game"),
        str(DUMPER), str(source), "-o", str(binary),
    ]
    subprocess.run(command, cwd=ROOT, check=True, capture_output=True, text=True)
    return subprocess.run(
        [str(binary)], cwd=ROOT, check=True, capture_output=True, text=True
    ).stdout


def validate(output: str) -> list[str]:
    got = {}
    for line in output.splitlines():
        parts = line.split("|")
        if len(parts) == 3 and parts[0] == "K":
            got[parts[1]] = int(parts[2])

    errors = []
    missing = sorted(set(EXPECTED) - set(got))
    if missing:
        errors.append("probe did not report: " + ", ".join(missing))
    extra = sorted(set(got) - set(EXPECTED))
    if extra:
        errors.append("probe reported unexpected keys: " + ", ".join(extra))
    for name in sorted(set(EXPECTED) & set(got)):
        if got[name] != EXPECTED[name]:
            errors.append(f"{name}={got[name]} != {EXPECTED[name]}")
    return errors


def mutate(text: str, needle: str, replacement: str, tmp: Path, tag: str) -> Path:
    if text.count(needle) != 1:
        raise SystemExit(
            f"pictures selftest FAIL: mutation site '{tag}' matched "
            f"{text.count(needle)} times, expected 1"
        )
    path = tmp / f"gamestate_mutant_{tag}.c"
    path.write_text(text.replace(needle, replacement))
    return path


def run_selftest(tmp: Path) -> None:
    text = SOURCE.read_text()
    for n, (label, needle, replacement) in enumerate(MUTANTS):
        mutant = mutate(text, needle, replacement, tmp, str(n))
        try:
            survived = not validate(build_and_run(mutant, tmp, f"m{n}"))
        except subprocess.CalledProcessError:
            survived = False   # a mutant that will not build is also caught
        if survived:
            raise SystemExit(f"pictures selftest FAIL: mutant survived -- {label}")


def run_corpus() -> None:
    """Re-derive the pre-grant table and the closure claim from the corpus."""
    sys.path.insert(0, str(ROOT / "tools"))
    from asset_paths import gam_root                 # noqa: E402
    from gam_parser import parse_gam                 # noqa: E402
    from gen_picture_pregrants import render, scan   # noqa: E402

    gam_dir = gam_root()
    files = sorted(gam_dir.glob("*.gam")) if gam_dir.is_dir() else []
    if not files:
        print(f"pictures corpus SKIP: no .gam corpus at {gam_dir}")
        return

    if PREGRANTS.read_text() != render(scan(gam_dir)):
        raise SystemExit(
            "pictures corpus FAIL: src/game/picture_pregrants_generated.h is "
            "stale -- re-run tools/gen_picture_pregrants.py"
        )

    awarded, required = collections.Counter(), collections.Counter()
    for path in files:
        for obj in parse_gam(path)["objects"]:
            props = obj["properties"]
            pic = props.get("PIC_NUMBER", -1)
            if isinstance(pic, int) and pic >= 0:
                awarded[pic] += 1
            req = props.get("RequiredPicNum", -1)
            if isinstance(req, int) and req >= 0:
                required[req] += 1

    # The gating graph closing globally is what makes the gate safe to ship:
    # no required picture is unobtainable, so no content is stranded.
    stranded = sorted(pic for pic in required if awarded[pic] == 0)
    if stranded:
        raise SystemExit(
            "pictures corpus FAIL: required picture(s) no row awards: "
            + ", ".join(str(p) for p in stranded)
        )

    # Nothing may exceed the store's id range.
    over = sorted(p for p in set(awarded) | set(required) if p >= 96)
    if over:
        raise SystemExit(
            "pictures corpus FAIL: picture id(s) beyond PIC_ID_MAX: "
            + ", ".join(str(p) for p in over)
        )

    print(
        f"pictures corpus PASS: {len(files)} levels, {sum(awarded.values())} "
        f"awarding rows over {len(awarded)} ids, {sum(required.values())} "
        f"gating rows over {len(required)} ids, all obtainable; pre-grant "
        f"header current"
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--selftest", action="store_true")
    parser.add_argument("--corpus", action="store_true")
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix="jn-pictures-") as raw:
        tmp = Path(raw)
        errors = validate(build_and_run(SOURCE, tmp, "real"))
        if errors:
            raise SystemExit("pictures FAIL: " + "; ".join(errors))
        if args.selftest:
            run_selftest(tmp)

    suffix = f"; {len(MUTANTS)} mutants rejected" if args.selftest else ""
    print(
        "pictures PASS: counts, (level, PickupIndex) collected table, "
        "swap/new-game persistence, and the cold-entry pre-grant table" + suffix
    )

    if args.corpus:
        run_corpus()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
