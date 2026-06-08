# TSK + Menu Session Plan

*Created 2026-06-08. One-shot plan for a Claude session to fix the tsk parser,
extract all .tsk and menu.dat data, and spec the wave-8/10 level_game_controllers.*

---

## What this session does

Extracts two untapped data sources (`NewGame.tsk` and `menu.dat`) that directly
enable specs for ~34 of the remaining 122 `todo` classes, mostly the
`level_game_controllers` family (wave 10) plus `CTaskList` (wave 8).

All other waves (6 mechanisms, 7 world props, 8 effects, 9 creatures) are
.gam-driven and do not need this session — they can be specced from Ghidra +
`docs/gam_schema.md` alone.

---

## Step 1 — Fix `tools/tsk_parser.py`

The parser has a bug in its entity-list search. The `search_start` calculation
aligns to a 0x80 boundary *after* `se_pos + 0x80`, which pushes past EOF on
the 17 KB `NewGame.tsk`. The entity list actually starts directly after the
four pstrings + 12-byte float triple of the STARTEXP record — no padding.

**Known format (measured 2026-06-08):**

```
[4B]  magic 'LV1B'
[1B]  version  (0x41 for NewGame.tsk)
[1B]  variant  (0xa0)
[~16 KB of zero-filled lookup table]
[STARTEXP record at offset 0x4226]:
  pstring  task_name   e.g. 'STARTEXP'
  pstring  param1      e.g. 'none'
  pstring  gam_file    e.g. 'level1b.gam'
  pstring  param2      e.g. 'none'
  3× BE float          spawn X, Y, Z
[entity state list — immediately follows spawn floats, NO alignment/padding]:
  repeat until EOF:
    pstring  object_tag   e.g. 'MOM', 'LIBBY', 'REACTOR'
    u32 BE   state        0 = default/inactive; non-zero = custom initial state
```

There is **no count byte** before the entity list. Read pstring+u32 pairs until EOF.

**Fix**: replace the `search_start` alignment block with a sequential read.
After parsing spawn floats, `pos` is sitting right at the first entity record.
Read `(pstring, u32 BE)` pairs until `pos >= len(data)`. If a pstring length
byte is 0, stop (end sentinel or padding).

**Entities in NewGame.tsk** (Level 1 initial state, ground-truth):
```
MOM        state=0
LIBBY      state=0
BENNY      state=0
SCENE      state=0x1e  (30)
DINO       state=0
CLONE      state=0
KITTY1     state=0
KITTY2     state=0
KITTY3     state=0
HYDRANT1   state=0
HYDRANT2   state=0
REACTOR    state=0
```

After fixing, add `--json` flag: `tools/tsk_parser.py FILE --json` writes
`FILE.json` alongside the input.

---

## Step 2 — Extract all .tsk files

```bash
# JNBG new-game template (Level 1 only, 12 entities)
python3 tools/tsk_parser.py ~/xp-jnbg-original/NewGame.tsk --json

# JNvsJN save files (all 19 levels, richer state)
for f in ~/jnvsjn-original/_installshield/Program_Executable_Files/save/JimmyGame*.tsk; do
    python3 tools/tsk_parser.py "$f" --json
done
```

Write JSON output to `docs/tsk_data/` (create dir). Each file: `<stem>.json`.
Also write a combined `docs/tsk_data/all_entities.json` mapping
`{level_stem: [{name, state}, ...]}` across all files — this is the reference
Codex will use when speccing the `CLevel*Game` classes.

---

## Step 3 — Parse `menu.dat`

File: `~/jnvsjn-original/_installshield/Program_Executable_Files/dat/menu.dat`

Plaintext CSV-like format:
```
BACKGROUND,<sprite_id>,<screen>,<x>,<y>,...
MENUITEM,<id>,<screen>,<x>,<y>,<w>,<h>,<level_index>
HELPMENUITEM,<id>,<screen>,<x>,<y>,<label>,<font>,<align>,<level_index>,<r>,<g>,<b>
```

Write `tools/menu_parser.py` that reads the file and emits
`docs/menu_data/menu.json`:
```json
{
  "screens": ["MAIN_MENU", ...],
  "items": [
    {"type": "MENUITEM", "id": "MMVRTANK", "screen": "MAIN_MENU",
     "x": 111, "y": 251, "level_index": 33},
    ...
  ]
}
```

---

## Step 4 — Spec the target classes

With the extracted data, spec these classes in `docs/decomp/` and mark them
`spec` in `docs/decomp_ledger.csv`. Follow the exact format of existing spec
files (see e.g. `docs/decomp/C3DJimmy.md`).

### CTaskList (wave 8, `effects_triggers_nav_cameras_sound`)

vtable: `004d1c7c`
base: `CLocalGameObject -> CGameObject -> OMediaClassStreamer`
n_methods: 2

The .tsk file IS the CTaskList serialisation format. Document:
- The `LV1B` magic + version + variant header
- The lookup table region (purpose TBD — may be a flat ObjectTag hash table)
- STARTEXP record: task name, gam reference, spawn coords
- Entity state table: ObjectTag → initial state u32
- Role: loaded at level start; drives initial object state + spawn point for
  `C3DPlayer`; `TaskName`/`NewTaskState` props in .gam files point at these tags

### CJimmyGame (wave 10)

vtable: `004c2c74;004c2c84;004c30d4;004c30e8`
base: `CGameType -> OMediaElement -> ... -> CGameObject`
n_methods: 11

The parent of all `CLevel*Game` subclasses. Manages:
- Loading and holding the active `CTaskList`
- Registering the level's .gam file (from tsk `gam_file` field)
- Driving mission state transitions (`TaskName`/`NewTaskState` .gam props)
- Cross-reference: `docs/tsk_data/NewGame.tsk.json` for JNBG Level 1 entity states

### CLevel01AGame … CLevel07Game (30 subclasses, wave 10)

All inherit `CJimmyGame`. vtables in ledger. n_methods = 0 or 1 (leaf overrides).

Each one:
- Registers the level-specific .gam FourCCs not covered by the parent
- The `all_entities.json` survey shows which ObjectTags appear in each level
- Pattern spec: "leaf override of CJimmyGame; registers <N> level-specific
  objects; ObjectTags: <from all_entities.json>; gam: <from tsk start_info>"

These can be specced as a batch — one template, each gets its own file with
the correct vtable, level name, entity list, and gam reference.

### CLevelVR01 … CLevelVR08 (8 subclasses, wave 10)

Same pattern as CLevel*Game but for VR challenge levels. vtables in ledger.
n_methods = 1. Entity states likely minimal (VR levels are self-contained).
Note: no corresponding .tsk data for VR levels — spec from Ghidra ctor only.

### CMainMenu (wave 10)

vtable: `004d0bfc;004d0c0c;004d105c;004d1070`
base: `CJimmyGame -> CGameType -> ...`
n_methods: 1

Drives the front-end menu. Cross-reference `docs/menu_data/menu.json` for
the full screen/item layout it manages. Document screen names, MENUITEM ids,
and level_index routing.

### CMenuElement (wave 10)

vtable: `004d11ec;004d11fc;004d161c;004d1630;004d1640`
base: `OMediaCanvasElement -> OMediaElement -> ... -> CGameObject`
n_methods: 2

Individual menu item sprite. Properties driven by `menu.dat` MENUITEM rows:
position (x,y), size (w,h), sprite canvas, level routing index. Activated by
`CMainMenu`.

### C2DInGameMenu (wave 10)

vtable: `0048d414`
base: `CLocalGameObject -> CGameObject -> OMediaClassStreamer`
n_methods: 17  ← highest method count in the family; the HUD/in-game overlay

Manages HUD during gameplay (score, gadgets, health). Separate from `CMainMenu`
(front-end) — this is the overlay rendered on top of the 3D scene. See
`docs/decomp/hud.c` and the captured frame 8881 HUD analysis in
`docs/hud_chrome_digit_recapture.md` for the visual ground-truth.

---

## Files to commit

```
docs/tsk_data/NewGame.tsk.json
docs/tsk_data/JimmyGame*.tsk.json   (11 files)
docs/tsk_data/all_entities.json
docs/menu_data/menu.json
tools/menu_parser.py
tools/tsk_parser.py                 (fixed)
docs/decomp/CTaskList.md
docs/decomp/CJimmyGame.md
docs/decomp/CLevel01AGame.md … (all CLevel*Game + CLevelVR*)
docs/decomp/CMainMenu.md
docs/decomp/CMenuElement.md
docs/decomp/C2DInGameMenu.md
docs/decomp_ledger.csv              (mark all above spec)
```

Commit in logical groups: (1) parser fixes + data extraction, (2) CTaskList +
CJimmyGame, (3) CLevel batch, (4) menu classes.

---

## Definition of done

- `tsk_parser.py` correctly extracts all 12 entities from `NewGame.tsk`
- `docs/tsk_data/all_entities.json` covers all 11 sequel save files
- `docs/menu_data/menu.json` has all MENUITEM + HELPMENUITEM rows parsed
- 34 spec files exist in `docs/decomp/` for the classes listed above
- All 34 are marked `spec,claude` in `decomp_ledger.csv`
- All committed and pushed to `decomp-campaign`
