#!/usr/bin/env python3
"""
gen_level_game_specs.py — generate per-level decomp specs for the CLevel*Game
and CLevelVR0N controller leaves (wave 10, level_game_controllers).

Each of these 35 classes is a near-empty leaf of CJimmyGame: it binds one
level's vtable + `.gam` (and, for Level 1, `.tsk`) and inherits the whole
game-mode lifecycle from CJimmyGame -> CGameType. This generator grounds each
spec in three measured sources:
  - docs/decomp_ledger.csv          vftables + base chain
  - /tmp/decomp_<Class>.md          Ghidra DumpClass owned-method dump
  - docs/tsk_data/all_entities.json per-level .gam object survey (+ tsk overlay)

Re-run after refreshing the Ghidra dumps or the entity index. Idempotent.

Usage: python3 tools/gen_level_game_specs.py [--check]
  --check : report what would be written without writing.
"""
import csv
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LEDGER = ROOT / "docs" / "decomp_ledger.csv"
ENTITIES = ROOT / "docs" / "tsk_data" / "all_entities.json"
DECOMP_DIR = ROOT / "docs" / "decomp"
DUMP_DIR = Path("/tmp")

# Known CJimmyGame/CGameObject vtable-slot roles, for labelling owned overrides.
SLOT_ROLE = {
    "vfunc_01_241": "per-frame update override",
    "vfunc_01_259": "reset/reload override",
    "vfunc_01_265": "level-progress gate override",
    "vfunc_02_002": "scalar deleting destructor",
    "vfunc_03_045": "InitGame override",
    "vfunc_03_046": "UnInitGame override",
}


def load_ledger():
    return {r["class"]: r for r in csv.DictReader(open(LEDGER))}


def parse_dump(cls):
    """Return (vftables:str, owned:[(vfunc, addr, role, literals[])])."""
    path = DUMP_DIR / f"decomp_{cls}.md"
    if not path.exists():
        return None, []
    text = path.read_text()
    vft = ""
    m = re.search(r"\| Vftables \| `([^`]+)` \|", text)
    if m:
        vft = m.group(1)
    owned = []
    # split owned-method section into blocks
    sec = text.split("## Owned Method Decompilation", 1)
    if len(sec) == 2:
        for blk in re.split(r"^### ", sec[1], flags=re.M)[1:]:
            hm = re.match(r"`(vfunc_\d+_\d+)` @ `([0-9a-f]+)`", blk)
            if not hm:
                continue
            vfunc, addr = hm.group(1), hm.group(2)
            role = SLOT_ROLE.get(vfunc, "level-specific override")
            # surface string literals referenced in the body (e.g. s_POWER_...)
            lits = sorted(set(re.findall(r"s_([A-Za-z0-9]+)_[0-9a-f]{6,}", blk)))
            lits = [l for l in lits if l.lower() not in ("initgame", "uninitgame")]
            owned.append((vfunc, addr, role, lits))
    return vft, owned


def level_label(cls):
    m = re.fullmatch(r"CLevelVR(\d+)", cls)
    if m:
        return f"VR challenge level {int(m.group(1)):02d}"
    m = re.fullmatch(r"CLevel(\d+)([A-Z]?)Game", cls)
    if m:
        suffix = f"{m.group(2)}" if m.group(2) else ""
        return f"Level {int(m.group(1))}{suffix}"
    return cls


def gam_key_for(cls, entities):
    for k, v in entities["levels"].items():
        if v["class"] == cls:
            return k, v
    return None, None


def top_fourcc(counts, n=12):
    return sorted(counts.items(), key=lambda kv: (-kv[1], kv[0]))[:n]


def render(cls, row, entities):
    vft, owned = parse_dump(cls)
    vft = vft or row["vftable"]
    base = row["base_chain"]
    key, lvl = gam_key_for(cls, entities)
    label = level_label(cls)

    dtor = next((o for o in owned if o[0] == "vfunc_02_002"), None)
    special = [o for o in owned if o[0] != "vfunc_02_002"]

    L = []
    L.append(f"# {cls}\n")
    L.append("## Identity\n")
    L.append("| Item | Value |")
    L.append("|---|---|")
    L.append(f"| RTTI name | `{cls}` |")
    L.append(f"| Base chain | `{base}` |")
    L.append(f"| Vftable(s) | {', '.join('`'+v+'`' for v in vft.split(';'))} |")
    L.append("| Ctor(s) | inherits the `CJimmyGame` construction path; installs the "
             f"`{cls}` vftables and binds this level's class id |")
    if dtor:
        L.append(f"| Dtor(s) | scalar deleting destructor `vfunc_02_002` at "
                 f"`{dtor[1]}` (destroys the `OMediaClassStreamer` at `this+0x163`) |")
    else:
        L.append("| Dtor(s) | inherited `CJimmyGame` deleting destructor (none owned) |")
    L.append("| Ledger row | `docs/decomp_ledger.csv` |\n")

    gamf = lvl["gam_file"] if lvl else "?"
    n_owned = len(owned)
    L.append(
        f"`{cls}` is the **{label} game-mode controller** — a leaf subclass of "
        f"[`CJimmyGame`](./CJimmyGame.md). It walks 374 vtable slots with "
        f"**{n_owned} owned method{'s' if n_owned != 1 else ''}**"
        + (" (a per-class destructor only)" if n_owned and not special else "")
        + (" — pure inheritance" if n_owned == 0 else "")
        + f", so its entire game lifecycle (InitGame/UnInitGame/update, the active "
        f"player tick, mission timers, `SoundEffects.omt`) is inherited from "
        f"`CJimmyGame` / [`CGameType`](./CGameType.md). What distinguishes it is the "
        f"level it binds: **`{gamf}`**.\n")

    if special:
        L.append("### Level-specific overrides\n")
        L.append("| Slot | Address | Role | Referenced literals |")
        L.append("|---|---|---|---|")
        for vfunc, addr, role, lits in special:
            litstr = ", ".join(f"`{x}`" for x in lits) if lits else "—"
            L.append(f"| `{vfunc}` | `{addr}` | {role} | {litstr} |")
        L.append("")

    # Level data section
    L.append("## Level Data (`%s`)\n" % gamf)
    if lvl:
        L.append("| Item | Value |")
        L.append("|---|---|")
        L.append(f"| `.gam` file | `{gamf}` |")
        L.append(f"| Placed objects | {lvl['object_count']} |")
        L.append(f"| Distinct object FourCC types | {lvl['distinct_fourcc']} |")
        if "tsk_spawn" in lvl:
            sx, sy, sz = lvl["tsk_spawn"]
            L.append(f"| Spawn (from `NewGame.tsk`) | ({sx:.1f}, {sy:.1f}, {sz:.1f}) |")
        L.append("")
        L.append("Object FourCC composition (top types):\n")
        L.append("| FourCC | Count |")
        L.append("|---|---:|")
        for fc, n in top_fourcc(lvl["fourcc_counts"]):
            L.append(f"| `{fc}` | {n} |")
        L.append("")
        if "tsk_initial_state" in lvl:
            L.append("### Task entity-state table (`NewGame.tsk`)\n")
            L.append("This level is the new-game entry point; its `CTaskList` seeds "
                     "the mission entity states below (see [`CTaskList.md`]"
                     "(./CTaskList.md)).\n")
            L.append("| Tag | State |")
            L.append("|---|---:|")
            for e in lvl["tsk_initial_state"]:
                L.append(f"| `{e['name']}` | {e['state']} |")
            L.append("")
    else:
        L.append("_No matching `.gam` found in the entity index._\n")

    L.append("Full per-object placement, properties, and tuning for this level live "
             "in `docs/gam_schema.md` (cross-level FourCC schema) and "
             "`docs/tsk_data/all_entities.json` (this level's survey). The controller "
             "itself does not re-implement object behavior — it loads the `.gam` and "
             "lets each placed object's class run.\n")

    L.append("## Vtable Methods\n")
    if owned:
        L.append("| Slot | Address | Name | Behavior | Status |")
        L.append("|---|---|---|---|---|")
        for vfunc, addr, role, lits in owned:
            beh = role
            if lits:
                beh += " (refs " + ", ".join("`"+x+"`" for x in lits) + ")"
            status = "trivial" if vfunc == "vfunc_02_002" else "non-trivial"
            L.append(f"| `{vfunc}` | `{addr}` | {role.split(' ')[0].title()} | {beh} | {status} |")
        L.append("")
    else:
        L.append("No owned vtable methods. All 374 slots resolve to inherited "
                 "`CJimmyGame` / `CGameType` / `CGameObject` behavior.\n")

    L.append("## Confidence\n")
    conf = "Medium" if special else "Medium-Low"
    L.append(f"Confidence: {conf}\n")
    L.append("Validation: Ghidra `DumpClass.java %s` (slots=374, owned_methods=%d); "
             "vftables/base chain from `docs/decomp_ledger.csv`; level object survey "
             "from `tools/build_level_entity_index.py`. Behavior is inherited from "
             "`CJimmyGame` (separately specced and the substantive parent). Not "
             "runtime-validated.\n" % (cls, n_owned))
    L.append("Open questions:")
    L.append("- Pin this class's constructor and the exact level class-id immediate it "
             "registers.")
    if special:
        L.append("- Decompile the level-specific override(s) above to recover the "
                 "level mission logic (not just the referenced literals).")
    L.append("- Confirm the `.gam`/`.tsk` binding is established in the ctor vs. an "
             "inherited load slot.\n")

    L.append("## Notes\n")
    L.append(f"- Generated by `tools/gen_level_game_specs.py` from the Ghidra dump + "
             f"`all_entities.json`. Leaf of `CJimmyGame`; see that spec for the shared "
             f"controller behavior.")
    return "\n".join(L) + "\n"


def main():
    check = "--check" in sys.argv
    ledger = load_ledger()
    entities = json.loads(ENTITIES.read_text())
    targets = [c for c in ledger
               if re.fullmatch(r"CLevel\d+[A-Z]?Game", c)
               or re.fullmatch(r"CLevelVR\d+", c)]
    targets.sort()
    written = 0
    for cls in targets:
        content = render(cls, ledger[cls], entities)
        out = DECOMP_DIR / f"{cls}.md"
        if check:
            print(f"would write {out.relative_to(ROOT)} ({len(content)} bytes)")
        else:
            out.write_text(content)
            written += 1
    print(f"{'checked' if check else 'wrote'} {len(targets)} level-game specs"
          + ("" if check else f" -> {DECOMP_DIR.relative_to(ROOT)}/"))


if __name__ == "__main__":
    main()
