# CTaskList

## Identity

| Item | Value |
|---|---|
| RTTI name | `CTaskList` |
| Base chain | `CLocalGameObject -> CGameObject -> OMediaClassStreamer` |
| Vftable(s) | `004d1c7c` |
| Ctor(s) | vtable installer reached through the `CLocalGameObject` streamer path; class id immediate not separately pinned this pass |
| Dtor(s) | scalar deleting destructor at vtable slot 0 `0045f8c0` (calls `FUN_0045f8e0`, then `FUN_004789a0`) |
| Ledger row | `docs/decomp_ledger.csv` |

`CTaskList` is the **task / game-state object** loaded at level start. It is a thin
`CLocalGameObject` streamer wrapper (only 2 owned vtable methods out of 276 walked
slots) — almost all behavior is inherited. Its substance is the data it
deserializes: the `*.tsk` file. A `.tsk` carries the level's spawn point, the `.gam`
file to load, and an initial **entity-state override table**. `CJimmyGame`
(and its `CLevel*Game` subclasses) own a `CTaskList` and consult it during
`InitGame`; the `TaskName` / `NewTaskState` properties in `.gam` object rows refer
into this table's tag namespace.

Two `.tsk` files ship in the executable's file table (`.rdata:004ec71c` region):
`NewGame.tsk` (start a fresh game → Level 1) and `RestartLevel.tsk` (respawn after
death — see `C2DInGameMenu::vfunc_00_312`). The on-disk copies live in the game
root (`NewGame.tsk`).

## The `.tsk` Serialization Format

Measured from `NewGame.tsk` (2026-06-08). Parser: `tools/tsk_parser.py`; extracted
data: `docs/tsk_data/NewGame.tsk.json`. All multi-byte integers are **big-endian**;
floats are BE IEEE-754; strings are Pascal-style (`u8 length` + bytes), matching the
`.gam` convention.

```
offset  field
0x0000  char[4]   magic   = 'LV1B'
0x0004  u8        version = 0x41
0x0005  u8        variant = 0xa0
0x0006  ...       zero-filled lookup-table region (~16 KB; see Open Questions)
0x4226  STARTEXP record:
          pstring   task_name   'STARTEXP'
          pstring   param1      'none'
          pstring   gam_file    'level1b.gam'   <- level .gam this task loads
          pstring   param2      'none'
          f32 BE    spawn_x     470.5965
          f32 BE    spawn_y     609.2417
          f32 BE    spawn_z     -87.7631
0x4251  zero gap (variable length; a lone 0x01 byte sits at 0x4278)
0x42a0  entity-state table:
          u8        count       = 12
          count x:
            pstring   object_tag      'MOM', 'SCENE', 'REACTOR', ...
            u32 BE    state           0 = default/inactive; non-zero = custom
0x4321  EOF (the table runs to end of file)
```

The table has a single `u8` count, **not** a per-entry count, and is preceded by a
variable-length zero gap after the spawn floats (the parser scans past the gap to the
count byte, anchoring on the records consuming the file to EOF).

### `NewGame.tsk` entity-state table (Level 1 / `level1b.gam`)

| Tag | State | Note |
|---|---:|---|
| `MOM` | 0 | mission NPC |
| `LIBBY` | 0 | mission NPC |
| `BENNY` | 0 | mission NPC |
| `SCENE` | **30** | scene/sequence controller seeded to state 30 (`0x1e`) |
| `DINO` | 0 | |
| `CLONE` | 0 | |
| `KITTY1` | 0 | |
| `KITTY2` | 0 | |
| `KITTY3` | 0 | |
| `HYDRANT1` | 0 | |
| `HYDRANT2` | 0 | |
| `REACTOR` | 0 | mission objective object |

**Namespace note (measured):** none of these 12 tags appears as an `ObjectTag` in
`level1b.gam`'s 129 placed objects. The `.tsk` entity table is the task system's own
mission/actor namespace (NPCs, the `SCENE` sequencer, the `REACTOR` objective), kept
separate from per-instance `.gam` placement tags. The state values are initial
mission progress, not transform data — only `SCENE=30` is non-default in a new game.

## Field Map

`CTaskList` adds little instance state of its own over the `CLocalGameObject`
streamer; the streamed payload is the `.tsk` content above. Offsets below are the few
the owned methods touch (Ghidra still prints them as `this[N].vftable` against the
incomplete struct).

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `~this[0x12d]+1` | bool | `reset_flag` | `vfunc_00_259` `004601a0` | Byte cleared by the reset/reload slot after the inherited `CGameObject` reset runs. |

## Vtable Methods

276 slots walked; 2 owned. The remainder are inherited `CGameObject` /
`CLocalGameObject` / `OMediaClassStreamer` behavior.

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 0 | `0045f8c0` | `ScalarDeletingDestructor` | Runs `FUN_0045f8e0` cleanup, then frees via `FUN_004789a0` when the low bit of the deleting flag is set. | non-trivial |
| 259 | `004601a0` | `ResetTask` | Calls inherited `CGameObject::vfunc_00_013`, then clears the reset flag byte near `this[0x12d]`. | trivial |

## Constants And Wiring

| Item | Value | Evidence |
|---|---|---|
| `NewGame.tsk` | New-game task → loads `level1b.gam`, spawn `PHONEBOOTH`/`STARTEXP` | `.rdata:004ec71c`; referenced by the menu/new-game path |
| `RestartLevel.tsk` | Respawn task loaded on death/restart | `.rdata:004ec7e4`; `C2DInGameMenu::vfunc_00_312` calls `FUN_00460e70("RestartLevel.tsk")` |
| `STARTEXP` | Default task-record name inside `.tsk` | `NewGame.tsk` byte `0x4227` |
| `.gam` `TaskName` / `NewTaskState` | Per-object props that drive task-state transitions over this tag table | `docs/gam_schema.md`; `gam_loader.c` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| Task file | `NewGame.tsk` | game root; `docs/tsk_data/NewGame.tsk.json` | Level-1 new-game template, 12-entity table. |
| Task file | `RestartLevel.tsk` | executable reference only | Respawn template; on-disk copy not in `xp-jnbg-original/` this pass. |

## Confidence

Confidence: Medium-High

Validation: `.tsk` byte format measured directly from `NewGame.tsk` and round-tripped
through `tools/tsk_parser.py` (all 12 entities, `SCENE=30`); class vtable/base/owned
methods from Ghidra `DumpClass.java CTaskList`; file-table and restart wiring from
executable string/xref scan. Not runtime-validated against a live load.

Open questions:
- Purpose of the ~16 KB zero-filled lookup region between the header and the
  `STARTEXP` record (flat `ObjectTag` hash table reserve? fixed save-slot layout?).
- The lone `0x01` byte in the zero gap at `0x4278` — sub-record flag or alignment.
- Pin the `CTaskList` constructor / class-id immediate and the exact streamer slot
  that parses the `.tsk` byte stream into the entity table.
- Decode `RestartLevel.tsk` (recover its on-disk copy from XP) and confirm it shares
  the `NewGame.tsk` layout.

## Notes

- Evidence: `DumpClass.java CTaskList /tmp/decomp_CTaskList.md` (`slots=276`,
  `owned_methods=2`); `tools/tsk_parser.py` (rewritten 2026-06-08 — the prior
  `0x80`-aligned scan landed past the count byte and returned 0 entities);
  `tools/build_level_entity_index.py` for the per-level cross-reference.
- The `.tsk` is the `CTaskList` on-disk serialization; documenting the file format is
  equivalent to documenting the class's persistent state.
