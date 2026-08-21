#!/usr/bin/env python3
"""CLoadLevel RETURN round-trip: the menu takes you into a VR level, the VR
level's RETURN portal brings you back.

Ten of the corpus's 97 `LOAD` rows author `LevelName = "RETURN"`, and every one
of them is in VR01..VR08. Nothing in the corpus loads a VR level through a
portal, so the departure point those rows read back is recorded by the only
route in: the main menu. That makes the wiring cross three subsystems --
`menu.c`'s routing table, `gamestate_request_level_swap()` promoting the current
level entry to the departure pair, that pair surviving the swap's world rebuild
and `gamestate_reset_for_new_level()`, and `behavior_load.c` reading it back --
and it is the half of the CLoadLevel port that is *inferred* from the recovered
handoff's call shape rather than transcribed from a recovered body. So it gets
a real end-to-end check rather than a unit one.

The engine does the work behind `JN_TEST_RETURN=<level>`: open the real
CMainMenu, land the selection on that level's item, let the main loop's confirm
path take it, and when the swap lands, fire the RETURN portal there and compare
what it asks the loader for against the level the menu left from.

`--selftest` is a **negative control of this script**, not a mutation of the
engine: it asks for a level the routing table has no item for, and requires
this script to report the failure. The engine-side sensitivity was demonstrated
by deleting the entry-to-departure promotion in `gamestate_request_level_swap`,
which turns the round-trip red ("no recorded departure point"); that mutation is
recorded in the commit that added this check rather than run here, because it
needs a rebuild.

Exit 0 + a PASS line, or non-zero + the engine's output.
"""
from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "jnengine"

FROM_LEVEL = "level1b"     # NewGame's start level, and a plausible VR entry point
INTO_LEVEL = "vr01"        # 2 RETURN rows; VR01..VR08 all have at least one


def run(into: str, frm: str) -> tuple[int, str]:
    env = dict(os.environ)
    env["JN_TEST_RETURN"] = into
    # The seam is the point; make sure no other JN_TEST_* hook is armed.
    env.pop("JN_TEST_LOAD", None)
    env.pop("JN_TEST_LOAD_RETURN", None)
    r = subprocess.run(
        [str(ENGINE), "--level", frm, "--headless"],
        cwd=str(ROOT), env=env, capture_output=True, text=True, timeout=300,
    )
    return r.returncode, r.stdout + r.stderr


def check(into: str, frm: str) -> tuple[bool, str]:
    if not ENGINE.exists():
        return False, f"engine not built: {ENGINE}"
    code, out = run(into, frm)
    line = next((l for l in out.splitlines()
                 if l.startswith("[JN_TEST_RETURN]") and
                 (" PASS:" in l or " FAIL:" in l)), "")
    if code != 0 or " PASS:" not in line:
        tail = "\n      ".join(out.strip().splitlines()[-8:])
        return False, f"exit {code}\n      {tail}"
    if f"asks for '{frm}'" not in line:
        return False, f"unexpected destination:\n      {line}"
    return True, line


def main(argv) -> int:
    selftest = "--selftest" in argv
    ok, detail = check(INTO_LEVEL, FROM_LEVEL)
    if not ok:
        print("return round-trip FAILED: " + detail)
        return 1

    if selftest:
        # Negative control: a level the routing table cannot reach must be
        # reported as a failure, not quietly passed over.
        bad_ok, _ = check("level2", FROM_LEVEL)
        if bad_ok:
            print("selftest FAIL: an unroutable level was reported as a pass")
            return 1
        print("return round-trip PASS: %s -> %s -> %s through the real menu "
              "route; selftest rejects an unroutable level"
              % (FROM_LEVEL, INTO_LEVEL, FROM_LEVEL))
        return 0

    print("return round-trip PASS: %s -> %s -> %s through the real menu route"
          % (FROM_LEVEL, INTO_LEVEL, FROM_LEVEL))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
