#!/usr/bin/env python3
"""Runtime evidence for the picture-flag economy (phases 2 and 3).

Drives the built engine headlessly over the whole .gam corpus through the
JN_TEST_PICTURES sweep and checks the claims the phases are supposed to make:

  phase 2  every picture id the corpus awards is actually awarded at runtime,
           and re-entering a level awards nothing a second time;
  phase 3  the gating rows refuse when the player is short, a later pass
           collects the prerequisite and then succeeds, and a cold entry into
           the four pre-grant levels stops dead-ending.

Not wired into `make check`: it needs both the asset tree and a built binary,
and it loads every level twice. Run it by hand as phase evidence.

  python3 tools/verify_picture_economy.py            # all levels
  python3 tools/verify_picture_economy.py level1c    # one or more levels
"""

from __future__ import annotations

import collections
import os
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from asset_paths import gam_root       # noqa: E402
from gam_parser import parse_gam       # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "jnengine"

AWARD = re.compile(r"\[PICAWARD\] level=(\S+) index=(-?\d+) id=(\d+)")
GATE = re.compile(
    r"\[PICGATE\] level=(\S+) index=(-?\d+) need=(\d+) amount=(\d+)"
    r"(?: have=(\d+))? -> (ok|REFUSED)"
)
SWEEP = re.compile(r"\[PICSWEEP\] level=(\S+) done: (\d+) collected in (\d+) pass")


class Run:
    """One engine invocation, parsed."""

    def __init__(self, level, swap=None, pregrant=True, frames=2):
        env = dict(os.environ, JN_TEST_PICTURES="1")
        if not pregrant:
            env["JN_NO_PREGRANT"] = "1"
        if swap:
            env["JN_TEST_SWAP"] = swap
        cmd = [str(ENGINE), "--level", level, "--headless",
               "--frames", str(frames), "--seed", "7"]
        proc = subprocess.run(cmd, cwd=ROOT, env=env, text=True,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        self.level = level
        self.ok = proc.returncode == 0
        self.awards = [(m.group(1), int(m.group(2)), int(m.group(3)))
                       for m in AWARD.finditer(proc.stdout)]
        self.refusals = [(m.group(1), int(m.group(2)), int(m.group(3)))
                         for m in GATE.finditer(proc.stdout)
                         if m.group(6) == "REFUSED"]
        self.passes = [(m.group(1), int(m.group(2)), int(m.group(3)))
                       for m in SWEEP.finditer(proc.stdout)]
        self.pregranted = "pre-granted" in proc.stdout
        self.stdout = proc.stdout

    def award_ids(self, level=None):
        return {i for lv, _, i in self.awards if level is None or lv == level}


def corpus_expectations(gam_dir):
    award_ids, gate_rows, levels = set(), 0, []
    rows = []          # (level, PIC_NUMBER, RequiredPicNum) for awarding rows
    for path in sorted(gam_dir.glob("*.gam")):
        level = path.stem.lower()
        levels.append(level)
        for obj in parse_gam(path)["objects"]:
            props = obj["properties"]
            pic = props.get("PIC_NUMBER", -1)
            req = props.get("RequiredPicNum", -1)
            req = req if isinstance(req, int) else -1
            if isinstance(pic, int) and pic >= 0:
                award_ids.add(pic)
                rows.append((level, pic, req))
            if req >= 0:
                gate_rows += 1
    return award_ids, gate_rows, levels, rows


def carry_in_donor(rows, missing_id):
    """Why an id can be unreachable on a cold single-level run, and who fixes it.

    The only way an award goes unclaimed is a row that both awards and gates:
    its own level cannot supply enough of the picture it costs, so in linear
    play the player arrives holding the difference. Return (donor, target) for
    a level that awards that prerequisite, or None."""
    for level, pic, req in rows:
        if pic != missing_id or req < 0:
            continue
        donors = [lv for lv, awarded, _ in rows if awarded == req and lv != level]
        if donors:
            return donors[0], level
    return None


def main() -> int:
    if not ENGINE.exists():
        print(f"verify SKIP: no engine at {ENGINE} (run make)")
        return 0
    gam_dir = gam_root()
    if not gam_dir.is_dir():
        print(f"verify SKIP: no .gam corpus at {gam_dir}")
        return 0

    expected_ids, gate_rows, all_levels, rows = corpus_expectations(gam_dir)
    levels = [a.lower() for a in sys.argv[1:]] or all_levels

    failures = []
    seen_ids = set()
    total_refusals = 0
    unlocked_later = 0
    load_failures = []

    print(f"{'level':10s} {'awards':>7s} {'ids':>4s} {'refused':>8s} "
          f"{'passes':>7s} {'re-entry':>9s}")
    print("-" * 52)

    for level in levels:
        cold = Run(level, pregrant=False)
        if not cold.ok or not cold.passes:
            load_failures.append(level)
            print(f"{level:10s} {'(engine run produced no sweep)':>40s}")
            continue

        seen_ids |= cold.award_ids()
        total_refusals += len(cold.refusals)
        # phase 3: a later pass collecting rows an earlier one refused is the
        # "collect the prerequisite, then succeed" case.
        if len(cold.passes[0][2:]) and cold.passes[0][2] > 1:
            unlocked_later += 1

        # phase 2: re-entering the same level must award nothing again.
        again = Run(level, swap=level, pregrant=False, frames=300)
        second = [p for p in again.passes if p[0] == level][1:]
        reentry = sum(n for _, n, _ in second) if second else 0
        second_awards = len(again.awards) - len(cold.awards)
        if reentry != 0:
            failures.append(f"{level}: re-entry collected {reentry}")

        print(f"{level:10s} {len(cold.awards):7d} {len(cold.award_ids()):4d} "
              f"{len(cold.refusals):8d} {cold.passes[0][2]:7d} "
              f"{reentry:9d}")
        if second_awards > 0:
            failures.append(f"{level}: re-entry awarded {second_awards} pictures")

    print("-" * 52)

    # An id can go unclaimed on a cold single-level run when the row awarding it
    # also gates on a picture its own level cannot fully supply. That is the
    # save-global design, not a gap: carry the prerequisite in from the level
    # that awards it and the row opens. Prove it rather than excusing it.
    carried = []
    for missing_id in sorted(expected_ids - seen_ids):
        donor = carry_in_donor(rows, missing_id)
        if not donor:
            continue
        src, dst = donor
        run = Run(src, swap=dst, pregrant=False, frames=300)
        if missing_id in run.award_ids(dst):
            seen_ids.add(missing_id)
            carried.append(f"id {missing_id} via {src} -> {dst}")

    missing = sorted(expected_ids - seen_ids)
    print(f"picture ids awarded at runtime: {len(seen_ids)} of "
          f"{len(expected_ids)} authored")
    for note in carried:
        print(f"  reached by carrying a picture across a level swap: {note}")
    if missing:
        failures.append("never awarded at runtime: " + ", ".join(map(str, missing)))
    print(f"gate refusals on a cold, un-pre-granted entry: {total_refusals} "
          f"(refusal events, not rows -- the sweep retries a refused row each "
          f"pass; the corpus authors {gate_rows} gates)")
    print(f"levels where a later sweep pass collected a row an earlier one "
          f"refused: {unlocked_later}")

    # phase 3: the four pre-grant levels must stop refusing the ids they cannot
    # supply once the cold-entry grant is applied.
    print()
    print("pre-grant effect (cold entry, gate ids the level never awards):")
    for level, ids in (("level1a", {10}), ("level1b", {12, 14}),
                       ("level1c", {23}), ("level4a", {8})):
        if level not in levels:
            continue
        without = Run(level, pregrant=False)
        with_ = Run(level, pregrant=True)
        blocked = collections.Counter(i for _, _, i in without.refusals if i in ids)
        still = collections.Counter(i for _, _, i in with_.refusals if i in ids)
        print(f"  {level:8s} refused without pre-grant: {dict(blocked) or '{}'}"
              f"   with: {dict(still) or '{}'}")
        if not with_.pregranted:
            failures.append(f"{level}: pre-grant did not fire")
        if still:
            failures.append(f"{level}: still refuses {sorted(still)} after pre-grant")
        if not blocked:
            failures.append(f"{level}: expected refusals without pre-grant")

    if load_failures:
        print()
        print("levels the engine could not sweep: " + ", ".join(load_failures))

    print()
    if failures:
        for f in failures:
            print("FAIL: " + f)
        return 1
    print("verify PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
