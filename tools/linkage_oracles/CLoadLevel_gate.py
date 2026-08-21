#!/usr/bin/env python3
"""Linkage oracle: CLoadLevel contact gate (`RequiredTask` / `RequiredLevel` /
`ExactLevel`).

**L1** is the recovered contact body at `00457ec0`
(`docs/decomp/evidence/cloadlevel_gate_00457ec0.md`), whose gate reads:

    if RequiredTask != "none":
        state = task_state(RequiredTask)              # FUN_0045fea0, -1 = absent
        if state != -1:
            if state < RequiredLevel:                    return   # blocked
            if ExactLevel != -1 and state != ExactLevel: return   # blocked
        else:
            log "ERROR: Task %s not found in in %s"               # and CONTINUE

Three properties of that shape are the whole point, and each is a place a
plausible implementation goes wrong:

* both tests apply -- `ExactLevel` does not replace `RequiredLevel`;
* neither applies unless `RequiredTask` names something;
* a task the store does not hold does **not** block: it logs and falls through.

**L2** is `behavior_load_gate_allows()` in `src/game/behaviors/behavior_load.c`,
compiled here unmodified by `cloadlevel_gate_dump.c`.

**L3** drives it over all 97 `LOAD` rows in the 35 shipped `.gam` files, at 54
story states -- every `RequiredLevel` and `ExactLevel` value authored anywhere
in the corpus, each with its two neighbours, plus 0, a value past the top of
the story, and the store-absent case -- and diffs all 5238 verdicts against
the rule above evaluated on the row's own authored properties.

The story state is the real `CTaskList` store, seeded through the real
`game_flow_test_seed_state`. The reference side models it from the NewGame
table transcribed in `docs/decomp/CTaskList.md` (`SCENE` 30, eleven other tags
at 0) -- so `level4.gam`'s `RequiredTask "tunneldt"`, which that table does not
hold, exercises the missing-task branch on both sides.

**L4**: nothing here is tuned. The properties come from the shipped rows, the
rule from the recovered body, the task table from the class doc.

`--selftest` mutation-tests the oracle against the three defects above: giving
`ExactLevel` precedence over `RequiredLevel` (what the shared native window
helper does), blocking on a missing task instead of continuing, and applying
the window when `RequiredTask` is "none". Each mutant must build, run, and be
caught -- a mutant that fails to compile is a selftest failure, not a pass.

**Scope note on the missing-task branch.** This oracle certifies the gate's
response *to a state*. What state an unmatched `RequiredTask` produces belongs
to the getter, `FUN_0045fea0`, and is contradicted in the tree: the recovered
`CLoadLevel` body compares against `-1`, while `docs/decomp/_scene_sequencer.md`
and `docs/decomp/CTaskList.md` both read the getter's own body and say it
returns `0` on no-match -- which would make that branch dead code. Native's
`task_entity_state` returns `-1` (a divergence `CTaskList.md` already flags),
and both sides of this oracle model that same native getter, so the diff stays
well-defined either way. Settling it needs the executable; see the 2026-08-21
section of `docs/decomp/evidence/cloadlevel_gate_00457ec0.md`.

**Not covered**: everything after the gate -- the `RETURN` branch, the
`LevelName == "none"` refusal and the portal hide/handoff are behavior with
side effects rather than a pure verdict, and the unported `SoundIndex` /
`FadeType` / `FadeTime` tail and the `DAT_004f0588` mode switch are outside the
port (see the file header in `behavior_load.c`).

Exit 0 + a `PASS` line on faithful reproduction; non-zero + a diff otherwise.
"""
from __future__ import annotations

import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(REPO / "tools"))
import gam_parser  # noqa: E402
from asset_paths import gam_root  # noqa: E402

GAM_DIR = gam_root()
EXPECTED_ROWS = 97       # shipped LOAD instances; a corpus change re-verifies

SRC = REPO / "src"
BEHAVIOR_LOAD = SRC / "game/behaviors/behavior_load.c"
SOURCES = [
    BEHAVIOR_LOAD,
    SRC / "game/gamestate.c",
    SRC / "game/game_flow.c",
    SRC / "game/task_loader.c",
    SRC / "game/camera_record.c",
    SRC / "engine/assets/asset_paths.c",
    SRC / "engine/assets/gam_loader.c",
    SRC / "engine/player_physics.c",
]
BUILD_FAILED = "dumper failed to compile"

# The NewGame task table, transcribed in docs/decomp/CTaskList.md and baked
# into task_loader.c because the .tsk files are proprietary and uncommitted.
# Only SCENE is ever seeded; the rest stay at their table value, and a tag that
# is not here is what "task not found" means.
NEWGAME_TABLE = {
    "MOM": 0, "LIBBY": 0, "BENNY": 0, "SCENE": 30,
    "DINO": 0, "CLONE": 0, "KITTY1": 0, "KITTY2": 0,
    "KITTY3": 0, "HYDRANT1": 0, "HYDRANT2": 0, "REACTOR": 0,
}


def load_rows(path: Path):
    """(file_index, ObjectTag, props) for every LOAD row.

    file_index counts every object in the file, not just the LOAD ones: it is
    the same index the dumper emits, so a desync anywhere upstream in the
    record walk shows up here as a missing or mislabelled row rather than as a
    silently shifted comparison."""
    doc = gam_parser.parse_gam(path)
    out = []
    for idx, obj in enumerate(doc["objects"]):
        if obj["type"] == "LOAD":
            out.append((idx, obj["properties"].get("ObjectTag", ""),
                        obj["properties"]))
    return out


def state_vectors(all_rows) -> list[str]:
    """Every authored gate value with its neighbours, plus the edges."""
    seen = {0, 1000}
    for _, _, props in all_rows:
        for key in ("RequiredLevel", "ExactLevel"):
            v = props.get(key)
            if isinstance(v, int) and v >= 0:
                seen.update((v - 1, v, v + 1))
    return ["none"] + [str(v) for v in sorted(x for x in seen if x >= 0)]


def expected_verdict(props: dict, state_arg: str) -> int:
    """The recovered 00457ec0 gate, on this row's own authored properties."""
    task = str(props.get("RequiredTask", "none")).strip()
    if not task or task.lower() == "none":
        return 1                                  # __strcmpi(...,"none") skip

    if state_arg == "none":
        state = -1                                # no store: every task absent
    elif task.upper() == "SCENE":
        state = int(state_arg)                    # the seeded entry
    else:
        state = NEWGAME_TABLE.get(task.upper(), -1)

    if state == -1:
        return 1                                  # logged, and falls through

    required = props.get("RequiredLevel", -1)
    if not isinstance(required, int):
        required = -1
    if state < required:
        return 0
    exact = props.get("ExactLevel", -1)
    if not isinstance(exact, int):
        exact = -1
    if exact != -1 and state != exact:
        return 0
    return 1


def build_dumper(tmp: Path, replace: dict | None = None):
    replace = replace or {}
    binp = tmp / "cloadlevel_gate_dump"
    cmd = ["cc", "-O0", str(HERE / "cloadlevel_gate_dump.c")]
    # A mutant compiled from a temp directory still has to resolve the quoted
    # includes the real source resolves relative to its own directory.
    for d in sorted({str(s.parent) for s in SOURCES}):
        cmd += ["-iquote", d]
    cmd += [str(replace.get(s, s)) for s in SOURCES]
    cmd += ["-lm", "-o", str(binp)]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        return None, BUILD_FAILED + "\n" + r.stderr
    return binp, ""


def run_all(replace: dict | None = None):
    """-> (checks, "") on agreement, (None, diff) on any disagreement."""
    gams = sorted(GAM_DIR.glob("*.gam"))
    if not gams:
        return None, f"no .gam files under {GAM_DIR}"

    corpus = []
    for gam in gams:
        corpus.extend(load_rows(gam))
    states = state_vectors(corpus)

    checks, rows_seen, problems = 0, 0, []
    with tempfile.TemporaryDirectory() as d:
        tmp = Path(d)
        dumper, err = build_dumper(tmp, replace)
        if dumper is None:
            return None, err
        for gam in gams:
            rows = {i: (tag, props) for i, tag, props in load_rows(gam)}
            if not rows:
                continue
            rows_seen += len(rows)
            r = subprocess.run([str(dumper), str(gam), *states],
                               capture_output=True, text=True)
            if r.returncode != 0:
                return None, f"dumper failed on {gam.name}\n{r.stderr}"

            for line in r.stdout.splitlines():
                f = line.split("|")
                if f[0] == "T":
                    idx, tag = int(f[1]), f[2]
                    if idx not in rows:
                        problems.append(f"{gam.name}: native reported a LOAD row "
                                        f"{idx} the reference parser does not have")
                    elif rows[idx][0] != tag:
                        problems.append(f"{gam.name} row {idx}: ObjectTag "
                                        f"{tag!r} != reference {rows[idx][0]!r}")
                elif f[0] == "G":
                    idx, state, got = int(f[1]), f[2], int(f[3])
                    if idx not in rows:
                        continue
                    tag, props = rows[idx]
                    want = expected_verdict(props, state)
                    checks += 1
                    if got != want:
                        problems.append(
                            f"{gam.name} row {idx} ({tag!r}) at SCENE={state}: "
                            f"native {'allow' if got else 'block'} != recovered "
                            f"{'allow' if want else 'block'} "
                            f"(RequiredTask={props.get('RequiredTask')!r} "
                            f"RequiredLevel={props.get('RequiredLevel')} "
                            f"ExactLevel={props.get('ExactLevel')})")
            if problems:
                return None, "\n".join(problems[:6])

    if rows_seen != EXPECTED_ROWS:
        return None, (f"expected {EXPECTED_ROWS} shipped LOAD rows, found "
                      f"{rows_seen} -- corpus changed, re-verify this oracle")
    return checks, ""


MUTANTS = [
    # What the shared native window helper does, and what this port replaced.
    ("ExactLevel takes precedence over RequiredLevel",
     "    if (state < (long)gam_prop_i(e, \"RequiredLevel\", -1)) return 0;\n"
     "    int exact = gam_prop_i(e, \"ExactLevel\", -1);\n"
     "    if (exact != -1 && state != (long)exact) return 0;",
     "    int exact = gam_prop_i(e, \"ExactLevel\", -1);\n"
     "    if (exact != -1) return state == (long)exact;\n"
     "    if (state < (long)gam_prop_i(e, \"RequiredLevel\", -1)) return 0;"),
    ("a missing task blocks instead of falling through",
     "    long state = load_task_state(task);\n    if (state < 0) {",
     "    long state = load_task_state(task);\n    if (state < 0) return 0;\n"
     "    if (state < 0) {"),
    ("the window is applied even when RequiredTask is \"none\"",
     "    if (!task || !task[0] || strcasecmp(task, \"none\") == 0) return 1;",
     "    long ignored = 0; (void)ignored;\n"
     "    if (!task || !task[0] || strcasecmp(task, \"none\") == 0)\n"
     "        return gam_prop_i(e, \"ExactLevel\", -1) == -1;"),
]


def selftest() -> int:
    text = BEHAVIOR_LOAD.read_text()
    for label, needle, replacement in MUTANTS:
        if text.count(needle) != 1:
            print(f"selftest FAIL: mutation anchor missing for {label!r}")
            return 1
        with tempfile.TemporaryDirectory() as d:
            mutant = Path(d) / BEHAVIOR_LOAD.name
            mutant.write_text(text.replace(needle, replacement))
            checks, err = run_all({BEHAVIOR_LOAD: mutant})
        if checks is not None:
            print(f"selftest FAIL: mutant survived -- {label}")
            return 1
        if err.startswith(BUILD_FAILED):
            print(f"selftest FAIL: the {label!r} mutant did not build, so it was "
                  f"never executed -- a mutation test that cannot run proves "
                  f"nothing\n{err}")
            return 1
        print(f"selftest ok: {label} rejected ({err.splitlines()[0]})")
    print(f"selftest PASS: oracle rejects {len(MUTANTS)} gate mutants "
          f"(each built and executed)")
    return 0


def main(argv) -> int:
    if "--selftest" in argv:
        return selftest()
    checks, err = run_all()
    if checks is None:
        print("MISMATCH CLoadLevel/contact-gate: " + err)
        return 1
    print(f"PASS CLoadLevel/contact-gate: behavior_load_gate_allows reproduces "
          f"the recovered 00457ec0 gate -- RequiredTask lookup, RequiredLevel "
          f"minimum and optional ExactLevel both applied, missing task falls "
          f"through -- over {checks} verdicts across all {EXPECTED_ROWS} shipped "
          f"LOAD rows in 35 levels")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
