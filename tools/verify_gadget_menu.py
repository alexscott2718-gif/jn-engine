#!/usr/bin/env python3
"""Runtime evidence for the gadget inventory and the action-menu dispatch.

Two independent things get checked, both against evidence held here rather than
against the engine's own tables, so a transcription slip in either place shows
up as a disagreement:

  1. The AMI dispatch table. SelectJimmyGadgetOrVRMode (00428d50) is a switch
     over the request id; every arm writes DAT_004f0588 and, in VR mode, routes
     to a vrNN.gam through PHONEBOOTH. JN_TEST_AMI drives all nine ids through
     the real ami_dispatch() and prints the result; this file holds the
     expected mapping read off docs/decomp/evidence/c3djimmy_target6.md.

  2. The gadget grants. The GADGET_GRANTS table claims certain .gam ObjectTags
     put something in the inventory. This re-derives which levels place those
     tags straight from assets/gam, runs each of those levels through the
     JN_TEST_PICTURES sweep, and checks the inventory lines that come out --
     including the negative that wrench1/wrench2 grant nothing, since the old
     speculative table treated the wrench as a permanent tool when the corpus
     says it is a picture spent at the hydrant.

Not wired into `make check`: it needs the asset tree and a built binary.

  python3 tools/verify_gadget_menu.py
"""

from __future__ import annotations

import os
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from gam_parser import parse_gam       # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "jnengine"
GAM_DIR = ROOT / "assets" / "gam"

# --- expected AMI table, read off the recovered switch ----------------------
# id -> (mode written to DAT_004f0588, VR level routed to)
# None means the arm writes no mode at all.
#   case 3 is the default arm (shared with -1) and writes no mode.
#   case 8 writes no mode and has no route.
#   cases 0 and 2 each write one of two modes on a branch whose condition was
#   not recovered; the engine takes the primary, which is what is asserted.
AMI_EXPECTED = {
    0: (0,    "vr01"),
    1: (1,    "vr02"),
    2: (2,    "vr03"),
    3: (None, "vr04"),
    4: (4,    "vr05"),
    5: (5,    "vr06"),
    6: (6,    "vr07"),
    7: (7,    "vr08"),
    8: (None, None),
}

# --- expected grants, mirroring GADGET_GRANTS ------------------------------
# .gam ObjectTag -> (inventory tag, kind, artist's canvas name for its sprite)
#
# The third column is the point. The level designer's ObjectTag and the
# artist's canvas name disagree for every row here, and for three of them they
# disagree about what the object *is* -- level1b's "shrinkray" row draws the
# canvas named "Jetpack 1". Pinning both means the next person to read this
# cannot quietly assume they agree, which is the mistake this table already
# shipped once.
GRANTS = {
    "shrinkray":    ("jetpack",      "gadget",      "Jetpack 1"),
    "bubblepickup": ("bubble",       "gadget",      "bubshadw"),
    "invisibility": ("invisibility", "part+gadget", "yokpart"),
    "scooterpart":  ("scooterpart",  "part",        "wheel"),
    "sewerpart":    ("sewerpart",    "part",        "CompPart"),
    "foil":         ("foil",         "part",        "foil"),
    "godphone":     ("godphone",     "part",        "phone"),
}

# Tags that must NOT produce an inventory line. wrench1/wrench2 award
# PIC_NUMBER 18 and are consumed by hydrant/water2 in the same level, so they
# are economy items; the table they replaced modelled them as a tool.
NO_GRANT = ("wrench1", "wrench2", "passcard", "water2", "hydrant")

INV = re.compile(r"\[INVENTORY\] \+(part\+gadget|gadget|part) '([^']+)'")
DUMP = re.compile(r"\[INVDUMP\] slot=\d+ tag='([^']*)' sprite=(-?\d+) art='([^']*)'")
AMIT = re.compile(r"\[AMITABLE\] id=(\d+) mode=(-?\d+) changed=(\d+) vr=(\S+)")


def run(level, ami=False, frames=4):
    env = dict(os.environ, JN_TEST_PICTURES="1")
    if ami:
        env["JN_TEST_AMI"] = "1"
    cmd = [str(ENGINE), "--level", level, "--headless",
           "--frames", str(frames), "--seed", "7"]
    p = subprocess.run(cmd, cwd=ROOT, env=env, text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       timeout=180)
    return p.returncode, p.stdout


def levels_placing(tags):
    """Which levels place each tag, straight from the .gam corpus."""
    where = {t: set() for t in tags}
    active = {}
    for fn in sorted(os.listdir(GAM_DIR)):
        if not fn.lower().endswith(".gam"):
            continue
        lvl = fn[:-4].lower()
        for o in parse_gam(str(GAM_DIR / fn))["objects"]:
            if o["type"] != "3PIC":
                continue
            p = o.get("properties", {})
            tag = str(p.get("ObjectTag", "")).lower()
            if tag in where:
                where[tag].add(lvl)
                # InitallyActive=0 rows start gated and are not collectable by
                # a cold sweep, so they are expected to stay out of the
                # inventory -- see foil and refill in level1b.
                active[(lvl, tag)] = int(p.get("InitallyActive", 1)) != 0
    return where, active


def main():
    if not ENGINE.exists():
        print(f"verify SKIP: no engine at {ENGINE} (run make)")
        return 0

    fails = []

    # --- 1. AMI table -----------------------------------------------------
    rc, out = run("level1", ami=True, frames=2)
    seen = {int(m.group(1)): (int(m.group(2)), int(m.group(3)), m.group(4))
            for m in AMIT.finditer(out)}
    if not seen:
        fails.append("AMI: JN_TEST_AMI produced no [AMITABLE] lines")
    for aid, (want_mode, want_vr) in sorted(AMI_EXPECTED.items()):
        if aid not in seen:
            fails.append(f"AMI id={aid}: no line")
            continue
        mode, changed, vr = seen[aid]
        if want_mode is None:
            if changed:
                fails.append(f"AMI id={aid}: wrote mode {mode}, "
                             f"evidence says this arm writes none")
        elif mode != want_mode:
            fails.append(f"AMI id={aid}: mode {mode}, evidence says {want_mode}")
        got_vr = None if vr == "-" else vr
        if got_vr != want_vr:
            fails.append(f"AMI id={aid}: vr {got_vr}, evidence says {want_vr}")
    print(f"AMI table: {len(seen)} ids dispatched")

    # --- 2. grants --------------------------------------------------------
    where, active = levels_placing(list(GRANTS) + list(NO_GRANT))

    for tag, lvls in sorted(where.items()):
        if not lvls:
            fails.append(f"corpus: no level places '{tag}' -- table is stale")

    checked = 0
    for lvl in sorted({l for t in GRANTS for l in where[t]}):
        rc, out = run(lvl)
        if rc != 0:
            fails.append(f"{lvl}: engine exit {rc}")
            continue
        got = {(t, k) for k, t in INV.findall(out)}
        got_tags = {t for t, _ in got}

        art = {t: a for t, _s, a in DUMP.findall(out)}

        for tag, (inv_tag, kind, want_art) in GRANTS.items():
            if lvl not in where[tag]:
                continue
            checked += 1
            if not active.get((lvl, tag), True):
                if inv_tag in got_tags:
                    fails.append(f"{lvl}: '{tag}' is InitallyActive=0 but "
                                 f"granted '{inv_tag}' anyway")
                continue
            if (inv_tag, kind) not in got:
                fails.append(f"{lvl}: expected {kind} '{inv_tag}' from '{tag}', "
                             f"got {sorted(got) or 'nothing'}")
            # Only gadgets are dumped (parts are not menu-selectable), so the
            # art assertion applies to those.
            if "gadget" in kind and inv_tag in art and art[inv_tag] != want_art:
                fails.append(f"{lvl}: '{inv_tag}' draws canvas "
                             f"'{art[inv_tag]}', expected '{want_art}' -- the "
                             f"sprite map moved under the table")

        for tag in NO_GRANT:
            if lvl in where[tag] and tag in got_tags:
                fails.append(f"{lvl}: '{tag}' granted an inventory slot; "
                             f"it is a picture-economy row, not a gadget")
        print(f"  {lvl:9s} {sorted(got_tags) or '-'}")

    print(f"grants: {checked} tag/level pairs checked")

    if fails:
        print("\nverify FAIL")
        for f in fails:
            print("  " + f)
        return 1
    print("\nverify PASS: AMI dispatch table and gadget grants")
    return 0


if __name__ == "__main__":
    sys.exit(main())
