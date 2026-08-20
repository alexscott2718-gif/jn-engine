#!/usr/bin/env python3
"""Linkage oracle: C3DPickupItem collection.

Proves (L3) that the native pickup path — `item_on_trigger` /
`item_on_spawn` (src/game/behaviors/behavior_item.c) over
`behavior_pickup_core.c` and the `gamestate` picture store — reproduces the
recovered `C3DPickupItem::HandlePickupCollection` (Neutron.exe @ `00435ce0`),
its gate `CheckRequiredPicAndConsume` (vtable 3 slot 54, `00436830`), and the
load-time half of `PostLoadPickupItem` (`00436200`), over **every real shipped
`3PIC` row** (383 instances across the 35 levels in `assets/gam/*.gam`).

The recovered body, from docs/decomp/C3DPickupItem.md:

    if toucher != global_player          return
    if !CheckRequiredPicAndConsume()     return        <- gate, and it runs
    if PickupIndex > 0 and state != 0    return           BEFORE this check
    update state + visibility
    fire_tag(ActivateObject, Toggle)                   <- BEFORE the award
    fire_tag(ToggleObject,  Toggle)
    award PIC_NUMBER + score
    fire_next_trigger(NextTrigger)                     <- AFTER it
    play pickup sound

Four things are checked per row, all derived from the row's own authored
properties, never from a tuned constant:

  1. **Load gate.** After the real `on_spawn`, a row authoring
     `InitallyActive=0` is latched uncollectible and one authoring 1 is not.
  2. **Order and effects, funded.** Seed exactly `ReqPicNumAmount` of
     `RequiredPicNum`, touch the row, and diff the whole ordered event
     sequence — gate, each state dispatch, the award, the next-trigger, the
     sound — against the sequence the decompiled order predicts.
  3. **Refusal.** Seed one short. The gate must refuse, play `NeedMoreSound`
     when one is authored, emit nothing else, and leave the currency untouched
     (no partial consume).
  4. **Gate before collected-check.** Mark the row collected, fund it, touch it
     again: the currency must still be taken, because the gate runs first. A
     port that reordered those two would leave the count untouched here and is
     rejected.

`--selftest` mutation-tests the oracle itself against three defects it must
catch: swapping the gate and collected-check, moving the award ahead of the
side-effect dispatch, and consuming on a refusal.

**Not covered** (unported or unrecovered, see the class doc's Native Linkage
section): `PickedUpIndex`'s replacement-sprite swap (native hides instead);
`TimesToTrigger` / `trigger_count` repeat limiting (native latches once-only);
`IsAmbient` / `UpdateAmbientPickupSound`; `PassThru` and `ShowArrow`, whose
consumers are not isolated in the decomp either; the state slot on every class
except the pickup family, so `ActivateObject`/`ToggleObject` targets that are
not `3PIC` and every `NextTrigger` target resolve but find no native body; and
the sound *mix* (only its position in the sequence is certified). The
`3FIS`/`3GIR`/`3DIN` creature leaf is a different FourCC and out of scope.

See docs/linked_parity_plan.md for the Linkage Certificate. Exit 0 + PASS on
faithful reproduction; non-zero + a diff on any mismatch.
"""
from __future__ import annotations

import re
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
EXPECTED_ROWS = 383      # shipped 3PIC instances; a corpus change must re-verify

SRC = REPO / "src"
BEHAVIOR_ITEM = SRC / "game/behaviors/behavior_item.c"
PICKUP_CORE = SRC / "game/behaviors/behavior_pickup_core.c"

SOURCES = [
    BEHAVIOR_ITEM,
    PICKUP_CORE,
    SRC / "game/behaviors/behavior_ai_trigger.c",   # shared tag dispatch
    SRC / "game/gamestate.c",
    SRC / "game/camera_record.c",
    SRC / "game/game_flow.c",
    SRC / "game/task_loader.c",
    SRC / "engine/assets/asset_paths.c",
    SRC / "engine/assets/gam_loader.c",
    SRC / "engine/player_physics.c",
]

GATE = re.compile(r"\[PICGATE\] .*need=(\d+) amount=(\d+).*-> (ok|REFUSED)")
STATE = re.compile(r"\[PICSTATE\] .*index=(-?\d+) .*-> state (-?\d+)")
FIRE = re.compile(r"\[PICFIRE\] .*kind=(\S+) tag='([^']*)' target=\S+ outcome=(\S+)")
AWARD = re.compile(r"\[PICAWARD\] .*id=(\d+)")


def build_dumper(tmp: Path, replace: dict[Path, Path] | None = None) -> Path:
    replace = replace or {}
    binp = tmp / "c3dpickup_dump"
    cmd = ["cc", "-O0", str(HERE / "c3dpickupitem_dump.c")]
    cmd += [str(replace.get(s, s)) for s in SOURCES]
    cmd += ["-lm", "-o", str(binp)]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        return None, "dumper failed to compile\n" + r.stderr
    return binp, ""


def run_dumper(dumper: Path, gam_path: Path, level: str):
    r = subprocess.run([str(dumper), str(gam_path), level],
                       capture_output=True, text=True)
    if r.returncode != 0:
        return None, f"dumper failed on {gam_path.name}\n{r.stderr}"
    return r.stdout, ""


def parse(stdout: str) -> dict:
    """-> {file_index: {"spawn":..., "events":[...], "points":n, "state":..,
                        "refuse":[...], "refuse_count":n, "taken_count":n}}"""
    rows, cur, mode = {}, None, None
    for line in stdout.splitlines():
        if line.startswith("R|"):
            _, idx, index = line.split("|")
            cur = rows.setdefault(int(idx), {"index": int(index), "events": [],
                                             "refuse": [], "points": None,
                                             "state": None, "spawn": None,
                                             "refuse_count": None,
                                             "taken_count": None})
            mode = "spawn"
            continue
        if line.startswith("R1|"):
            cur = rows[int(line.split("|")[1])]
            mode = "events"
            continue
        if line.startswith("R2|"):
            cur = rows[int(line.split("|")[1])]
            mode = "refuse"
            continue
        if line.startswith("R3|"):
            cur = rows[int(line.split("|")[1])]
            mode = "taken"
            continue
        if cur is None:
            continue
        if line.startswith("B|"):
            cur["spawn"] = tuple(int(v) for v in line.split("|")[1:])
            continue
        if line.startswith("P|"):
            cur["points"] = int(line.split("|")[1])
            continue
        if line.startswith("E|"):
            if mode == "events":
                cur["state"] = tuple(int(v) for v in line.split("|")[1:])
            continue
        if line.startswith("C2|"):
            cur["refuse_count"] = int(line.split("|")[1])
            continue
        if line.startswith("C3|"):
            cur["taken_count"] = int(line.split("|")[1])
            continue

        bucket = cur["events"] if mode == "events" else (
            cur["refuse"] if mode == "refuse" else None)
        if bucket is None:
            continue
        m = GATE.search(line)
        if m:
            bucket.append(("gate", int(m.group(1)), int(m.group(2)),
                           m.group(3) == "ok"))
            continue
        m = STATE.search(line)
        if m:
            bucket.append(("setstate", int(m.group(1)), int(m.group(2))))
            continue
        m = FIRE.search(line)
        if m:
            bucket.append(("fire", m.group(1), m.group(2), m.group(3)))
            continue
        m = AWARD.search(line)
        if m:
            bucket.append(("award", int(m.group(1))))
            continue
        if line.startswith("N|"):
            bucket.append(("sound", int(line.split("|")[1])))
        elif line.startswith("M|"):
            bucket.append(("needmore", int(line.split("|")[1])))
    return rows


def _int(props, name, default):
    v = props.get(name, default)
    return v if isinstance(v, int) else default


def _tag(props, name):
    v = props.get(name, "none")
    if not isinstance(v, str) or v.strip().lower() in ("", "none"):
        return None
    return v


def reference(gam_path: Path) -> dict:
    """Expected results, computed from the recovered body + the row's props."""
    objs = gam_parser.parse_gam(str(gam_path))["objects"]

    # behavior_trigger_find_by_tag walks the world list, which gam_load builds
    # push-front, so the LAST authored row with a tag wins a lookup.
    by_tag = {}
    for obj in objs:
        tag = obj["properties"].get("ObjectTag")
        if isinstance(tag, str) and tag.strip().lower() not in ("", "none"):
            by_tag[tag.casefold()] = obj

    out = {}
    for idx, obj in enumerate(objs):
        if obj["type"] != "3PIC":
            continue
        p = obj["properties"]
        index = _int(p, "PickupIndex", -1)
        need = _int(p, "RequiredPicNum", -1)
        amount = _int(p, "ReqPicNumAmount", 1)
        if amount < 1:
            amount = 1
        pic = _int(p, "PIC_NUMBER", -1)
        toggle = _int(p, "Toggle", -1)
        snd = _int(p, "SoundIndex", -1)
        needmore = _int(p, "NeedMoreSound", -1)
        points = _int(p, "PointValue", 0)
        initally_active = _int(p, "InitallyActive", 1)

        events = []
        if need >= 0:
            events.append(("gate", need, amount, True))
        # ActivateObject then ToggleObject, each through the target's state slot
        for kind in ("ActivateObject", "ToggleObject"):
            tag = _tag(p, kind)
            if tag is None:
                continue
            target = by_tag.get(tag.casefold())
            if target is None:
                events.append(("fire", kind, tag, "unresolved"))
                continue
            # only the pickup family has a native state slot
            if target["type"] == "3PIC":
                if toggle in (0, 1):
                    events.append(("setstate",
                                   _int(target["properties"], "PickupIndex", -1),
                                   toggle))
                events.append(("fire", kind, tag, "fired"))
            else:
                events.append(("fire", kind, tag, "no-native-slot"))
        if pic >= 0:
            events.append(("award", pic))
        tag = _tag(p, "NextTrigger")
        if tag is not None:
            target = by_tag.get(tag.casefold())
            if target is None:
                events.append(("fire", "NextTrigger", tag, "unresolved"))
            elif target["type"] == "3PIC":
                events.append(("fire", "NextTrigger", tag, "fired"))
            else:
                events.append(("fire", "NextTrigger", tag, "no-native-slot"))
        if snd >= 0:
            events.append(("sound", snd))

        refuse = []
        if need >= 0:
            refuse.append(("gate", need, amount, False))
            if needmore >= 0:
                refuse.append(("needmore", needmore))

        out[idx] = {
            "index": index,
            "spawn": (1 if initally_active == 0 else 0, 1, initally_active),
            "events": events,
            "points": points if points > 0 else 0,
            # collected: latched, hidden, dead, and recorded when index > 0
            "state": (1, 0, 0, 1 if index > 0 else 0),
            "refuse": refuse,
            # one short: amount-1 held, nothing consumed
            "refuse_count": (amount - 1) if need >= 0 else None,
            # gate ran first, so the currency is gone even though it was taken
            "taken_count": 0 if (need >= 0 and index > 0) else None,
        }
    return out


def compare(native: dict, ref: dict, level: str) -> list[str]:
    problems = []
    if set(native) != set(ref):
        problems.append(f"{level}: row sets differ "
                        f"(native {len(native)}, reference {len(ref)})")
        return problems
    for idx in sorted(ref):
        n, r = native[idx], ref[idx]
        for field in ("index", "spawn", "events", "points", "state",
                      "refuse", "refuse_count", "taken_count"):
            if n.get(field) != r.get(field):
                problems.append(
                    f"{level} row {idx} (PickupIndex {r['index']}): {field}\n"
                    f"      native   = {n.get(field)}\n"
                    f"      expected = {r.get(field)}")
    return problems


def run_all(replace=None):
    gam_files = sorted(GAM_DIR.glob("*.gam"))
    if not gam_files:
        return None, "no .gam files found under assets/gam/"
    with tempfile.TemporaryDirectory() as d:
        tmp = Path(d)
        dumper, err = build_dumper(tmp, replace)
        if dumper is None:
            return None, err
        total = 0
        problems = []
        for gam_path in gam_files:
            level = gam_path.stem.lower()
            stdout, err = run_dumper(dumper, gam_path, level)
            if stdout is None:
                return None, err
            native = parse(stdout)
            ref = reference(gam_path)
            problems += compare(native, ref, level)
            total += len(ref)
            if problems:
                return None, "\n".join(problems[:6])
    return total, ""


MUTANTS = [
    ("gate and collected-check swapped", BEHAVIOR_ITEM,
     "    if (!behavior_pickup_gate_allows(e)) return;\n"
     "    if (behavior_pickup_taken(e)) return;",
     "    if (behavior_pickup_taken(e)) return;\n"
     "    if (!behavior_pickup_gate_allows(e)) return;"),
    ("award moved ahead of the side-effect dispatch", BEHAVIOR_ITEM,
     "    behavior_pickup_dispatch_state(e);\n\n"
     "    behavior_pickup_award_pictures(e);",
     "    behavior_pickup_award_pictures(e);\n\n"
     "    behavior_pickup_dispatch_state(e);"),
    ("refusal consumes anyway", PICKUP_CORE,
     "    if (gamestate_pic_consume(required, amount)) {",
     "    if (gamestate_pic_consume(required, amount) || "
     "gamestate_pic_consume(required, amount - 1)) {"),
]


def selftest() -> int:
    for label, source, needle, replacement in MUTANTS:
        text = source.read_text()
        if text.count(needle) != 1:
            print(f"selftest FAIL: mutation anchor missing for {label!r}")
            return 1
        with tempfile.TemporaryDirectory() as d:
            mutant = Path(d) / source.name
            mutant.write_text(text.replace(needle, replacement))
            total, err = run_all({source: mutant})
        if total is not None:
            print(f"selftest FAIL: mutant survived -- {label}")
            return 1
    print(f"selftest PASS: oracle rejects {len(MUTANTS)} ordering/consume mutants")
    return 0


def main(argv) -> int:
    if "--selftest" in argv:
        return selftest()
    total, err = run_all()
    if total is None:
        print("MISMATCH: " + err)
        return 1
    if total != EXPECTED_ROWS:
        print(f"MISMATCH: expected {EXPECTED_ROWS} shipped 3PIC rows, found "
              f"{total} -- corpus changed, re-verify this oracle's coverage")
        return 1
    print(
        f"PASS C3DPickupItem/collection: item_on_trigger reproduces "
        f"HandlePickupCollection's order and effects -- gate before the "
        f"collected-state check, ActivateObject/ToggleObject state dispatch "
        f"before the PIC_NUMBER and score award, NextTrigger after it, sound "
        f"last -- plus CheckRequiredPicAndConsume's consume-or-refuse "
        f"arithmetic and PostLoadPickupItem's InitallyActive gate, across all "
        f"{total} shipped 3PIC rows in 35 levels"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
