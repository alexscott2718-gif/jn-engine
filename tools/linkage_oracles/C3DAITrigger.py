#!/usr/bin/env python3
"""Linkage oracle: C3DAITrigger dispatch graph (target mutation + story-progress).

Proves (L3) that the native `aitrig_activate` / `aitrig_apply_story_progress`
(src/game/behaviors/behavior_ai_trigger.c) reproduce the two fully-decompiled,
deterministic pieces of `ActivateAITrigger` (Neutron.exe @ `0040c300`,
docs/decomp/C3DAITrigger.md) that don't depend on the deferred subsystems
(the general C3DAI AIState/AISpeed combat state machine, AIAnim/
ActivateByAnim animation selection, the ActivateObject0..4 state-gated
next-object machine, and the reward-flag/counter-popup/story-screen side
effects):

  1. Target mutation + dispatch: `AIHideObj` show/hide, `AINewPos` teleport,
     `AINewRotY`, `AIPatrol` repoint, then `ToggleObject` and `NextTrigger`
     forwarding, and `trigger_count` bookkeeping.
  2. `ApplyAITriggerStoryProgress` (`FUN_0040caa0`, docs/decomp/
     _scene_sequencer.md): the hardcoded `ObjectTag` x current-`SCENE` ->
     new-`SCENE` patch table, including the *gate* (a wrong current SCENE
     must NOT write).

Compiles and runs the real, unmodified `behavior_ai_trigger_fire_tag` (a test
hook already in the source specifically for headless exercise of the
activate-and-mutate core, bypassing the touch/arm/activator gates) via
`c3daitrigger_dump.c`, over 9 synthetic target-mutation scenarios and 7
story-progress scenarios (both real documented `ObjectTag`s and both a gate
hit and a gate miss per tag, plus the `KITTY`-family special case), and diffs
every observable field against expected values independently computed from
the recovered bodies.

See docs/linked_parity_plan.md for the Linkage Certificate. Exit 0 + PASS on
faithful reproduction; non-zero + a diff on any mismatch.
"""
from __future__ import annotations

import math
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]


def f32_bits(x: float) -> int:
    return struct.unpack("<I", struct.pack("<f", float(x)))[0]


def build_dumper(tmp: Path) -> Path:
    binp = tmp / "c3daitrig_dump"
    src_game = REPO / "src" / "game"
    src_engine = REPO / "src" / "engine"
    cmd = [
        "cc", "-O0",
        str(HERE / "c3daitrigger_dump.c"),
        str(src_game / "behaviors" / "behavior_ai_trigger.c"),
        str(src_game / "game_flow.c"),
        str(src_game / "task_loader.c"),
        str(src_engine / "assets" / "gam_loader.c"),
        str(src_engine / "player_physics.c"),
        "-lm", "-o", str(binp),
    ]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print("MISMATCH: dumper failed to compile\n" + r.stderr)
        sys.exit(1)
    return binp


# ---- expected results, independently derived from the recovered bodies ----

EXPECT_S = {
    "hide": (0, 0),                                                  # visible, runtime_flags
    "show": (1,),
    "hidenoop": (1,),
    "newpos": (f32_bits(111.5), f32_bits(222.25), f32_bits(-333.75)),
    "rot": (f32_bits(90.0 * math.pi / 180.0),),
    "rotnoop": (f32_bits(1.2345),),
    "patrol": ("NEWNODE", "NEWNODE"),
    "dispatch": (1, 1, 1, 1),                                        # toggle_fired, next_fired, fired->user_flag, trig->user_flag
    "count": (3,),
}

# ApplyAITriggerStoryProgress (docs/decomp/_scene_sequencer.md): gate SCENE ==
# seed -> new SCENE; anything else -> unchanged.
EXPECT_T = {
    "teleportexplanation_hit": 0x23,
    "teleportexplanation_hit_altgate": 0x23,
    "teleportexplanation_miss": 0x99,
    "CARLOUT_hit": 0x4b,
    "CARLOUT_miss": 0x99,
    "GIVEKEY_hit": 0xd2,
    "KITEND1": 10,
}


def main() -> int:
    with tempfile.TemporaryDirectory() as d:
        dumper = build_dumper(Path(d))
        r = subprocess.run([str(dumper)], capture_output=True, text=True)
        if r.returncode != 0:
            print(f"MISMATCH: dumper exited {r.returncode}\n{r.stderr}")
            return 1

        got_s: dict[str, tuple] = {}
        got_t: dict[str, int] = {}
        for line in r.stdout.splitlines():
            parts = line.split("|")
            if parts[0] == "S":
                label = parts[1]
                vals = parts[2:]
                # hex fields vs plain-int fields: hide/show/patrol/dispatch/count
                # mix types; parse per-scenario against its known field shape.
                if label in ("hide",):
                    got_s[label] = (int(vals[0]), int(vals[1], 16))
                elif label in ("newpos", "rot", "rotnoop"):
                    got_s[label] = tuple(int(v, 16) for v in vals)
                elif label in ("show", "hidenoop"):
                    got_s[label] = (int(vals[0]),)
                elif label == "patrol":
                    got_s[label] = (vals[0], vals[1])
                elif label in ("dispatch", "count"):
                    got_s[label] = tuple(int(v) for v in vals)
            elif parts[0] == "T":
                got_t[parts[1]] = int(parts[2])

        for label, expected in EXPECT_S.items():
            if got_s.get(label) != expected:
                print(f"MISMATCH in mutation scenario {label!r}: native={got_s.get(label)} expected={expected}")
                return 1

        for label, expected in EXPECT_T.items():
            if got_t.get(label) != expected:
                print(f"MISMATCH in story-progress scenario {label!r}: native SCENE={got_t.get(label)} expected={expected:#x}")
                return 1

    print(
        f"PASS C3DAITrigger/dispatch-graph: aitrig_activate byte/value-exact on "
        f"{len(EXPECT_S)} target-mutation scenarios (hide/show/AINewPos/AINewRotY/"
        f"AIPatrol/ToggleObject/NextTrigger/trigger_count) and "
        f"aitrig_apply_story_progress byte-exact on {len(EXPECT_T)} SCENE "
        f"story-progress scenarios (gate hit, gate miss, and the KITTY special "
        f"case) against the recovered ActivateAITrigger/ApplyAITriggerStoryProgress bodies"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
