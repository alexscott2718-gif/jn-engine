# CEditor

## Identity

| Item | Value |
|---|---|
| RTTI name | `CEditor` |
| Base chain | `OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort` |
| Vftable(s) | `004d5e7c`, `004d5e8c`, `004d5ea0` |
| Ctor(s) | installs the three `CEditor` vftables over the `OMediaElement` path |
| Dtor(s) | scalar deleting destructor `vfunc_01_002` at `0046ba70` (destroys the `OMediaClassStreamer` at `this+0x1f9`) |
| Ledger row | `docs/decomp_ledger.csv` |

`CEditor` is the **in-engine level editor / object-placement tool** — a developer mode
baked into the retail executable. Unlike the gameplay classes it derives directly from
`OMediaElement` (engine element), not `CGameObject`. It owns an object-selection
palette (a fixed table of `"Select Object"` entries), toggles the OS mouse cursor,
talks to the global game/database objects (`DAT_00509a4c`/`DAT_00509a50`/`DAT_00509984`),
and runs a boot/intro state sequence. It is `level_game_controllers` (wave 10) because
it is a top-level mode like `CMainMenu`/`CJimmyGame`, but its role is authoring, not
play. (Listed in the family alongside `CAweReal` and `C3DLabScreen`.)

## Field Map

Offsets from the primary `CEditor` pointer (`this[N]` slot arithmetic).

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `this[0x24..0x26]` | pointer | `saved_view_state` | `vfunc_02_045`, `vfunc_02_046` | Snapshot of the global render/camera state (`DAT_00509a50 + 0x44/0x48/0x4c`) saved on enter and restored on exit. |
| `this[0x28..0x2a]` | pointer | `enter_scratch` | `vfunc_02_043` | Cleared on the enter/reset path. |
| `this[0x2c..0x31]` | pointer | `exit_scratch` | `vfunc_02_046`, `vfunc_02_051` | Cleared on exit/uninit. |
| `this[0x3f]+` | object[] | `palette_objects` | `vfunc_02_045`, `vfunc_02_046` | Array walked in lockstep with the `"Select Object"` name table; each entry's slot `0x58` is enabled/disabled. |
| `this[0x1d3]` (byte) | bool | `boot_done` | `vfunc_02_049` | Set when the boot countdown (`this[0x1fe]+2`) expires. |
| `this[0x201]` | float | `boot_timer` | `vfunc_02_049` | Accumulates `dt` across boot phases. |
| `this[0x202]` | int | `boot_phase` | `vfunc_02_049` | Boot state machine index (0→3); phase 1 loads `level1d.gam`. |
| `this+0x1f9` | subobject | `class_streamer` | `vfunc_01_002` | `OMediaClassStreamer` tail destroyed by the deleting destructor. |

## Vtable Methods (owned)

12 owned methods (slots in the `0046b…`–`0046e…` range):

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| vtable 1 slot 2 | `0046ba70` | `ScalarDeletingDestructor` | Destroys `class_streamer`, frees the allocation. | non-trivial |
| vtable 2 slot 43 | `0046bc40` | `OnEnterReset` | Clears `enter_scratch`, registers with the global db (`slot 0x2c`, `DAT_00509a4c`). | non-trivial |
| vtable 2 slot 45 | `0046bcb0` | `InitEditor` | Saves `saved_view_state`, disables in-world objects' slot `0x58`, sets the `"Select Object"` mode label. | non-trivial |
| vtable 2 slot 46 | `0046bdc0` | `UnInitEditor` | Re-enables objects' slot `0x58`, `ShowCursor(0)`, restores state, clears `exit_scratch` (traces `UnInitEditor`). | non-trivial |
| vtable 2 slot 49 | `0046bec0` | `BootSequence` | Boot/intro state machine: counts `boot_timer += dt`, advances `boot_phase` 0→3; phase 1 loads `level1d.gam`; phases gate on `DAT_00509853`/global game objects. | non-trivial |
| vtable 2 slot 51 | `0046d270` | `vfunc_02_051` | Editor sub-op (clears `exit_scratch`; see body). | raw block |
| vtable 2 slot 55 | `0046cf30` | `vfunc_02_055` | Editor sub-op touching `this[0x16d]`. | raw block |
| vtable 2 slots 57/59/62/63/66 | `0046d4f0`…`0046e7b0` | editor ops | Palette/selection/placement handlers (`"%s %d"` formatting, object enable/disable). | raw block |

### Enter / exit behavior

```c
CEditor::InitEditor():                       // vfunc_02_045 @ 0046bcb0
    save saved_view_state <- DAT_00509a50[0x44/0x48/0x4c]   // render/camera state
    DAT_00509a50[0x114] = 5                                  // editor mode flag
    clear "in game" bits on DAT_00509a38 / DAT_00509a3c
    for name in palette_table("Select Object" .. 0x4f6090, stride 0x20):
        obj = palette_objects[i]
        obj->slot_0x58(0)            // take objects out of normal play
    set mode label "Select Object"

CEditor::UnInitEditor():                      // vfunc_02_046 @ 0046bdc0
    trace("UnInitEditor")
    for name in palette_table(...):
        palette_objects[i]->slot_0x58(1)       // restore objects to play
    ShowCursor(0)
    clear exit_scratch
```

`InitEditor`/`UnInitEditor` bracket the editor mode: they snapshot and restore the
global render state and flip every placeable object between "edit" (slot `0x58(0)`) and
"play" (slot `0x58(1)`), iterating a fixed `"Select Object"` palette name table
(0x20-byte stride, ending at `0x4f6090`). `BootSequence` is the startup intro the mode
runs before handing to `level1d.gam`.

## Constants And Wiring

| Item | Value | Evidence |
|---|---|---|
| Editor mode flag | `DAT_00509a50[0x114] = 5` | `InitEditor` |
| Palette table | `"Select Object"` rows, stride `0x20`, end `0x4f6090` | `InitEditor`/`UnInitEditor` strcmpi loop |
| Boot level | `level1d.gam` | `BootSequence` phase 1 |
| Cursor | `ShowCursor(0)` (Win32) | `UnInitEditor` |
| Globals | `DAT_00509a4c`, `DAT_00509a50`, `DAT_00509984/88`, `DAT_00509853` | enter/boot paths |

No `.gam` properties — `CEditor` is a mode, not a placed object.

## Assets

None registered directly; it operates on whatever level (`level1d.gam`) and objects are
loaded.

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java CEditor` (`slots=83`, `owned_methods=12`,
`offsets=6`); the enter/exit bracketing, the `"Select Object"` palette loop, the boot
state machine, and the global-state save/restore are read from the decompiled bodies.
The five `0046d…/0046e…` editor sub-ops (placement/selection) are not fully decoded.
Not runtime-validated (likely a dev-only mode not reachable in normal retail play).

Open questions:
- Decode the placement/selection ops (slots 51/55/57/59/62/63/66) — how an object is
  picked from the palette and instantiated/moved.
- How is `CEditor` entered (key combo? debug flag?) and is it reachable in the shipped
  build?
- Map the `"Select Object"` palette table contents (the FourCC/class list it can place).

## Notes

- Evidence: `DumpClass.java CEditor /tmp/dumps2/decomp_CEditor.md`.
- Hand-written from the decompiled bodies (supersedes the generated skeleton). A retail
  dev tool; the only `level_game_controllers` member that authors rather than plays.
