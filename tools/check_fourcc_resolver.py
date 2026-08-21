#!/usr/bin/env python3
"""The spec generator must never contradict a spec's stated FourCC.

`tools/gen_placeable_specs.py` writes `docs/decomp/*.md`. The 2026-08 audit
found 17 specs carrying a FourCC that was not theirs (task B-01) and those
specs were fixed by hand; the generator was not, so a regeneration would have
put all 17 back. This locks the fix in: for every class in the ledger, the
resolver's answer must either match what that class's spec states, or be None.

`None` is a fine answer and deliberately so. The branch that used to fill those
gaps -- "nearest preceding class-id registrar within 0x800" -- matched on
address proximity with no identity check, and it is what gave `C3DDarwinFish`
the id of `C3DDino` and `C3DSparrow` the id of a *level*. Every source the
resolver uses now names the class: the schema's FourCC-to-class map, the
class-id immediate in `InitObject`, the scan's own class column, and the
dominant `ObjectTag` of the shipped rows. `C3DSparrow` still comes back
unresolved, because its one placed row is tagged `vulta` -- which is the honest
answer, not a gap to paper over.

No binaries needed. `--selftest` is a negative control of this script: it
perturbs the resolver in memory and requires the failure to be reported.

Exit 0 + a PASS line, or non-zero + the contradictions.
"""
from __future__ import annotations

import csv
import importlib.util
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FOURCC_ROW = re.compile(r"^\|\s*FourCC\s*\|\s*(.*?)\s*\|?\s*$")
TICKED = re.compile(r"`([A-Za-z0-9]{4})`")
LEVEL_ID = re.compile(r"^LEV\d", re.I)


def load_generator():
    spec = importlib.util.spec_from_file_location(
        "gen_placeable_specs", ROOT / "tools" / "gen_placeable_specs.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def spec_fourcc(cls: str):
    p = ROOT / "docs" / "decomp" / (cls + ".md")
    if not p.exists():
        return None
    sec = None
    for ln in p.read_text(errors="replace").splitlines():
        if ln.startswith("## "):
            sec = ln[3:].strip()
            continue
        if sec != "Identity":
            continue
        m = FOURCC_ROW.match(ln)
        if m:
            mm = TICKED.search(m.group(1).rstrip("|").strip())
            return mm.group(1) if mm else None
    return None


def run(mod):
    ledger = [r["class"] for r in
              csv.DictReader(open(ROOT / "docs" / "decomp_ledger.csv"))]
    agree = unresolved = 0
    problems = []
    for cls in ledger:
        want = spec_fourcc(cls)
        got = mod.fourcc_for(cls, None, None)
        if got and LEVEL_ID.match(got):
            problems.append(f"{cls}: resolver returned the level id {got!r}")
            continue
        if got and want and got.lower() != want.lower():
            problems.append(
                f"{cls}: resolver says {got!r}, docs/decomp/{cls}.md says {want!r}")
        elif got and want:
            agree += 1
        elif not got:
            unresolved += 1
    return len(ledger), agree, unresolved, problems


def main(argv) -> int:
    mod = load_generator()
    total, agree, unresolved, problems = run(mod)
    if problems:
        print("fourcc-resolver MISMATCH: the generator contradicts %d spec(s)"
              % len(problems))
        for line in problems[:10]:
            print("   " + line)
        return 1

    if "--selftest" in argv:
        real = mod.fourcc_for
        victim = next(c for c in
                      (r["class"] for r in
                       csv.DictReader(open(ROOT / "docs" / "decomp_ledger.csv")))
                      if spec_fourcc(c))
        mod.fourcc_for = lambda cls, code, addr=None: (
            "ZZZZ" if cls == victim else real(cls, code, addr))
        _, _, _, mutant = run(mod)
        mod.fourcc_for = real
        if not mutant:
            print("selftest FAIL: a resolver that contradicts %s was not caught"
                  % victim)
            return 1
        print("fourcc-resolver PASS: %d classes, %d resolved and agreeing with "
              "their spec, %d honestly unresolved, 0 contradictions; selftest "
              "catches a resolver that disagrees" % (total, agree, unresolved))
        return 0

    print("fourcc-resolver PASS: %d classes, %d resolved and agreeing with their "
          "spec, %d honestly unresolved, 0 contradictions"
          % (total, agree, unresolved))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
