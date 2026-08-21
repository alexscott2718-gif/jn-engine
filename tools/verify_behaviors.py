#!/usr/bin/env python3
"""verify_behaviors.py — static behavior-classification audit.

Cross-checks every FourCC -> behavior-vtable registration in
src/game/entities.c against the decompiled class's base chain in
docs/decomp_ledger.csv (joined via docs/_gam_classids.tsv, FourCC -> class).

The native port assigns each placeable FourCC an EntityVTable that implements a
specific gameplay role. That role implies a structural invariant on the class's
inheritance: a *rideable* vehicle must derive C3DFlyingObject (flown via the
flying movement base); an *AI-driven* vehicle/enemy/walker must derive C3DAI; a
projectile/pickup/camera must derive its matching base. This tool asserts those
invariants so a misclassification — e.g. wiring a flight vehicle onto the AI
patrol base, or (the bug that motivated this) confusing the rideable
C3DRocketShip with the autonomous toy C3DRocket — fails mechanically instead of
only surfacing in play.

  Vehicle  C3DRocketShip (3ROC)  -> C3DFlyingObject   (player flight)
           C3DBus/C3DAISuv/...   -> C3DAI             (self-driving)
  Enemy    C3DYokian* (3SOL/...) -> C3DAI
  Walker   C3DCarl (3CAR)        -> C3DAI

Exit code 0 = all checked mappings consistent; 1 = at least one mismatch.

  python3 tools/verify_behaviors.py          # summary, fails on mismatch
  python3 tools/verify_behaviors.py -v       # also list SKIP/unmapped rows
"""

import csv
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Behavior vtable -> required base-chain ancestor (any-of). A FourCC wired to
# this vtable must resolve to a class whose base chain contains one of these.
# Vtables not listed are structural/cosmetic (static props, mechanisms, the
# generic default) and are reported as SKIP rather than asserted.
VTABLE_REQUIRED_ANCESTOR = {
    "vt_player":          ["C3DPlayer"],
    "vt_walker":          ["C3DAI"],
    "vt_yokian":          ["C3DAI"],
    "vt_ai_vehicle":      ["C3DAI", "C3DVehicle"],
    "vt_rocket":          ["C3DFlyingObject"],
    "vt_balloon":         ["C3DSpriteType", "C3DBalloon"],
    "vt_baseball_pickup": ["C3DPickupType", "CPickupType", "C3DBaseballPickup"],
    "vt_bubble_pickup":   ["C3DPickupType", "CPickupType", "C3DBubblePickup"],
    "vt_helmet":          ["C3DPickupType", "CPickupType", "C3DHelmet"],
    "vt_metal_pickup":    ["C3DPickupType", "CPickupType", "C3DMetalPickup"],
    "vt_cutscene_camera": ["C3DCutSceneCamera", "C3DTriggerType"],
    "vt_multi_cutscene":  ["C3DMultiCutSceneCamera", "C3DTriggerType"],
    "vt_patrolpoint":     ["C3DPatrolPoint"],
}


def parse_entities():
    """[(fourcc, vtable_name)] from the entity_types[] table in entities.c."""
    path = os.path.join(REPO, "src/game/entities.c")
    out = []
    row_re = re.compile(r'\{\s*"([0-9A-Za-z]{3,4})"\s*,\s*"[^"]*"\s*,\s*&(\w+)\s*\}')
    for line in open(path):
        m = row_re.search(line)
        if m:
            out.append((m.group(1), m.group(2)))
    if not out:
        raise SystemExit(
            "verify_behaviors: parsed 0 rows from src/game/entities.c -- the "
            "entity_types[] layout changed and this regex did not. Refusing to "
            "report every mapping consistent when none was read.")
    return out


def parse_fourcc_to_class():
    """{fourcc: class} from _gam_classids.tsv.

    The file is whitespace-aligned (not actually tab-delimited): positional
    columns [imm_fourcc, reversed, @site, function, class?]. The readable FourCC
    is the `reversed` column (index 1); the class, when present, is the last
    column (index 4, e.g. 'C3DRocketShip()')."""
    path = os.path.join(REPO, "docs/_gam_classids.tsv")
    out = {}
    for line in open(path):
        f = line.split()
        if len(f) < 5 or f[0] == "imm_fourcc":
            continue
        fourcc = f[1]
        cls = re.sub(r"\(.*$", "", f[4]).strip()  # strip "()" / trailing text
        if fourcc and cls and cls != "??":
            out.setdefault(fourcc, cls)
    return out


def parse_ledger():
    """{class: (family, base_chain)} from decomp_ledger.csv."""
    path = os.path.join(REPO, "docs/decomp_ledger.csv")
    out = {}
    with open(path) as f:
        for r in csv.DictReader(f):
            out[r["class"]] = (r.get("family", ""), r.get("base_chain", ""))
    return out


def main():
    verbose = "-v" in sys.argv
    entities = parse_entities()
    fourcc_cls = parse_fourcc_to_class()
    ledger = parse_ledger()
    # _gam_classids casing (C3DBUS) can differ from the ledger (C3DBus) — join
    # case-insensitively.
    ledger_ci = {k.lower(): v for k, v in ledger.items()}

    fails, passes, skips, unmapped = [], [], [], []

    for fourcc, vt in entities:
        req = VTABLE_REQUIRED_ANCESTOR.get(vt)
        if req is None:
            skips.append((fourcc, vt))
            continue
        cls = fourcc_cls.get(fourcc)
        entry = ledger.get(cls) or (ledger_ci.get(cls.lower()) if cls else None)
        if not entry:
            unmapped.append((fourcc, vt, cls))
            continue
        family, base = entry
        # The class itself or any base-chain ancestor satisfies the requirement.
        chain = cls + " -> " + base
        if any(a in chain for a in req):
            passes.append((fourcc, vt, cls, family))
        else:
            fails.append((fourcc, vt, cls, family, req, base))

    print(f"behavior classification: {len(passes)} ok, {len(fails)} MISMATCH, "
          f"{len(skips)} skipped (structural/cosmetic), {len(unmapped)} unmapped\n")

    for fourcc, vt, cls, fam in passes:
        print(f"  OK    {fourcc}  {vt:<20} {cls}  [{fam}]")
    for fourcc, vt, cls, fam, req, base in fails:
        print(f"  FAIL  {fourcc}  {vt:<20} {cls}  [{fam}]")
        print(f"        expected ancestor one of {req}")
        print(f"        actual base chain: {base[:110]}")
    if verbose:
        for fourcc, vt, cls in unmapped:
            print(f"  UNMAP {fourcc}  {vt:<20} class={cls or '(no FourCC->class)'}")
        for fourcc, vt in skips:
            print(f"  SKIP  {fourcc}  {vt}")

    if fails:
        print(f"\n{len(fails)} behavior mismatch(es) -> FAIL")
        return 1
    print(f"\nall {len(passes)} checked mappings consistent with the decomp base chains")
    return 0


if __name__ == "__main__":
    sys.exit(main())
