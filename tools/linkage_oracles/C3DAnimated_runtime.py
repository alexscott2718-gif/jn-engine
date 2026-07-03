#!/usr/bin/env python3
"""Runtime wiring oracle for C3DAnimated/event-animation-dispatch.

This complements C3DAnimated_dispatch.py. The standalone oracle proves the
dispatcher module; this one proves the module is reached from real native
runtime paths:

* behavior_animated_update_base calls animated_dispatch_update for wired
  C3DAnimated entities.
* behavior_cutscene.c routes target actor animation selection through
  animated_dispatch_set_by_name while preserving the resolved ASE path.
* player_anim.c binds entity-owned records and consumes the represented ladder
  AnimEnded case by returning to STOP.

The harness uses deterministic fake AseModel instances so the test is not a
filesystem asset check.
"""
from __future__ import annotations

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent

EXPECTED = [
    "BASE alias=END idx=0 rec_fc=2 frame=1 sample=1:1:0.000 fires=1 loop=0 model=- user=0",
    "CUT alias=WALK idx=2 rec_fc=5 frame=1 sample=1:2:0.000 fires=0 loop=0 model=assets/ase/carlwalk.ASE user=0",
    "PLAYER alias=STOP idx=0 rec_fc=2 frame=-1 sample=0:1:0.000 fires=1 loop=1 model=- user=0",
]


def build_dumper(
    tmp: Path,
    *,
    behavior_cutscene: Path | None = None,
    behavior_base: Path | None = None,
    player_anim: Path | None = None,
) -> Path:
    binp = tmp / "c3danimated_runtime_dump"
    behavior_cutscene = behavior_cutscene or REPO / "src/game/behaviors/behavior_cutscene.c"
    behavior_base = behavior_base or REPO / "src/game/behaviors/behavior_base.c"
    player_anim = player_anim or REPO / "src/game/player_anim.c"
    cmd = [
        "cc", "-O0",
        f"-DBEHAVIOR_CUTSCENE_SOURCE=\"{behavior_cutscene}\"",
        str(HERE / "c3danimated_runtime_dump.c"),
        str(REPO / "src/game/animated_dispatch.c"),
        str(player_anim),
        str(behavior_base),
        "-I", str(REPO / "src/game/behaviors"),
        "-I", str(REPO / "src/game"),
        "-I", str(REPO / "src/engine"),
        "-lm", "-o", str(binp),
    ]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError("dumper failed to compile\n" + r.stderr)
    return binp


def run_oracle(**sources: Path | None) -> tuple[bool, str]:
    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)
        try:
            binp = build_dumper(tmp, **sources)
        except RuntimeError as exc:
            return False, str(exc)
        r = subprocess.run([str(binp)], capture_output=True, text=True)
        if r.returncode != 0:
            return False, "dumper run failed\n" + r.stderr

    got = [ln.rstrip() for ln in r.stdout.splitlines()
           if ln.startswith(("BASE ", "CUT ", "PLAYER "))]
    if got != EXPECTED:
        for i, (g, e) in enumerate(zip(got, EXPECTED), 1):
            if g != e:
                return False, (
                    f"MISMATCH line {i}\n"
                    f"  got     {g}\n"
                    f"  expect  {e}"
                )
        return False, f"MISMATCH line count got={len(got)} expected={len(EXPECTED)}"

    msg = (
        "PASS C3DAnimated/runtime-wiring: behavior_base advances dispatch, "
        "cutscene actor selection routes through SetAnim3DByName, and the "
        "entity-owned player ladder AnimEnded hook consumes completion to STOP"
    )
    print(msg)
    return True, msg


def replace_once(text: str, old: str, new: str) -> str:
    if old not in text:
        raise RuntimeError(f"mutant anchor not found: {old!r}")
    return text.replace(old, new, 1)


def selftest() -> int:
    base_src = REPO / "src/game/behaviors/behavior_base.c"
    cut_src = REPO / "src/game/behaviors/behavior_cutscene.c"
    player_src = REPO / "src/game/player_anim.c"
    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)
        mutants: list[tuple[str, dict[str, Path]]] = []

        body = replace_once(base_src.read_text(),
                            "    animated_dispatch_update(e, dt);\n",
                            "")
        p = tmp / "behavior_base_no_dispatch.c"
        p.write_text(body)
        mutants.append(("base update dispatch removed", {"behavior_base": p}))

        body = replace_once(cut_src.read_text(),
                            "        (void)animated_dispatch_set_by_name(target, anim, loop);\n",
                            "")
        p = tmp / "behavior_cutscene_no_select.c"
        p.write_text(body)
        mutants.append(("cutscene selection dispatch removed", {"behavior_cutscene": p}))

        body = replace_once(player_src.read_text(),
                            '    if (strcasecmp(alias, "LADDER") == 0) {\n',
                            '    if (0 && strcasecmp(alias, "LADDER") == 0) {\n')
        p = tmp / "player_anim_no_ladder_hook.c"
        p.write_text(body)
        mutants.append(("player ladder hook disabled", {"player_anim": p}))

        rejected = 0
        for name, sources in mutants:
            ok, msg = run_oracle(**sources)
            if ok:
                print(f"SELFTEST FAIL: mutant unexpectedly passed: {name}")
                return 1
            print(f"SELFTEST ok: {name} rejected ({msg.splitlines()[0]})")
            rejected += 1

    print(f"SELFTEST PASS C3DAnimated/runtime-wiring: {rejected} mutants rejected")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()
    if args.selftest:
        return selftest()
    ok, msg = run_oracle()
    if not ok:
        print(msg)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
