# CMainMenu

## Identity

| Item | Value |
|---|---|
| RTTI name | `CMainMenu` |
| Base chain | `CJimmyGame -> CGameType -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004d0bfc`, `004d0c0c`, `004d105c`, `004d1070` |
| Ctor(s) | inherits `CJimmyGame` construction; installs the four `CMainMenu` vftables |
| Dtor(s) | scalar deleting destructor `vfunc_02_002` at `00459200` |
| Ledger row | `docs/decomp_ledger.csv` |

`CMainMenu` is the **front-end menu game mode**. It is a thin `CJimmyGame` subclass
(1 owned method — the destructor) that runs as a `CGameType` game mode like a level,
but instead of gameplay it drives the title/level-select screens and routes the
player's selection to the chosen level's `.tsk`/`.gam`. Individual clickable items
are [`CMenuElement`](./CMenuElement.md) canvas objects; the gameplay HUD overlay is
the separate [`C2DInGameMenu`](./C2DInGameMenu.md).

Because it inherits `CJimmyGame` (and thus `CGameType`), the menu gets the same
init/uninit/update lifecycle. Target 4 recovered the shared menu-manager slots
behind the `LoadMyMenu` / `displayMenu` / `Activating Item` traces; Ghidra names
some of these under `C2DInGameMenu` because the canvas-menu subsystem is shared,
but the same `DAT_004f8164` menu tables and `CMenuElement` items are the missing
front-end screen-graph layer beyond the routing table.

## Level Routing Table (executable)

The new-game / level-select routing targets are a string table in the executable at
`.rdata:004ec71c`:

| String | Role |
|---|---|
| `NewGame.tsk` | "New Game" → load Level 1 (`level1b.gam`, spawn `PHONEBOOTH`); see [`CTaskList.md`](./CTaskList.md) |
| `VR01.gam` … `VR08.gam` | VR challenge levels, listed in menu order: `VR01, VR03, VR02, VR08, VR06, VR07, VR05, VR04` |
| `level1f.gam` | level reload target (shared with the death/restart path) |
| `RestartLevel.tsk` | respawn task |
| `PHONEBOOTH` | default start-point tag |
| `JIM1` | player object tag |

> **JNBG note:** the original game has **no standalone `menu.dat`** — menu layout and
> level routing come from the executable table above (and the menu-manager slots).
> The `menu.dat` file shipped only with the later *Jimmy vs. Jimmy Negatron* sequel
> and is a different game's asset; it was deliberately **not** used to spec this
> class.

## Field Map

`CMainMenu` adds no owned instance fields beyond the inherited `CJimmyGame` /
`CGameType` controller state; its only owned method is the destructor.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `this+0x163` | subobject | `class_streamer` | `vfunc_02_002` `00459200` | `OMediaClassStreamer` tail destroyed by the deleting destructor (same shape as `CJimmyGame`). |

## Vtable Methods

374 slots walked; 1 owned.

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| vtable 2 slot 2 | `00459200` | `ScalarDeletingDestructor` | Owned deleting destructor; destroys the streamer subobject and frees the adjusted allocation. All other behavior inherited from `CJimmyGame`/`CGameType`. | non-trivial |

## Constants And Wiring

| Item | Value | Evidence |
|---|---|---|
| `NewGame.tsk` | New-game route → Level 1 | `.rdata:004ec71c`; [`CTaskList.md`](./CTaskList.md) |
| VR `.gam` list | 8 VR levels in menu order | `.rdata:004ec728`–`004ec77c` |
| Menu-manager traces | `LoadMyMenu: CurrMenu = %d`, `displayMenu`, `Activating Item %d for %d IsActive:%d` | `.rdata:004ec620`+; recovered in `docs/decomp/evidence/menu_manager_target4.md` |

Not a level-placeable `.gam` object — instantiated as a game mode by the load path.

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| Task file | `NewGame.tsk` | game root | New-game route. |
| Level data | `VR0N.gam` | game root | VR level-select routes. |
| Menu items | `CMenuElement` instances | runtime | Clickable canvas children. |

## Confidence

Confidence: Low-Medium

Validation: Ghidra `DumpClass.java CMainMenu` (`slots=374`, `owned_methods=1`); base
chain confirms the `CJimmyGame` lineage; routing table from executable string scan.
Target 4 recovered the shared canvas menu-manager functions in
`docs/decomp/evidence/menu_manager_target4.md`. Not runtime-validated.

Open questions:
- ~~Decompile the menu-manager slots (`LoadMyMenu`/`displayMenu`/`Activating Item`) and
  determine whether they are owned by `CMainMenu`, a shared manager, or
  `CMenuElement`.~~ **DONE 2026-07-02**: target 4 pins them as a shared canvas-menu manager over `DAT_004f8164` tables, with Ghidra naming some slots under `C2DInGameMenu`; item-side dispatch remains `CMenuElement::UpdateItemLogic`.
- Recover the screen graph and level-index → route mapping for the original game (the
  sequel's `menu.dat` is **not** authoritative for JNBG).
- Confirm how `CMainMenu` hands off to the level `CLevel*Game` controller on select.

## Notes

- Evidence: `DumpClass.java CMainMenu /tmp/decomp_CMainMenu.md`; executable table
  `.rdata:004ec71c`.
- Sibling of the `CLevel*Game` controllers under `CJimmyGame`; front-end counterpart
  to the in-game `C2DInGameMenu` HUD.

## Target 4 Menu-Manager L1

Target 4 recovered the menu-manager body cluster:

```c
LoadCurrentMenu(this):
    table = DAT_004f8164[this->current_menu_index]
    this->slot_0x4a0(table)          // LoadMyMenu
    this->slot_0x4d8(current_menu_index)

LoadMyMenu(this, menu_index):
    table = DAT_004f8164[menu_index]
    allocate root canvas if table[0] is null
    attach root to DAT_00509a34
    for item in 0..28:
        if active_canvas missing and active sprite/index != -1:
            allocate active OMediaCanvasElement
            attach, position via slot 0x450, cache rounded x/y
            state 0 or 4 -> show active; state 1 -> hide active
        if rollover_canvas missing and rollover sprite/index != -1:
            allocate rollover OMediaCanvasElement
            attach, position via slot 0x450, cache rounded x/y
            state 0 or 4 -> show rollover; state 1 -> hide rollover

ActivateItem(menu, item, state):
    if 0 <= item < 29:
        *(DAT_004f8164[menu] + 0x24 + item*0x28) = state

UnloadMyMenu(table):
    hide root, active, and rollover canvases for all 29 records

DisplayMenu(table):
    show root canvas
    for item in 0..28:
        if item state is neither 0 nor 4:
            hide active canvas
            show rollover canvas
        if table == DAT_004f816c and item counter is non-zero:
            draw counter at active-item coordinates
```

`Menu_NewGameRoute_0040caa0` is a story/action dispatcher over strings at
`this+0x468`; it updates task state, activates menu items, pulses counters, and
routes `RESTARTGAME` to `NewGame.tsk`. `Menu_VRRouteTable_004603f0` loads a
save/task stream, maps level FourCCs to `.gam` filenames including the
`VR01..VR08` strings, writes the selected level to `DAT_00509980+0x74d`, and
refreshes task/menu state.

## Native Linkage (linked-parity branch)

Aspect: **`level-routing-table`** — status `linked`.
Certificate: `docs/linkage_certificates.csv`; oracle:
`tools/linkage_oracles/CMainMenu.py`.

This aspect certifies exactly the decoded Level Routing Table above
(`.rdata:004ec71c`) against the native front-end (`src/game/menu.c`):

| Decomp (Neutron.exe) | Native (`src/game/menu.c`) | Deviation |
|---|---|---|
| `NewGame.tsk` first route | `g_items[0]` = New Game -> `level1b` via the NewGame task (`is_newgame`) | the `NewGame->level1b.gam` binding is certified separately by `CTaskList`/tsk-deserialization |
| `VR01..VR08.gam` in menu order `VR01, VR03, VR02, VR08, VR06, VR07, VR05, VR04` | `g_items[1..8]`, same order | `VRxx.gam` -> `vrxx` is the native loader's level-name normalization |
| (menu-manager quit control) | trailing `Quit` row, no route | native UI convention; asserted only as the no-route terminator |

Oracle: `cmainmenu_dump.c` compiles the real, unmodified `menu.c` (input
stubbed to a scripted key queue, renderer overlay stubbed) and walks every
index through the real `menu_open`/`menu_input`/`menu_take_confirm` path,
diffing each routed `(level, is_newgame)` against the doc table; probing past
the end must wrap to index 0, pinning the item count at 10.

The native stand-in gained visible labels through the shared `ui_text.c`
font-atlas renderer on 2026-07-16. That presentation change does not expand
this certificate beyond the route ordering and no-route terminator above.

### Aspect: `menu-manager-screen-graph` -- status `linked-blocked` (target 4, 2026-07-02)

Target 4 opens the original L1 menu-manager graph: `LoadMyMenu`/`displayMenu`
allocate and toggle 29 active/rollover `OMediaCanvasElement` item pairs from
`DAT_004f8164`, `ActivateItem`/`DeactivateItem` write item state, and
`CMenuElement::UpdateItemLogic` performs mouse/canvas target dispatch.

This is not certifiable against the current native port. `src/game/menu.c` is
the approved keyboard-list stand-in and only claims the executable routing
table certified above. It has no `DAT_004f8164` canvas tables, active/rollover
item pairs, item-state writer, counter pulse table, mouse cursor path, or
save/task stream refresh. A green oracle would require porting the recovered
canvas menu subsystem first; do not expand the existing `level-routing-table`
certificate to cover this behavior.

Fresh `9a2b908` review also confirms that the immutable evidence does not carry
the contents of the `DAT_004f8164` tables themselves. The target-4 dump proves
the 29-record loop and state mechanics, but `Menu_ItemRolloverState_00403890`
still failed function recovery, the concrete sprite/position records are
absent, activation audio is not body-backed, and the document still lists the
screen graph and level-controller handoff as open. Recreating those values
from the native keyboard list would be a hand approximation, not a linked port.

### Not covered / open

- The menu-manager screen graph is now decompiled, but it remains outside
  the linked scope because native `menu.c` does not port the canvas subsystem.
  Rollover/activation drawing, audio cues, mouse input, and save/task refresh
  still require native-port work before another menu oracle is meaningful.
