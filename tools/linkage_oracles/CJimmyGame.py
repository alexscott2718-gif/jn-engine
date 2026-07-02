#!/usr/bin/env python3
"""Linkage oracle: CJimmyGame `InitGame` mission seed.

Proves (L3) that the native `game_flow_init_game` (src/game/game_flow.c)
reproduces the decompiled `CJimmyGame::InitGame` (Neutron.exe @ 0044d3d0, see
docs/decomp/CJimmyGame.md "Per-Frame Behavior") mission-seed constants exactly:

    mission_counter_a = mission_counter_b = 5     (lives)
    mission_value     = 0x64 (100)
    mission_active    = 1

Compiles and runs the real, unmodified `game_flow_init_game`/`game_flow_lives`/
`game_flow_mission_value` (cjimmygame_dump.c links game_flow.c + its only
non-libc dependency, task_loader.c) and asserts:

  - Pre-seed, the globals read 0 (BSS zero-init) -- proves the post-seed
    values genuinely come from calling InitGame, not a lucky static default.
  - Post-seed: lives==5, mission_value==100 -- the decompiled constants.
  - Re-seeding is idempotent (calling InitGame again still yields 5/100, not
    an accumulation) -- matches the decompiled body unconditionally
    overwriting the fields rather than incrementing them.

Deliberately out of scope (see "Not covered" in docs/decomp/CJimmyGame.md's
Native Linkage section): `game_flow_player_died`'s lives-decrement/death-
restart flow and `game_flow_level_objective_met`'s win latch. Both are native-
port bridging mechanisms (C2DInGameMenu death-path semantics; a gamestate
level-clear signal) with no corresponding decompiled CJimmyGame method in
docs/decomp/CJimmyGame.md's Vtable Methods table -- there is no recovered body
to certify them against, so this oracle only covers the seed itself.

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

# The decompiled InitGame constants (docs/decomp/CJimmyGame.md).
EXPECT_LIVES = 5
EXPECT_MISSION_VALUE = 100


def build_dumper(tmp: Path) -> Path:
    binp = tmp / "cjimmygame_dump"
    src_game = REPO / "src" / "game"
    cmd = [
        "cc", "-O0", "-I", str(src_game),
        str(HERE / "cjimmygame_dump.c"),
        str(src_game / "game_flow.c"),
        str(src_game / "task_loader.c"),
        "-o", str(binp),
    ]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print("MISMATCH: dumper failed to compile\n" + r.stderr)
        sys.exit(1)
    return binp


def main() -> int:
    with tempfile.TemporaryDirectory() as d:
        dumper = build_dumper(Path(d))
        r = subprocess.run([str(dumper)], capture_output=True, text=True)
        if r.returncode != 0:
            print(f"MISMATCH: dumper exited {r.returncode}\n{r.stderr}")
            return 1

        rows = {}
        for line in r.stdout.splitlines():
            parts = line.split("|")
            if len(parts) == 3:
                rows[parts[0]] = (int(parts[1]), int(parts[2]))

        if rows.get("PRE") != (0, 0):
            print(f"MISMATCH: pre-seed globals not zero-initialized: {rows.get('PRE')} "
                  f"-- can't distinguish 'InitGame set this' from 'always was this'")
            return 1

        expected = (EXPECT_LIVES, EXPECT_MISSION_VALUE)
        if rows.get("SEED1") != expected:
            print(f"MISMATCH: post-InitGame (lives, mission_value) = {rows.get('SEED1')} "
                  f"!= decompiled constants {expected}")
            return 1
        if rows.get("SEED2") != expected:
            print(f"MISMATCH: re-seeding InitGame diverged: {rows.get('SEED2')} != {expected} "
                  f"-- expected an idempotent reset, not accumulation")
            return 1

    print(
        f"PASS CJimmyGame/initgame-seed: game_flow_init_game reproduces the "
        f"decompiled CJimmyGame::InitGame (0044d3d0) mission seed exactly "
        f"(lives={EXPECT_LIVES}, mission_value={EXPECT_MISSION_VALUE}), "
        f"confirmed against a zero pre-seed baseline and idempotent re-seed"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
