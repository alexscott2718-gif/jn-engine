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
init/uninit/update lifecycle; the menu-specific work is the inherited `CGameType`
update plus the menu-manager slots (`LoadMyMenu` / `displayMenu` / `Activating Item`
traces in `.rdata:004ec620`+).

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
| Menu-manager traces | `LoadMyMenu: CurrMenu = %d`, `displayMenu`, `Activating Item %d for %d IsActive:%d` | `.rdata:004ec620`+ |

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
The menu *flow* (screen graph, per-item activation) is inherited/menu-manager code
not yet decompiled here. Not runtime-validated.

Open questions:
- Decompile the menu-manager slots (`LoadMyMenu`/`displayMenu`/`Activating Item`) and
  determine whether they are owned by `CMainMenu`, a shared manager, or
  `CMenuElement`.
- Recover the screen graph and level-index → route mapping for the original game (the
  sequel's `menu.dat` is **not** authoritative for JNBG).
- Confirm how `CMainMenu` hands off to the level `CLevel*Game` controller on select.

## Notes

- Evidence: `DumpClass.java CMainMenu /tmp/decomp_CMainMenu.md`; executable table
  `.rdata:004ec71c`.
- Sibling of the `CLevel*Game` controllers under `CJimmyGame`; front-end counterpart
  to the in-game `C2DInGameMenu` HUD.
